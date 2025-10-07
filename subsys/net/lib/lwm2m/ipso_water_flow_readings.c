/*
 * Water Flow Readings (Object ID 10266)
 *
 * Scaffolding generado a partir del XML de objeto (Recursos 6000..6029)
 *
 * Recursos incluidos:
 * 6000 Interval Period            (R, Integer)
 * 6001 Interval Start Offset     (R, Integer)
 * 6002 Interval UTC Offset       (R, String)
 * 6003 Interval Collection Start Time (R, Time/Integer)
 * 6004 Oldest Recorded Interval  (R, Time/Integer)
 * 6005 Last Delivered Interval   (RW, Time/Integer)
 * 6006 Latest Recorded Interval  (R, Time/Integer)
 * 6007 Interval Delivery Midnight Aligned (RW, Boolean)
 * 6008 Interval Historical Read  (E, Execute)  -- placeholder callback
 * 6009 Interval Historical Read Payload (R, Opaque)
 * 6010 Interval Change Configuration (E, Execute) -- placeholder
 * 6026 Start (E, Execute)       -- placeholder
 * 6027 Stop (E, Execute)        -- placeholder
 * 6028 Status (R, Integer)
 * 6029 Latest Payload (R, Opaque)
 *
 * Nota: completar las implementaciones de los callbacks EXEC y la gestión
 * de payloads OPAQUE con buffer dinámico según tu aplicación.
 */

#define LOG_MODULE_NAME net_ipso_water_flow
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdint.h>
#include <zephyr/init.h>
#include <string.h>
#include <stdio.h>

#include "lwm2m_object.h"
#include "lwm2m_engine.h"
#include "lwm2m_resource_ids.h"


#define WATER_FLOW_VERSION_MAJOR 1
#define WATER_FLOW_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_WATER_FLOW_READINGS_INSTANCE_COUNT

/* Tunables for OPAQUE buffers */
#define LATEST_PAYLOAD_BUF_SIZE      1024
#define HISTORICAL_PAYLOAD_BUF_SIZE  2048
#define UTC_OFFSET_STR_MAX_SIZE      32

/* Number of fields we register (choose the resources we create) */
#define NUMBER_OF_OBJ_FIELDS 15
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS)

static int32_t interval_period[MAX_INSTANCE_COUNT];
static int32_t interval_start_offset[MAX_INSTANCE_COUNT];
static char interval_utc_offset[MAX_INSTANCE_COUNT][UTC_OFFSET_STR_MAX_SIZE];

static int32_t interval_collection_start[MAX_INSTANCE_COUNT];
static int32_t oldest_recorded_interval[MAX_INSTANCE_COUNT];
static int32_t last_delivered_interval[MAX_INSTANCE_COUNT];
static int32_t latest_recorded_interval[MAX_INSTANCE_COUNT];

static bool delivery_midnight_aligned[MAX_INSTANCE_COUNT];
static int32_t status_value[MAX_INSTANCE_COUNT];

/* Opaque payload buffers */
static uint8_t latest_payload[MAX_INSTANCE_COUNT][LATEST_PAYLOAD_BUF_SIZE];
static size_t latest_payload_len[MAX_INSTANCE_COUNT];

static uint8_t historical_payload[MAX_INSTANCE_COUNT][HISTORICAL_PAYLOAD_BUF_SIZE];
static size_t historical_payload_len[MAX_INSTANCE_COUNT];

/* placeholders for execute callbacks */
static int interval_historical_read_exec(uint16_t obj_inst_id, uint16_t res_id,
                     uint16_t res_inst_id, uint8_t *args, uint16_t args_len)
{
    ARG_UNUSED(obj_inst_id);
    ARG_UNUSED(res_id);
    ARG_UNUSED(res_inst_id);
    ARG_UNUSED(args);
    ARG_UNUSED(args_len);

    /* Implementar la lectura histórica: rellenar historical_payload[...] y
     * notificar/actualizar INTERVAL_HISTORICAL_PAYLOAD_RID si procede.
     */
    return 0;
}

static int interval_change_config_exec(uint16_t obj_inst_id, uint16_t res_id,
                       uint16_t res_inst_id, uint8_t *args, uint16_t args_len)
{
    ARG_UNUSED(obj_inst_id);
    ARG_UNUSED(res_id);
    ARG_UNUSED(res_inst_id);
    ARG_UNUSED(args);
    ARG_UNUSED(args_len);

    /* Implementar reconfiguración de period/start/utc. */
    return 0;
}

static int start_exec(uint16_t obj_inst_id, uint16_t res_id,
              uint16_t res_inst_id, uint8_t *args, uint16_t args_len)
{
    ARG_UNUSED(obj_inst_id);
    ARG_UNUSED(res_id);
    ARG_UNUSED(res_inst_id);
    ARG_UNUSED(args);
    ARG_UNUSED(args_len);

    /* Implementa la lógica de Start (habilitar grabación de intervalos). */
    return 0;
}

static int stop_exec(uint16_t obj_inst_id, uint16_t res_id,
             uint16_t res_inst_id, uint8_t *args, uint16_t args_len)
{
    ARG_UNUSED(obj_inst_id);
    ARG_UNUSED(res_id);
    ARG_UNUSED(res_inst_id);
    ARG_UNUSED(args);
    ARG_UNUSED(args_len);

    /* Implementa la lógica de Stop (deshabilitar grabación). */
    return 0;
}

