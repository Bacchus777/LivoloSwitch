
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

// Состояние кнопок
static uint8 halKeySavedKeys;

// Структура для отправки отчета
afAddrType_t zclApp_DstAddr;
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
// Изменение состояние реле
static void applySensor( void );
// Отправка отчета о включении датчика
void zclApp_ReportOnOff( void );
// Отправка отчета о присутствии
void zclApp_ReportOutput( void );


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

    LREP("BV(3) =  %X BV(3) = %X\r\n", BV(3), BV(5));
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    static byte prevKeyCode = 0;
/*    if (keyCode == 0x28) {
        return;
    }
*/
//    LREP("zclApp_HandleKeys PRESS = %X keyCode1=%X  keyCode2=%X\r\n", portAndAction & HAL_KEY_PRESS ? true : false, keyCode & HAL_KEY_BIT3 ? true : false, keyCode & HAL_KEY_BIT5 ? true : false);

//    LREP("%X\r\n", keyCode);

    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
/*  
    if (portAndAction & HAL_KEY_PRESS) {
       HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
    if (portAndAction & HAL_KEY_RELEASE) {
       HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
    }
*/    
    
/*    if ((portAndAction & HAL_KEY_PRESS) & (keyCode & HAL_KEY_BIT3)) {
       LREP("1\r\n");
       HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
    if ((portAndAction & HAL_KEY_RELEASE) & (keyCode & HAL_KEY_BIT3)) {
       LREP("2\r\n");
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
    }
    if ((portAndAction & HAL_KEY_PRESS) & (keyCode & HAL_KEY_BIT5)) {
       LREP("3\r\n");
        HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
    }
    if ((portAndAction & HAL_KEY_RELEASE) & (keyCode & HAL_KEY_BIT5)) {
       LREP("4\r\n");
        HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
    }

    if (keyCode & HAL_KEY_BIT3 ? true : false) {
       if (portAndAction & HAL_KEY_PRESS ? true : false) {
        HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
        LREP("LEFT ON\r\n");
       }
       else if (portAndAction & HAL_KEY_RELEASE ? true : false) {
        HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
        LREP("LEFT OFF\r\n");
       }
    }
         
    if (keyCode & HAL_KEY_BIT5 ? true : false) {
       if (portAndAction & HAL_KEY_PRESS ? true : false) {
        HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
        LREP("RIGHT ON\r\n");
       }
       else if (portAndAction & HAL_KEY_RELEASE ? true : false) {
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
        LREP("RIGHT OFF\r\n");
       }
    }
*/         
    if (keyCode & HAL_KEY_BIT3) {
       if (portAndAction & HAL_KEY_PRESS) {
        HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
       }
       else if (portAndAction & HAL_KEY_RELEASE) {
        HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
       }
    }
         
    if (keyCode & HAL_KEY_BIT5) {
       if (portAndAction & HAL_KEY_PRESS) {
        HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
       }
       else if (portAndAction & HAL_KEY_RELEASE) {
        HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
       }
    }
         
       
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
//    LREP("events 0x%x \r\n", events);
    if (events & SYS_EVENT_MSG) {
        afIncomingMSGPacket_t *MSGpkt;
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
//            LREP("MSGpkt->hdr.event 0x%X clusterId=0x%X\r\n", MSGpkt->hdr.event, MSGpkt->clusterId);
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                LREP("KEY_CHANGE\r\n");
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
        applySensor();
    }
}

// Изменение состояния левого выключателя
void updateLeft ( bool value )
{
  if (value) {
    zclApp_Config.Left = 1;
  } else {
    zclApp_Config.Left = 0;
  }
  // сохраняем состояние датчика
  zclApp_SaveAttributesToNV();
}
  
// Изменение состояния левого выключателя
void updateRight ( bool value )
{
  if (value) {
    zclApp_Config.Right = 1;
  } else {
    zclApp_Config.Right = 0;
  }
  // сохраняем состояние датчика
  zclApp_SaveAttributesToNV();
}
  
// Применение состояние реле
void applySensor ( void )
{
/*  // если выключено
  if (zclApp_Config.SensorEnabled == 0) {
    // то гасим светодиод 1
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
  } else {
    // иначе включаем светодиод 1
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
  }
*/
}

