# Board C - UDS Diagnostic Client

Board C는 UDS 진단 클라이언트입니다. 코드에서는 보드의 CAN1 주변장치를 사용하지만, 통합 구성에서는 이 CAN1 트랜시버를 물리적으로 Board B의 CAN2 계기판/진단 버스에 연결합니다.

## 현재 계약

| 항목 | 값 |
|---|---|
| 보드 주변장치 | CAN1 |
| 물리 연결 | Board B CAN2 / Cluster diagnostic bus |
| 기본 요청 ID | `0x714` |
| 기본 응답 ID | `0x77E` |
| 지원 전송 | ISO-TP Single Frame, First Frame 수신 시 Flow Control CTS 자동 송신 |
| UART | USART3 / ST-Link VCP, `115200 8N1` |

## 동작 방식

- `read vin`, `read part` 등은 기본 ID `0x714`로 UDS `0x22 ReadDataByIdentifier` 요청을 송신합니다.
- 응답 `0x77E`가 Single Frame이면 즉시 payload를 출력합니다.
- 응답이 First Frame이면 `30 00 00 00 00 00 00 00` Flow Control CTS를 자동 송신하고 Consecutive Frame을 조립합니다.
- `read adas`, `read front`, `read rear`, `read fault`, `clear dtc`는 Board B Gateway ADAS UDS server를 대상으로 한 데모 명령입니다.
- `read rpm`, `read speed`, `read temp`, `read all`은 UDS 요청이 아니라 특정 CAN ID를 직접 듣는 수동 listen/debug 모드입니다.

## UDS ID

| 방향 | CAN ID | 설명 |
|---|---:|---|
| Board C -> Gateway/Cluster | `0x714` | UDS request |
| Gateway/Cluster -> Board C | `0x77E` | UDS response |

`cl_target <req_id> <resp_id>`로 요청/응답 ID를 바꿀 수 있습니다.

## Gateway ADAS DID

Board B Gateway가 현재 직접 응답하는 DID입니다.

| 명령 | 요청 | 응답 값 |
|---|---|---|
| `read adas` | `22 F4 10` | flags, risk, front cm, rear cm |
| `read front` | `22 F4 11` | front cm |
| `read rear` | `22 F4 12` | rear cm |
| `read fault` | `22 F4 13` | active fault bitmap, latched DTC bitmap |
| `clear dtc` | `14 FF FF FF` | positive response `54` |

같은 CAN2 버스에서 계기판도 `0x714/0x77E`를 사용할 수 있습니다. Board B가 켜져 있으면 Gateway가 지원하지 않는 DID에 대해 NRC를 같이 응답할 수 있으므로, 계기판 단독 진단을 할 때는 타깃 ID나 전원 구성을 확인해야 합니다.

## CLI 명령

```text
cluster_info

read vin
read part
read sw
read swver
read serial
read hw
read system

read adas
read front
read rear
read fault
clear dtc

read rpm
read speed
read temp
read all

cl_target <req_id> <resp_id>
cl_read <did>
can_log on|off
can_tx <id> <b0> [b1] ... [b7]
can_stat
```

## 주요 소스 구조

| 경로 | 설명 |
|---|---|
| `board_c_uds/ap_main.c` | `UDSMainTask()` 구현. UART/CAN/CLI 초기화와 main loop |
| `board_c_uds/uds_client.c/h` | UDS client, ISO-TP 수신 조립, Flow Control 송신 |
| `board_c_uds/cli_cmd.c/h` | CLI 명령 등록 |
| `common/can_bsp.c/h` | CAN1 송수신 BSP |
| `common/protocol_ids.h` | UDS ID와 DID 정의 |

## Build

```bash
cmake --preset Debug --fresh
cmake --build --preset Debug --parallel
```
