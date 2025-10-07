// ...existing code...
/*
 * Event Data Delivery (Object ID 10263)
 *
 * Scaffolding adaptado al estilo de ipso_generic_sensor.c
 *
 * Recursos:
 * 0 - Name (String, RW, Single, Mandatory)
 * 1 - Event Data Links (Objlnk, RW, Multiple, Mandatory)
 * 2 - Latest Eventlog (Opaque, R, Multiple, Mandatory)
 * 3 - Schedule (Objlnk, RW, Single, Mandatory)
 *
 * Nota: completar detalles de registro/res_inst para múltiples instancias según
 * la versión exacta del engine LwM2M en tu árbol.
 */

#define LOG_MODULE_NAME net_ipso_event_data_delivery
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


#define EVENT_VERSION_MAJOR 1
#define EVENT_VERSION_MINOR 0

/* Configure via Kconfig like other IPSO objects (define in your app Kconfig if needed) */
#define MAX_INSTANCE_COUNT CONFIG_LWM2M_IPSO_EVENT_DATA_DELIVERY_INSTANCE_COUNT

/* Sizes and limits (tune as needed) */
#define NAME_STR_MAX_SIZE 64
#define MAX_LINKS_PER_INST 8
#define MAX_EVENTLOGS_PER_INST 8
#define MAX_EVENTLOG_SIZE 256

/* Fields */
#define NUMBER_OF_OBJ_FIELDS 4
#define RESOURCE_INSTANCE_COUNT (NUMBER_OF_OBJ_FIELDS) /* ajuste si hay exec resources */

static char name_buf[MAX_INSTANCE_COUNT][NAME_STR_MAX_SIZE];

/* Represent an object link as 32-bit: high 16 = obj id, low 16 = obj instance */
static uint32_t event_links[MAX_INSTANCE_COUNT][MAX_LINKS_PER_INST];
static size_t event_links_count[MAX_INSTANCE_COUNT];

/* Latest eventlogs: fixed allocation per instance */
static uint8_t latest_eventlogs[MAX_INSTANCE_COUNT][MAX_EVENTLOGS_PER_INST][MAX_EVENTLOG_SIZE];
static size_t latest_eventlogs_len[MAX_INSTANCE_COUNT][MAX_EVENTLOGS_PER_INST];
static bool latest_eventlogs_present[MAX_INSTANCE_COUNT][MAX_EVENTLOGS_PER_INST];
static size_t latest_eventlogs_count[MAX_INSTANCE_COUNT];

/* Schedule: single objlnk */
static uint32_t schedule_link[MAX_INSTANCE_COUNT];
static bool schedule_set[MAX_INSTANCE_COUNT];

static struct lwm2m_engine_obj event_obj;
static struct lwm2m_engine_obj_field fields[] = {
    OBJ_FIELD_DATA(NAME_RID, RW, STRING),
    OBJ_FIELD_DATA(EVENT_DATA_LINKS_RID, RW, OBJLNK),
    OBJ_FIELD_DATA(LATEST_EVENTLOG_RID, R, OPAQUE),
    OBJ_FIELD_DATA(SCHEDULE_RID, RW, OBJLNK),
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
static struct lwm2m_engine_obj_inst *event_create(uint16_t obj_inst_id)
{
    int index, i = 0, j = 0;

    /* Check duplicates */
    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (inst[index].obj && inst[index].obj_inst_id == obj_inst_id) {
            LOG_ERR("Can not create instance - already existing: %u", obj_inst_id);
            return NULL;
        }
    }

    for (index = 0; index < MAX_INSTANCE_COUNT; index++) {
        if (!inst[index].obj) {
            break;
        }
    }

    if (index >= MAX_INSTANCE_COUNT) {
        LOG_ERR("Can not create instance - no more room: %u", obj_inst_id);
        return NULL;
    }

    /* defaults */
    name_buf[index][0] = '\0';
    event_links_count[index] = 0;
    latest_eventlogs_count[index] = 0;
    schedule_set[index] = false;
    memset(latest_eventlogs_present[index], 0, sizeof(latest_eventlogs_present[index]));

    (void)memset(res[index], 0, sizeof(res[index][0]) * ARRAY_SIZE(res[index]));
    init_res_instance(res_inst[index], ARRAY_SIZE(res_inst[index]));

