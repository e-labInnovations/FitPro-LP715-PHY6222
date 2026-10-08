// OSAL task table. Adapted from amir1387aht/phy6222_smartwatch (MIT) and the
// SDK's simpleBlePeripheral example.
//
// Without BLE: only the link layer and the callback-timer task are set up, and
// the app runs as a plain loop from osalInitTasks(), so OSAL's scheduler never
// takes over.
//
// With BLE (CFG_BLE): the full host stack runs under the scheduler, followed by
// lib/ble's task and an app task that calls app_init() once and app_update()
// every APP_TICK_MS. app_update() must then return quickly.
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "osal_cbtimer.h"
#include "ll.h"
#include "app.h"

#ifdef CFG_BLE
#include "hci_tl.h"
#include "l2cap.h"
#include "gap.h"
#include "gatt.h"
#include "sm.h"
#include "peripheral.h"
#include "gattservapp.h"
#include "ble/ble.h"

#define APP_TICK_MS  10
#define APP_TICK_EVT 0x0001

uint8 app_task_id;

static uint16 App_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        uint8 *msg;
        while ((msg = osal_msg_receive(task_id)) != NULL)
            osal_msg_deallocate(msg);
        return events ^ SYS_EVENT_MSG;
    }
    if (events & APP_TICK_EVT) {
        app_update();
        return events ^ APP_TICK_EVT;
    }
    return 0;
}

const pTaskEventHandlerFn tasksArr[] = {
    LL_ProcessEvent,
    HCI_ProcessEvent,
#if defined(OSAL_CBTIMER_NUM_TASKS)
    OSAL_CBTIMER_PROCESS_EVENT(osal_CbTimerProcessEvent),
#endif
    L2CAP_ProcessEvent,
    SM_ProcessEvent,
    GAP_ProcessEvent,
    GATT_ProcessEvent,
    GAPRole_ProcessEvent,
    GATTServApp_ProcessEvent,
    ble_process_event,
    App_ProcessEvent,
};
#else
const pTaskEventHandlerFn tasksArr[] = {
    LL_ProcessEvent,
#if defined(OSAL_CBTIMER_NUM_TASKS)
    OSAL_CBTIMER_PROCESS_EVENT(osal_CbTimerProcessEvent),
#endif
};
#endif

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

void osalInitTasks(void) {
    uint8 taskID = 0;

    tasksEvents = (uint16 *)osal_mem_alloc(sizeof(uint16) * tasksCnt);
    osal_memset(tasksEvents, 0, sizeof(uint16) * tasksCnt);

    LL_Init(taskID++);
#ifdef CFG_BLE
    HCI_Init(taskID++);
#endif
#if defined(OSAL_CBTIMER_NUM_TASKS)
    osal_CbTimerInit(taskID);
    taskID += OSAL_CBTIMER_NUM_TASKS;
#endif

#ifdef CFG_BLE
    L2CAP_Init(taskID++);
    SM_Init(taskID++);
    GAP_Init(taskID++);
    GATT_Init(taskID++);
    GAPRole_Init(taskID++);
    GATTServApp_Init(taskID++);
    ble_init(taskID++);
    app_task_id = taskID++;
    app_init();
    osal_start_reload_timer(app_task_id, APP_TICK_EVT, APP_TICK_MS);
#else
    app_init();
    while (1)
        app_update();
#endif
}
