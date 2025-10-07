/**
 * @file lwm2m_resource_ids.h
 * @brief
 *
 * Copyright (c) 2020 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __LWM2M_RESOURCE_IDS__
#define __LWM2M_RESOURCE_IDS__

/* clang-format off */
/*#---------------------------- NEW OBJECT ---------------------------------*/
/* Additions for IPSO Object (10262) */
#define NAME_RID                 0
#define INTERVAL_LINKS_RID       1
#define LATEST_PAYLOAD_RID       2
#define SCHEDULE_RID             3
/* Additions for IPSO Object (10263) */
#define EVENT_DATA_LINKS_RID     1
#define LATEST_EVENTLOG_RID      2

/* Additions for IPSO Delivery Schedule Object (10264) */
#define SCHEDULE_START_TIME_RID  0
#define SCHEDULE_UTC_OFFSET_RID  1
#define DELIVERY_FREQUENCY_RID   2
#define RANDOMISED_WINDOW_RID    3
#define NUMBER_OF_RETRIES_RID    4
#define RETRY_PERIOD_RID         5

/* Additions for IPSO Leakage Detection Configuration Object (10265) */
#define SAMPLE_TIMES_RID              0
#define SAMPLE_UTC_OFFSET_RID         1
#define DETECTION_MODE_RID            2
#define TOP_FREQUENCY_COUNT_RID       3
#define FREQUENCY_THRESHOLDS_RID      4
#define FREQUENCY_VALUES_RID          5
#define FIRMWARE_VERSION_RID          7

/* Additions for IPSO Water Flow Readings Object (10266)*/
#define INTERVAL_PERIOD_RID             6000
#define INTERVAL_START_OFFSET_RID       6001
#define INTERVAL_UTC_OFFSET_RID         6002
#define INTERVAL_COLLECTION_START_RID   6003
#define OLDEST_RECORDED_INTERVAL_RID    6004
#define LAST_DELIVERED_INTERVAL_RID     6005
#define LATEST_RECORDED_INTERVAL_RID    6006
#define DELIVERY_MIDNIGHT_ALIGNED_RID   6007
#define INTERVAL_HISTORICAL_READ_RID    6008
#define INTERVAL_HISTORICAL_PAYLOAD_RID 6009
#define INTERVAL_CHANGE_CONFIG_RID      6010
#define START_RID                       6026
#define STOP_RID                        6027
#define STATUS_RID                      6028
#define LATEST_PAYLOAD_RID_10266        6029
/*#--------------------------------------------------------------------------*/

#define DIGITAL_INPUT_STATE_RID           5500
#define DIGITAL_INPUT_COUNTER_RID         5501
#define TIMESTAMP_RID                     5518
#define DELAY_DURATION_RID                5521
#define TRIGGER_RID                       5523
#define MINIMUM_OFF_TIME_RID              5525
#define TIMER_MODE_RID                    5526
#define COUNTER_RID                       5534
#define REMAINING_TIME_RID                5538
#define DIGITAL_STATE_RID                 5543
#define CUMULATIVE_TIME_RID               5544
#define LEVEL_RID                         5548
#define MIN_MEASURED_VALUE_RID            5601
#define MAX_MEASURED_VALUE_RID            5602
#define MIN_RANGE_VALUE_RID               5603
#define MAX_RANGE_VALUE_RID               5604
#define RESET_MIN_MAX_MEASURED_VALUES_RID 5605
#define SENSOR_VALUE_RID                  5700
#define SENSOR_UNITS_RID                  5701
#define X_VALUE_RID                       5702
#define Y_VALUE_RID                       5703
#define Z_VALUE_RID                       5704
#define COLOUR_RID                        5706
#define APPLICATION_TYPE_RID              5750
#define SENSOR_TYPE_RID                   5751
#define CUMULATIVE_ACTIVE_POWER_RID       5805
#define POWER_FACTOR_RID                  5820
#define CURRENT_CALIBRATION_RID           5821
#define ON_OFF_RID                        5850
#define DIMMER_RID                        5851
#define ON_TIME_RID                       5852
#define OFF_TIME_RID                      5854
#define MEASUREMENT_QUALITY_INDICATOR_RID 6042
#define MEASUREMENT_QUALITY_LEVEL_RID     6049
#define FRACTIONAL_TIMESTAMP_RID          6050
/* clang-format on */

#endif /* __LWM2M_RESOURCE_IDS__ */
