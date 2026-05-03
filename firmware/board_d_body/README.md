# Board D - Body / BCM

Board D는 도어, 방향지시등, 하이빔, 안개등 같은 Body 신호를 만들고 CAN1에 송신하는 보드입니다.

현재 연동 기준:

- IGN 수신: 기존 Board A `0x100` EngineData `byte5 bit0`
- Golf 6 IGN 수신도 지원: `0x570`/`0x572` Klemme 15 `byte0 bit1`
- Body 송신: Golf 6 DBC `BO_ 912 mGate_Komf_1`, CAN ID `0x390`, DLC 8, 100ms

## 역할

| 파일 | 역할 |
|---|---|
| `src/bcm_input.c` | DIP 스위치 / 버튼 GPIO 입력 |
| `src/bcm_cli.c` | PC GUI/터미널용 UART CLI, GPIO/UART 입력 모드 전환 |
| `src/bcm_signal.c` | Golf 6 `mGate_Komf_1`(`0x390`) 신호 패킹 |
| `src/bcm_can.c` | `common/can_bsp.c/h`를 통한 CAN 송수신 |
| `src/bcm_main.c` | FreeRTOS Task 연결, BCM/CLI task orchestration |

## CAN 메시지

| ID | 방향 | 내용 |
|---|---|---|
| `0x100` | Board A -> Board D | 현재 프로젝트 EngineData, IGN = byte5 bit0 |
| `0x570` | Golf 6 -> Board D | `mBSG_Last`, Klemme 15 = byte0 bit1 |
| `0x572` | Golf 6 -> Board D | `mZAS_1`, Klemme 15 = byte0 bit1 |
| `0x390` | Board D -> CAN1 | Golf 6 `mGate_Komf_1`: turn, door, high beam, fog |

## Golf 6 mGate_Komf_1 Mapping

`docs/Golf_6_PQ35.dbc` 기준:

| Signal | Bit | Board D input |
|---|---:|---|
| `GK1_Sta_Tuerkont` | 4 | any door open |
| `GK1_Fa_Tuerkont` | 16 | any door open |
| `GK1_Blinker_li` | 34 | left turn blink state |
| `GK1_Blinker_re` | 35 | right turn blink state |
| `GK1_LS1_Fernlicht` | 37 | high beam |
| `GK1_Fernlicht` | 49 | high beam |
| `GK1_Nebel_ein` | 58 | fog light |

`GK1_SleepAckn`, `GK1_Rueckfahr`, `GK1_Warnblk_Status`는 DBC 기본 상태 유지를 위해 1로 송신합니다.

## 기본 입력 매핑

입력은 pull-up 기준입니다. 스위치/버튼이 GND로 연결되면 active입니다.

| 기능 | F429ZI 기본 핀 |
|---|---|
| FL Door | PE2 |
| FR Door | PE3 |
| RL Door | PE4 |
| RR Door | PE5 |
| Left Turn Button | PE6 |
| Right Turn Button | PF6 |
| High Beam | PF7 |
| Fog Light | PF8 |

## UART CLI / GUI 제어

Board D는 `common/uart.c` + `common/cli.c` 기반의 USART3/ST-LINK VCP CLI를 사용합니다.

기본 연결:

```text
Baudrate : 115200
Data     : 8 bit
Parity   : None
Stop     : 1 bit
Flow     : None
```

입력 모드는 두 가지입니다.

| 모드 | 의미 |
|---|---|
| `gpio` | 기존 DIP/button GPIO 입력을 읽어 `0x390` 생성 |
| `uart` | PC GUI/터미널에서 보낸 CLI 값으로 `0x390` 생성 |

주요 명령:

```text
body mode gpio
body mode uart

body ign auto
body ign on
body ign off

body door fl 1
body door fr 0
body door rl 0
body door rr 0
body door all 0

body turn left 1
body turn right 0
body turn both 0

body high 1
body fog 0

body all doors 0
body all lamps 0
body all off

body status
body monitor on 200
body monitor off
body monitor once
body reset
```

`body door`, `body turn`, `body high`, `body fog`, `body all` 명령은 자동으로 `uart` 모드로 전환합니다.

GUI 파싱용 모니터 라인 포맷:

```text
BODY mode=uart ign=1 fl=1 fr=0 rl=0 rr=0 left=1 right=0 high=1 fog=0 tx=1234 rx=12
```

`body ign auto`는 기존처럼 CAN에서 수신한 IGN 상태를 따릅니다. `body ign on`은 벤치/GUI 테스트용으로 `0x390` 송신을 강제로 허용합니다.

## CAN 테스트

1. Board A가 `0x100`을 보내고 `byte5 bit0 = 1`이면 Board D가 IGN ON으로 인식합니다.
2. Board D는 IGN ON 동안 Golf 6 `0x390`을 100ms 주기로 송신합니다.
3. CAN monitor에서 `0x390`이 보이고, 입력 변화에 따라 위 bit들이 변하는지 확인합니다.

주의: 현재 `board_b_gateway` 코드는 `0x100`만 CAN2로 포워딩합니다. `board_b_gateway`를 수정하지 않는 조건이면 `0x390`은 CAN1에서는 보이지만 CAN2/계기판 쪽으로는 자동 포워딩되지 않습니다.
