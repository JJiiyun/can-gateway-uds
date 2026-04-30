# Board E - Safety / ADAS ECU

Board E는 초음파 센서 또는 가변저항, 버튼 입력으로 전방 추돌 위험, 차선 이탈, 급제동 위험, 후방 장애물, 센서 고장 상태를 만들고 CAN1에 `0x3A0 ADAS_Status`를 송신하는 Safety 판단 ECU입니다.

## 역할

```text
Board E Safety ECU
  PA0 ADC  -> front distance demo input
  PA3 ADC  -> rear distance demo input
  PE6 GPIO -> lane departure button
  PF6 GPIO -> harsh brake button
  PF7 GPIO -> sensor fault button
        |
        v
CAN1 0x3A0 ADAS_Status, 100ms
        |
        v
Board B Gateway
  risk_level >= 2 -> CAN2 0x480 mMotor_5 warning bits
  sensor_fault    -> UDS fault/DTC state
```

## DBC / K-Matrix 기준

- `docs/872521829-PQ35-46-ACAN-KMatrix-V5-20-6F-20160530-MH.xlsx`와 `docs/Golf_6_PQ35.dbc` 기준으로 Golf6 CAN `0x480`은 `mMotor_5`입니다.
- Gateway는 ADAS 위험 상태를 임의 byte0 bitfield가 아니라 `mMotor_5`의 warning 성격 bit로 변환합니다.
- `0x3A0`은 Golf6 K-Matrix에서 `mBremse_10`으로도 쓰입니다. 그래서 Board E의 `0x3A0 ADAS_Status`는 CAN1 내부 프로젝트 프레임으로 사용하고, Gateway의 CAN2 포워딩은 기본 OFF입니다.

## CAN1 `0x3A0 ADAS_Status`

| Byte | 의미 |
|---:|---|
| 0 | flags: bit0 front collision, bit1 lane departure, bit2 harsh brake, bit3 rear obstacle, bit4 sensor fault, bit5 active |
| 1 | risk level: 0 none, 1 info, 2 warning, 3 danger |
| 2 | front distance, cm, 0-250 |
| 3 | rear distance, cm, 0-250 |
| 4 | active fault bitmap |
| 5 | vehicle speed km/h, clamped to 255 |
| 6 | input bitmap: bit0 lane button, bit1 brake button, bit2 sensor fault button |
| 7 | alive counter |

## 판단 규칙

| 조건 | 결과 |
|---|---|
| speed >= 40km/h and front <= 30cm | front collision, risk 3 |
| speed >= 20km/h and front <= 50cm | front collision, risk 2 |
| lane button active | lane departure, risk 2 |
| harsh brake button active and speed >= 30km/h | harsh brake, risk 2 |
| rear <= 25cm | rear obstacle, risk 1 |
| sensor fault button active | sensor fault, fault bitmap, risk 2 |

## UDS 확장

Board B Gateway가 Board E 상태를 보관하고 Board C의 UDS CLI 요청에 응답합니다.

| CLI | DID | 응답 |
|---|---:|---|
| `read adas` | `0xF410` | flags, risk, front cm, rear cm |
| `read front` | `0xF411` | front distance cm |
| `read rear` | `0xF412` | rear distance cm |
| `read fault` | `0xF413` | active fault bitmap, latched DTC bitmap |
| `clear dtc` | SID `0x14` | Gateway latched ADAS DTC clear |

## 빌드

```bash
cmake --preset Debug --fresh -S firmware/board_e_safety
cmake --build firmware/board_e_safety/build/Debug --parallel
```

루트 통합 빌드에는 `board_e_safety`가 포함되어 있습니다.
