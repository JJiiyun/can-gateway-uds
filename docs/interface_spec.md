# Interface Specification

이 문서는 현재 펌웨어 구현 기준의 보드 간 계약입니다. 코드와 문서가 다를 때는 각 보드의 실제 송수신 구현과 `protocol_ids.h`를 우선합니다.

## 공통 전제

| 항목 | 현재 기준 |
|---|---|
| MCU | STM32F429ZI |
| CAN bitrate | 500 kbps |
| CAN1 | Board A/D/E/B가 공유하는 차량 내부 버스 |
| CAN2 | Board B가 계기판/Board C와 연결하는 진단/계기판 버스 |
| Board C | 코드상 CAN1 사용, 통합 시 물리적으로 Board B CAN2 버스에 연결 |

## 공통 CAN BSP

```c
HAL_StatusTypeDef CAN_BSP_Init(void);
HAL_StatusTypeDef CAN_BSP_Send(uint32_t id, uint8_t *data, uint8_t len);
HAL_StatusTypeDef CAN_BSP_SendTo(CAN_HandleTypeDef *hcan, uint32_t id, uint8_t *data, uint8_t len);
bool CAN_BSP_Read(CAN_RxMessage_t *p_msg, uint32_t timeout);
HAL_StatusTypeDef CAN_BSP_GetRxMessage(CAN_RxMessage_t *p_msg);
```

- Board A/C/D/E는 CAN1 중심으로 사용합니다.
- Board B는 `BOARD_B_GATEWAY` 정의로 CAN1과 CAN2를 모두 활성화합니다.
- RX는 HAL callback에서 RTOS queue로 들어가며, 보드 앱은 `CAN_BSP_Read()`로 읽습니다.

## Board A - EngineSim

```c
void EngineSim_Init(void);
void EngineSim_Task(void *argument);
void EngineSim_SetThrottle(uint8_t throttle);
void EngineSim_GetStatus(EngineSimStatus_t *status);
```

책임:

- CAN1 `0x100` IGN Status를 50 ms마다 송신합니다.
- CAN1 `0x280`, `0x1A0`, `0x5A0`, `0x288`, `0x481`을 송신합니다.
- 현재 `0x1A0`은 zero-filled keepalive입니다.
- 현재 speed reference는 `0x5A0 byte[2] = km/h / 2`입니다.

## Board B - Gateway

```c
void GatewayRouter_OnRx(const CAN_RxMessage_t *rx_msg);
void GatewaySafetyBridge_OnRx(const CAN_RxMessage_t *rx_msg);
void GatewaySafetyBridge_Task10ms(void);
void GatewayUdsServer_OnRx(const CAN_RxMessage_t *rx_msg);
```

책임:

- CAN1의 라우팅 대상 ID를 CAN2로 포워딩합니다.
- 라우팅 대상은 `0x100`, `0x280`, `0x1A0`, `0x5A0`, `0x288`, `0x481`, `0x531`, `0x635`, legacy `0x390`입니다.
- Board E `0x3A0 ADAS_Status`를 수신해 진단 상태와 DTC bitmap을 갱신합니다.
- `risk_level >= 2`일 때 CAN2 `0x5D6` Parking Assist sweep을 송신합니다.
- CAN2 `0x714` Gateway ADAS DID 요청과 Clear DTC 요청에 `0x77E`로 응답합니다.

현재 미구현 또는 비활성:

- ADAS 상태를 CAN2 `0x480 mMotor_5`로 변환 송신하지 않습니다.
- ADAS 상태를 CAN2 `0x050 mAirbag_1`로 변환 송신하지 않습니다.
- Board E 원본 `0x3A0`은 기본 설정에서 CAN2로 포워딩하지 않습니다.

## Board C - UDS Client

```c
void UDS_Execute_Diagnostic(uint16_t did, const char *label);
void UDS_Execute_ClearDtc(void);
bool UDS_Client_ReadDID(uint16_t did);
bool UDS_Client_ClearDtc(void);
```

책임:

- UART CLI `read vin|part|sw|swver|serial|hw|system|adas|front|rear|fault`를 UDS request로 송신합니다.
- 기본 request ID는 `0x714`, response ID는 `0x77E`입니다.
- First Frame 응답을 받으면 Flow Control CTS를 자동 송신하고 Consecutive Frame을 조립합니다.
- `clear dtc`는 `14 FF FF FF`를 송신합니다.

주의:

- `read rpm`, `read speed`, `read temp`, `read all`은 현재 수동 CAN listen/debug 모드입니다.
- Gateway ADAS server와 계기판이 같은 `0x714/0x77E` ID를 쓸 수 있으므로, unsupported DID에 대한 Gateway NRC가 같이 보일 수 있습니다.

## Board D - Turn Signal ECU

```c
void BCM_Body_Init(void);
void BCM_Body_Task(void *argument);
uint8_t BCM_Body_IsIgnOn(void);
void BCM_Body_SetIgnOverride(int8_t override);
```

책임:

- CAN1 `0x100 byte[5] bit0` 또는 Golf6 fallback `0x570`/`0x572 byte[0] bit1`로 IGN 상태를 갱신합니다.
- IGN ON일 때 CAN1 `0x531` Turn Status를 100 ms마다 송신합니다.
- IGN OFF -> ON edge에서 CAN1 `0x635` Brightness fade를 송신하고, 이후 100 ms 주기로 hold합니다.
- GPIO 또는 UART override로 좌/우/비상 입력을 제어합니다.

현재 미구현:

- `0x390 mGate_Komf_1` Body Status 송신.
- door/high-beam/fog-lamp body bitfield 생성.

## Board E - Safety / ADAS ECU

```c
void ADAS_Safety_Task(void *argument);
int ADAS_Can_SendStatus(const AdasStatus_t *status);
```

책임:

- HC-SR04 전방/후방 거리 센서를 60 ms 주기로 읽습니다.
- CAN1 `0x100` IGN과 `0x5A0` speed reference를 기준으로 risk를 계산합니다.
- 호환 목적으로 non-zero `0x1A0 byte[2..3] / 80` 속도 프레임도 받을 수 있습니다.
- CAN1 `0x3A0 ADAS_Status`를 100 ms마다 송신합니다.

## 핵심 CAN Payload 계약

| CAN ID | 송신자 | 핵심 payload |
|---:|---|---|
| `0x100` | Board A | `byte[5] bit0 = IGN ON` |
| `0x280` | Board A | `byte[2..3] = rpm * 4`, little-endian |
| `0x1A0` | Board A | 현재 all zero keepalive |
| `0x5A0` | Board A | `byte[2] = km/h / 2`, `byte[4] bit4..5` engine warning chime |
| `0x288` | Board A | `byte[1]` coolant raw |
| `0x481` | Board A | `byte[0]` warning bits, `byte[1]` coolant, `byte[2..3]` rpm |
| `0x531` | Board D | `byte[2] bit0 left`, `bit1 right`, `bit2 hazard` |
| `0x635` | Board D | `byte[0] = brightness 0..100` |
| `0x3A0` | Board E | flags, risk, front, rear, fault, speed, input, alive |

## 변경 프로세스

1. CAN ID, DID, payload layout 변경은 `docs/can_db.md`, `docs/uds_did_map.md`, 전체 README, 해당 보드 README를 함께 수정합니다.
2. 한 보드의 `protocol_ids.h`만 바꾸지 말고 수신 보드의 decode 코드와 문서를 같이 확인합니다.
3. 변경 후 최소 해당 보드 단위 빌드와 루트 `build_firmware` 타깃을 확인합니다.