    /* Initialize resources
     *
     * For single-instance string resource use INIT_OBJ_RES_DATA_LEN.
     * For multiple-instance resources (Event Data Links and Latest Eventlog)
     * dynamic resource instances should be created when entries are added.
     * Here we register container/placeholder entries.
     */

    /* Name (single string) */
    INIT_OBJ_RES_DATA_LEN(NAME_RID, res[index], i, res_inst[index], j,
                  name_buf[index], NAME_STR_MAX_SIZE, 0);

    /* Event Data Links (objlnk) - RW, multiple */
    INIT_OBJ_RES_OPTDATA(EVENT_DATA_LINKS_RID, res[index], i, res_inst[index], j);

    /* Latest Eventlog (opaque) - R, multiple */
    INIT_OBJ_RES_OPTDATA(LATEST_EVENTLOG_RID, res[index], i, res_inst[index], j);

    /* Schedule (single objlnk, mandatory) */
    INIT_OBJ_RES_OPTDATA(SCHEDULE_RID, res[index], i, res_inst[index], j);

    inst[index].resources = res[index];
    inst[index].resource_count = i;

    LOG_DBG("Created IPSO Event Data Delivery instance: %d", obj_inst_id);
    return &inst[index];
}

/* Helper: set an event link (indexed) */
int event_set_link(uint16_t obj_inst_id, size_t link_index,
           uint16_t target_obj, uint16_t target_inst)
{
    int idx;
    for (idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
        if (inst[idx].obj && inst[idx].obj_inst_id == obj_inst_id) {
            if (link_index >= MAX_LINKS_PER_INST) {
                return -EINVAL;
            }
            event_links[idx][link_index] = pack_objlnk(target_obj, target_inst);
            if (link_index >= (int)event_links_count[idx]) {
                event_links_count[idx] = link_index + 1;
            }
            /* TODO: crear res_inst para la instancia de recurso en el engine
             * y llamar a lwm2m_engine_set_res_data() para fijar el valor objlnk.
             */
            return 0;
        }
    }
    return -ENOENT;
}

/* Helper: set latest eventlog payload */
int event_set_latest_eventlog(uint16_t obj_inst_id, size_t el_index,
                  const uint8_t *buf, size_t len, bool present)
{
    int idx;
    for (idx = 0; idx < MAX_INSTANCE_COUNT; idx++) {
        if (inst[idx].obj && inst[idx].obj_inst_id == obj_inst_id) {
            if (el_index >= MAX_EVENTLOGS_PER_INST) {
                return -EINVAL;
            }
            if (!present) {
                latest_eventlogs_present[idx][el_index] = false;
                latest_eventlogs_len[idx][el_index] = 0;
                return 0;
            }
            if (len > MAX_EVENTLOG_SIZE) {
                return -EINVAL;
            }
            memcpy(latest_eventlogs[idx][el_index], buf, len);
            latest_eventlogs_len[idx][el_index] = len;
            latest_eventlogs_present[idx][el_index] = true;
            if (el_index >= (int)latest_eventlogs_count[idx]) {
                latest_eventlogs_count[idx] = el_index + 1;
            }
            /* TODO: notificar observadores si procede:
             * lwm2m_notify_observer(IPSO_OBJECT_EVENT_DATA_DELIVERY_ID, obj_inst_id, LATEST_EVENTLOG_RID);
             * y fijar el data del res_inst con lwm2m_engine_set_res_data()
             */
            return 0;
        }
    }
    return -ENOENT;
}

/* Helper: set schedule objlnk */
int event_set_schedule(uint16_t obj_inst_id, uint16_t target_obj, uint16_t target_inst)
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

static int ipso_event_data_delivery_init(void)
{
    event_obj.obj_id = IPSO_OBJECT_EVENT_DATA_DELIVERY_ID;
    event_obj.version_major = EVENT_VERSION_MAJOR;
    event_obj.version_minor = EVENT_VERSION_MINOR;
    event_obj.is_core = false;
    event_obj.fields = fields;
    event_obj.field_count = ARRAY_SIZE(fields);
    event_obj.max_instance_count = MAX_INSTANCE_COUNT;
    event_obj.create_cb = event_create;

    lwm2m_register_obj(&event_obj);

    LOG_DBG("IPSO Event Data Delivery object registered (ID %d)",
        IPSO_OBJECT_EVENT_DATA_DELIVERY_ID);

    return 0;
}

LWM2M_OBJ_INIT(ipso_event_data_delivery_init);