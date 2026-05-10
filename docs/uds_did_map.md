# UDS DID Map

현재 Board C는 UDS client이고, 기본 CAN ID는 request `0x714`, response `0x77E`입니다. 같은 CAN2 버스에서 계기판과 Board B Gateway ADAS server가 모두 응답할 수 있으므로, 어떤 타깃이 켜져 있는지 확인해야 합니다.

## Gateway ADAS DID

Board B `gateway_uds_server.c`가 실제로 응답하는 DID입니다. 모두 ISO-TP Single Frame으로 처리됩니다.

| DID (hex) | 이름 | 응답 값 길이 | 설명 |
|---:|---|---:|---|
| `0xF410` | ADAS Status | 4 bytes | flags, risk level, front distance cm, rear distance cm |
| `0xF411` | Front Distance | 1 byte | front distance cm |
| `0xF412` | Rear Distance | 1 byte | rear distance cm |
| `0xF413` | ADAS Fault Bitmap | 2 bytes | active fault bitmap, latched DTC bitmap |

Clear DTC:

| Service | 요청 | 긍정 응답 | 설명 |
|---|---|---|---|
| `0x14` | `14 FF FF FF` | `54` | Gateway가 보관한 ADAS latched DTC bitmap clear |

## Gateway 요청/응답 예시

### ADAS Status

```text
Request:  714  03 22 F4 10 00 00 00 00

Response: 77E  07 62 F4 10 20 02 28 50
                         │  │  │  └ rear=80 cm
                         │  │  └ front=40 cm
                         │  └ risk=2
                         └ flags=0x20
```

### Fault Bitmap

```text
Request:  714  03 22 F4 13 00 00 00 00

Response: 77E  05 62 F4 13 03 03 00 00
                         │  └ latched DTC bitmap
                         └ active fault bitmap
```

### Clear DTC

```text
Request:  714  04 14 FF FF FF 00 00 00
Response: 77E  01 54 00 00 00 00 00 00
```

## 계기판 DID 명령

Board C CLI에는 계기판 UDS 요청을 위한 명령도 남아 있습니다.

| CLI | DID | 설명 |
|---|---:|---|
| `read vin` | `0xF190` | VIN |
| `read part` | `0xF187` | manufacturer spare part number |
| `read sw` | `0xF188` | ECU software number |
| `read swver` | `0xF189` | ECU software version |
| `read serial` | `0xF18C` | ECU serial number |
| `read hw` | `0xF191` | ECU hardware number |
| `read system` | `0xF197` | system name |

계기판이 멀티프레임으로 응답하면 Board C는 First Frame을 감지하고 Flow Control CTS를 자동 송신합니다.

## 수동 CAN Listen 명령

아래 명령은 UDS DID 요청이 아니라 현재 코드에서 특정 CAN ID를 직접 기다리는 debug/listen 모드입니다.

| CLI | 현재 동작 |
|---|---|
| `read rpm` | CAN ID `0x280` 수신 대기 |
| `read speed` | CAN ID `0x1A0` 수신 대기 |
| `read temp` | CAN ID `0xF40E` 수신 대기 |
| `read all` | 아무 키 입력 전까지 모든 CAN frame 출력 |

현재 Board A 기준으로 실제 속도 기준은 `0x5A0`이고 `0x1A0`은 zero keepalive입니다. 따라서 `read speed`는 계기판 UDS가 아니라 raw CAN 확인용으로 보는 편이 맞습니다.

## NRC 테이블

| NRC | 의미 | 발생 조건 |
|---:|---|---|
| `0x11` | Service Not Supported | 지원하지 않는 SID |
| `0x13` | Incorrect Message Length | PCI length 또는 payload length 불일치 |
| `0x31` | Request Out of Range | Gateway ADAS server가 지원하지 않는 DID |

## Board C CLI 요약

| 명령 | 설명 |
|---|---|
| `cluster_info` | 현재 request/response ID와 지원 명령 출력 |
| `cl_target <req> <resp>` | UDS request/response CAN ID 변경 |
| `cl_read <did>` | 임의 DID 요청 |
| `read adas` | Gateway ADAS status 조회 |
| `read fault` | Gateway ADAS fault/DTC 조회 |
| `clear dtc` | Gateway ADAS latched DTC clear |
