#ifndef ADVERTISING_H__
#define ADVERTISING_H__

#include "bsp.h"
#include "ble_gap.h"

#include "util.h"

ConstantData nameFromReport(ble_gap_evt_adv_report_t const* report);

/**@brief Function to get the parameters used for scanning. */
const ble_gap_scan_params_t* getScanParams();

/**@brief Function for setting up advertising data. */
ret_code_t setAdvertisingData();

/**@brief Function for starting advertising. */
ret_code_t startAdvertising();

/**@brief Function for starting advertising. */
void stopAdvertising();

/**@brief Function to start scanning. */
ret_code_t startScan();

/**@brief Function to stop scanning. */
ret_code_t stopScan();

#endif
