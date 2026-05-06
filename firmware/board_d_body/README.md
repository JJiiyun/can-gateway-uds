# Board D - Turn Signal ECU

Board D는 좌/우 방향지시등 입력만 담당합니다. 하이빔, 도어, 안개등 같은 body 기능은 이 보드에서 제거했습니다.

## 역할 요약

| 구분 | 내용 |
|---|---|
| 담당 | Left/Right 방향지시등 버튼 입력 |
| 참조 | IGN ON 상태 |
| 송신 | `0x531` 방향지시등 프레임만 송신 |
| 송신하지 않음 | `0x635` body 상태, 하이빔, 도어, 안개등, RPM, Speed, Coolant, ADAS |
| 주기 | 100ms |

## 참조 / 수신 데이터

Board D는 CAN1에서 IGN 상태만 참조합니다. RPM, Speed, Coolant 값은 읽어도 사용하지 않습니다.

| CAN ID | DLC | 송신원 | 참조 위치 | 의미 |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A Engine | `byte[5] bit0` | 프로젝트 EngineData의 IGN ON |
| `0x570` | 8 이상 | Golf6 body 계열 | `byte[0] bit1` | `mBSG_Last` Klemme 15 |
| `0x572` | 8 이상 | Golf6 ignition 계열 | `byte[0] bit1` | `mZAS_1` Klemme 15 |

마지막 유효 IGN 프레임 수신 후 500ms가 지나면 IGN OFF로 취급합니다. CLI에서 `body ign on`으로 벤치 테스트용 강제 송신도 가능합니다.

## 물리 입력

입력은 pull-up입니다. 버튼이 GND에 연결되면 active로 읽습니다.

| 기능 | 기본 핀 | 처리 |
|---|---|---|
| Left Turn Button | `PE6` | 누를 때마다 left enable 토글 |
| Right Turn Button | `PF6` | 누를 때마다 right enable 토글 |

제거된 입력:

| 기능 | 상태 |
|---|---|
| Door FL/FR/RL/RR | 사용하지 않음 |
| High Beam | 사용하지 않음 |
| Fog Light | 사용하지 않음 |

## 송신 데이터

### `0x531` Turn Status

| 항목 | 값 |
|---|---|
| CAN ID | `0x531` |
| DLC | 8 |
| 주기 | 100ms |
| 사용 byte | `byte[2]` |

| 위치 | 의미 |
|---|---|
| `byte[2] bit0` | Left turn blink 상태 |
| `byte[2] bit1` | Right turn blink 상태 |
| `byte[2] bit2` | Hazard 상태. left/right가 동시에 blink 중이면 1 |
| 나머지 byte/bit | 0 |

방향지시등은 enable 상태일 때 500ms 기준으로 ON/OFF blink 됩니다.

## UART CLI

기본 연결은 USART3/ST-LINK VCP, 115200 8N1입니다.

```text
body mode gpio
body mode uart

body ign auto
body ign on
body ign off

body turn left 1
body turn right 0
body turn both 0

body all off
body status
body monitor on 200
body monitor off
body monitor once
body reset
```

## 확인 포인트

1. Board A가 `0x100 byte[5] bit0 = 1`을 보내거나 CLI에서 `body ign on`을 설정합니다.
2. CAN1에서 `0x531`이 DLC 8로 100ms마다 나오는지 확인합니다.
3. L/R 버튼을 누르면 `0x531 byte[2] bit0/bit1`이 500ms 간격으로 깜빡이는지 확인합니다.
4. `0x635`는 Board D에서 더 이상 송신되지 않아야 합니다.

## 빌드

```bash
cmake --preset Debug --fresh -S firmware/board_d_body
cmake --build firmware/board_d_body/build/Debug --parallel
```
