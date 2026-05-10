# Board E - Safety / ADAS ECU

Board E는 Safety / ADAS ECU입니다. 전방/후방 HC-SR04 거리 센서를 읽고, CAN에서 받은 IGN/차속 기준으로 위험도를 판단한 뒤 CAN1 `0x3A0 ADAS_Status`를 Board B에 송신합니다.

## 현재 계약

| 항목 | 값 |
|---|---|
| CAN bus | CAN1 |
| 기준 입력 | Board A `0x100` IGN Status |
| Speed input | Board A `0x5A0` speed reference. 오래된 non-zero `0x1A0` speed frame도 호환 수신 |
| TX frame | `0x3A0 ADAS_Status` |
| TX period | 100 ms |
| Input poll period | 60 ms |
| Validity timeout | IGN과 speed reference 모두 500 ms |

Board E는 Board A 내부 변수를 직접 읽지 않고, 모든 기준 데이터를 CAN1 RX queue에서 받습니다.

## CAN RX

| CAN ID | DLC | Sender | Field | Use |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A | `byte[5] bit0` | IGN ON gate |
| `0x5A0` | 8 | Board A | `byte[2]`, `km/h / 2` | 현재 speed reference |
| `0x1A0` | 8 | Board A 또는 legacy sender | `byte[2..3]`, `km/h * 80` | non-zero legacy speed frame만 반영 |

중요 동작:

- `0x100`은 IGN Status 전용으로 처리합니다.
- Board E는 `0x100`에서 속도를 읽지 않습니다.
- 현재 Board A의 `0x1A0`은 zero-filled keepalive이므로, Board E는 이 값으로 최신 `0x5A0` 속도를 0으로 덮지 않습니다.
- 유효한 IGN ON 프레임이 500 ms 안에 없으면 위험도 계산용 speed는 0입니다.
- speed frame이 500 ms 이상 끊겨도 speed는 0입니다.

## CAN TX

### `0x3A0` ADAS_Status

Board B의 `GatewaySafetyBridge`는 이 payload를 CAN1에서 기대합니다.

| 항목 | 값 |
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

Board B는 현재 `risk_level >= 2`일 때 CAN2 `0x5D6` Parking Assist 프레임을 sweep 송신합니다. 문서나 과거 코드 조각에 보이는 `0x480 mMotor_5` 또는 `0x050 mAirbag_1` 변환 송신은 현재 Safety Bridge 구현에 없습니다.

## Physical Inputs

### HC-SR04 Sensors

현재 코드의 기본 핀입니다.

| Sensor | TRIG | ECHO | Use |
|---|---|---|---|
| Front | `PF15` | `PF14` | Front obstacle distance |
| Rear | `PE11` | `PE9` | Rear obstacle distance |

거리 계산:

| 항목 | 값 |
|---|---|
| Conversion | approximate `distance_cm = (echo_us + 29) / 58` |
| Clamp range | 5 cm to 250 cm |
| Timeout | Sensor fault |

전기적 주의: HC-SR04 ECHO는 보통 5 V입니다. STM32 GPIO에는 3.3 V 이하가 들어가도록 전압 분배기 또는 level shifter를 사용해야 합니다.

## UART CLI

기본 UART는 USART3 / ST-LINK VCP, 115200 8N1입니다.

부팅 시 debug log는 기본 off입니다. `adas status`로 단발 확인, `adas monitor on 100`으로 주기 확인, `adas log on`으로 1초 CAN summary를 켭니다.

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

Board E에는 Board D 같은 UART 입력 override 모드가 없습니다. ADAS 입력은 항상 HC-SR04 센서와 CAN-derived IGN/speed에서 옵니다.

## Verification

1. Board A가 CAN1 `0x100 byte[5] bit0 = 1`을 송신하는지 확인합니다.
2. Board A가 CAN1 `0x5A0` speed reference를 송신하는지 확인합니다.
3. Board E가 CAN1 `0x3A0`을 100 ms마다 송신하는지 확인합니다.
4. `0x3A0 byte[5]`가 IGN ON이고 speed reference가 최근일 때만 CAN speed를 따라가는지 확인합니다.
5. 전방/후방 거리 경고 조건을 만들고 `flags`, `risk_level`, `fault_bitmap`이 변하는지 확인합니다.
6. Board B가 `0x3A0`을 수신하고, `risk_level >= 2`에서 CAN2 `0x5D6`을 송신하는지 확인합니다.
7. Board C에서 `read adas`, `read fault`, `clear dtc`로 Gateway ADAS 상태를 조회/초기화합니다.

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
