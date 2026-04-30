# CAN Database

> 이 문서는 사람용 요약. 실제 코드는 [`common/signal_db.h`](../common/signal_db.h)를 사용.
> 원본 DBC는 [`docs/Golf_6_PQ35.dbc`](Golf_6_PQ35.dbc)와 K-Matrix Excel 문서입니다.

## CAN1 - Powertrain Bus (500kbps)

| CAN ID | 이름 | DLC | 주기 | 송신자 | 수신자 | 인코딩 |
|---|---|---|---|---|---|---|
| 0x280 | Motor_1 / RPM | 8 | 50ms | 보드A | 보드B | `Motordrehzahl`, start 16, len 16, scale 0.25 |
| 0x1A0 | Bremse_1 / Speed | 8 | 100ms | 보드A | 보드B | `BR1_Rad_kmh`, start 17, len 15, scale 0.01 |
| 0x288 | Motor_2 / Coolant | 8 | 1000ms | 보드A | 보드B | `MO2_Kuehlm_T`, start 8, len 8, scale 0.75, offset -48 |
| 0x300 | IGN/Keepalive | 8 | 100ms | 보드A | 보드D | byte[0] bit0 = IGN ON |
| 0x3A0 | ADAS_Status | 8 | 100ms | 보드E | 보드B | 프로젝트 내부 Safety 판단 프레임 |
| 0x390 | mGate_Komf_1 Body Status | 8 | 100ms | 보드D | 보드B | Golf6 DBC body bitfield |

## CAN2 - Diagnostic Bus (500kbps)

| CAN ID | 이름 | DLC | 주기 | 송신자 | 수신자 | 인코딩 |
|---|---|---|---|---|---|---|
| 0x280 | RPM (포워딩) | 8 | 50ms | 보드B | 계기판, 보드C | 동일 |
| 0x1A0 | Speed (포워딩) | 8 | 100ms | 보드B | 계기판, 보드C | 동일 |
| 0x288 | Coolant Temp (포워딩) | 8 | 1000ms | 보드B | 계기판, 보드C | 동일 |
| 0x480 | mMotor_5 Warning | 8 | 100ms while ADAS recent | 보드B | 계기판 | Golf6 DBC `mMotor_5` warning bit overlay |
| 0x390 | Body Status (포워딩) | 8 | 100ms | 보드B | 계기판, 보드C | 동일 |
| 0x050 | Airbag off | 8 | 100ms | 보드B | 계기판 | byte[1] = 0x80 |
| 0x714 | UDS Request | 8 | 이벤트 | 보드C CLI | 게이트웨이/타깃 | ISO-TP Single Frame |
| 0x77E | UDS Response | 8 | 이벤트 | 게이트웨이/타깃 | 보드C CLI | ISO-TP Single Frame |

## ADAS_Status (0x3A0)

`0x3A0`은 K-Matrix/Golf6 DBC에서는 `mBremse_10`으로도 쓰이므로, 이 프로젝트에서는 CAN1 내부 ADAS 프레임으로만 사용합니다. Gateway에서 CAN2로 원본 포워딩하려면 `GATEWAY_SAFETY_FORWARD_ADAS_STATUS=1`을 명시적으로 켜야 합니다.

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

## Golf6 Warning Mapping (0x480 mMotor_5)

K-Matrix/DBC 기준 `0x480`은 `mMotor_5`입니다. Gateway는 ADAS 상태를 아래 bit로 변환합니다.

| DBC Signal | Bit | 조건 |
|---|---:|---|
| `MO5_HLeuchte` | 40 | `risk_level >= 2` |
| `MO5_Heissl` | 12 | `risk_level >= 3` |
| `MO5_TDE_Lampe` | 44 | active fault 또는 sensor fault |
| `MO5_Motortext3` | 54 | active fault 또는 sensor fault |

## Bit Timing (500kbps @ STM32F429ZI)

| 파라미터 | 값 |
|---|---|
| Prescaler | 9 |
| BS1 | 4 tq 또는 8 tq |
| BS2 | 5 tq 또는 1 tq |
| SJW | 1 |
| Baudrate | 500kbps |

> 보드 A/B/C/D 모두 STM32F429ZI 기준이며, 보드 C는 리팩터링된 UDS CubeMX 설정을 기준으로 합니다.
