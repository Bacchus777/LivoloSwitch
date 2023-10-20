
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "math.h"

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_ms.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"

#include "gp_interface.h"

#include "Debug.h"

#include "OnBoard.h"

#include "commissioning.h"
#include "factory_reset.h"
/* HAL */

#include "hal_adc.h"
#include "hal_drivers.h"
#include "hal_i2c.h"
#include "hal_key.h"
#include "hal_led.h"

#include "utils.h"
#include "version.h"

#include <stdint.h>
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclApp_TaskID;

// Структура для отправки отчета
//afAddrType_t zclApp_DstAddr;
// Номер сообщения
uint8 SeqNum = 0;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */
void user_delay_ms(uint32_t period);
void user_delay_ms(uint32_t period) { MicroWait(period * 1000); }
/*********************************************************************
 * LOCAL VARIABLES
 */

afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 0, .addr.shortAddr = 0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_Report(void);
static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static void zclApp_HandleKeys(byte portAndAction, byte keyCode);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);

// Изменение состояние левого выключателя
static void updateLeft( bool );
// Изменение состояние правого выключателя
static void updateRight( bool );

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB, // Basic Cluster Reset command
    NULL,                // Identify Trigger Effect command
    zclApp_OnOffCB,      // On/Off cluster commands
    NULL,                // On/Off cluster enhanced command Off with Effect
    NULL,                // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                // On/Off cluster enhanced command On with Timed Off
    NULL,                // RSSI Location command
    NULL                 // RSSI Location Response command
};

void zclApp_Init(byte task_id) {

    zclApp_RestoreAttributesFromNV();

    zclApp_TaskID = task_id;

    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);
    zclGeneral_RegisterCmdCallbacks(zclApp_FirstEP.EndPoint, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsFirstEPCount, zclApp_AttrsFirstEP);
    zcl_registerReadWriteCB(zclApp_FirstEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);

    bdb_RegisterSimpleDescriptor(&zclApp_SecondEP);
    zclGeneral_RegisterCmdCallbacks(zclApp_SecondEP.EndPoint, &zclApp_CmdCallbacks);
    zcl_registerAttrList(zclApp_SecondEP.EndPoint, zclApp_AttrsSecondEPCount, zclApp_AttrsSecondEP);
    zcl_registerReadWriteCB(zclApp_SecondEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);

    zcl_registerForMsg(zclApp_TaskID);
    RegisterForKeys(zclApp_TaskID);

    LREP("Build %s \r\n", zclApp_DateCodeNT);

    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, APP_REPORT_DELAY);
    osal_start_reload_timer(zclApp_TaskID, HAL_KEY_EVENT, 100);

}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {

    updateLeft(keyCode & HAL_KEY_SW_2 ? true : false);
    updateRight(keyCode & HAL_KEY_SW_3 ? true : false);

}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
//    LREP("events 0x%x \r\n", events);
    if (events & SYS_EVENT_MSG) {
        afIncomingMSGPacket_t *MSGpkt;
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
//            LREP("MSGpkt->hdr.event 0x%X clusterId=0x%X\r\n", MSGpkt->hdr.event, MSGpkt->clusterId);
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
//                LREP("KEY_CHANGE\r\n");
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;

            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_SAVE_ATTRS_EVT) {
        LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }
    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
///////////////////////
        return (events ^ APP_READ_SENSORS_EVT);
    }

    return 0;
}

static void zclApp_Report(void) {
//  osal_start_reload_timer(zclApp_TaskID, APP_READ_SENSORS_EVT, 500); 
}

static void zclApp_BasicResetCB(void) {
    LREPMaster("BasicResetCB\r\n");
    zclApp_ResetAttributesToDefaultValues();
    zclApp_SaveAttributesToNV();
}

static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {
    LREPMaster("AUTH CB called\r\n");
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);

    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}

static void zclApp_SaveAttributesToNV(void) {
    uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    LREP("Saving attributes to NV write=%d\r\n", writeStatus);
}

static void zclApp_RestoreAttributesFromNV(void) {
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    LREP("Restoring attributes from NV  status=%d \r\n", status);
    if (status == NV_ITEM_UNINIT) {
        uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
        LREP("NV was empty, writing %d\r\n", writeStatus);
    }
    if (status == ZSUCCESS) {
        LREPMaster("Reading from NV\r\n");
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}

// Изменение состояния левого выключателя
void updateLeft ( bool value )
{
  zclApp_Config.Left = value;
  
  LREP("zclApp_Config.Left=%d \r\n", zclApp_Config.Left);

  // сохраняем состояние датчика
  zclApp_SaveAttributesToNV();
  bdb_RepChangedAttrValue(zclApp_FirstEP.EndPoint, GEN_ON_OFF, ATTRID_ON_OFF);
}
  
// Изменение состояния левого выключателя
void updateRight ( bool value )
{
  zclApp_Config.Right = value;
  
  LREP("zclApp_Config.Right=%d \r\n", zclApp_Config.Right);
  // сохраняем состояние датчика
  zclApp_SaveAttributesToNV();
  bdb_RepChangedAttrValue(zclApp_SecondEP.EndPoint, GEN_ON_OFF, ATTRID_ON_OFF);
}
  
// Обработчик команд кластера OnOff
static void zclApp_OnOffCB(uint8 cmd)
{

  // запомним адрес откуда пришла команда
  // чтобы отправить обратно отчет
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();
  
  if (pPtr->endPoint == FIRST_ENDPOINT) {
    if ((cmd == COMMAND_ON) & !zclApp_Config.Left) {
      HalLedSet(HAL_LED_2, HAL_LED_MODE_BLINK);
    }
    // Выключить
    else if ((cmd == COMMAND_OFF) & zclApp_Config.Left) {
      HalLedSet(HAL_LED_2, HAL_LED_MODE_BLINK);
    }
  }
  else {
    if ((cmd == COMMAND_ON) & !zclApp_Config.Right) {
      HalLedSet(HAL_LED_3, HAL_LED_MODE_BLINK);
    }
    // Выключить
    else if ((cmd == COMMAND_OFF) & zclApp_Config.Right) {
      HalLedSet(HAL_LED_3, HAL_LED_MODE_BLINK);
    }
  }
}

/****************************************************************************
****************************************************************************/
