// ...existing code...
/*
 * Interval Data Delivery (Object ID 10262)
 *
 * Scaffolding adaptado al estilo de ipso_generic_sensor.c
 * 
 * Recursos:
 * 0 - Name (String, RW, Single, Mandatory)
 * 1 - Interval Data Links (Objlnk, RW, Multiple, Mandatory)
 * 2 - Latest Payload (Opaque, R, Multiple, Mandatory)
 * 3 - Schedule (Objlnk, RW, Single, Optional)
 *
 * Nota: completar detalles de registro/res_inst para múltiples instancias según
 * la versión exacta del engine LwM2M en tu árbol.
 */

#define LOG_MODULE_NAME net_ipso_interval_data_delivery
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


#define INTERVAL_VERSION_MAJOR 1
#define INTERVAL_VERSION_MINOR 0

/* Configure via Kconfig like other IPSO objects (define in your app Kconfig if needed) */
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_INTERVAL_DATA_DELIVERY_INSTANCE_COUNT

/* Sizes */
#define NAME_STR_MAX_SIZE 64
#define MAX_LINKS_PER_INST 8
#define MAX_PAYLOADS_PER_INST 8
#define MAX_PAYLOAD_SIZE 256

/* Calculate fields */
#define NUMBER_OF_OBJ_FIELDS 4
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS) /* adjust if any exec resources present */

/* Data storage per instance */
static char name_buf[MAX_INSTANCE_COUNT][NAME_STR_MAX_SIZE];

/* Represent an object link as 32-bit: high 16 = obj id, low 16 = obj instance */
static uint32_t interval_links[MAX_INSTANCE_COUNT][MAX_LINKS_PER_INST];
static size_t interval_links_count[MAX_INSTANCE_COUNT];

/* Latest payloads: for simplicity allocate fixed count per instance */
static uint8_t latest_payloads[MAX_INSTANCE_COUNT][MAX_PAYLOADS_PER_INST][MAX_PAYLOAD_SIZE];
static size_t latest_payloads_len[MAX_INSTANCE_COUNT][MAX_PAYLOADS_PER_INST];
static bool latest_payloads_present[MAX_INSTANCE_COUNT][MAX_PAYLOADS_PER_INST];
static size_t latest_payloads_count[MAX_INSTANCE_COUNT];

/* Schedule: single objlnk */
static uint32_t schedule_link[MAX_INSTANCE_COUNT];
static bool schedule_set[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj interval_obj;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(NAME_RID, RW, STRING),
    OBJ_FIELD_DATA(INTERVAL_LINKS_RID, RW, OBJLNK),
    OBJ_FIELD_DATA(LATEST_PAYLOAD_RID, R, OPAQUE),
    OBJ_FIELD_DATA(SCHEDULE_RID, RW_OPT, OBJLNK),
};

static struct lwm2m_engine_obj_inst inst[MAX_INSTANCE_COUNT];
static struct lwm2m_engine_res res[MAX_INSTANCE_COUNT][NUMBER_OF_OBJ_FIELDS];
static struct lwm2m_engine_res_inst res_inst[MAX_INSTANCE_COUNT][RESOURCE_INSTANCE_COUNT];

/* Helpers to pack/unpack objlnk */
static inline uint32_t pack_objlnk(uint16_t objid, uint16_t instid)
{
    return ((uint32_t)objid << 16) | (instid & 0xFFFF);
}
static inline uint16_t objlnk_obj(uint32_t packed)
{
    return (uint16_t)(packed >> 16);
}
static inline uint16_t objlnk_inst(uint32_t packed)
{
    return (uint16_t)(packed & 0xFFFF);
}

/* create callback */
static struct lwm2m_engine_obj_inst *interval_create(uint16_t obj_inst_id)
{
    int index, i = 0, j = 0;

