# Board A Engine ECU Simulator

STM32F429ZI 기반 Board A 엔진 ECU 시뮬레이터입니다. 페달 입력을 받아 RPM, 속도, 냉각수 상태를 계산하고 CAN1으로 계기판 반응용 CAN 프레임을 송신합니다.

## 역할

- ADC 또는 UART CLI로 throttle/brake 입력을 받습니다.
- 간단한 엔진 모델로 RPM, 속도, 냉각수 온도를 갱신합니다.
- CAN1으로 RPM, speed, coolant 계기판용 프레임을 주기 송신합니다.
- USART3/ST-Link VCP로 CLI와 Qt GUI 제어를 제공합니다.

## Hardware Interface

| 기능 | 포트/주변장치 | 설명 |
| --- | --- | --- |
| Throttle ADC | `PA0 / ADC1_IN0` | throttle 가변저항 입력 |
| Brake ADC | `PA1 / ADC1_IN3` | brake 가변저항 입력 |
| CAN1 RX | `PD0` | CAN 수신 |
| CAN1 TX | `PD1` | CAN 송신 |
| CLI UART | `USART3 / ST-Link VCP` | `115200 8-N-1` |

## FreeRTOS Task

| Task | 역할 |
| --- | --- |
| `defaultTask` | UART, CLI, EngineSim 초기화 및 CLI 처리 |
| `EngSimTask` | 엔진 모델 업데이트와 CAN 송신 |

`defaultTask`는 `uartInit()`, `cliInit()`, `CliCmd_Init()`, `CAN_BSP_Init()`, `EngineSim_Init()`을 수행합니다. `EngSimTask`는 `EngineSim_Task()`를 실행합니다.

## EngineSim 동작

EngineSim 입력 모드는 두 가지입니다.

- `ADC`: 보드의 ADC 값을 읽어 throttle/brake로 사용합니다.
- `UART`: CLI 또는 GUI에서 받은 throttle/brake 값을 사용합니다.

기본 모드는 `ADC`입니다. `throttle`, `brake`, `pedal`, `stop` 명령을 사용하면 자동으로 `UART` 모드로 전환됩니다. 실제 ADC 입력으로 돌아가려면 `mode adc`를 실행합니다.

주요 주기:

| 항목 | 주기 |
| --- | --- |
| 엔진 모델 업데이트 | 50 ms |
| RPM frame `0x280` 송신 | 50 ms |
| Speed frame `0x1A0` 송신 | 50 ms |
| Speed 보조 frame `0x5A0` 송신 | 50 ms |
| Coolant frame `0x288` 송신 | 100 ms |
| Engine warning status frame `0x481` 송신 | 100 ms |
| 냉각수 모델 업데이트 | 1000 ms |

## CAN Protocol

기존에는 `0x100` 단일 EngineData 프레임에 RPM, speed, coolant를 통합해서 송신했습니다. 현재는 계기판 반응 ID 기준으로 아래 4개 프레임을 각각 송신합니다.

```c
#define CAN_ID_CLUSTER_RPM              0x280U
#define CAN_ID_CLUSTER_SPEED_1A0        0x1A0U
#define CAN_ID_CLUSTER_SPEED_5A0        0x5A0U
#define CAN_ID_CLUSTER_COOLANT          0x288U
#define CAN_ID_ENGINE_WARNING_STATUS    0x481U

#define CAN_CLUSTER_DLC                 8U
#define CAN_ENGINE_WARNING_DLC          8U
```

### `0x280` RPM

| 항목 | 값 |
| --- | --- |
| CAN ID | `0x280` |
| DLC | `8` |
| 주기 | `50 ms` |
| Byte 2-3 | RPM raw, little-endian |
| 해석 | `rpm = raw / 4` |

예시:

```text
0x280 8 00 00 15 15 00 00 00 00
```

### `0x1A0` Speed

| 항목 | 값 |
| --- | --- |
| CAN ID | `0x1A0` |
| DLC | `8` |
| 주기 | `50 ms` |
| Payload | `00 00 00 00 00 00 00 00` |
| 해석 | Speed raw 없음 |

예시:

```text
0x1A0 8 00 00 00 00 00 00 00 00
```

### `0x5A0` Speed 보조

| 항목 | 값 |
| --- | --- |
| CAN ID | `0x5A0` |
| DLC | `8` |
| 주기 | `50 ms` |
| Byte 2 | Speed 값 |

### `0x288` Coolant

| 항목 | 값 |
| --- | --- |
| CAN ID | `0x288` |
| DLC | `8` |
| 주기 | `100 ms` |
| Byte 1 | Coolant raw 값 |

`0x288` payload의 `byte[1]`에는 EngineSim coolant 모델 값을 DBC raw로 인코딩해서 넣습니다.
현재 모델 coolant와 계기판 송신용 coolant는 모두 `60~130` 범위를 사용합니다.

