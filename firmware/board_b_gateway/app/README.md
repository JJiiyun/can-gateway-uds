# Gateway App 설명서

Board B Gateway의 `app/` 코드는 CAN RX queue에서 프레임을 읽고, 라우팅 테이블에 등록된 CAN ID만 CAN1에서 CAN2로 포워딩합니다. Board A의 RPM/Speed/Coolant payload를 해석해서 새 계기판 프레임을 만들지 않고, Board A가 만든 계기판용 payload를 그대로 전달합니다.

## 현재 구현 파일

| 파일 | 역할 |
|---|---|
| `gateway_tasks.c` | FreeRTOS task 진입점. CAN 초기화, RX queue 처리, router/safety/UDS 호출, 상태 로그 출력 |
| `gateway_router.c/h` | 라우팅 테이블 기반 CAN1 -> CAN2 포워딩 |
| `gateway_safety_bridge.c/h` | Board E `0x3A0` ADAS 상태를 진단 상태로 저장하고 `0x5D6` parking assist sweep 생성 |
| `gateway_uds_server.c/h` | CAN2 `0x714` Gateway ADAS DID 요청과 Clear DTC에 `0x77E`로 응답 |
| `can_bsp.c/h` | CAN1/CAN2 초기화, 송신, RX interrupt queue 처리 |
| `can_cli_monitor.c/h` | UART CLI CAN frame 로그 |
| `cli.c/h`, `uart.c/h` | UART CLI 기반 명령 처리 |
| `protocol_ids.h` | 현재 Gateway가 사용하는 CAN ID와 payload index 정의 |

## 런타임 흐름

```text
CAN1/CAN2 RX interrupt
  -> can_rx_q
  -> GatewayTask
       -> CanCliMonitor_LogRx()
       -> GatewayRouter_OnRx()
       -> GatewaySafetyBridge_OnRx()
       -> GatewayUdsServer_OnRx()

ClusterTask
  -> GatewaySafetyBridge_Task10ms()
       -> ADAS risk >= 2이면 CAN2 0x5D6 sweep 송신
```

`ClusterTask`는 Board A 데이터를 계기판 프레임으로 변환 송신하지 않습니다. 현재는 Safety Bridge의 10 ms tick만 담당합니다.

## Board A 라우팅 대상

| CAN ID | DLC | 의미 | 주요 payload |
|---:|---:|---|---|
| `0x100` | 8 | IGN Status | `byte[5] bit0 = 1` |
| `0x280` | 8 | RPM | `byte[2..3] = rpm * 4`, little-endian |
| `0x1A0` | 8 | Speed keepalive | 현재 zero-filled |
| `0x5A0` | 8 | Speed reference | `byte[2] = km/h / 2` |
| `0x288` | 8 | Coolant | `byte[1] = ((degC + 48) * 4) / 3` |
| `0x481` | 8 | Engine warning status | `byte[0]` warning bits |

## 추가 라우팅 대상

| CAN ID | 의미 |
|---:|---|
| `0x531` | Board D 방향지시등 |
| `0x635` | Board D 계기판 밝기 |
| `0x390` | Legacy Body Status route. 현재 Board D는 송신하지 않음 |

## Safety / UDS 처리

- `0x3A0`은 라우팅 테이블에 넣지 않고 `GatewaySafetyBridge_OnRx()`가 내부 진단 상태로 저장합니다.
- `risk_level >= 2`이면 `gateway_safety_bridge.c`가 CAN2 `0x5D6`을 송신합니다.
- 현재 구현은 ADAS를 `0x480` 또는 `0x050`으로 변환 송신하지 않습니다.
- `gateway_uds_server.c`는 CAN2 `0x714` 요청 중 `0xF410`-`0xF413` DID와 `0x14 ClearDTC`에 응답합니다.

## 삭제된 이전 구조

아래 구조는 현재 빌드 흐름에서 사용하지 않습니다.

| 이전 구조 | 현재 상태 |
|---|---|
| `gateway_engine_bridge.c/h` | Gateway가 엔진 데이터를 해석해 `0x280/0x1A0`을 생성하지 않음 |
| `gateway_body_bridge.c/h` | Body 저장/주기 재송신 대신 router에서 직접 포워딩 |
| `CAN_ID_ENGINE_DATA` 통합 엔진 파싱 | `0x100`은 IGN 전용이고, RPM/speed/coolant는 별도 ID |

## CLI 확인

특정 ID만 보고 싶을 때:

```text
canlog id 3A0
canlog on
```

전체 라우팅 카운터는 1초 상태 로그의 `route`, `ok`, `fail`, `ignore` 필드로 확인합니다.