// Обработчик команд кластера OnOff
static void zclApp_OnOffCB(uint8 cmd)
{

  // запомним адрес откуда пришла команда
  // чтобы отправить обратно отчет
  afIncomingMSGPacket_t *pPtr = zcl_getRawAFMsg();
  zclApp_DstAddr.addr.shortAddr = pPtr->srcAddr.addr.shortAddr;
  
  if (pPtr->endPoint == FIRST_ENDPOINT) {
    if (cmd == COMMAND_ON) {
      HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
    }
    // Выключить
    else if (cmd == COMMAND_OFF) {
      HalLedSet(HAL_LED_2, HAL_LED_MODE_OFF);
    }
  }
  else {
    if (cmd == COMMAND_ON) {
      HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
    }
    // Выключить
    else if (cmd == COMMAND_OFF) {
      HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
    }
  }
    
  // Включить
/*  if (cmd == COMMAND_ON) {
    updateSensor(TRUE);
  }
  // Выключить
  else if (cmd == COMMAND_OFF) {
    updateSensor(FALSE);
  }
  // Переключить
  else if (cmd == COMMAND_TOGGLE) {
    updateSensor(zclApp_Config.SensorEnabled == 0);
  }*/
}

// Информирование о включении датчика
void zclApp_ReportOnOff(void) {
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd;

  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&zclApp_Config.Left);

    zclApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclApp_DstAddr.addr.shortAddr = 0;
    zclApp_DstAddr.endPoint = FIRST_ENDPOINT;

    zcl_SendReportCmd(FIRST_ENDPOINT, &zclApp_DstAddr,
                      ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
                      ZCL_FRAME_CLIENT_SERVER_DIR, false, SeqNum++);
  }

  osal_mem_free(pReportCmd);
}

// Информирование о присутствии
void zclApp_ReportOutput(void) {
  const uint8 NUM_ATTRIBUTES = 1;

  zclReportCmd_t *pReportCmd;

  pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) +
                              (NUM_ATTRIBUTES * sizeof(zclReport_t)));
  if (pReportCmd != NULL) {
    pReportCmd->numAttr = NUM_ATTRIBUTES;

    pReportCmd->attrList[0].attrID = ATTRID_ON_OFF;
    pReportCmd->attrList[0].dataType = ZCL_DATATYPE_BOOLEAN;
    pReportCmd->attrList[0].attrData = (void *)(&zclApp_Output);

    zclApp_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;
    zclApp_DstAddr.addr.shortAddr = 0;
    zclApp_DstAddr.endPoint = SECOND_ENDPOINT;

    zcl_SendReportCmd(SECOND_ENDPOINT, &zclApp_DstAddr,
                      ZCL_CLUSTER_ID_GEN_ON_OFF, pReportCmd,
                      ZCL_FRAME_CLIENT_SERVER_DIR, false, SeqNum++);
    
    if (zclApp_Output == 1)
      zclGeneral_SendOnOff_CmdOn(zclApp_SecondEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
    else
      zclGeneral_SendOnOff_CmdOff(zclApp_SecondEP.EndPoint, &inderect_DstAddr, TRUE, bdb_getZCLFrameCounter());
  }

  osal_mem_free(pReportCmd);
}

void App_HalKeyInit(void)
{
  LREP("App_HalKeyInit\r\n");

  PUSH1_SEL &= ~(PUSH1_BV); // Выставляем функцию пина - GPIO 
  PUSH1_DIR &= ~(PUSH1_BV); // Выставляем режим пина - Вход 
  
//  PUSH1_ICTL &= ~(PUSH1_ICTLBIT); // Не генерируем прерывания на пине 
//  PUSH1_IEN &= ~(PUSH1_IENBIT);   // Очищаем признак включения прерываний 
  
/*  PUSH2_SEL &= ~(PUSH2_BV); // Set pin function to GPIO 
  PUSH2_DIR &= ~(PUSH2_BV); // Set pin direction to Input 
  
  PUSH2_ICTL &= ~(PUSH2_ICTLBIT); // don't generate interrupt 
  PUSH2_IEN &= ~(PUSH2_IENBIT);   // Clear interrupt enable bit 
*/
}

// Считывание кнопок
void App_HalKeyPoll (void)
{
  LREPMaster("App_HalKeyPoll\r\n");
  uint8 keys = 0;

  // нажата кнопка 1 ?
  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_1;
  }
  
  // нажата кнопка 2 ?
  if (HAL_PUSH_BUTTON2())
  {
    keys |= HAL_KEY_SW_2;
  }
  
  if (keys == halKeySavedKeys)
  {
    // Выход - нет изменений
    return;
  }
  // Сохраним текущее состояние кнопок для сравнения в след раз
  halKeySavedKeys = keys;

  // Вызовем генерацию события изменений кнопок
  OnBoard_SendKeys(keys, HAL_KEY_STATE_NORMAL);
}

/****************************************************************************
****************************************************************************/
