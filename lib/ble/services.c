// Battery Service and the LP715 service, following the SDK's battservice.c.
#include "ble/services.h"
#include "ble/ble.h"
#include "OSAL.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "linkdb.h"
#include "peripheral.h"
#include "log.h"

#define UUID_BATT_SERVICE 0x180F
#define UUID_BATT_LEVEL   0x2A19

// 4c50xxxx-3731-3500-b000-000000000000, least significant byte first.
#define LP715_UUID(id) \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x35, 0x31, 0x37, LO_UINT16(id), HI_UINT16(id), 0x50, 0x4c

static CONST uint8 batt_service_uuid[ATT_BT_UUID_SIZE] = {LO_UINT16(UUID_BATT_SERVICE), HI_UINT16(UUID_BATT_SERVICE)};
static CONST uint8 batt_level_uuid[ATT_BT_UUID_SIZE] = {LO_UINT16(UUID_BATT_LEVEL), HI_UINT16(UUID_BATT_LEVEL)};
static CONST uint8 lp_service_uuid[ATT_UUID_SIZE] = {LP715_UUID(0x0001)};
static CONST uint8 lp_control_uuid[ATT_UUID_SIZE] = {LP715_UUID(0x0002)};
static CONST uint8 lp_events_uuid[ATT_UUID_SIZE] = {LP715_UUID(0x0003)};

static CONST gattAttrType_t batt_service = {ATT_BT_UUID_SIZE, batt_service_uuid};
static CONST gattAttrType_t lp_service = {ATT_UUID_SIZE, lp_service_uuid};

static uint8 batt_props = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8 batt_level = 0;
static gattCharCfg_t batt_ccc[GATT_MAX_NUM_CONN];

static uint8 control_props = GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8 control_value;
static uint8 events_props = GATT_PROP_NOTIFY;
static uint8 events_value;
static gattCharCfg_t events_ccc[GATT_MAX_NUM_CONN];

static svc_command_cb_t command_cb;

static gattAttribute_t batt_attrs[] = {
    {{ATT_BT_UUID_SIZE, primaryServiceUUID}, GATT_PERMIT_READ, 0, (uint8 *)&batt_service},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &batt_props},
    {{ATT_BT_UUID_SIZE, batt_level_uuid}, GATT_PERMIT_READ, 0, &batt_level},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *)batt_ccc},
};
#define BATT_LEVEL_IDX 2

static gattAttribute_t lp_attrs[] = {
    {{ATT_BT_UUID_SIZE, primaryServiceUUID}, GATT_PERMIT_READ, 0, (uint8 *)&lp_service},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &control_props},
    {{ATT_UUID_SIZE, lp_control_uuid}, GATT_PERMIT_WRITE, 0, &control_value},
    {{ATT_BT_UUID_SIZE, characterUUID}, GATT_PERMIT_READ, 0, &events_props},
    {{ATT_UUID_SIZE, lp_events_uuid}, 0, 0, &events_value},
    {{ATT_BT_UUID_SIZE, clientCharCfgUUID}, GATT_PERMIT_READ | GATT_PERMIT_WRITE, 0, (uint8 *)events_ccc},
};
#define EVENTS_IDX 4

static bStatus_t read_cb(uint16 conn, gattAttribute_t *attr, uint8 *value, uint16 *len, uint16 offset,
                         uint8 max_len) {
    (void)conn;
    (void)max_len;
    if (offset > 0)
        return ATT_ERR_ATTR_NOT_LONG;
    if (attr->pValue == &batt_level) {
        value[0] = batt_level;
        *len = 1;
        return SUCCESS;
    }
    return ATT_ERR_ATTR_NOT_FOUND;
}

static bStatus_t write_cb(uint16 conn, gattAttribute_t *attr, uint8 *value, uint16 len, uint16 offset) {
    if (attr->type.len == ATT_BT_UUID_SIZE &&
        BUILD_UINT16(attr->type.uuid[0], attr->type.uuid[1]) == GATT_CLIENT_CHAR_CFG_UUID)
        return GATTServApp_ProcessCCCWriteReq(conn, attr, value, len, offset, GATT_CLIENT_CFG_NOTIFY);

    if (attr->pValue == &control_value) {
        if (offset > 0)
            return ATT_ERR_ATTR_NOT_LONG;
        if (len == 0)
            return ATT_ERR_INVALID_VALUE_SIZE;
        LOG("ble: command %02x, %d bytes", value[0], len);
        if (command_cb)
            command_cb(value, len);
        return SUCCESS;
    }
    return ATT_ERR_ATTR_NOT_FOUND;
}

static CONST gattServiceCBs_t callbacks = {read_cb, write_cb, NULL};

static void notify(gattAttribute_t *attr, gattCharCfg_t *ccc, uint8 value) {
    uint16 conn;
    if (ble_state() != BLE_CONNECTED)
        return;
    GAPRole_GetParameter(GAPROLE_CONNHANDLE, &conn);
    if (!(GATTServApp_ReadCharCfg(conn, ccc) & GATT_CLIENT_CFG_NOTIFY))
        return;
    attHandleValueNoti_t n;
    n.handle = attr->handle;
    n.len = 1;
    n.value[0] = value;
    GATT_Notification(conn, &n, FALSE);
}

// Forget subscriptions when the client goes, so the next one starts clean.
static void on_disconnect(void) {
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, batt_ccc);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, events_ccc);
}

void svc_init(svc_command_cb_t on_command) {
    command_cb = on_command;
    on_disconnect();
    GATTServApp_RegisterService(batt_attrs, GATT_NUM_ATTRS(batt_attrs), &callbacks);
    GATTServApp_RegisterService(lp_attrs, GATT_NUM_ATTRS(lp_attrs), &callbacks);
    ble_on_disconnect(on_disconnect);
}

void svc_battery_set(uint8 percent) {
    if (percent == batt_level)
        return;
    batt_level = percent;
    notify(&batt_attrs[BATT_LEVEL_IDX], batt_ccc, percent);
}

void svc_event(svc_event_t event) {
    events_value = event;
    notify(&lp_attrs[EVENTS_IDX], events_ccc, event);
}
