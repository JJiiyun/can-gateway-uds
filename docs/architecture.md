# System Architecture

## 전체 구조

```text
┌──────────────────────────┐      ┌────────────────────┐      ┌────────────────────┐
│   Board A Engine ECU     │      │ Board D Turn ECU   │      │ Board E ADAS ECU   │
│   - throttle/brake ADC   │      │ - 0x100 IGN RX     │      │ - 0x100 IGN RX     │
│   - 0x100 IGN TX         │      │ - 0x531 Turn TX    │      │ - 0x5A0 speed RX   │
│   - 0x280/1A0/5A0/288 TX │      │ - 0x635 Bright TX  │      │ - 0x3A0 ADAS TX    │
│   - 0x481 Warning TX     │      │                    │      │ - HC-SR04 sensors  │
└────────────┬─────────────┘      └─────────┬──────────┘      └─────────┬──────────┘
             │ CAN1 Powertrain / Body / Safety Bus, 500 kbps            │
             └──────────────────────────────┬───────────────────────────┘
                                            │
                              ┌─────────────▼─────────────┐
                              │  Board B Central Gateway  │
                              │  - CAN1 -> CAN2 router    │
                              │  - ADAS diagnostic state  │
                              │  - 0x5D6 parking sweep    │
                              │  - Gateway ADAS UDS       │
                              └─────────────┬─────────────┘
                                            │ CAN2 Cluster / Diagnostic Bus, 500 kbps
                  ┌─────────────────────────┴─────────────────────────┐
                  │                                                   │
                  ▼                                                   ▼
          ┌──────────────────┐                              ┌──────────────────┐
          │ Board C UDS      │                              │ Golf 6 Cluster   │
          │ - CAN1 peripheral│                              │ - routed frames  │
          │ - connected to   │                              │ - 0x5D6 receive  │
          │   Gateway CAN2   │                              │ - UDS target     │
          └──────────────────┘                              └──────────────────┘
```

## 도메인 분리

| 도메인 | 버스 | 메시지 |
|---|---|---|
| Powertrain | CAN1 | Board A `0x100`, `0x280`, `0x1A0`, `0x5A0`, `0x288`, `0x481` |
| Body | CAN1 -> CAN2 | Board D `0x531` 방향지시등, `0x635` 밝기 |
| Safety | CAN1 | Board E `0x3A0 ADAS_Status`, risk/fault 판단 |
| Gateway | CAN1 + CAN2 | 라우팅 테이블, ADAS DTC 저장, `0x5D6` 생성, Gateway ADAS UDS server |
| Diagnostic/Cluster | CAN2 | Board C `0x714/0x77E`, 계기판 수신 프레임 |

## FreeRTOS Task 구조

### Board A - Engine ECU

| Task | 역할 |
|---|---|
| `defaultTask` | UART/CLI/CAN/EngineSim 초기화 |
| `EngSimTask` | 50 ms 모델 업데이트, CAN 송신 |

### Board B - Gateway

| Task | 역할 |
|---|---|
| `StartDefaultTask` | UART/CLI/CAN 초기화 |
| `GatewayTask` | CAN RX queue 처리, router/safety/UDS 호출 |
| `ClusterTask` | Safety Bridge 10 ms tick, `0x5D6` 송신 |
| `LoggerTask` | 1초 상태 로그 |

### Board C - UDS Client

| Task | 역할 |
|---|---|
| `UDSMainTask` | UART/CLI/CAN 초기화, UDS response polling |

### Board D - Turn Signal ECU

| Task | 역할 |
|---|---|
| `EngineSim_Task` override | Body main loop. `0x531` 송신과 `0x635` fade/hold |
| `bcmInput` | 20 ms GPIO polling |
| `bcmIgnRx` | CAN RX pending, IGN 상태 갱신 |

### Board E - Safety / ADAS ECU

| Task | 역할 |
|---|---|
| `EngineSim_Task` override | ADAS main loop. `0x3A0` 100 ms 송신 |
| `adasInput` | 60 ms HC-SR04 polling |

## 데이터 흐름 예시 - 페달 입력에서 계기판 반응까지

1. 사용자가 Board A throttle/brake를 조작합니다.
2. Board A가 RPM/속도/냉각수 모델을 갱신합니다.
3. Board A가 CAN1에 `0x280`, `0x1A0`, `0x5A0`, `0x288`, `0x481`을 송신합니다.
4. Board B `GatewayTask`가 CAN RX queue에서 프레임을 읽습니다.
5. `GatewayRouter_OnRx()`가 라우팅 테이블을 조회하고 매칭된 프레임을 CAN2로 포워딩합니다.
6. Golf 6 Cluster는 CAN2의 `0x280`, `0x5A0`, `0x288` 등을 수신해 표시합니다.
7. Board B logger는 monitor 값으로 rpm, speed, coolant, ignition, warning 상태를 출력합니다.

## 데이터 흐름 예시 - 방향지시등과 밝기

1. Board A가 `0x100 byte[5] bit0 = 1`을 송신합니다.
2. Board D가 CAN RX queue에서 IGN ON을 인식합니다.
3. IGN OFF -> ON edge에서 Board D가 `0x635 byte[0]`을 `0x00`에서 `0x64`까지 fade 송신합니다.
4. 사용자가 좌/우/비상 버튼을 누르면 Board D가 `0x531 byte[2]`의 해당 bit를 500 ms phase로 blink합니다.
5. Board B가 `0x531`, `0x635`를 CAN2로 포워딩합니다.

## 데이터 흐름 예시 - ADAS 위험에서 UDS 조회까지

1. Board A가 CAN1 `0x100` IGN과 `0x5A0` speed reference를 송신합니다.
2. Board E가 HC-SR04 거리와 CAN-derived speed를 기준으로 risk를 계산합니다.
3. Board E가 CAN1 `0x3A0 ADAS_Status`를 100 ms 주기로 송신합니다.
4. Board B Safety Bridge가 `0x3A0`을 진단 상태로 저장하고 DTC bitmap을 누적합니다.
5. `risk_level >= 2`이면 Board B가 CAN2 `0x5D6` Parking Assist sweep을 송신합니다.
6. Board C에서 `read adas`, `read fault`, `clear dtc`를 실행하면 Board B Gateway UDS server가 `0x77E`로 응답합니다.

## 주의할 점

- Board B는 현재 ADAS 상태를 `0x480` 또는 `0x050`으로 송신하지 않습니다.
- Board D는 현재 `0x390 mGate_Komf_1` body-status를 송신하지 않습니다.
- Board C의 CAN1은 통합 시스템에서는 물리적으로 Gateway CAN2 버스에 연결되는 진단 포트 역할입니다.
- Board B Gateway UDS server와 계기판 UDS가 같은 `0x714/0x77E` ID를 쓸 수 있으므로, 순수 계기판 진단 시에는 연결 구성을 확인해야 합니다.