    /* Check duplicates */
    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
            LOG_ERR("Instance already exists: %u", obj_inst_id);
            return NULL;
        }
    }

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
    name_buf[index][0] = '\0';
    interval_links_count[index] = 0;
    latest_payloads_count[index] = 0;
    schedule_set[index] = false;
    memset(latest_payloads_present[index], 0, sizeof(latest_payloads_present[index]));

    (void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* Initialize resources
     *
     * NOTE:
     * - For single-instance string resource use INIT_OBJ_RES_DATA_LEN.
     * - For multiple-instance resources (Interval Links and Latest Payload)
     *   you will need to initialize resource instances (one per entry) when
     *   the application adds links/payloads. Here we register the resource
     *   containers; actual dynamic instantiation must be handled in your code
     *   (see TODO below).
     */

    /* Name (single string) */
    INIT_OBJ_RES_DATA_LEN(NAME_RID, res[index], i, res_inst[index], j,
                  name_buf[index], NAME_STR_MAX_SIZE, 0);

    /* Interval Data Links (objlnk) - RW, multiple */
    /* TODO: register as resource container; dynamically create res_inst entries
     * for each link when link is added via write.
     * For now create an empty container entry so engine knows about the resource.
     */
    INIT_OBJ_RES_OPTDATA(INTERVAL_LINKS_RID, res[index], i, res_inst[index], j);

    /* Latest Payload (opaque) - R, multiple */
    /* TODO: similar to Interval Links, create res_inst entries per payload index */
    INIT_OBJ_RES_OPTDATA(LATEST_PAYLOAD_RID, res[index], i, res_inst[index], j);

    /* Schedule (single objlnk, optional) */
    INIT_OBJ_RES_OPTDATA(SCHEDULE_RID, res[index], i, res_inst[index], j);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created IPSO Interval Data Delivery instance: %d", obj_inst_id);
    return &inst[index];
}

/* Example helper to set a link (to be called by higher level on write) */
int interval_set_link(uint16_t obj_inst_id, size_t link_index,
              uint16_t target_obj, uint16_t target_inst)
{
    int idx;
    for (idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
        if (inst[idx].obj && inst[idx].obj_inst_id == obj_inst_id) {
            if (link_index >= MAX_LINKS_PER_INST) {
                return -EINVAL;
            }
            interval_links[idx][link_index] = pack_objlnk(target_obj, target_inst);
            if (link_index >= (int)interval_links_count[idx]) {
                interval_links_count[idx] = link_index + 1;
            }
            /* TODO: create resource instance in engine and set data using
             * lwm2m_engine_set_res_data() or equivalent.
             */
            return 0;
        }
    }
    return -ENOENT;
}

/* Example helper to update a latest payload */
int interval_set_latest_payload(uint16_t obj_inst_id, size_t pl_index,
                const uint8_t *buf, size_t len, bool present)
{
    int idx;
    for (idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
        if (inst[idx].obj && inst[idx].obj_inst_id == obj_inst_id) {
            if (pl_index >= MAX_PAYLOADS_PER_INST) {
                return -EINVAL;
            }
            if (!present) {
                latest_payloads_present[idx][pl_index] = false;
                latest_payloads_len[idx][pl_index] = 0;
                return 0;
            }
            if (len > MAX_PAYLOAD_SIZE) {
                return -EINVAL;
            }
            memcpy(latest_payloads[idx][pl_index], buf, len);
            latest_payloads_len[idx][pl_index] = len;
            latest_payloads_present[idx][pl_index] = true;
            if (pl_index >= (int)latest_payloads_count[idx]) {
                latest_payloads_count[idx] = pl_index + 1;
            }
            /* TODO: notify observers if needed: lwm2m_notify_observer(...) */
            /* TODO: set resource instance data in engine for this payload */
            return 0;
        }
    }
    return -ENOENT;
}

/* Helper: set schedule objlnk */
int interval_set_schedule(uint16_t obj_inst_id, uint16_t target_obj, uint16_t target_inst)
{
    int idx;
    for (idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
        if (inst[idx].obj && inst[idx].obj_inst_id == obj_inst_id) {
            schedule_link[idx] = pack_objlnk(target_obj, target_inst);
            schedule_set[idx] = true;
            /* TODO: actualizar recurso Schedule en engine con lwm2m_engine_set_res_data() */
            return 0;
        }
    }
    return -ENOENT;
}

static int ipso_interval_data_delivery_init(void)
{
    interval_obj.obj_id = IPSO_OBJECT_INTERVAL_DATA_DELIVERY_ID;
    interval_obj.version_major = INTERVAL_VERSION_MAJOR;
    interval_obj.version_minor = INTERVAL_VERSION_MINOR;
    interval_obj.is_core = false;
    interval_obj.fields = fields;
    interval_obj.field_count = ARRAY_SIZE(fields);
    interval_obj.max_instance_count = MAX_INSTANCE_COUNT;
    interval_obj.create_cb = interval_create;

    lwm2m_register_obj(&interval_obj);

    LOG_DBG("IPSO Interval Data Delivery object registered (ID %d)",
        IPSO_OBJECT_INTERVAL_DATA_DELIVERY_ID);

    return 0;
}

LWM2M_OBJ_INIT(ipso_interval_data_delivery_init);
