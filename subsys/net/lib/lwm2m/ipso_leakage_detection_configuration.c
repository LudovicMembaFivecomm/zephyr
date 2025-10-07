/*
 * Leakage Detection Configuration (Object ID 10265)
 *
 * Scaffolding adaptado al estilo de ipso_generic_sensor.c
 *
 * Recursos (según XML):
 * 0 - Sample Times (Integer, RW, Multiple, Mandatory)
 * 1 - Sample UTC Offset (String, RW, Single, Optional)
 * 2 - Detection Mode (Integer, RW, Single, Mandatory)
 * 3 - Top Frequency Count (Integer, RW, Single, Optional)
 * 4 - Frequency Thresholds (Integer, RW, Multiple, Optional)
 * 5 - Frequency Values (Integer, R, Multiple, Optional)
 * 7 - Firmware Version (String, R, Single, Mandatory)
 *
 * Nota: completar integración con engine (crear res_inst dinámicos,
 * lwm2m_engine_set_res_data, notificaciones, callbacks de lectura/escritura).
 */

#define LOG_MODULE_NAME net_ipso_leakage_detection
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


#define LEAKAGE_VERSION_MAJOR 1
#define LEAKAGE_VERSION_MINOR 0

#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_LEAKAGE_DETECTION_CONFIGURATION_INSTANCE_COUNT

/* Tunables */
#define MAX_SAMPLE_COUNT_PER_INST     16
#define MAX_FREQ_BANDS_PER_INST       64
#define UTC_OFFSET_STR_MAX_SIZE       32
#define FIRMWARE_VER_STR_MAX_SIZE     32

/* Fields */
#define NUMBER_OF_OBJ_FIELDS 7
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS)

static int32_t sample_times[MAX_INSTANCE_COUNT][MAX_SAMPLE_COUNT_PER_INST];
static size_t sample_times_count[MAX_INSTANCE_COUNT];

static char sample_utc_offset[MAX_INSTANCE_COUNT][UTC_OFFSET_STR_MAX_SIZE];

static int32_t detection_mode[MAX_INSTANCE_COUNT];
static int32_t top_frequency_count[MAX_INSTANCE_COUNT];

static int32_t frequency_thresholds[MAX_INSTANCE_COUNT][MAX_FREQ_BANDS_PER_INST];
static size_t frequency_thresholds_count[MAX_INSTANCE_COUNT];

static int32_t frequency_values[MAX_INSTANCE_COUNT][MAX_FREQ_BANDS_PER_INST];
static size_t frequency_values_count[MAX_INSTANCE_COUNT];

static char firmware_version[MAX_INSTANCE_COUNT][FIRMWARE_VER_STR_MAX_SIZE];

static struct lwm2m_engine_obj leakage_obj;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(SAMPLE_TIMES_RID, RW, INT),
    OBJ_FIELD_DATA(SAMPLE_UTC_OFFSET_RID, RW_OPT, STRING),
    OBJ_FIELD_DATA(DETECTION_MODE_RID, RW, INT),
    OBJ_FIELD_DATA(TOP_FREQUENCY_COUNT_RID, RW_OPT, INT),
    OBJ_FIELD_DATA(FREQUENCY_THRESHOLDS_RID, RW_OPT, INT),
    OBJ_FIELD_DATA(FREQUENCY_VALUES_RID, R_OPT, INT),
    OBJ_FIELD_DATA(FIRMWARE_VERSION_RID, R, STRING),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJ_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

/* create callback */
static struct lwm2m_engine_obj_inst *leakage_create(uint16_t obj_inst_id)
{
    int index, i = 0, j = 0;

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
    sample_times_count[index] = 0;
    sample_utc_offset[index][0] = '\0';
    detection_mode[index] = 1; /* default Alarm Only */
    top_frequency_count[index] = 3; /* default if not provided */
    frequency_thresholds_count[index] = 0;
    frequency_values_count[index] = 0;
    firmware_version[index][0] = '\0';

    (void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* Initialize resources.
     * For multi-instance resources (sample times, thresholds, values) we
     * register a container here. When the application adds entries it must
     * create resource instances and call lwm2m_engine_set_res_data() or
     * equivalent to point engine to the storage above.
     */

    /* Sample Times (multiple, integer) */
    INIT_OBJ_RES_OPTDATA(SAMPLE_TIMES_RID, res[index], i, res_inst[index], j);

    /* Sample UTC Offset (single string) */
    INIT_OBJ_RES_DATA_LEN(SAMPLE_UTC_OFFSET_RID, res[index], i, res_inst[index], j,
                 sample_utc_offset[index], UTC_OFFSET_STR_MAX_SIZE, 0);

    /* Detection Mode (single int) */
    INIT_OBJ_RES_OPTDATA(DETECTION_MODE_RID, res[index], i, res_inst[index], j);

    /* Top Frequency Count (single int, optional) */
    INIT_OBJ_RES_OPTDATA(TOP_FREQUENCY_COUNT_RID, res[index], i, res_inst[index], j);

    /* Frequency Thresholds (multiple int, optional) */
    INIT_OBJ_RES_OPTDATA(FREQUENCY_THRESHOLDS_RID, res[index], i, res_inst[index], j);

    /* Frequency Values (multiple int, read-only, optional) */
    INIT_OBJ_RES_OPTDATA(FREQUENCY_VALUES_RID, res[index], i, res_inst[index], j);

    /* Firmware Version (single string, readonly, mandatory) */
    INIT_OBJ_RES_DATA_LEN(FIRMWARE_VERSION_RID, res[index], i, res_inst[index], j,
                 firmware_version[index], FIRMWARE_VER_STR_MAX_SIZE, 0);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created IPSO Leakage Detection Configuration instance: %d", obj_inst_id);
    return &inst[index];
}

static int ipso_leakage_detection_init(void)
{
    /* Evitar advertencias de variables definidas pero aún no usadas.
     * Cuando se implementen las funciones que manipulan recursos multi-instancia,
     * estas referencias se eliminarán.
     */
    (void)sample_times;
    (void)sample_times_count;
    (void)frequency_thresholds;
    (void)frequency_thresholds_count;
    (void)frequency_values;
    (void)frequency_values_count;

    leakage_obj.obj_id = IPSO_OBJECT_LEAKAGE_DETECTION_CONFIGURATION_ID;
    leakage_obj.version_major = LEAKAGE_VERSION_MAJOR;
    leakage_obj.version_minor = LEAKAGE_VERSION_MINOR;
    leakage_obj.is_core = false;
    leakage_obj.fields = fields;
    leakage_obj.field_count = ARRAY_SIZE(fields);
    leakage_obj.max_instance_count = MAX_INSTANCE_COUNT;
    leakage_obj.create_cb = leakage_create;

    lwm2m_register_obj(&leakage_obj);

    LOG_DBG("IPSO Leakage Detection Configuration object registered (ID %d)",
        IPSO_OBJECT_LEAKAGE_DETECTION_CONFIGURATION_ID);

    return 0;
}

LWM2M_OBJ_INIT(ipso_leakage_detection_init);
