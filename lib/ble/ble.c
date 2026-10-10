// Follows the SDK's simpleBlePeripheral example, without bonding or the
// sample profile.
#include "ble/ble.h"
#include "OSAL.h"
#include "gap.h"
#include "gatt.h"
#include "peripheral.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "log.h"
#include "config.h"

#define START_DEVICE_EVT 0x0001

#define ADV_INTERVAL 160   // 100 ms, in 625 us units

static uint8 task;
static ble_state_t state = BLE_IDLE;
static char name[GAP_DEVICE_NAME_LEN] = "LP715";

static void (*disconnect_cb)(void);

static uint8 adv_data[31];
static uint8 scan_rsp[31];

static void build_adv(void) {
    uint8 n = 0, len = 0;
    while (name[len] && len < GAP_DEVICE_NAME_LEN - 1)
        len++;

    adv_data[n++] = 2;
    adv_data[n++] = GAP_ADTYPE_FLAGS;
    adv_data[n++] = GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    adv_data[n++] = len + 1;
    adv_data[n++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    osal_memcpy(&adv_data[n], name, len);
    n += len;
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, n, adv_data);

    uint8 m = 0;
    scan_rsp[m++] = len + 1;
    scan_rsp[m++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    osal_memcpy(&scan_rsp[m], name, len);
    m += len;
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, m, scan_rsp);

    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, name);
}

static void on_state(gaprole_States_t s) {
    switch (s) {
    case GAPROLE_ADVERTISING:
        state = BLE_ADVERTISING;
        LOG("ble: advertising");
        break;
    case GAPROLE_CONNECTED:
    case GAPROLE_CONNECTED_ADV:
        state = BLE_CONNECTED;
        LOG("ble: connected");
        break;
    case GAPROLE_WAITING:
    case GAPROLE_WAITING_AFTER_TIMEOUT: {
        // Disconnected: advertise again.
        uint8 on = TRUE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(on), &on);
        state = BLE_IDLE;
        if (disconnect_cb)
            disconnect_cb();
        LOG("ble: disconnected");
        break;
    }
    default:
        state = BLE_IDLE;
        LOG("ble: state %d", s);
        break;
    }
}

static gapRolesCBs_t callbacks = {
    on_state,
    NULL,
};

void ble_set_name(const char *n) {
    uint8 i = 0;
    for (; n[i] && i < GAP_DEVICE_NAME_LEN - 1; i++)
        name[i] = n[i];
    name[i] = 0;
    build_adv();
}

void ble_on_disconnect(void (*fn)(void)) {
    disconnect_cb = fn;
}

ble_state_t ble_state(void) {
    return state;
}

// The ROM advertises from ownPublicAddr: in this SDK init_config() points the
// ROM's MAC_ADDRESS_LOC at that variable itself, so it stays all zeros (and
// scanners ignore the advertisements) unless the app fills it in. The watch's
// own address is in flash at 0x4000, lowest byte first, as the stock firmware
// left it.
#define MAC_FLASH ((const uint8 *)0x11004000)
extern uint8 ownPublicAddr[B_ADDR_LEN];

static void load_mac(void) {
    bool blank = true;
    for (int i = 0; i < B_ADDR_LEN; i++)
        if (MAC_FLASH[i] != 0xff && MAC_FLASH[i] != 0x00)
            blank = false;
    if (blank) {
        static const uint8 fallback[B_ADDR_LEN] = {0x15, 0x07, 0x22, 0x62, 0xd0, 0x00};
        osal_memcpy(ownPublicAddr, fallback, B_ADDR_LEN);
    } else {
        osal_memcpy(ownPublicAddr, MAC_FLASH, B_ADDR_LEN);
    }
    LOG("ble: address %02x:%02x:%02x:%02x:%02x:%02x%s", ownPublicAddr[5], ownPublicAddr[4],
        ownPublicAddr[3], ownPublicAddr[2], ownPublicAddr[1], ownPublicAddr[0],
        blank ? " (fallback)" : "");
}

void ble_init(uint8 task_id) {
    task = task_id;
    load_mac();

    uint8 adv_on = TRUE;
    uint8 adv_type = GAP_ADTYPE_ADV_IND;
    uint8 channels = GAP_ADVCHAN_37 | GAP_ADVCHAN_38 | GAP_ADVCHAN_39;
    uint16 adv_off = 0;
    uint8 update = TRUE;
    uint16 min_int = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16 max_int = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16 latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16 timeout = DEFAULT_DESIRED_CONN_TIMEOUT;

    GAP_SetParamValue(TGAP_CONN_PAUSE_PERIPHERAL, 6);
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(adv_type), &adv_type);
    GAPRole_SetParameter(GAPROLE_ADV_CHANNEL_MAP, sizeof(channels), &channels);
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(adv_on), &adv_on);
    GAPRole_SetParameter(GAPROLE_ADVERT_OFF_TIME, sizeof(adv_off), &adv_off);
    GAPRole_SetParameter(GAPROLE_PARAM_UPDATE_ENABLE, sizeof(update), &update);
    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(min_int), &min_int);
    GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(max_int), &max_int);
    GAPRole_SetParameter(GAPROLE_SLAVE_LATENCY, sizeof(latency), &latency);
    GAPRole_SetParameter(GAPROLE_TIMEOUT_MULTIPLIER, sizeof(timeout), &timeout);
    build_adv();

    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MIN, ADV_INTERVAL);
    GAP_SetParamValue(TGAP_LIM_DISC_ADV_INT_MAX, ADV_INTERVAL);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MIN, ADV_INTERVAL);
    GAP_SetParamValue(TGAP_GEN_DISC_ADV_INT_MAX, ADV_INTERVAL);

    GGS_AddService(GATT_ALL_SERVICES);
    GATTServApp_AddService(GATT_ALL_SERVICES);

    // GAPRole must start from this task's event loop, not during init.
    osal_set_event(task, START_DEVICE_EVT);
}

uint16 ble_process_event(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        uint8 *msg;
        while ((msg = osal_msg_receive(task_id)) != NULL)
            osal_msg_deallocate(msg);
        return events ^ SYS_EVENT_MSG;
    }
    if (events & START_DEVICE_EVT) {
        GAPRole_StartDevice(&callbacks);
        LOG("ble: started as \"%s\"", name);
        return events ^ START_DEVICE_EVT;
    }
    return 0;
}
