# Board B - Central Gateway

Board B는 CAN1 차량 내부 버스와 CAN2 계기판/진단 버스를 잇는 Central Gateway입니다. 현재 구현은 CAN1 프레임을 해석해서 새 계기판 프레임을 만드는 방식이 아니라, 라우팅 테이블에 등록된 CAN ID만 CAN2로 그대로 포워딩합니다.

## 개요

```text
CAN1 500 kbps
  Board A EngineSim -> 0x100, 0x280, 0x1A0, 0x5A0, 0x288, 0x481
  Board D Body      -> 0x531, 0x635
  Board E Safety    -> 0x3A0
        |
        v
Board B Gateway
  CAN RX queue
  GatewayTask
  Router + Safety Bridge + Gateway ADAS UDS Server
        |
        v
CAN2 500 kbps
  Golf 6 Cluster + Board C diagnostic client
```

## 주요 기능

| 기능 | 설명 |
|---|---|
| CAN1/CAN2 BSP | CAN 초기화, open filter, RX interrupt, RTOS queue 기반 수신 |
| Routing table | 등록된 CAN ID만 CAN1 -> CAN2 포워딩 |
| Safety bridge | Board E `0x3A0` ADAS 상태를 저장하고 DTC를 누적 |
| Parking assist sweep | `risk_level >= 2`일 때 CAN2 `0x5D6`을 주기적으로 송신 |
| Gateway ADAS UDS server | CAN2 `0x714` 요청 중 ADAS DID와 Clear DTC에 `0x77E`로 응답 |
| Logger task | RX/TX, busy/error, route, ADAS, engine/body monitor 카운터 UART 출력 |
| CAN CLI monitor | `canlog` 명령으로 특정 CAN ID 로그 필터링 |

## 파일 구조

| 파일 | 역할 |
|---|---|
| `app/gateway_tasks.c` | Gateway task 허브. CAN 초기화, RX queue 처리, router/safety/UDS 호출, 상태 로그 |
| `app/gateway_router.c/h` | CAN ID 라우팅 테이블과 CAN1 -> CAN2 포워딩 |
| `app/gateway_safety_bridge.c/h` | ADAS 상태 저장, DTC 관리, `0x5D6` parking assist sweep 생성 |
| `app/gateway_uds_server.c/h` | Gateway ADAS DID와 Clear DTC Single Frame 응답 |
| `app/can_bsp.c/h` | CAN1/CAN2 송수신 BSP |
| `app/can_cli_monitor.c/h` | UART CLI CAN 로그 |
| `app/cli.c/h`, `app/uart.c/h` | UART CLI |
| `app/protocol_ids.h` | 현재 사용하는 CAN ID와 payload index 정의 |

## 라우팅 테이블

현재 CAN1 -> CAN2 라우팅 대상:

| CAN ID | 송신 보드 | 의미 |
|---:|---|---|
| `0x100` | Board A | IGN Status |
| `0x280` | Board A | RPM |
| `0x1A0` | Board A | Speed keepalive, 현재 zero-filled |
| `0x5A0` | Board A | Speed reference |
| `0x288` | Board A | Coolant |
| `0x481` | Board A | Engine warning status |
| `0x531` | Board D | Turn Status |
| `0x635` | Board D | Cluster brightness |
| `0x390` | Legacy route | 과거 Body Status 호환용. 현재 Board D는 송신하지 않음 |

Board E의 `0x3A0`은 Safety Bridge가 내부 진단 상태로 소비합니다. Golf 6 K-Matrix에서도 `0x3A0`이 다른 의미로 쓰일 수 있어서 기본 설정에서는 CAN2로 원본 포워딩하지 않습니다. 필요하면 `GATEWAY_SAFETY_FORWARD_ADAS_STATUS=1` 컴파일 옵션을 명시적으로 켭니다.

## Board A 송신 포맷

| CAN ID | DLC | 의미 | 주요 payload |
|---:|---:|---|---|
| `0x100` | 8 | IGN Status | `byte[5] bit0 = 1` |
| `0x280` | 8 | RPM | `byte[2..3] = rpm * 4`, little-endian |
| `0x1A0` | 8 | Speed keepalive | 현재 `00 00 00 00 00 00 00 00` |
| `0x5A0` | 8 | Speed reference | `byte[2] = km/h / 2`, 경고 시 `byte[4] bit4..5 = 1` |
| `0x288` | 8 | Coolant | `byte[1] = ((degC + 48) * 4) / 3` |
| `0x481` | 8 | Engine warning | `byte[0]` warning bits, `byte[1]` coolant, `byte[2..3]` rpm |