```text
cluster_coolant = clamp(model_coolant, 60, 130)
raw = ((cluster_coolant + 48) * 4) / 3
cluster_coolant = (raw * 0.75) - 48
```

예를 들어 모델 coolant가 `90`이면 CAN raw는 `184`, 즉 `0xB8`입니다.

### `0x481` Engine Warning Status

| 항목 | 값 |
| --- | --- |
| CAN ID | `0x481` |
| DLC | `8` |
| 주기 | `100 ms` |
| Byte 0 | warning bitfield |
| Byte 1 | coolant 값 |
| Byte 2-3 | RPM 값, little-endian |
| Byte 7 | alive counter |

Warning bitfield:

| Bit | 의미 | 조건 |
| --- | --- | --- |
| 0 | RPM warning | `rpm >= 5000` |
| 1 | Coolant warning | `coolant >= 115` |
| 2 | General warning | bit0 또는 bit1 ON |
| 3-7 | Reserved | `0` |

## Board B 연동

Board B는 기존 `CAN_ID_ENGINE_DATA = 0x100` 기반 파싱/변환 대신, 아래 ID를 CAN1에서 수신해 CAN2로 포워딩해야 합니다.

```c
{0x280, CAN1_PORT, CAN2_PORT},
{0x1A0, CAN1_PORT, CAN2_PORT},
{0x5A0, CAN1_PORT, CAN2_PORT},
{0x288, CAN1_PORT, CAN2_PORT},
{0x481, CAN1_PORT, CAN2_PORT},
```

Board B 담당자 전달사항:

```text
Board A에서 엔진 warning 상태 프레임을 추가했습니다.
CAN ID: 0x481
DLC: 8
Period: 100 ms
Payload:
  byte0 bit0 = RPM warning       (rpm >= 5000)
  byte0 bit1 = Coolant warning   (coolant >= 115)
  byte0 bit2 = General warning   (bit0 또는 bit1 ON)
  byte1      = coolant value
  byte2~3    = rpm value, little-endian
  byte7      = alive counter

Gateway에서는 0x481을 CAN1에서 수신해 CAN2로 포워딩하거나,
계기판용 warning 프레임이 따로 있으면 0x481을 입력으로 받아 해당 프레임으로 변환해 주세요.
```

`0x1A0`에는 speed raw를 싣지 않습니다. Speed 보조 표시는 `0x5A0`의 `byte[2]`를 사용합니다.

## CLI 명령

USART3/ST-Link VCP 터미널에서 사용할 수 있는 명령입니다.

```text
sim_help
mode adc
mode uart
throttle <0~100>
brake <0~100>
pedal <throttle 0~100> <brake 0~100>
stop
status
monitor on [interval_ms]
monitor off
monitor once
sim_reset
```

예시:

```text
pedal 70 0
pedal 20 50
monitor on 200
status
mode adc
```

`monitor` 출력은 GUI와 로그 파싱을 위해 고정 형식으로 유지합니다.

```text
MON mode=uart throttle=50 brake=0 rpm=3200 speed=60 coolant=102 tx=1234
```

## 주요 소스 구조

| 경로 | 설명 |
| --- | --- |
| `MyApp/engine_sim.c` | 엔진 모델, 입력 처리, CAN 송신 |
| `MyApp/engine_sim.h` | EngineSim public API와 상태 구조체 |
| `MyApp/protocol_ids.h` | CAN ID와 payload index 정의 |
| `MyApp/engine_can.h` | CAN little-endian helper |
| `MyApp/can_bsp.c` | CAN 초기화, 송수신 helper |
| `MyApp/cli_cmd.c` | EngineSim CLI 명령 |
| `MyApp/adc_driver.c` | throttle/brake ADC 읽기 |
| `Core/Src/freertos.c` | FreeRTOS task 생성 |
| `GUI/` | Qt 기반 PC 제어 GUI |

## Build

STM32CubeMX/CMake 기반 프로젝트입니다. 프로젝트 루트의 `CMakePresets.json` 또는 STM32/ARM GCC kit 설정이 된 IDE에서 빌드합니다.

Qt GUI는 별도 PC 프로그램이며, `GUI/CMakeLists.txt`를 Qt Creator에서 열어 빌드합니다. 자세한 내용은 `GUI/README.md`를 참고합니다.

## Notes

- CAN1 필터는 현재 모든 standard ID를 수신하도록 열려 있습니다.
- CAN 송신은 TX mailbox가 없으면 `HAL_BUSY`를 반환합니다.
- 새 프로토콜은 기존 `0x100` 통합 프레임과 호환되지 않습니다.
- Board B와 같은 CAN ID, DLC, byte layout을 공유해야 정상 포워딩됩니다.
