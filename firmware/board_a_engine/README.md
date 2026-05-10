# Board A - Engine ECU Simulator

Board A는 STM32F429ZI 기반 엔진 ECU 시뮬레이터입니다. ADC 또는 UART CLI/GUI 입력으로 throttle/brake를 받고, RPM/속도/냉각수/엔진 경고 상태를 계산해 CAN1으로 송신합니다.

## 역할

- Throttle/brake 입력을 ADC 또는 UART 명령으로 받습니다.
- 50 ms 주기로 엔진 모델을 갱신합니다.
- CAN1에 IGN, RPM, speed reference, coolant, engine warning 프레임을 송신합니다.
- USART3/ST-Link VCP로 CLI와 Qt GUI 제어를 제공합니다.

## Hardware Interface

| 기능 | 포트/주변장치 | 설명 |
|---|---|---|
| Throttle ADC | `PA0 / ADC1_IN0` | throttle 가변저항 입력 |
| Brake ADC | `PA1 / ADC1_IN3` | brake 가변저항 입력 |
| CAN1 RX | `PD0` | CAN 수신 |
| CAN1 TX | `PD1` | CAN 송신 |
| CLI UART | `USART3 / ST-Link VCP` | `115200 8-N-1` |

## FreeRTOS Task

| Task | 역할 |
|---|---|
| `defaultTask` | UART, CLI, EngineSim 초기화 및 CLI 처리 |
| `EngSimTask` | 엔진 모델 업데이트와 CAN 송신 |

`defaultTask`는 `uartInit()`, `cliInit()`, `CliCmd_Init()`, `CAN_BSP_Init()`, `EngineSim_Init()`을 수행합니다. `EngSimTask`는 `EngineSim_Task()`를 실행합니다.

## 입력 모드

기본 입력 모드는 `ADC`입니다. `throttle`, `brake`, `pedal`, `stop` 명령을 사용하면 자동으로 `UART` 모드로 전환됩니다. 실제 ADC 입력으로 돌아가려면 `mode adc`를 실행합니다.

| 모드 | 설명 |
|---|---|
| `ADC` | 보드의 ADC 값을 throttle/brake로 사용 |
| `UART` | CLI 또는 GUI에서 받은 throttle/brake 값을 사용 |

## CAN 송신 주기

| CAN ID | 주기 | 설명 |
|---:|---:|---|
| `0x100` | 50 ms | IGN Status. 현재 항상 ON |
| `0x280` | 50 ms | RPM |
| `0x1A0` | 50 ms | Speed keepalive. 현재 zero-filled |
| `0x5A0` | 50 ms | Speed needle/reference |
| `0x288` | 100 ms | Coolant |
| `0x481` | 100 ms | Engine warning status |
| 모델 업데이트 | 50 ms | RPM/속도 계산 |
| 냉각수 모델 업데이트 | 1000 ms | coolant 계산 |

## CAN Protocol

현재 Board A는 기존 `0x100` 통합 EngineData 프레임을 사용하지 않습니다. `0x100`은 IGN Status 전용으로 남기고, 계기판 반응용 프레임을 별도 CAN ID로 송신합니다.

```c
#define CAN_ID_CLUSTER_IGN_STATUS       0x100U
#define CAN_ID_CLUSTER_RPM              0x280U
#define CAN_ID_CLUSTER_SPEED_1A0        0x1A0U
#define CAN_ID_CLUSTER_SPEED_5A0        0x5A0U
#define CAN_ID_CLUSTER_COOLANT          0x288U
#define CAN_ID_ENGINE_WARNING_STATUS    0x481U
```

### `0x100` IGN Status

| 항목 | 값 |
|---|---|
| CAN ID | `0x100` |
| DLC | `8` |
| 주기 | `50 ms` |
| Payload | `byte[5] bit0 = 1` |

현재 구현은 bench integration을 위해 IGN ON을 항상 송신합니다.