Board B의 monitor 값은 `0x280`, non-zero `0x1A0`, `0x5A0`, `0x288`, `0x100`, `0x481`을 관찰해서 계산합니다. 현재 Board A의 실제 속도 기준은 `0x5A0`입니다.

## Safety Bridge - ADAS 처리

Board E가 CAN1 `0x3A0 ADAS_Status`를 송신하면 Gateway는 아래 상태를 저장합니다.

| 항목 | 설명 |
|---|---|
| flags/risk/front/rear | `0x3A0 byte[0..3]` |
| active fault bitmap | `0x3A0 byte[4]` |
| speed/alive | `0x3A0 byte[5]`, `byte[7]` |
| DTC bitmap | sensor fault 또는 active fault 발생 시 누적 |

현재 Safety Bridge가 실제로 CAN2에 생성하는 프레임은 `0x5D6`입니다.

| Risk level | CAN2 `0x5D6` 동작 |
|---:|---|
| 0, 1 | off. 위험 해제 시 0으로 한 번 정리 송신 |
| 2 | `byte[1]`을 10 ms 기준으로 연속 sweep |
| 3 | `byte[1]`을 5 ms 기준으로 더 빠르게 sweep |
| 4 이상 | `byte[1]`을 2 ms 기준으로 sweep |

주의: `protocol_ids.h`에는 `0x480 mMotor_5`와 `0x050 mAirbag_1` 관련 ID가 남아 있지만, 현재 `gateway_safety_bridge.c`는 ADAS 상태를 `0x480` 또는 `0x050`으로 송신하지 않습니다.

## Board C / UDS

Board C는 보드의 CAN1 주변장치를 물리적으로 Board B의 CAN2 진단 버스에 연결해서 사용합니다. 기본 요청/응답 ID는 아래와 같습니다.

| 방향 | CAN ID | 설명 |
|---|---:|---|
| Board C -> Gateway/Cluster | `0x714` | UDS request |
| Gateway/Cluster -> Board C | `0x77E` | UDS response |

Board B Gateway UDS server는 ADAS 데모용 DID만 처리합니다.

| 요청 | 응답 |
|---|---|
| `22 F4 10` | flags, risk, front cm, rear cm |
| `22 F4 11` | front cm |
| `22 F4 12` | rear cm |
| `22 F4 13` | active fault, latched DTC |
| `14 FF FF FF` | DTC clear positive response `54` |

같은 버스에서 계기판도 `0x714/0x77E`를 쓸 수 있으므로, Board B가 켜져 있으면 unsupported DID에 대해 Gateway NRC가 같이 보일 수 있습니다.

## UART CLI

기본 UART 설정:

```text
115200 baud, 8N1
```

자주 쓰는 명령:

| 명령 | 의미 |
|---|---|
| `canlog stat` | CAN 로그 설정과 RX/TX 카운터 출력 |
| `canlog on` | CAN RX/TX 로그 출력 시작 |
| `canlog off` | CAN RX/TX 로그 중지 |
| `canlog id 280` | `0x280`만 출력 |
| `canlog id 5A0` | `0x5A0`만 출력 |
| `canlog id 3A0` | Board E ADAS 프레임 확인 |
| `canlog all` | ID 필터 해제 |
| `log off` | 1초 상태 로그 중지 |
| `log on` | 1초 상태 로그 재시작 |

상태 로그 예:

```text
[GW] RX1=120 TX1=0 RX2=0 TX2=118 busy=0 err=0 route=118 ok=118 fail=0 ignore=2 adas=1 risk=2 fault=0x00 dtc=0x00 gong=0x10 front=40 rear=80 speed=60 alive=12 eng=1 rpm=1200 spd1=0 spd5=60 coolant=90 ign=1 ewarn=0 rpmwarn=0 coolwarn=0 genwarn=0 body=1 left=0 right=1 hazard=0
```

`route`는 라우팅 테이블에 매칭된 수, `ok`는 CAN2 송신 성공 수, `fail`은 송신 실패 수, `ignore`는 테이블에 없는 ID 수입니다.

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
