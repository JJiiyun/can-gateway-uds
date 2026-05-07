# Board B - Central Gateway

Board B는 CAN1의 각 보드 메시지를 수신하고, 라우팅 테이블에 등록된 CAN ID만 CAN2 계기판/진단 버스로 전달하는 Central Gateway입니다.

## 개요

```text
CAN1 500kbps
  Board A EngineSim -> 0x280, 0x1A0, 0x5A0, 0x288
  Board D Body      -> 0x390
  Board E Safety    -> 0x3A0
        |
        v
Board B Gateway
  CAN RX queue
  GatewayTask
  Routing table + Safety handling
        |
        v
CAN2 500kbps
  Cluster + Board C diagnostic bus
```

Gateway는 Board A의 엔진 payload를 해석해서 계기판 프레임을 새로 만들지 않습니다. Board A가 계기판 반응 ID 기준으로 프레임을 직접 만들고, Board B는 CAN ID 기준으로 선별 포워딩합니다.

## 주요 기능

| 기능 | 설명 |
|---|---|
| CAN1/CAN2 BSP | CAN 초기화, open filter, RX interrupt, RTOS queue 기반 수신 |
| Routing table | 등록된 CAN ID만 CAN1 -> CAN2 포워딩 |
| Safety bridge | Board E `0x3A0` ADAS 상태를 진단 상태로 저장하고 필요 시 `0x480` warning 송신 |
| Logger task | RX/TX, busy/error, routing 카운터를 UART로 출력 |
| CAN CLI monitor | `canlog` 명령으로 특정 CAN ID 로그 필터링 |

## 파일 구조

| 파일 | 역할 |
|---|---|
| `app/gateway_tasks.c` | Gateway task 허브. CAN 초기화, RX queue 처리, router/safety 호출, 상태 로그 |
| `app/gateway_router.c/h` | CAN ID 라우팅 테이블과 CAN1 -> CAN2 포워딩 |
| `app/gateway_safety_bridge.c/h` | ADAS 상태 저장, DTC 관리, `0x480` warning 생성 |
| `app/can_bsp.c/h` | CAN1/CAN2 송수신 BSP |
| `app/can_cli_monitor.c/h` | UART CLI CAN 로그 |
| `app/cli.c/h`, `app/uart.c/h` | UART CLI |
| `app/protocol_ids.h` | 현재 사용하는 CAN ID와 payload index 정의 |

## 라우팅 테이블

현재 CAN1 -> CAN2 라우팅 대상:

| CAN ID | 송신 보드 | 의미 |
|---:|---|---|
| `0x280` | Board A | RPM 계기판 바늘 |
| `0x1A0` | Board A | Speed 바늘용 주기 프레임 |
| `0x5A0` | Board A | Speed 보조 프레임 |
| `0x288` | Board A | 냉각수 바늘 |
| `0x390` | Board D | Body `mGate_Komf_1` |
| `0x531` | Board D/Cluster | 방향지시등 후보 프레임 |
| `0x635` | Board D/Cluster | Body 밝기 조절 후보 프레임 |

## Board A 송신 포맷

| CAN ID | DLC | 의미 | 주요 payload |
|---:|---:|---|---|
| `0x280` | 8 | RPM 계기판 바늘 | `byte[2]~byte[3]` little-endian raw, `rpm = raw / 4` |
| `0x1A0` | 8 | Speed 바늘용 주기 프레임 | `byte[2]~byte[3]` little-endian raw, `speed = raw / 80` |
| `0x5A0` | 8 | Speed 보조 프레임 | `byte[2]` speed 값 |
| `0x288` | 8 | 냉각수 바늘 | `byte[1]` coolant 값 |

Board A 송신 주기:

| CAN ID | 주기 |
|---:|---:|
| `0x280` | 50ms |
| `0x1A0` | 50ms |
| `0x5A0` | 50ms |
| `0x288` | 100ms |

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
| `canlog id 1A0` | `0x1A0`만 출력 |
| `canlog id 5A0` | `0x5A0`만 출력 |
| `canlog id 288` | `0x288`만 출력 |
| `canlog id 390` | `0x390`만 출력 |
| `canlog all` | ID 필터 해제 |
| `log off` | 1초 상태 로그 중지 |
| `log on` | 1초 상태 로그 재시작 |

상태 로그 예:

```text
[GW] RX1=120 TX1=0 RX2=0 TX2=118 busy=0 err=0 route=118 ok=118 fail=0 ignore=2 adas=0 risk=0 fault=0x00 dtc=0x00 front=0 rear=0 speed=0 alive=0
```

`route`는 라우팅 테이블에 매칭된 수, `ok`는 CAN2 송신 성공 수, `fail`은 송신 실패 수, `ignore`는 테이블에 없는 ID 수입니다.

## Board C / UDS

Board C는 CAN2에 연결된 UDS client입니다. 계기판도 같은 CAN2 버스에 있으므로 Board B는 UDS server로 응답하지 않습니다.

```text
Board C UDS request
  -> CAN2 bus
  -> Cluster UDS server
  -> CAN2 bus
  -> Board C UDS response
```

따라서 Board B의 역할은 CAN2에서 오가는 UDS 요청/응답을 방해하지 않는 것입니다. `gateway_uds_server.c/h`와 Board B UDS 응답 ID/DID 정의는 제거되었습니다.

## 이전 구조에서 변경된 점

기존 구조:

```text
Board A 0x100 통합 엔진 프레임
  -> Gateway가 RPM/Speed/Coolant를 파싱
  -> Gateway가 0x280/0x1A0 생성
```

현재 구조:

```text
Board A가 0x280/0x1A0/0x5A0/0x288 직접 송신
  -> Gateway는 라우팅 테이블 기준으로 CAN1 -> CAN2 포워딩
```

따라서 `CAN_ID_ENGINE_DATA = 0x100`, `CAN_ENGINE_DATA_*`, `CAN_ENGINE_WARNING_*` 기반 로직은 제거되었습니다.
