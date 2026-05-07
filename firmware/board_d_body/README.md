# Board D - Turn Signal ECU

Board D is the turn-signal ECU. It only owns left/right turn input and `0x531` Turn Status transmission. It does not generate RPM, speed, coolant, ADAS, door, high-beam, fog-lamp, or body-status frames.

## Current Contract

| Item | Value |
|---|---|
| CAN bus | CAN1 |
| Reference input | Board A `0x100` IGN Status from CAN |
| TX frame | `0x531` Turn Status |
| TX period | 100 ms |
| Blink period | 500 ms |
| Integration gate | Send turn status only while IGN is ON, unless CLI override is used |

## CAN RX

Board D reads CAN frames through the shared CAN BSP RX queue. It does not read Board A directly.

| CAN ID | DLC | Sender | Field | Meaning |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A | `byte[5] bit0` | IGN ON |
| `0x570` | >= 1 | Golf 6 compatible body frame | `byte[0] bit1` | Klemme 15 fallback |
| `0x572` | >= 1 | Golf 6 compatible ignition frame | `byte[0] bit1` | Klemme 15 fallback |

If the last valid IGN frame is older than 500 ms, Board D treats IGN as OFF.

## CAN TX

### `0x531` Turn Status

| Item | Value |
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

Board B's gateway router expects this frame on CAN1 and routes it to CAN2.

## Physical Inputs

Inputs are active low with pull-up. A pressed button connects the pin to GND.

| Function | Default pin | Behavior |
|---|---|---|
| Left Turn Button | `PA3` (A0) | Toggles left turn enable |
| Right Turn Button | `PC0` (A1) | Toggles right turn enable |
| Hazard Button | `PC3` (A2) | Toggles hazard (both) enable |

## UART CLI

Default UART is USART3 / ST-LINK VCP, 115200 8N1.

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
body monitor on 200
body monitor off
body monitor once
body reset
```

`body ign auto` uses CAN-derived IGN. `body ign on` and `body ign off` are bench-test overrides. The periodic `[BCM] INPUT` log plus `body status` and `body monitor` include raw GPIO state, where `1` means the active-low switch input is currently pressed.

## Verification

1. Confirm Board A sends CAN1 `0x100` with `byte[5] bit0 = 1`.
2. Confirm Board D logs or reports IGN ON in `body status`.
3. Press left/right input and verify CAN1 `0x531` appears every 100 ms.
4. Verify `byte[2] bit0` and `byte[2] bit1` blink with the 500 ms blink phase.
5. Verify Board B routes `0x531` from CAN1 to CAN2.

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
