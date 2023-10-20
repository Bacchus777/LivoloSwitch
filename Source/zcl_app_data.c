#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"

#include "zcl_app.h"

#include "version.h"

#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"
#include "stub_aps.h"

/*********************************************************************
 * CONSTANTS
 */

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0

#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Global attributes
const uint16 zclApp_clusterRevision_all = 0x0002;

// Basic Cluster
const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

const uint8 zclApp_ManufacturerName[] = {7, 'B', 'a', 'c', 'c', 'h', 'u', 's'};
#ifndef HAL_2_CHANNEL
const uint8 zclApp_ModelId[] = {16, 'L', 'i', 'v', 'o', 'l', 'o', '_', '1', '_', 'C', 'h', 'a', 'n', 'n', 'e', 'l'};
#endif
#ifdef HAL_2_CHANNEL
const uint8 zclApp_ModelId[] = {16, 'L', 'i', 'v', 'o', 'l', 'o', '_', '2', '_', 'C', 'h', 'a', 'n', 'n', 'e', 'l'};
#endif
const uint8 zclApp_PowerSource = POWER_SOURCE_MAINS_1_PHASE;


#define DEFAULT_Left FALSE
#ifdef HAL_2_CHANNEL
#define DEFAULT_Right FALSE
#endif

application_config_t zclApp_Config = {
    .Left = DEFAULT_Left,
#ifdef HAL_2_CHANNEL
    .Right = DEFAULT_Right,
#endif
};


/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */

// First EP

CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_UINT16, R, (void *)&zclApp_clusterRevision_all}},
    {GEN_ON_OFF, {ATTRID_ON_OFF, ZCL_BOOLEAN, RW | RR, (void *)&zclApp_Config.Left}},

};

uint8 CONST zclApp_AttrsFirstEPCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));

const cId_t zclApp_InClusterListFirstEP[] = {
  ZCL_CLUSTER_ID_GEN_BASIC,
  GEN_ON_OFF
};

const cId_t zclApp_OutClusterListFirstEP[] = {
  GEN_ON_OFF,
};

#define APP_MAX_OUTCLUSTERS_FIRST_EP (sizeof(zclApp_OutClusterListFirstEP) / sizeof(zclApp_OutClusterListFirstEP[0]))
#define APP_MAX_INCLUSTERS_FIRST_EP (sizeof(zclApp_InClusterListFirstEP) / sizeof(zclApp_InClusterListFirstEP[0]))

SimpleDescriptionFormat_t zclApp_FirstEP = {
    FIRST_ENDPOINT,                             //  int Endpoint;
    ZCL_HA_PROFILE_ID,                          //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_ON_OFF_SWITCH,              //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    APP_FLAGS,                                  //  int   AppFlags:4;
    APP_MAX_INCLUSTERS_FIRST_EP,                //  byte  AppNumInClusters;
    (cId_t *)zclApp_InClusterListFirstEP,       //  byte *pAppInClusterList;
    APP_MAX_OUTCLUSTERS_FIRST_EP,              //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterListFirstEP       //  byte *pAppOutClusterList;
};

// Second EP
#ifdef HAL_2_CHANNEL

CONST zclAttrRec_t zclApp_AttrsSecondEP[] = {
    {GEN_ON_OFF, {ATTRID_ON_OFF, ZCL_BOOLEAN, RW | RR, (void *)&zclApp_Config.Right}},
};

uint8 CONST zclApp_AttrsSecondEPCount = (sizeof(zclApp_AttrsSecondEP) / sizeof(zclApp_AttrsSecondEP[0]));

const cId_t zclApp_InClusterListSecondEP[] = {
  ZCL_CLUSTER_ID_GEN_BASIC,
  GEN_ON_OFF
};

const cId_t zclApp_OutClusterListSecondEP[] = {
  GEN_ON_OFF,
};

#define APP_MAX_OUTCLUSTERS_SECOND_EP (sizeof(zclApp_OutClusterListSecondEP) / sizeof(zclApp_OutClusterListSecondEP[0]))
#define APP_MAX_INCLUSTERS_SECOND_EP (sizeof(zclApp_InClusterListSecondEP) / sizeof(zclApp_InClusterListSecondEP[0]))


SimpleDescriptionFormat_t zclApp_SecondEP = {
    SECOND_ENDPOINT,                            //  int Endpoint;
    ZCL_HA_PROFILE_ID,                          //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_ON_OFF_SWITCH,              //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,                         //  int   AppDevVer:4;
    APP_FLAGS,                                  //  int   AppFlags:4;
    APP_MAX_INCLUSTERS_SECOND_EP,               //  byte  AppNumInClusters;
    (cId_t *)zclApp_InClusterListSecondEP,      //  byte *pAppInClusterList;
    APP_MAX_OUTCLUSTERS_SECOND_EP,             //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterListSecondEP      //  byte *pAppOutClusterList;

};
#endif

void zclApp_ResetAttributesToDefaultValues(void) {
    zclApp_Config.Left = DEFAULT_Left;
#ifdef HAL_2_CHANNEL
    zclApp_Config.Right = DEFAULT_Right;
#endif    
}