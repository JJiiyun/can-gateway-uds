# Gateway App 설명서

Board B Gateway의 `app/` 코드는 CAN RX queue에서 프레임을 읽고, 라우팅 테이블에 등록된 CAN ID만 CAN1에서 CAN2로 포워딩합니다.

Gateway는 Board A의 RPM/Speed/Coolant payload를 해석해서 새 계기판 프레임을 만들지 않습니다. Board A가 계기판 반응 ID 기준으로 payload를 만들어 송신하고, Gateway는 ID 기준으로 선별 전달합니다.

## 현재 구현 파일

| 파일 | 역할 |
|---|---|
| `gateway_tasks.c` | FreeRTOS task 진입점. CAN 초기화, RX queue 처리, router/safety 호출, 상태 로그 출력 |
| `gateway_router.c/h` | 라우팅 테이블 기반 CAN1 -> CAN2 포워딩 |
| `gateway_safety_bridge.c/h` | Board E `0x3A0` ADAS 상태를 진단 상태로 저장하고 `0x480` warning 프레임 생성 |
| `can_bsp.c/h` | CAN1/CAN2 초기화, 송신, RX interrupt queue 처리 |
| `can_cli_monitor.c/h` | UART CLI CAN frame 로그 |
| `cli.c/h`, `uart.c/h` | UART CLI 기반 명령 처리 |
| `protocol_ids.h` | 현재 Gateway가 사용하는 CAN ID와 payload index 정의 |

## 런타임 흐름

```text
CAN1 RX interrupt
  -> can_rx_q
  -> GatewayTask
       -> CanCliMonitor_LogRx()
       -> GatewayRouter_OnRx()
       -> GatewaySafetyBridge_OnRx()
```

`ClusterTask`는 더 이상 Board A 데이터를 계기판 프레임으로 변환 송신하지 않습니다. 현재는 Safety warning의 10ms tick만 담당합니다.

Board C는 CAN2에 연결된 UDS client이고, 계기판이 UDS server입니다. Board B는 UDS 요청에 응답하지 않습니다.

## Board A 라우팅 대상

Board A EngineSim은 기존 `0x100` 통합 프레임 대신 아래 계기판 CAN ID를 직접 송신합니다.

| CAN ID | DLC | 의미 | 주요 payload |
|---:|---:|---|---|
| `0x100` | 8 | IGN 상태 | `byte[5] bit0 = IGN ON` |
| `0x280` | 8 | RPM 계기판 바늘 | `byte[2]~byte[3]` little-endian raw, `rpm = raw / 4` |
| `0x1A0` | 8 | Speed 바늘용 주기 프레임 | `byte[2]~byte[3]` little-endian raw, `speed = raw / 80` |
| `0x5A0` | 8 | Speed 보조 프레임 | `byte[2] = speed / 2`, Gateway monitor는 `* 2`로 복원 |
| `0x288` | 8 | 냉각수 바늘 | `byte[1]` raw, `coolant = (raw * 3 / 4) - 48` |
| `0x481` | 8 | Engine warning status | RPM/coolant warning bitfield + alive counter |

Gateway routing table에는 위 6개 ID가 CAN1 -> CAN2로 등록되어 있습니다.

## 추가 라우팅 대상

Board D/계기판 테스트용으로 아래 ID도 CAN1 -> CAN2 라우팅 대상에 유지합니다.

| CAN ID | 의미 |
|---:|---|
| `0x390` | Board D Body `mGate_Komf_1` |
| `0x531` | 방향지시등 관련 프레임 |
| `0x635` | Body 밝기 조절 후보 프레임 |

## 삭제된 이전 구조

아래 구조는 제거되었습니다.

| 이전 구조 | 제거 이유 |
|---|---|
| `CAN_ID_ENGINE_DATA = 0x100` | Board A가 더 이상 통합 엔진 프레임을 송신하지 않음 |
| `CAN_ENGINE_DATA_*` index/mask | `0x100` 패킹 해석용이라 현재 Gateway protocol에서 불필요 |
| `gateway_engine_bridge.c/h` | Gateway가 엔진 데이터를 해석해 `0x280/0x1A0`을 생성하지 않음 |
| `gateway_body_bridge.c/h` | Body도 별도 저장/주기 재송신 대신 router에서 직접 포워딩 |
| `gateway_uds_server.c/h` | Board C가 CAN2에서 계기판 UDS server와 직접 통신하므로 Board B 응답 불필요 |

## CLI 확인

특정 ID만 보고 싶을 때:

```text
canlog id 280
canlog on
```

전체 라우팅 카운터는 1초 상태 로그의 `route`, `ok`, `fail`, `ignore` 필드로 확인합니다.
