# Board D - Turn Signal ECU

Board D는 body/turn-signal ECU입니다. 현재 구현 범위는 좌/우/비상 방향지시등 입력, `0x531` Turn Status 송신, IGN ON edge에서 `0x635` 계기판 밝기 fade/hold 송신입니다. RPM, speed, coolant, ADAS, door, high-beam, fog-lamp, `0x390` body-status 프레임은 현재 Board D가 생성하지 않습니다.

## 현재 계약

| 항목 | 값 |
|---|---|
| CAN bus | CAN1 |
| 기준 입력 | Board A `0x100` IGN Status |
| 보조 IGN 입력 | Golf6 호환 `0x570`/`0x572` Klemme 15 fallback |
| TX frames | `0x531` Turn Status, `0x635` Cluster Brightness |
| TX period | `0x531`: 100 ms, `0x635` hold: 100 ms with 50 ms offset |
| Blink period | 500 ms |
| Integration gate | CLI override가 없으면 IGN ON일 때만 turn status 송신 |
| Brightness fade/hold | IGN OFF -> ON edge에서 `0x635 byte[0]`을 `0x00`에서 `0x64`까지 올린 뒤 IGN ON 동안 `0x64` 유지 |

## CAN RX

Board D는 shared CAN BSP RX queue를 통해 CAN 프레임을 읽습니다. Board A 변수를 직접 참조하지 않습니다.

| CAN ID | DLC | Sender | Field | Meaning |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A | `byte[5] bit0` | IGN ON |
| `0x570` | >= 1 | Golf6 compatible body frame | `byte[0] bit1` | Klemme 15 fallback |
| `0x572` | >= 1 | Golf6 compatible ignition frame | `byte[0] bit1` | Klemme 15 fallback |

마지막 유효 IGN 프레임이 500 ms 이상 갱신되지 않으면 IGN OFF로 처리합니다.

## CAN TX

### `0x531` Turn Status

| 항목 | 값 |
|---|---|
| CAN ID | `0x531` |
| DLC | 8 |
| Period | 100 ms |
| Used byte | `byte[2]` |

| Byte / Bit | Meaning |
|---|---|
| `byte[2] bit0` | Left turn blink ON |
| `byte[2] bit1` | Right turn blink ON |
| `byte[2] bit2` | Hazard ON when left and right blink together |
| other bits | 0 |

Board B router는 이 프레임을 CAN1에서 CAN2로 포워딩합니다.

### `0x635` Cluster Brightness Fade

| 항목 | 값 |
|---|---|
| CAN ID | `0x635` |
| DLC | 8 |
| Trigger | boot 후 또는 IGN timeout/off 후 첫 IGN OFF -> ON edge |
| Step period | fade 중 20 ms |
| Hold period | IGN ON 동안 100 ms, turn task와 50 ms offset |
| Used byte | `byte[0]` |

| Byte | Meaning |
|---|---|
| `byte[0]` | Cluster brightness level, `0x00` to `0x64` |
| `byte[1]..byte[7]` | 0 |

CAN arbitration 때문에 `0x531`과 버스 충돌은 나지 않지만, 50 ms software offset으로 두 주기 송신이 같은 scheduler tick에 몰리는 일을 줄입니다.

## Physical Inputs

입력은 pull-up 기반 active-low입니다. 버튼을 누르면 해당 핀이 GND에 연결됩니다.

| Function | Default pin | Behavior |
|---|---|---|
| Left Turn Button | `PA3` (A0) | left turn enable toggle |
| Right Turn Button | `PC0` (A1) | right turn enable toggle |
| Hazard Button | `PC3` (A2) | hazard enable toggle |

## UART CLI

기본 UART는 USART3 / ST-LINK VCP, 115200 8N1입니다.

```text
body mode gpio
body mode uart

body ign auto
body ign on
body ign off

body turn left 1
body turn right 0
body turn hazard 1
body turn both 0

body all off
body status
body log on
body log off
body log stat
body monitor on 200
body monitor off
body monitor once
body reset
```

`body ign auto`는 CAN에서 얻은 IGN을 사용합니다. `body ign on/off`는 bench-test override입니다. 부팅 시 raw debug log는 기본 off이고, `body status`나 `body monitor on 100`으로 상태를 확인합니다. raw GPIO 상태에서 `1`은 active-low 스위치가 눌린 상태입니다.

## Verification

1. Board A가 CAN1 `0x100 byte[5] bit0 = 1`을 송신하는지 확인합니다.
2. `body status`에서 Board D가 IGN ON을 인식하는지 확인합니다.
3. CAN1 `0x635`가 `00 00 00 00 00 00 00 00`에서 `64 00 00 00 00 00 00 00`까지 올라가고, 이후 `64`를 100 ms마다 유지하는지 확인합니다.
4. 좌/우 입력을 누르고 CAN1 `0x531`이 100 ms마다 나타나는지 확인합니다.
5. `byte[2] bit0`과 `byte[2] bit1`이 500 ms blink phase에 맞춰 변하는지 확인합니다.
6. Board B가 `0x531`과 `0x635`를 CAN1에서 CAN2로 포워딩하는지 확인합니다.

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
