
#include "logging.h"
#include "advertising.h"
#include "config.h"
#include "util.h"

#include "ble_advdata.h"
#include "sdk_errors.h"

// Scan parameters requested for scanning and connection.
static ble_gap_scan_params_t const scanParameters = {
    .active         = 0x00,
    .interval       = SCAN_INTERVAL,
    .window         = SCAN_WINDOW,
    .use_whitelist  = 0x00,
    .adv_dir_report = 0x00,
    .timeout        = 0x0000, // No timeout.
};

ble_gap_adv_params_t const advertisingParameters = {
    .type        = BLE_GAP_ADV_TYPE_ADV_IND,
    .p_peer_addr = NULL,
    .fp          = BLE_GAP_ADV_FP_ANY,
    .interval    = ADV_INTERVAL,
    .timeout     = 0,
};

ble_advdata_t const advertisingData = {
    .name_type          = BLE_ADVDATA_SHORT_NAME,
    .short_name_len     = DEVICE_SHORT_NAME_LENGTH,
    .flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
    .include_appearance = false,
};

/**@brief Function to get the parameters used for scanning. */
const ble_gap_scan_params_t* getScanParams() {
    return &scanParameters;
}

/**@brief Function for setting up advertising data. */
ret_code_t setAdvertisingData() {
    ret_code_t error = ble_advdata_set(&advertisingData, NULL);
    RETURN_WARNING_ON(error, "0x%04x", error);
    return NRF_SUCCESS;
}

/**@brief Function for starting advertising. */
ret_code_t startAdvertising() {
    ret_code_t error = sd_ble_gap_adv_start(&advertisingParameters, CONN_CFG_TAG);
    RETURN_INFO_ON(error, "0x%04x", error);
    return NRF_SUCCESS;
}

/**@brief Function for starting advertising. */
void stopAdvertising() {
    (void) sd_ble_gap_adv_stop();
}

/**@brief Function to start scanning. */
ret_code_t startScan() {
    ret_code_t error = sd_ble_gap_scan_start(&scanParameters);
    RETURN_INFO_ON(error, "0x%04x", error);
    return NRF_SUCCESS;
}

/**@brief Function to stop scanning. */
ret_code_t stopScan() {
    ret_code_t error = sd_ble_gap_scan_stop();
    RETURN_INFO_ON(error, "0x%04x", error);
    return NRF_SUCCESS;
}

/**@brief Function for searching a given name in the advertisement packets.
 *
 * @details Use this function to parse received advertising data and to find a given
 * name in them either as 'complete_local_name' or as 'short_local_name'.
 *
 * @param[in]   report   advertising data to parse.
 * @return   NRF_SUCCESS if the given name was found, otherwise an error.
 */
ConstantData nameFromReport(ble_gap_evt_adv_report_t const* report) {
    uint32_t  index = 0;
    while (index < report->dlen) {
        uint8_t fieldLength = report->data[index];
        uint8_t fieldType   = report->data[index + 1];

        if (fieldType == BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME) {
            if (fieldLength - 1 == DEVICE_SHORT_NAME_LENGTH) {
                ConstantData name = {
                    .data = &report->data[index + 2],
                    .length = DEVICE_SHORT_NAME_LENGTH,
                };
                return name;
            }
            break;
        }
        index += fieldLength + 1;
    }
    ConstantData name = {
        .data = 0,
        .length = 0,
    };
    return name;
}
