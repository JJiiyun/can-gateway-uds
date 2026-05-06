# EngineSim GUI

`board_a_engine` 펌웨어의 EngineSim CLI를 조작하기 위한 Qt Widgets 기반 PC GUI입니다.
보드의 UART CLI 명령을 사용해서 엔진 입력값을 바꾸고, `monitor` 출력으로 RPM, 속도,
냉각수 온도, 전송 카운트 등을 실시간으로 표시합니다.

## Qt Creator에서 열기

1. Qt Creator에서 `GUI/CMakeLists.txt`를 엽니다.
2. Desktop Qt Kit를 선택합니다.
3. `engine_sim` 타겟을 빌드하고 실행합니다.

기존에 `GUI/engine_sim` 경로로 열었던 빌드 설정이 남아 있으면, Qt Creator에서 새로
`GUI/CMakeLists.txt`를 열고 CMake configure를 다시 실행하세요.

qmake 프로젝트 파일도 함께 제공합니다.

- `GUI/engine_sim.pro`

## 연결 설정

보드 CLI는 USART3/ST-Link VCP를 사용합니다.

- Baudrate: `115200`
- Data bits: `8`
- Parity: `None`
- Stop bits: `1`
- Flow control: `None`

Windows에서는 COM 포트를 직접 열기 때문에 Qt SerialPort 모듈이 필요하지 않습니다.

## 사용 방법

1. `Port`에서 보드의 COM 포트를 선택합니다.
2. `Connect`를 누릅니다.
3. 연결되면 GUI가 자동으로 `monitor on <interval_ms>`를 전송합니다.
4. `MONITOR`의 `-`/`+` 버튼으로 모니터 주기를 50 ms 단위로 조절합니다.
   모니터가 켜진 상태라면 바뀐 주기는 자동으로 보드에 적용됩니다.
5. `ACCEL` 또는 `BRAKE` 버튼을 누르고 있으면 페달 값이 천천히 증가합니다.
6. 버튼에서 손을 떼면 페달 값이 다시 내려가며, 내려가는 값도 실시간으로 전송됩니다.

기본 입력 모드는 `ADC`입니다. GUI에서 `ACCEL` 또는 `BRAKE`를 누르면 `pedal` 명령이
전송되면서 펌웨어가 자동으로 `UART` 모드로 전환됩니다. 다시 가변저항/ADC 입력을
사용하려면 GUI에서 `ADC` 모드를 선택하세요.

## CLI 명령

GUI는 기존 펌웨어 CLI 명령을 그대로 사용합니다.

- `mode adc`
- `mode uart`
- `pedal <throttle> <brake>`
- `stop`
- `sim_reset`
- `status`
- `monitor on <interval_ms>`
- `monitor off`
- `monitor once`

연결 후 GUI는 아래 형식의 모니터 라인을 파싱합니다.

```text
MON mode=uart throttle=50 brake=0 rpm=3200 speed=60 coolant=102 tx=1234
```

`MON` 라인은 Serial Console에도 표시되고 Engine Dashboard 값으로도 반영됩니다.
대시보드의 `TARGET TX PERIOD`는 펌웨어에서 의도한 CAN 송신 주기인 `50 ms`이고,
`EST. TX PERIOD`는 모니터 라인의 `tx` 카운트 증가량과 PC 수신 시간을 이용해 GUI에서
추정한 실제 송신 간격입니다. `MONITOR INTERVAL`은 GUI가 모니터 라인을 받은 간격입니다.

## 콘솔 표시

Serial Console은 사용자가 직접 입력한 CLI 명령과 일반 응답 확인용입니다.
GUI가 보드로 보낸 직접 명령은 `TX` 라벨로 구분되고, 보드 응답은 `RX`로 표시됩니다.
단, `ACCEL`/`BRAKE` 버튼을 누르면서 자동으로 반복 전송되는 `pedal` 명령과 그 응답은
콘솔에 표시하지 않고 대시보드 값만 갱신합니다.
