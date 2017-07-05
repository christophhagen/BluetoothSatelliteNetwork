#define NRF_LOG_MODULE_NAME "CON"
#include "config.h"

#include "connect.h"
#include "logging.h"

#include "app_timer.h"

/**< Connection supervisory timeout, 4 s in 10 ms units. */
#define CONN_SUP_TIMEOUT                400
/**< Slave latency. */
#define SLAVE_LATENCY                   0

/**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (0.5 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(500)
/**< Time between each call to sd_ble_gap_conn_param_update after the first call (0.5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(500)
/**< Number of attempts before giving up the connection parameter negotiation. */
#define MAX_CONN_PARAMS_UPDATE_COUNT    2

// Connection parameters requested for connection.
static ble_gap_conn_params_t connectionParameters = {
    .min_conn_interval = CONN_INTERVAL_MIN,   // Minimum connection interval.
    .max_conn_interval = CONN_INTERVAL_MAX,   // Maximum connection interval.
    .slave_latency     = SLAVE_LATENCY,       // Slave latency.
    .conn_sup_timeout  = CONN_SUP_TIMEOUT     // Supervisory timeout.
};

ret_code_t connectInternal(const ble_gap_addr_t* address, const ble_gap_scan_params_t* scanP, uint16_t interval) {
    // Initiate connection.
    connectionParameters.min_conn_interval = interval;
    connectionParameters.max_conn_interval = interval;

    ret_code_t error;
    error = sd_ble_gap_connect(address, scanP, &connectionParameters, CONN_CFG_TAG);
    RETURN_WARNING_ON(error, "Failed, %d", error);
    return NRF_SUCCESS;
}

ret_code_t changeIntervalToCentral(uint16_t interval) {
    connectionParameters.min_conn_interval = interval;
    connectionParameters.max_conn_interval = interval + 1;
    ret_code_t error = ble_conn_params_change_conn_params(&connectionParameters);
    RETURN_WARNING_ON(error, "Failed, %d", error);
    return NRF_SUCCESS;
}

ret_code_t changeIntervalToPeripheral(uint16_t handle, uint16_t interval) {
    connectionParameters.min_conn_interval = interval;
    connectionParameters.max_conn_interval = interval;
    ret_code_t error = sd_ble_gap_conn_param_update(handle, &connectionParameters);
    RETURN_WARNING_ON(error, "Failed, %d", error);
    return NRF_SUCCESS;
}


/**@brief Function for handling a Connection Parameters event.
 *
 * @param[in] p_evt  event.
 */
static void onConnectionParamsEvent(ble_conn_params_evt_t * p_evt) {
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED) {
        LOG_DEBUG("Configured");
    } else {
        // TODO: Notify manager that update has failed
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void onConnectionParamsError(uint32_t error) {
    PRINT_ERROR_ON(error, "Error %d", error);
}

/**@brief  */
ret_code_t setConnectionParameters() {
    ret_code_t error = sd_ble_gap_ppcp_set(&connectionParameters);
    RETURN_ERROR_ON(error, "Failed, %d", error);

    ble_conn_params_init_t const connection_params_init = {
        .p_conn_params                  = NULL,
        .first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY,
        .next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY,
        .max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT,
        .disconnect_on_fail             = true,
        .evt_handler                    = onConnectionParamsEvent,
        .error_handler                  = onConnectionParamsError,
    };

    error = ble_conn_params_init(&connection_params_init);
    RETURN_ERROR_ON(error, "Init failed, %d", error);
    return NRF_SUCCESS;
}
