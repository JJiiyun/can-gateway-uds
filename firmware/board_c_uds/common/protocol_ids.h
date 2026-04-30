#ifndef PROTOCOL_IDS_H
#define PROTOCOL_IDS_H

/*
 * Board C -> Instrument Cluster UDS communication IDs
 * CAN1 is used in this project.
 */
#define CAN_ID_CLUSTER_UDS_REQ      0x714u  /* Board C -> Cluster request */
#define CAN_ID_CLUSTER_UDS_RESP     0x77Eu  /* Cluster -> Board C response */

/*
 * The same physical Board C CAN1 link can be connected to the Gateway CAN2
 * diagnostic bus. Keep the existing cluster names for compatibility and add
 * gateway aliases for the Safety/ADAS demo path.
 */
#define CAN_ID_GATEWAY_UDS_REQ      CAN_ID_CLUSTER_UDS_REQ
#define CAN_ID_GATEWAY_UDS_RESP     CAN_ID_CLUSTER_UDS_RESP
#define CAN_ID_ADAS_STATUS          0x3A0u

/* UDS DID (Data Identifier) values used for cluster diagnosis */
#define UDS_DID_PART_NUMBER         0xF187u /* Vehicle manufacturer spare part number */
#define UDS_DID_SW_NUMBER           0xF188u /* ECU software number */
#define UDS_DID_SW_VERSION          0xF189u /* ECU software version number */
#define UDS_DID_SERIAL_NUMBER       0xF18Cu /* ECU serial number */
#define UDS_DID_VIN                 0xF190u /* VIN */
#define UDS_DID_HW_NUMBER           0xF191u /* ECU hardware number */
#define UDS_DID_SYSTEM_NAME         0xF197u /* System name */

/* Optional dynamic signal DIDs. These may not be supported by the real cluster. */
#define UDS_DID_RPM                 0x280u /* Engine RPM */
#define UDS_DID_SPEED               0x1A0u /* Vehicle speed */
#define UDS_DID_COOLANT             0xF40Eu /* Coolant temperature */
#define UDS_DID_TEMP                UDS_DID_COOLANT
#define UDS_DID_ADAS_STATUS         0xF410u /* Gateway ADAS flags/risk/distances */
#define UDS_DID_ADAS_FRONT_DISTANCE 0xF411u /* Gateway ADAS front distance */
#define UDS_DID_ADAS_REAR_DISTANCE  0xF412u /* Gateway ADAS rear distance */
#define UDS_DID_ADAS_FAULT_BITMAP   0xF413u /* Gateway ADAS active fault/DTC bitmap */

#define CAN_ID_RPM_DATA             UDS_DID_RPM      // 0x280
#define CAN_ID_SPEED_DATA           UDS_DID_SPEED    // 0x1A0
#define CAN_ID_COOLANT_DATA         UDS_DID_COOLANT 

#define UDS_DID_ALL                 0x0000u
#endif /* PROTOCOL_IDS_H */
