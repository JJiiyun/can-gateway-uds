Board C - CAN1 UDS Client with Automatic ISO-TP Flow Control

Purpose
- Board C works as a UDS tester/client.
- Board C sends UDS ReadDataByIdentifier requests to the instrument cluster or
  to Board B Gateway's ADAS diagnostic server.
- Request CAN ID : 0x714
- Response CAN ID: 0x77E
- CAN bus        : CAN1

Dedicated commands
- read vin      -> DID F190, VIN
- read part     -> DID F187, manufacturer spare part number
- read sw       -> DID F188, ECU software number
- read swver    -> DID F189, software version
- read serial   -> DID F18C, ECU serial number
- read hw       -> DID F191, hardware number
- read system   -> DID F197, system name
- read adas     -> DID F410, Gateway ADAS flags/risk/front/rear
- read front    -> DID F411, Gateway ADAS front distance
- read rear     -> DID F412, Gateway ADAS rear distance
- read fault    -> DID F413, Gateway ADAS active fault and latched DTC bitmap
- clear dtc     -> SID 14 FF FF FF, clear Gateway ADAS latched DTC

Automatic Flow Control
- If the cluster response is a Single Frame, the payload is decoded immediately.
- If the cluster response is a First Frame, for example:
    ID=0x77E DATA=10 xx ...
  the code automatically sends Flow Control CTS:
    ID=0x714 DATA=30 00 00 00 00 00 00 00
- Then it receives Consecutive Frames:
    ID=0x77E DATA=21 ...
    ID=0x77E DATA=22 ...
- After the full ISO-TP payload is assembled, it prints:
    [CLUSTER MF COMPLETE]
    [CLUSTER UDS RESP]
    [CLUSTER ASCII]

Recommended test
1. cluster_info
2. read vin
3. read part
4. read sw
5. read swver
6. read adas
7. read fault
8. clear dtc

Success examples
- Positive response: 62 F1 90 ...
- Negative response: 7F 22 31
  This still means CAN/UDS communication succeeded, but the DID was not supported or allowed.
- Gateway ADAS response: 62 F4 10 <flags> <risk> <front_cm> <rear_cm>
- Gateway clear DTC response: 54
