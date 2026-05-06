# Board E - Safety / ADAS ECU

Board E는 전방/후방 HC-SR04 초음파 센서와 Board A의 차량 속도를 참조해서 위험 신호를 생성합니다. 차선 이탈, 급제동 같은 버튼 기반 기능은 제거했습니다.

## 역할 요약

| 구분 | 내용 |
|---|---|
| 담당 | 전방/후방 거리 측정, 속도 기반 전방 위험 판단, 후방 장애물 판단 |
| 참조 | Board A `0x100`의 speed와 IGN |
| 송신 | `0x5A0` ADAS 상태 프레임 |
| 송신하지 않음 | RPM, speedometer용 speed, coolant, 방향지시등, body 상태, 차선 이탈, 급제동 |
| 주기 | 입력 poll 60ms, CAN 송신 100ms |

## 참조 / 수신 데이터

Board E는 CAN1에서 Board A EngineData를 받아 내부 위험 판단에만 사용합니다. 이 속도 값을 다시 속도계용 CAN 값으로 보내지는 않습니다.

| CAN ID | DLC | 송신원 | 참조 위치 | 사용 목적 |
|---:|---:|---|---|---|
| `0x100` | 8 | Board A Engine | `byte[2..3]` little-endian speed | 전방 충돌 위험 판단 기준 |
| `0x100` | 8 | Board A Engine | `byte[5] bit0` IGN ON | IGN ON일 때만 speed 참조값 갱신 |

마지막 유효 EngineData 수신 후 500ms가 지나면 speed 참조값은 0km/h로 취급합니다.

## 물리 입력

### HC-SR04 초음파 센서

기본 설정은 HC-SR04 2개입니다.

| 센서 | TRIG | ECHO | 용도 |
|---|---|---|---|
| Front | `PE2` | `PE3` | 전방 거리 |
| Rear | `PE4` | `PE5` | 후방 거리 |

거리 계산:

| 항목 | 내용 |
|---|---|
| 측정값 | HC-SR04 echo pulse width |
| 변환 | `distance_cm = echo_us / 58` 근사 |
| 유효 범위 | 5cm~250cm로 clamp |
| timeout | 전방 또는 후방 센서 timeout 시 sensor fault |

전기적 주의:

HC-SR04 ECHO는 보통 5V 출력입니다. STM32F429ZI GPIO에는 전압분배 또는 레벨시프터로 3.3V 이하로 낮춰서 연결해야 합니다.

### 제거된 입력

| 기능 | 상태 |
|---|---|
| Lane departure button | 사용하지 않음 |
| Harsh brake button | 사용하지 않음 |
| Sensor fault 강제 버튼 | 사용하지 않음 |

## 내부 판단 규칙

| 조건 | 결과 |
|---|---|
| speed >= 40km/h and front <= 30cm | front collision, risk 3 |
| speed >= 20km/h and front <= 50cm | front collision, risk 2 |
| rear <= 25cm | rear obstacle, risk 1 |
| HC-SR04 timeout | sensor fault, fault bitmap, risk 2 |

## 송신 데이터

### `0x5A0` ADAS Status

| 항목 | 값 |
|---|---|
| CAN ID | `0x5A0` |
| DLC | 8 |
| 주기 | 100ms |

| Byte | 의미 |
|---:|---|
| `byte[0]` | front distance cm |
| `byte[1]` | rear distance cm |
| `byte[2]` | reserved, 항상 0. speed는 Board A/B 책임 |
| `byte[3]` | warning bitfield |
| `byte[4]` | gong one-shot bitfield |
| `byte[5]` | risk level: 0 none, 1 info, 2 warning, 3 danger |
| `byte[6]` | active fault bitmap |
| `byte[7]` | alive counter |

`byte[3]` warning bitfield:

| Bit | 의미 |
|---:|---|
| 0 | front collision |
| 1 | unused, 항상 0 |
| 2 | unused, 항상 0 |
| 3 | rear obstacle |
| 4 | sensor fault |

`byte[4]` gong bitfield:

| Bit | 의미 |
|---:|---|
| 4 | warning gong one-shot. risk가 0/1에서 2로 올라갈 때 1회 |
| 5 | danger gong one-shot. risk가 0/1/2에서 3으로 올라갈 때 1회 |

## 확인 포인트

1. Board A가 `0x100`을 송신하면 Board E는 `byte[2..3]` speed를 내부 판단에만 사용합니다.
2. HC-SR04 front/rear 거리에 따라 `0x5A0 byte[0]`, `byte[1]`이 바뀌는지 확인합니다.
3. `0x5A0 byte[2]`는 항상 0인지 확인합니다.
4. 전방 거리와 speed 조건을 만들면 `byte[3] bit0`, `byte[5]` risk level이 바뀌는지 확인합니다.
5. 후방 거리가 25cm 이하가 되면 `byte[3] bit3`이 set되는지 확인합니다.
6. 차선/급제동 관련 `byte[3] bit1/bit2`는 항상 0이어야 합니다.

## 빌드

```bash
cmake --preset Debug --fresh -S firmware/board_e_safety
cmake --build firmware/board_e_safety/build/Debug --parallel
```
