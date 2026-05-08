# Board E - Safety / ADAS ECU

Board E is the Safety / ADAS ECU. It measures front/rear distance, uses vehicle speed from CAN, evaluates risk, and sends `0x3A0 ADAS_Status` to Board B.

Board E does not read Board A directly. It reads all reference data from the CAN1 RX queue.

## Current Contract

| Item | Value |
|---|---|
| CAN bus | CAN1 |
| Reference input | Board A `0x100` IGN Status |
| Speed input | Board A `0x1A0` and `0x5A0` speed frames |
| TX frame | `0x3A0 ADAS_Status` |
| TX period | 100 ms |
| Input poll period | 60 ms |
| Validity timeout | 500 ms for IGN and speed references |

## CAN RX

Board E reads CAN frames through `CAN_BSP_Read()`.

| CAN ID | DLC | Sender | Field | Use |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A | `byte[5] bit0` | IGN ON gate |
| `0x1A0` | 8 | Board A | `byte[2..3]` little-endian raw, `km/h * 80` | Vehicle speed reference |
| `0x5A0` | 8 | Board A | `byte[2]` | Vehicle speed reference |

Important behavior:

- `0x100` is treated as IGN Status only.
- Board E does not read speed from `0x100`.
- Vehicle speed is valid only while a recent `0x100` IGN ON frame exists.
- If IGN is OFF or no valid IGN frame is received for 500 ms, Board E reports speed as 0 for risk evaluation.
- If speed frames stop for 500 ms, Board E also reports speed as 0.

## CAN TX

### `0x3A0` ADAS_Status

Board B's `GatewaySafetyBridge` expects this exact payload on CAN1.

| Item | Value |
|---|---|
| CAN ID | `0x3A0` |
| DLC | 8 |
| Period | 100 ms |
| Sender | Board E |
| Receiver | Board B |

| Byte | Meaning |
|---:|---|
| `byte[0]` | flags |
| `byte[1]` | risk level |
| `byte[2]` | front distance cm |
| `byte[3]` | rear distance cm |
| `byte[4]` | active fault bitmap |
| `byte[5]` | vehicle speed km/h |
| `byte[6]` | input bitmap |
| `byte[7]` | alive counter |

`byte[0]` flags:

| Bit | Meaning |
|---:|---|
| 0 | front collision |
| 1 | lane departure, currently unused |
| 2 | harsh brake, currently unused |
| 3 | rear obstacle |
| 4 | sensor fault |
| 5 | ADAS active |

Risk levels:

| Value | Meaning |
|---:|---|
| 0 | none |
| 1 | info |
| 2 | warning |
| 3 | danger |

Fault bitmap:

| Bit | Meaning |
|---:|---|
| 0 | front sensor fault |
| 1 | rear sensor fault |
| 2 | button stuck, currently unused |
| 3 | message timeout, currently unused |

Input bitmap:

| Bit | Meaning |
|---:|---|
| 0 | lane button, currently unused |
| 1 | brake button, currently unused |
| 2 | sensor fault input |

## Risk Logic

| Condition | Result |
|---|---|
| `speed >= 40 km/h` and `front <= 30 cm` | front collision, risk 3 |
| `speed >= 20 km/h` and `front <= 50 cm` | front collision, risk 2 |
| `rear <= 25 cm` | rear obstacle, risk 1 |
| HC-SR04 timeout | sensor fault, fault bitmap, risk 2 |

Board B converts `risk_level >= 2`, sensor fault, or active fault bitmap into a CAN2 `0x480 mMotor_5` warning frame. For audible cluster feedback, Board B also pulses CAN2 `0x050 mAirbag_1` with `byte[2] = 0x04` while `risk_level >= 2`.

Audible warning cadence:

| Risk level | CAN2 `0x050` pulse |
|---:|---|
| 0 | off |
| 1 | off |
| 2 | `byte[2] = 0x04` for 100 ms every 600 ms |
| 3 | `byte[2] = 0x04` for 100 ms every 300 ms |

## Physical Inputs

### HC-SR04 Sensors

| Sensor | TRIG | ECHO | Use |
|---|---|---|---|
| Front | `PE2` | `PE3` | Front obstacle distance |
| Rear | `PE4` | `PE5` | Rear obstacle distance |

Distance is calculated from echo pulse width:

| Item | Value |
|---|---|
| Conversion | approximate `distance_cm = echo_us / 58` |
| Clamp range | 5 cm to 250 cm |
| Timeout | Sensor fault |

Electrical note: HC-SR04 ECHO is commonly 5 V. Use a voltage divider or level shifter so STM32 GPIO sees 3.3 V or less.

## UART CLI

Default UART is USART3 / ST-LINK VCP, 115200 8N1.

Boot-time debug logs are off by default. Use `adas status` for one-shot inspection, `adas monitor on 100` for periodic verification output, and `adas log on` only when the 1-second automatic CAN summary is needed.

```text
adas status

adas log on
adas log off
adas log stat

adas monitor on 100
adas monitor off
adas monitor once

adas event on
adas event off
adas event stat
```

Board E does not have a UART input override mode like Board D. Its ADAS inputs always come from the HC-SR04 sensors plus CAN-derived IGN/speed.

## Verification

1. Confirm Board A sends CAN1 `0x100` with `byte[5] bit0 = 1`.
2. Confirm Board A sends CAN1 `0x1A0` and/or `0x5A0` speed frames.
3. Confirm Board E sends CAN1 `0x3A0` every 100 ms.
4. Confirm `0x3A0 byte[5]` follows the CAN speed reference only while IGN is ON.
5. Create front/rear distance warning conditions and verify `flags`, `risk_level`, and `fault_bitmap` change.
6. Confirm Board B receives `0x3A0` and emits CAN2 `0x480` when risk/fault is active.

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