/* LwM2M object structures */
static struct lwm2m_engine_obj water_flow_obj;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(INTERVAL_PERIOD_RID, R, INT),
    OBJ_FIELD_DATA(INTERVAL_START_OFFSET_RID, R_OPT, INT),
    OBJ_FIELD_DATA(INTERVAL_UTC_OFFSET_RID, R_OPT, STRING),
    OBJ_FIELD_DATA(INTERVAL_COLLECTION_START_RID, R, INT),
    OBJ_FIELD_DATA(OLDEST_RECORDED_INTERVAL_RID, R, INT),
    OBJ_FIELD_DATA(LAST_DELIVERED_INTERVAL_RID, RW, INT),
    OBJ_FIELD_DATA(LATEST_RECORDED_INTERVAL_RID, R, INT),
    OBJ_FIELD_DATA(DELIVERY_MIDNIGHT_ALIGNED_RID, RW, BOOL),

    OBJ_FIELD_DATA(INTERVAL_HISTORICAL_READ_RID, X_OPT, OPAQUE),
    OBJ_FIELD_DATA(INTERVAL_HISTORICAL_PAYLOAD_RID, R_OPT, OPAQUE),
    OBJ_FIELD_DATA(INTERVAL_CHANGE_CONFIG_RID, X_OPT, OPAQUE),
    OBJ_FIELD_DATA(START_RID, X_OPT, OPAQUE),
    OBJ_FIELD_DATA(STOP_RID, X_OPT, OPAQUE),
    OBJ_FIELD_DATA(STATUS_RID, R, INT),
    OBJ_FIELD_DATA(LATEST_PAYLOAD_RID_10266, R, OPAQUE),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJ_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

/* create callback */
static struct lwm2m_engine_obj_inst *water_flow_create(uint16_t obj_inst_id)
{
    int index, i = 0, j = 0;

    /* prevent unused-warning for arrays until fully implemented */
    (void)latest_payload;
    (void)latest_payload_len;
    (void)historical_payload;
    (void)historical_payload_len;

    /* check duplicates */
    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
            LOG_ERR("Instance already exists: %u", obj_inst_id);
            return NULL;
        }
    }

    /* find free slot */
    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (!inst[index].obj) {
            break;
        }
    }
    if (index >= MAX_INSTANCE_COUNT) {
        LOG_ERR("No room for new instance: %u", obj_inst_id);
        return NULL;
    }

    /* defaults */
    interval_period[index] = 0;
    interval_start_offset[index] = 0;
    interval_utc_offset[index][0] = '\0';
    interval_collection_start[index] = 0;
    oldest_recorded_interval[index] = 0;
    last_delivered_interval[index] = 0;
    latest_recorded_interval[index] = 0;
    delivery_midnight_aligned[index] = true;
    status_value[index] = 0;
    latest_payload_len[index] = 0;
    historical_payload_len[index] = 0;

    (void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* Register resources */
    INIT_OBJ_RES_DATA(INTERVAL_PERIOD_RID, res[index], i, res_inst[index], j,
              &interval_period[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA(INTERVAL_START_OFFSET_RID, res[index], i, res_inst[index], j,
              &interval_start_offset[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA_LEN(INTERVAL_UTC_OFFSET_RID, res[index], i, res_inst[index], j,
                  interval_utc_offset[index], UTC_OFFSET_STR_MAX_SIZE, 0);
    INIT_OBJ_RES_DATA(INTERVAL_COLLECTION_START_RID, res[index], i, res_inst[index], j,
              &interval_collection_start[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA(OLDEST_RECORDED_INTERVAL_RID, res[index], i, res_inst[index], j,
              &oldest_recorded_interval[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA(LAST_DELIVERED_INTERVAL_RID, res[index], i, res_inst[index], j,
              &last_delivered_interval[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA(LATEST_RECORDED_INTERVAL_RID, res[index], i, res_inst[index], j,
              &latest_recorded_interval[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA(DELIVERY_MIDNIGHT_ALIGNED_RID, res[index], i, res_inst[index], j,
              &delivery_midnight_aligned[index], sizeof(bool));

    /* Execute resources: create placeholders (engine needs exec callbacks registered) */
    INIT_OBJ_RES_OPTDATA(INTERVAL_HISTORICAL_READ_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_DATA_LEN(INTERVAL_HISTORICAL_PAYLOAD_RID, res[index], i, res_inst[index], j,
                  historical_payload[index], HISTORICAL_PAYLOAD_BUF_SIZE, 0);
    INIT_OBJ_RES_OPTDATA(INTERVAL_CHANGE_CONFIG_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(START_RID, res[index], i, res_inst[index], j);
    INIT_OBJ_RES_OPTDATA(STOP_RID, res[index], i, res_inst[index], j);

    INIT_OBJ_RES_DATA(STATUS_RID, res[index], i, res_inst[index], j, &status_value[index], sizeof(int32_t));
    INIT_OBJ_RES_DATA_LEN(LATEST_PAYLOAD_RID_10266, res[index], i, res_inst[index], j,
                  latest_payload[index], LATEST_PAYLOAD_BUF_SIZE, 0);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created IPSO Water Flow Readings instance: %d", obj_inst_id);
    return &inst[index];
}

static int ipso_water_flow_init(void)
{
    water_flow_obj.obj_id = IPSO_OBJECT_WATER_FLOW_READINGS_ID;
    water_flow_obj.version_major = WATER_FLOW_VERSION_MAJOR;
    water_flow_obj.version_minor = WATER_FLOW_VERSION_MINOR;
    water_flow_obj.is_core = false;
    water_flow_obj.fields = fields;
    water_flow_obj.field_count = ARRAY_SIZE(fields);
    water_flow_obj.max_instance_count = MAX_INSTANCE_COUNT;
    water_flow_obj.create_cb = water_flow_create;

    lwm2m_register_obj(&water_flow_obj);

    LOG_DBG("IPSO Water Flow Readings object registered (ID %d)",
        IPSO_OBJECT_WATER_FLOW_READINGS_ID);

    return 0;
}

LWM2M_OBJ_INIT(ipso_water_flow_init);
