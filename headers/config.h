#ifndef CONFIG_H__
#define CONFIG_H__

/** The type of logging to be done */
#define LOG_LEVEL                       1

/** Include source file and line number in log entries */
#define LOG_CODE_POSITION               1

/** Print all logging to the uart */
#define LOG_ON_CONSOLE                  1

#define DEVICE_NAME                     "DEV"
#define DEVICE_SHORT_NAME_LENGTH        3

/**< The maximum number of simultaneous connections. */
#define MAX_CONNECTIONS_AS_CENTRAL      3
#define MAX_CONNECTIONS_AS_PERIPHERAL   3
#define MAX_CONNECTIONS (MAX_CONNECTIONS_AS_PERIPHERAL+MAX_CONNECTIONS_AS_CENTRAL)

#define NRF_BLE_GATT_MAX_MTU_SIZE       247 // Maximum is 247

/** The time set aside for this connection on every connection interval in 1.25 ms units. */
#define BLE_GAP_CONN_EVENT_LENGTH       320 // Min 2, default 3

// -40, -20, -16, -12, -8, -4, 0, and 4 dBm
#define TX_POWER_LEVEL_DEFAULT          0

/** The advertising interval in steps of 0.625 ms */
#define ADV_INTERVAL                    320
/** The scan interval in steps of 0.625 ms */
#define SCAN_INTERVAL                   280
/** The length of the scan window in steps of 0.625 ms */
#define SCAN_WINDOW                     160

#define NRF_ERROR_UUID_INVALID          (NRF_ERROR_BASE_NUM + 20)
#define NRF_ERROR_HANDLE_INVALID        (NRF_ERROR_BASE_NUM + 21)
#define NRF_ERROR_NO_DATA               (NRF_ERROR_BASE_NUM + 22)
#define NRF_ERROR_NO_MATCH              (NRF_ERROR_BASE_NUM + 23)
#define NRF_ERROR_ADDRESS_INVALID       (NRF_ERROR_BASE_NUM + 24)
#define NRF_ERROR_NO_NOTIFICATIONS      (NRF_ERROR_BASE_NUM + 25)

/**< A tag that refers to the BLE stack configuration we set with @ref sd_ble_cfg_set. Default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */
#define CONN_CFG_TAG                    1

#endif
