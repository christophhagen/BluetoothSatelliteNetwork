#ifndef CONNECT_H__
#define CONNECT_H__

#include <stdint.h>
#include <stdbool.h>

#include "app_error.h"
#include "ble_conn_params.h"
#include "ble_gap.h"
#include "ble.h"

/**< Minimum acceptable connection interval, 7.5 ms in 1.25 ms units. */
#define CONN_INTERVAL_MIN               6
/**< Maximum acceptable connection interval, 500 ms in 1.25 ms units. */
#define CONN_INTERVAL_MAX               400

ret_code_t connectInternal(const ble_gap_addr_t* address, const ble_gap_scan_params_t* scanP, uint16_t interval);

ret_code_t changeIntervalToCentral(uint16_t interval);

ret_code_t changeIntervalToPeripheral(uint16_t handle, uint16_t interval);

ret_code_t setConnectionParameters();

#define connectionEvent(event)          ble_conn_params_on_ble_evt(event) // Avoid overhead of function call

#endif
