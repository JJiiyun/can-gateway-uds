Board C - 자동 ISO-TP Flow Control UDS Client

목적
- Board C는 UDS tester/client로 동작합니다.
- 펌웨어는 보드의 CAN1 주변장치를 사용합니다.
- 통합 구성에서는 이 CAN1 트랜시버를 Board B의 CAN2 계기판/진단 버스에 연결합니다.
- Board C는 계기판 또는 Board B Gateway ADAS 진단 서버에 ReadDataByIdentifier 요청을 보낼 수 있습니다.
- Request CAN ID : 0x714
- Response CAN ID: 0x77E

전용 명령
- read vin      -> DID F190, VIN
- read part     -> DID F187, 제조사 부품 번호
- read sw       -> DID F188, ECU software number
- read swver    -> DID F189, software version
- read serial   -> DID F18C, ECU serial number
- read hw       -> DID F191, hardware number
- read system   -> DID F197, system name
- read adas     -> DID F410, Gateway ADAS flags/risk/front/rear
- read front    -> DID F411, Gateway ADAS front distance
- read rear     -> DID F412, Gateway ADAS rear distance
- read fault    -> DID F413, Gateway ADAS active fault and latched DTC bitmap
- clear dtc     -> SID 14 FF FF FF, Gateway ADAS latched DTC clear

수동 listen/debug 명령
- read rpm      -> CAN ID 0x280 수신 대기
- read speed    -> CAN ID 0x1A0 수신 대기
- read temp     -> 현재 코드 기준 CAN ID 0xF40E 수신 대기
- read all      -> 아무 키를 누를 때까지 모든 수신 CAN frame 출력

자동 Flow Control
- 응답이 Single Frame이면 payload를 바로 해석합니다.
- 응답이 First Frame이면, 예를 들어:
    ID=0x77E DATA=10 xx ...
  코드가 자동으로 Flow Control CTS를 송신합니다:
    ID=0x714 DATA=30 00 00 00 00 00 00 00
- 이후 Consecutive Frame을 수신합니다:
    ID=0x77E DATA=21 ...
    ID=0x77E DATA=22 ...
- 전체 ISO-TP payload가 조립되면 아래 로그가 출력됩니다:
    [CLUSTER MF COMPLETE]
    [CLUSTER UDS RESP]
    [CLUSTER ASCII]

권장 테스트
1. cluster_info
2. read adas
3. read fault
4. clear dtc
5. read vin 등 계기판 DID는 계기판 타깃 구성을 확인한 뒤 실행

성공 예시
- Positive response: 62 F1 90 ...
- Negative response: 7F 22 31
  DID가 지원되지 않거나 허용되지 않았다는 뜻이며, CAN/UDS 통신 자체는 성공한 것입니다.
- Gateway ADAS response: 62 F4 10 <flags> <risk> <front_cm> <rear_cm>
- Gateway clear DTC response: 54
