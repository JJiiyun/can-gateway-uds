# CAN Database

이 문서는 현재 펌웨어 구현 기준의 사람용 CAN 요약입니다. DBC 원본은 `docs/Golf_6_PQ35.dbc`를 참고하고, 실제 코드의 CAN ID와 payload index는 각 보드의 `protocol_ids.h`와 송수신 구현을 우선합니다.

## 현재 구현에서 확정된 점

- `0x100`은 통합 EngineData가 아니라 IGN Status 전용입니다.
- `0x1A0`은 현재 Board A가 zero-filled keepalive로 송신합니다.
- 현재 속도 기준은 `0x5A0 byte[2] = km/h / 2`입니다.
- Board D는 현재 `0x390`을 송신하지 않고 `0x531`, `0x635`만 송신합니다.
- Board B는 ADAS를 현재 `0x480` 또는 `0x050`으로 송신하지 않습니다. 위험도 2 이상에서 생성하는 CAN2 프레임은 `0x5D6`입니다.

## CAN1 - Powertrain / Body / Safety Bus (500 kbps)

| CAN ID | 이름 | DLC | 주기 | 송신자 | 수신자 | 현재 인코딩 |
|---:|---|---:|---:|---|---|---|
| `0x100` | IGN Status | 8 | 50 ms | Board A | Board B/D/E | `byte[5] bit0 = IGN ON` |
| `0x280` | Motor_1 / RPM | 8 | 50 ms | Board A | Board B | `byte[2..3] = rpm * 4`, little-endian |
| `0x1A0` | Speed keepalive | 8 | 50 ms | Board A | Board B/E | 현재 all zero. legacy non-zero는 `byte[2..3] / 80`으로 해석 가능 |
| `0x5A0` | Speed reference | 8 | 50 ms | Board A | Board B/E | `byte[2] = km/h / 2`, warning chime request는 `byte[4] bit4..5` |
| `0x288` | Motor_2 / Coolant | 8 | 100 ms | Board A | Board B | `byte[1] = ((degC + 48) * 4) / 3` |
| `0x481` | Engine Warning Status | 8 | 100 ms | Board A | Board B | `byte[0]` warning bits, `byte[1]` coolant, `byte[2..3]` rpm |
| `0x531` | Turn Status | 8 | 100 ms | Board D | Board B | `byte[2] bit0 left`, `bit1 right`, `bit2 hazard` |
| `0x635` | Cluster Brightness | 8 | fade 20 ms, hold 100 ms | Board D | Board B | `byte[0] = 0..100` |
| `0x3A0` | ADAS_Status | 8 | 100 ms | Board E | Board B | 프로젝트 내부 ADAS 판단 프레임 |
| `0x390` | Legacy Body Status | 8 | N/A | legacy | Board B | Router에는 남아 있지만 현재 Board D 송신 없음 |

## CAN2 - Cluster / Diagnostic Bus (500 kbps)

| CAN ID | 이름 | DLC | 주기 | 송신자 | 수신자 | 현재 동작 |
|---:|---|---:|---:|---|---|---|
| `0x100` | IGN Status routed | 8 | source 주기 | Board B | CAN2 bus | CAN1 `0x100` 포워딩 |
| `0x280` | RPM routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x280` 포워딩 |
| `0x1A0` | Speed keepalive routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x1A0` 포워딩 |
| `0x5A0` | Speed reference routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x5A0` 포워딩 |
| `0x288` | Coolant routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x288` 포워딩 |
| `0x481` | Engine Warning routed | 8 | source 주기 | Board B | CAN2 bus | CAN1 `0x481` 포워딩 |
| `0x531` | Turn Status routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x531` 포워딩 |
| `0x635` | Brightness routed | 8 | source 주기 | Board B | Cluster | CAN1 `0x635` 포워딩 |
| `0x390` | Legacy Body routed | 8 | source 주기 | Board B | CAN2 bus | CAN1 `0x390` 수신 시 포워딩 |
| `0x5D6` | Parking Assist sweep | 8 | 10/5/2 ms sweep | Board B | Cluster | ADAS `risk_level >= 2`일 때 생성 |
| `0x714` | UDS Request | 8 | 이벤트 | Board C | Gateway/Cluster | ISO-TP request |
| `0x77E` | UDS Response | 8 | 이벤트 | Gateway/Cluster | Board C | ISO-TP response |

## `0x3A0` ADAS_Status

`0x3A0`은 K-Matrix/Golf6 DBC에서는 다른 brake-related message로도 쓰일 수 있으므로, 이 프로젝트에서는 CAN1 내부 ADAS 프레임으로만 사용합니다. Gateway에서 CAN2로 원본 포워딩하려면 `GATEWAY_SAFETY_FORWARD_ADAS_STATUS=1`을 명시적으로 켜야 합니다.

| Byte | 의미 |
|---:|---|
| 0 | flags: bit0 front collision, bit1 lane departure, bit2 harsh brake, bit3 rear obstacle, bit4 sensor fault, bit5 active |
| 1 | risk level: 0 none, 1 info, 2 warning, 3 danger |
| 2 | front distance cm |
| 3 | rear distance cm |
| 4 | active fault bitmap |
| 5 | vehicle speed km/h |
| 6 | input bitmap |
| 7 | alive counter |

## Gateway ADAS Output

현재 `gateway_safety_bridge.c`의 실제 출력은 `0x5D6`입니다.

| 조건 | CAN2 출력 |
|---|---|
| `risk_level < 2` | off. 위험 해제 시 0 payload를 한 번 송신 |
| `risk_level = 2` | `0x5D6` `byte[1]` sweep, 기본 10 ms 간격 |
| `risk_level = 3` | `0x5D6` `byte[1]` sweep, 기본 5 ms 간격 |
| `risk_level >= 4` | `0x5D6` `byte[1]` sweep, 기본 2 ms 간격 |

`0x480 mMotor_5`와 `0x050 mAirbag_1` 관련 define은 남아 있지만 현재 구현에서 송신 경로가 없습니다.

## Bit Timing (500 kbps @ STM32F429ZI)

| 파라미터 | 값 |
|---|---|
| Prescaler | 9 |
| BS1 | 4 tq 또는 8 tq |
| BS2 | 5 tq 또는 1 tq |
| SJW | 1 |
| Baudrate | 500 kbps |