### `0x280` RPM

| 항목 | 값 |
|---|---|
| CAN ID | `0x280` |
| DLC | `8` |
| 주기 | `50 ms` |
| Byte 2-3 | RPM raw, little-endian |
| 해석 | `rpm = raw / 4` |

예시:

```text
0x280 8 00 00 80 0C 00 00 00 00  # 800 rpm
```

### `0x1A0` Speed Keepalive

| 항목 | 값 |
|---|---|
| CAN ID | `0x1A0` |
| DLC | `8` |
| 주기 | `50 ms` |
| Payload | `00 00 00 00 00 00 00 00` |

현재 구현에서는 `0x1A0`에 speed raw를 싣지 않습니다. Board B/Board E에는 과거 non-zero `0x1A0 byte[2..3] = km/h * 80` 프레임을 해석하는 호환 코드가 남아 있지만, 현재 Board A가 실제 속도 기준으로 사용하는 프레임은 `0x5A0`입니다.

### `0x5A0` Speed Reference

| 항목 | 값 |
|---|---|
| CAN ID | `0x5A0` |
| DLC | `8` |
| 주기 | `50 ms` |
| Byte 2 | `speed / 2` |
| Byte 4 bit4..5 | 엔진 경고 chime request, mask `0x30` |

`0x5A0 byte[2]`는 계기판/ADAS 기준 속도입니다. 수신 측은 `raw * 2`로 km/h를 복원합니다.

RPM 또는 coolant 경고가 새로 발생하면 `byte[4] bit4..5`를 1초 동안 켭니다.

```c
if (warning_chime_active) {
    data[4] |= 0x30U;
}
```

### `0x288` Coolant

| 항목 | 값 |
|---|---|
| CAN ID | `0x288` |
| DLC | `8` |
| 주기 | `100 ms` |
| Byte 1 | Coolant raw 값 |

모델 coolant는 `60~130` 범위로 제한하고, 계기판 raw는 아래 식으로 인코딩합니다.

```text
raw = ((coolant_degC + 48) * 4) / 3
coolant_degC = (raw * 0.75) - 48
```

예를 들어 coolant가 `90`이면 raw는 `184`, 즉 `0xB8`입니다.

### `0x481` Engine Warning Status

| 항목 | 값 |
|---|---|
| CAN ID | `0x481` |
| DLC | `8` |
| 주기 | `100 ms` |
| Byte 0 | warning bitfield |
| Byte 1 | coolant 값 |
| Byte 2-3 | RPM 값, little-endian |
| Byte 7 | alive counter |

Warning bitfield:

| Bit | 의미 | 조건 |
|---|---|---|
| 0 | RPM warning | `rpm >= 5000` |
| 1 | Coolant warning | `coolant >= 115` |
| 2 | General warning | bit0 또는 bit1 ON |
| 3-7 | Reserved | `0` |

## Board B 연동

Board B의 현재 routing table은 Board A의 아래 프레임을 CAN1에서 CAN2로 그대로 포워딩합니다.

```text
0x100, 0x280, 0x1A0, 0x5A0, 0x288, 0x481
```

Gateway가 RPM/Speed/Coolant payload를 재생성하지 않으므로, Board A payload가 계기판/ADAS가 기대하는 최종 형태여야 합니다.

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
|---|---|
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

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```

Qt GUI는 별도 PC 프로그램이며, `GUI/CMakeLists.txt`를 Qt Creator에서 열어 빌드합니다. 자세한 내용은 `GUI/README.md`를 참고합니다.

## Notes

- CAN1 필터는 현재 모든 standard ID를 수신하도록 열려 있습니다.
- CAN 송신은 TX mailbox가 없으면 `HAL_BUSY`를 반환합니다.
- `0x100`은 더 이상 RPM/speed/coolant 통합 프레임이 아닙니다.
- 현재 속도 기준은 `0x5A0 byte[2]`입니다.
