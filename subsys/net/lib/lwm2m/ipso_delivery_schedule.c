// ...existing code...
/*
 * Delivery Schedule (Object ID 10264)
 *
 * Scaffolding adaptado al estilo de ipso_generic_sensor.c
 *
 * Recursos:
 * 0 - Schedule Start Time (Integer, RW, Single, Mandatory)
 * 1 - Schedule UTC Offset (String, RW, Single, Mandatory)
 * 2 - Delivery Frequency (Integer, RW, Single, Mandatory)
 * 3 - Randomised Delivery Window (Integer, RW, Single, Optional)
 * 4 - Number of Retries (Integer, RW, Single, Optional)
 * 5 - Retry Period (Integer, RW, Single, Optional)
 *
 * Nota: completar integración con el engine LwM2M (creación res_inst, set_res_data, notify, etc.)
 */

#define LOG_MODULE_NAME net_ipso_delivery_schedule
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


#define DELIVERY_VERSION_MAJOR 1
#define DELIVERY_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_DELIVERY_SCHEDULE_INSTANCE_COUNT

/* Sizes */
#define UTC_OFFSET_STR_MAX_SIZE 32

/* Fields */
#define NUMBER_OF_OBJ_FIELDS 6
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS)

static int32_t schedule_start_time[MAX_INSTANCE_COUNT];
static char schedule_utc_offset[MAX_INSTANCE_COUNT][UTC_OFFSET_STR_MAX_SIZE];
static int32_t delivery_frequency[MAX_INSTANCE_COUNT];
static int32_t randomised_window[MAX_INSTANCE_COUNT];
static int32_t number_of_retries[MAX_INSTANCE_COUNT];
static int32_t retry_period[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj schedule_obj;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(SCHEDULE_START_TIME_RID, RW, INT),
    OBJ_FIELD_DATA(SCHEDULE_UTC_OFFSET_RID, RW, STRING),
    OBJ_FIELD_DATA(DELIVERY_FREQUENCY_RID, RW, INT),
    OBJ_FIELD_DATA(RANDOMISED_WINDOW_RID, RW_OPT, INT),
    OBJ_FIELD_DATA(NUMBER_OF_RETRIES_RID, RW_OPT, INT),
    OBJ_FIELD_DATA(RETRY_PERIOD_RID, RW_OPT, INT),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJ_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

static struct lwm2m_engine_obj *schedule_obj_ptr = &schedule_obj;

static struct lwm2m_engine_obj_inst *delivery_schedule_create(uint16_t obj_inst_id)
{
    int index, i = 0, j = 0;

    /* Check duplicates */
    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
            LOG_ERR("Instance already exists: %u", obj_inst_id);
            return NULL;
        }
    }

    /* Find free slot */
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
    schedule_start_time[index] = 0;
    schedule_utc_offset[index][0] = '\0';
    delivery_frequency[index] = 0;
    randomised_window[index] = 0;
    number_of_retries[index] = 0;
    retry_period[index] = 0;

    (void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* Initialize resources
     * - For single-instance integer use INIT_OBJ_RES_OPTDATA (container) here.
     * - For single-instance string use INIT_OBJ_RES_DATA_LEN.
     *
     * TODO: For full integration, call lwm2m_engine_set_res_data() / lwm2m_set_res_buf()
     * to point engine resources to the storage above, and register read/write callbacks
     * as required by your Zephyr version.
     */

    /* Schedule Start Time (Integer, single, mandatory) */
    INIT_OBJ_RES_OPTDATA(SCHEDULE_START_TIME_RID, res[index], i, res_inst[index], j);

    /* Schedule UTC Offset (String, single, mandatory) */
    INIT_OBJ_RES_DATA_LEN(SCHEDULE_UTC_OFFSET_RID, res[index], i, res_inst[index], j,
                 schedule_utc_offset[index], UTC_OFFSET_STR_MAX_SIZE, 0);

    /* Delivery Frequency (Integer, single, mandatory) */
    INIT_OBJ_RES_OPTDATA(DELIVERY_FREQUENCY_RID, res[index], i, res_inst[index], j);

    /* Randomised Delivery Window (Integer, single, optional) */
    INIT_OBJ_RES_OPTDATA(RANDOMISED_WINDOW_RID, res[index], i, res_inst[index], j);

    /* Number of Retries (Integer, single, optional) */
    INIT_OBJ_RES_OPTDATA(NUMBER_OF_RETRIES_RID, res[index], i, res_inst[index], j);

    /* Retry Period (Integer, single, optional) */
    INIT_OBJ_RES_OPTDATA(RETRY_PERIOD_RID, res[index], i, res_inst[index], j);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created IPSO Delivery Schedule instance: %d", obj_inst_id);
    return &inst[index];
}

static int ipso_delivery_schedule_init(void)
{
    schedule_obj.obj_id = IPSO_OBJECT_DELIVERY_SCHEDULE_ID;
    schedule_obj.version_major = DELIVERY_VERSION_MAJOR;
    schedule_obj.version_minor = DELIVERY_VERSION_MINOR;
    schedule_obj.is_core = false;
    schedule_obj.fields = fields;
    schedule_obj.field_count = ARRAY_SIZE(fields);
    schedule_obj.max_instance_count = MAX_INSTANCE_COUNT;
    schedule_obj.create_cb = delivery_schedule_create;

    lwm2m_register_obj(&schedule_obj);

    LOG_DBG("IPSO Delivery Schedule object registered (ID %d)",
        IPSO_OBJECT_DELIVERY_SCHEDULE_ID);

    return 0;
}

LWM2M_OBJ_INIT(ipso_delivery_schedule_init);