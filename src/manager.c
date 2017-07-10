
#include "manager.h"

#include "advertising.h"
#include "config.h"
#include "connect.h"

#include "app_timer.h"
#include "ble_db_discovery.h"
#include "ble_hci.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"
#include "sdk_common.h"
#include "softdevice_handler.h"
#include <nrf_log_ctrl.h>
#include "nrf_soc.h" // sd_app_evt_wait ?



/**@brief The vendor specific UUID (bb4aff4f-ad03-415d-a96c-9d6cddda8304) */
#define SERVICE_UUID_BASE   {0xBB, 0x4A, 0xFF, 0x4F, 0xAD, 0x03, 0x41, 0x5D, \
                             0xA9, 0x6C, 0x9D, 0x6C, 0xDD, 0xDA, 0x83, 0x04}
/**@brief The UUID of the service that is descovered for each connection */
#define SERVICE_UUID                0x1523
/**@brief The UUID for the characteristic to receive and send with */
#define RECEIVE_CHAR_UUID           0x1524

/**< Link layer header length. */
#define LL_HEADER_LEN                   4

/**@brief Structure containing all connection related data. */
typedef struct {
    uint8_t             ID;          //!< The ID by which the connection is identified */
    char                name[DEVICE_SHORT_NAME_LENGTH+1]; //!< The name of the system to connect to, only when central */
    ble_gap_addr_t      mainAddress; //!< The first address of the system to connect to */
    ble_gap_addr_t      redAddress;  //!< The second address of the system to connect to, for redundancy */
    uint16_t            handle;      //!< Connection handle as provided by the SoftDevice. */
    ble_db_discovery_t  database;    //!< The database for discovery of the services */
    uint16_t            cccdHandle;    //!<  Handle of the CCCD . */
    uint16_t            receiveHandle; //!<  Handle of the characteristic as provided by the SoftDevice. */
    bool                asCentral;   //!< The own role in the connection */
    bool                shouldConnect; //!< Set by the application and can suspend a connection. */
    bool                busy;        //!< Indicate that the connection is busy */
    bool                confirm;     //!< Request or send confirmations on this server */
    bool                confirmToPeer; //!< Request or send confirmations on this server */
    bool                updateInterval; //!< An update of the connection interval is in progress */
    uint16_t            payloadSize; //!< Maximum number of bytes which can be sent in one notification. */
    DataHandler         received;    //!< Callback for received data. */
    EventHandler        eventHandler;
    buffer_t            buffer;      //!< Transmit buffer. */
    uint16_t            interval;    //!< The connection interval for a central connection. */
} ConnectionInternal;

/**@brief The array containing all connections */
static ConnectionInternal connections[MAX_CONNECTIONS];
/**@brief The number of currently registered connections */
static uint8_t connectionsCount = 0;

/**@brief GATT module instance. */
static nrf_ble_gatt_t gattModule;
/**@brief The UUID type of the base UUID */
static uint8_t baseUuidType;
/**@brief The characteristic handles */
static ble_gatts_char_handles_t receiveHandles;

/** Start/stop scanning depending on the connections to be made */
static void startStopScan() {
    for (uint8_t index = 0; index < connectionsCount; index += 1) {
        ConnectionInternal* connection = &connections[index];
        if (!connection->asCentral || !connection->shouldConnect) { continue; }
        if (connection->handle != BLE_CONN_HANDLE_INVALID) { continue; }
        startScan();
        return;
    }
    stopScan();
}

/** Start/stop advertising depending on the connections to be made */
static void startStopAdvertising() {
    for (uint8_t index = 0; index < connectionsCount; index += 1) {
        ConnectionInternal* connection = &connections[index];
        if (connection->asCentral || !connection->shouldConnect) { continue; }
        if (connection->handle != BLE_CONN_HANDLE_INVALID) { continue; }
        startAdvertising();
        return;
    }
    stopAdvertising();
}

/** Start/stop advertising and scanning depending on the connections to be made */
static void startStopScanAndAvertising() {
    startStopScan();
    startStopAdvertising();
}

/**@brief Function for passing any pending request from the buffer to the stack. */
static ret_code_t processBuffer(ConnectionInternal* connection) {
    if (bufferHasElement(&connection->buffer)) {
        tx_message_t* message = bufferGetNext(&connection->buffer);

        if (message->isWrite) {
            uint32_t error = sd_ble_gattc_write(message->handle, &message->request.gattc.params);
            RETURN_DEBUG_ON(error, "write failed: %d", error);
        } else {
            uint32_t error = sd_ble_gattc_read(message->handle, message->request.readHandle, 0);
            RETURN_DEBUG_ON(error, "read failed: %d", error);
        }
        bufferDeleteNext(&connection->buffer);
    }
    return NRF_SUCCESS;
}

/**@brief Function for finding the connection for a handle */
static ConnectionInternal* getConnectionByHandle(uint16_t handle) {
    for (uint8_t i = 0; i < connectionsCount; i += 1) {
        if (connections[i].handle == handle) {
            return &connections[i];
        }
    }
    return 0;
}

/**@brief Function for finding the client connection for a name */
static ConnectionInternal* getClientByName(const uint8_t* name) {
    for (uint8_t i = 0; i < connectionsCount; i += 1) {
        if (memcmp(connections[i].name, name, DEVICE_SHORT_NAME_LENGTH) == 0) {
            return connections[i].asCentral ? &connections[i] : 0;
        }
    }
    return 0;
}

/**@brief Function for finding the connection for an address */
static ConnectionInternal* getConnectionByAddress(const ble_gap_addr_t* address) {
    for (uint8_t i = 0; i < connectionsCount; i += 1) {
        if (peerAddressEqual(address, &connections[i].mainAddress) ||
        peerAddressEqual(address, &connections[i].redAddress)) {
            return &connections[i];
        }
    }
    LOG_DEBUG("No connection");
    printPeerAddress(*address);
    return 0;
}

/**@brief Reset a connection to unconnected state */
static void resetConnection(ConnectionInternal* connection) {
    connection->handle         = BLE_CONN_HANDLE_INVALID;
    connection->cccdHandle     = BLE_GATT_HANDLE_INVALID;
    connection->receiveHandle  = BLE_GATT_HANDLE_INVALID;
    connection->busy           = false;
    connection->payloadSize    = PAYLOAD_LENGTH;
    connection->confirmToPeer  = false;
    connection->updateInterval = false;
    bufferInit(&connection->buffer);
}

/**@brief on advertisment reports, connect to matching device name. */
static ret_code_t onAdvertismentReport(const ble_gap_evt_t* event) {
    ConstantData name = nameFromReport(&event->params.adv_report);
    if(name.length == 0) { return NRF_SUCCESS; }

    ConnectionInternal* client = getClientByName(name.data);
    if(client == 0) {
        LOG_DEBUG("Device %s", (uint32_t) name.data);
        return NRF_SUCCESS;
    }

    if (client->shouldConnect == false) { return NRF_SUCCESS; }

    RETURN_DEBUG_IF(client->handle != BLE_CONN_HANDLE_INVALID, NRF_SUCCESS, "Already connected to \"%s\"", (uint32_t) client->name);

    // TODO: Save state, notify application about type of device?
    if (peerAddressEqual(&event->params.adv_report.peer_addr, &client->mainAddress)) {
        LOG_INFO("Main device");
    } else if (peerAddressEqual(&event->params.adv_report.peer_addr, &client->mainAddress)) {
        LOG_INFO("Redundant device");
    } else {
        LOG_INFO("Unknown device ");
        client->mainAddress = event->params.adv_report.peer_addr;
        printPeerAddress(client->mainAddress);
    }
    ret_code_t error = connectInternal(&event->params.adv_report.peer_addr, getScanParams(), client->interval);
    RETURN_WARNING_ON(error, "Connecting to \"%s\" failed: %d", (uint32_t) client->name, error);
    LOG_INFO("Connecting to \"%s\"", (uint32_t) client->name);
    return NRF_SUCCESS;
}

/**@brief Propagate received data to application */
static ret_code_t onNotification(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gattc_evt.conn_handle);
    RETURN_WARNING_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Handle %d", event->evt.gattc_evt.conn_handle);
    // Check if this is a notification.
    uint16_t handle = event->evt.gattc_evt.params.hvx.handle;
    bool handlesEqual = connection->receiveHandle == handle;
    RETURN_WARNING_IF(!handlesEqual, NRF_ERROR_HANDLE_INVALID, "Receive handle mismatch 0x%02x", handle);

    ConstantData data = {
        .data = event->evt.gattc_evt.params.hvx.data,
        .length = event->evt.gattc_evt.params.hvx.len,
    };

    if (connection->confirmToPeer) {
        // TODO: Return error after notifying application ?
        ret_code_t error = sd_ble_gattc_hv_confirm(connection->handle, handle);
        RETURN_WARNING_ON(error, "Could not confirm %d", error);
    }
    LOG_DEBUG("Received %d bytes on connection 0x%02x", data.length, connection->handle);
    if (connection->received != 0) {
        connection->received(data);
    }
    return NRF_SUCCESS;
}

/**@brief Indicate that connection is ready */
static ret_code_t onNotificationComplete(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gatts_evt.conn_handle);
    RETURN_WARNING_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Handle %d", event->evt.gatts_evt.conn_handle);
    connection->busy = false;
    if (connection->eventHandler != 0) {
        connection->eventHandler(DATA_SENT_TO_PEER);
    }
    return NRF_SUCCESS;
}

/**@brief Call the confirmation handler. */
static ret_code_t onConfirmationReceived(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gatts_evt.conn_handle);
    RETURN_WARNING_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Invalid handle: %d", event->evt.gatts_evt.conn_handle);
    uint16_t handle = event->evt.gatts_evt.params.hvc.handle;
    RETURN_WARNING_IF(handle != receiveHandles.value_handle, NRF_ERROR_HANDLE_INVALID, "Invalid handle: %d", handle);
    if (connection->eventHandler != 0) {
        connection->eventHandler(DATA_ARRIVED_AT_PEER);
    }
    return NRF_SUCCESS;
}

/**@brief Propagate requested data to the application*/
static ret_code_t onReadResponse(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gattc_evt.conn_handle);
    RETURN_WARNING_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Handle %d", event->evt.gattc_evt.conn_handle);
    // Check if this is a read response.
    uint16_t handle = event->evt.gattc_evt.params.read_rsp.handle;
    bool handlesEqual = connection->receiveHandle == handle;
    RETURN_WARNING_IF(!handlesEqual, NRF_ERROR_HANDLE_INVALID, "Request handle mismatch 0x%02x", handle);

    ConstantData data = {
        .data = event->evt.gattc_evt.params.read_rsp.data,
        .length = event->evt.gattc_evt.params.read_rsp.len,
    };

    LOG_DEBUG("Received %d bytes on connection 0x%02x", data.length, connection->handle);
    if (connection->received != 0) {
        connection->received(data);
    }
    return NRF_SUCCESS;
}

/**@brief Notice that the CCCD is configured */
static ret_code_t onWriteResponse(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gattc_evt.conn_handle);
    if(connection == 0) {return NRF_ERROR_HANDLE_INVALID; }
    // Check if this is a write response on the CCCD.
    uint16_t handle = event->evt.gattc_evt.params.write_rsp.handle;
    bool handlesEqual = connection->cccdHandle == handle;
    RETURN_WARNING_IF(!handlesEqual, NRF_ERROR_HANDLE_INVALID, "CCCD handle mismatch 0x%02x", handle);
    LOG_INFO("CCCD configured");
    return NRF_SUCCESS;
}

/**@brief Notice that notifications are enabled */
static ret_code_t onWrite(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gatts_evt.conn_handle);
    if(connection == 0) {return NRF_ERROR_HANDLE_INVALID; }
    const ble_gatts_evt_write_t* writeEvent = &event->evt.gatts_evt.params.write;

    bool equals = (writeEvent->handle == receiveHandles.cccd_handle) && (writeEvent->len == 2);
    RETURN_WARNING_IF(!equals, NRF_ERROR_HANDLE_INVALID, "CCCD handle mismatch: 0x%04x", writeEvent->handle);

    connection->confirmToPeer = ble_srv_is_indication_enabled(writeEvent->data);

    if (connection->confirmToPeer == false) {
        RETURN_WARNING_IF(!ble_srv_is_notification_enabled(writeEvent->data), NRF_ERROR_NO_NOTIFICATIONS, "Notify and indicate disabled");
    }
    if (connection->confirmToPeer) {
        LOG_DEBUG("Indications enabled");
    } else {
        LOG_DEBUG("Notifications enabled");
    }
    return NRF_SUCCESS;
}

/**@brief Discover database, start RSSI */
static ret_code_t onConnected(const ble_gap_evt_t* event) {
    ConnectionInternal* connection = getConnectionByAddress(&event->params.connected.peer_addr);
    if(connection == 0) { return NRF_ERROR_ADDRESS_INVALID; }
    connection->asCentral = (event->params.connected.role == BLE_GAP_ROLE_CENTRAL);
    connection->handle = event->conn_handle;

    if (connection->asCentral) {
        LOG_INFO("Connected to peripheral \"%s\"", (uint32_t) connection->name);
    } else {
        LOG_INFO("Connected to central ");
        printPeerAddress(event->params.connected.peer_addr);
    }

    // Discover the database on the other side
    memset(&connection->database, 0x00, sizeof(ble_db_discovery_t));
    ret_code_t error = ble_db_discovery_start(&connection->database, connection->handle);

    if (error != NRF_SUCCESS) {
        PRINT_WARNING_ON(error, "DB discovery start failed, %d", error);
        error = sd_ble_gap_disconnect(connection->handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        PRINT_ERROR_ON(error, "Disconnect failed: %d", error);
    } else {
        // Start RSSI measurement
        error = sd_ble_gap_rssi_start(connection->handle, BLE_GAP_RSSI_THRESHOLD_INVALID, 0);
        RETURN_WARNING_ON(error, "RSSI not enabled: %d", error);
    }

    LOG_DEBUG("Discovering GATT database");
    startStopScanAndAvertising();

    if (connection->eventHandler != 0) {
        connection->eventHandler(CONNECTED);
    }
    return NRF_SUCCESS;
}

/**@brief Invalidate all handles */
static ret_code_t onDisconnected(const ble_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->evt.gap_evt.conn_handle);
    RETURN_ERROR_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Invalid handle %d", event->evt.gap_evt.conn_handle);
    resetConnection(connection);

    if (connection->asCentral) {
        LOG_INFO("From: %s", (uint32_t) connection->name);
    } else {
        LOG_INFO("From: ");
        printPeerAddress(connection->mainAddress);
    }

    startStopScanAndAvertising(connection->asCentral, false);

    if (connection->eventHandler != 0) {
        connection->eventHandler(DISCONNECTED);
    }
    return NRF_SUCCESS;
}

/**@brief Successfully updated connection parameters */
static ret_code_t onGapParamUpdate(const ble_gap_evt_t* event) {
    const ble_gap_conn_params_t* params = &event->params.conn_param_update.conn_params;
    ConnectionInternal* connection = getConnectionByHandle(event->conn_handle);
    RETURN_ERROR_IF(connection == 0, NRF_ERROR_HANDLE_INVALID, "Invalid handle %d", event->conn_handle);
    connection->updateInterval = false;
    if (connection->eventHandler != 0) {
        connection->eventHandler(CONNECTION_UPDATED);
    }
    if (!connection->asCentral) {
        connection->interval = params->min_conn_interval;
        LOG_DEBUG("Conn interval is %d ticks\n", connection->interval);
        return NRF_SUCCESS;
    }
    RETURN_ERROR_IF(connection->interval != params->min_conn_interval, NRF_ERROR_INVALID_PARAM, "Conn interval %d", params->min_conn_interval);
    return NRF_SUCCESS;
}

/**@brief Disconnect */
static ret_code_t onTimeout(uint16_t handle) {
    // TODO: Really disconnect after first dropped packet?
    ret_code_t error = sd_ble_gap_disconnect(handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    RETURN_ERROR_ON(error, "Disconnect failed: %d", error);
    LOG_DEBUG("GATT timeout");
    return NRF_SUCCESS;
}

/**@brief Accept parameters requested by the peer */
static ret_code_t onParamUpdate(const ble_evt_t* event) {
    ble_gap_conn_params_t params;
    params = event->evt.gap_evt.params.conn_param_update_request.conn_params;
    params.max_conn_interval = params.min_conn_interval;
    ret_code_t error = sd_ble_gap_conn_param_update(event->evt.gap_evt.conn_handle, &params);
    RETURN_ERROR_ON(error, "GAP parameter update failed: %d", error);
    return NRF_SUCCESS;
}

/**@brief Reply that memory request is not possible */
static ret_code_t onMemoryRequest(const ble_evt_t* event) {
    ret_code_t error = sd_ble_user_mem_reply(event->evt.common_evt.conn_handle, NULL);
    RETURN_ERROR_ON(error, "Memory reply failed: %d", error);
    return NRF_SUCCESS;
}

/**@brief Set system attribute */
static ret_code_t onAttributeMissing(uint16_t handle) {
    ret_code_t error = sd_ble_gatts_sys_attr_set(handle, NULL, 0, 0);
    RETURN_ERROR_ON(error, "GATTS attribute set failed: %d", error);
    return NRF_SUCCESS;
}

/**@brief Handle the events for this module */
static ret_code_t onBleEvent(const ble_evt_t* event) {
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_ADV_REPORT: return onAdvertismentReport(&event->evt.gap_evt);
        case BLE_GATTC_EVT_HVX: return onNotification(event);
        case BLE_GATTS_EVT_HVN_TX_COMPLETE: return onNotificationComplete(event);
        case BLE_GATTS_EVT_HVC: return onConfirmationReceived(event);
        case BLE_GATTC_EVT_READ_RSP: return onReadResponse(event);
        case BLE_GATTC_EVT_WRITE_RSP: return onWriteResponse(event);
        case BLE_GATTS_EVT_WRITE: return onWrite(event);
        case BLE_GAP_EVT_CONNECTED: return onConnected(&event->evt.gap_evt);
        case BLE_GAP_EVT_DISCONNECTED: return onDisconnected(event);
        case BLE_GAP_EVT_CONN_PARAM_UPDATE: return onGapParamUpdate(&event->evt.gap_evt);
        case BLE_GATTC_EVT_TIMEOUT: return onTimeout(event->evt.gattc_evt.conn_handle);
        case BLE_GATTS_EVT_TIMEOUT: return onTimeout(event->evt.gatts_evt.conn_handle);
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST: return onParamUpdate(event);
        case BLE_EVT_USER_MEM_REQUEST: return onMemoryRequest(event);
        case BLE_GATTS_EVT_SYS_ATTR_MISSING: return onAttributeMissing(event->evt.gap_evt.conn_handle);
        default: return NRF_SUCCESS;
    }
}

/**@brief Distribute BLE events to the modules */
static void handleBluetoothEvent(ble_evt_t* event) {
    ret_code_t error = onBleEvent(event);
    PRINT_ERROR_ON(error, "Event error: %d", error);

    uint16_t handle = event->evt.gap_evt.conn_handle;
    ConnectionInternal* connection = getConnectionByHandle(handle);
    if (connection != 0) {
        ble_db_discovery_on_ble_evt(&connection->database, event);
    }

    // Check if the event is on the link for this instance
    handle = event->evt.gattc_evt.conn_handle;
    connection = getConnectionByHandle(handle);
    if(connection != 0 && handle != BLE_GATT_HANDLE_INVALID) {
        // Check if there is any message to be sent across to the peer and send it.
        error = processBuffer(connection);
        PRINT_ERROR_ON(error, "Connection %d buffer: %d", connection->handle, error);
    }

    connectionEvent(event);
    nrf_ble_gatt_on_ble_evt(&gattModule, event);
}

/**@brief Dicover the database and enable notifications */
static void handleDatabaseDiscovery(ble_db_discovery_evt_t* event) {
    ConnectionInternal* connection = getConnectionByHandle(event->conn_handle);
    RETURN_WITH_WARNING_IF(connection == 0, "No connection handle");

    bool equals = (event->evt_type == BLE_DB_DISCOVERY_COMPLETE);
    RETURN_WITH_WARNING_IF(!equals, "DB discovery incomplete");

    equals = (event->params.discovered_db.srv_uuid.uuid == SERVICE_UUID);
    RETURN_WITH_WARNING_IF(!equals, "AMT uuid mismatch");

    equals = (event->params.discovered_db.srv_uuid.type == baseUuidType);
    RETURN_WITH_WARNING_IF(!equals, "uuid type mismatch");

    const ble_gatt_db_srv_t* database = &event->params.discovered_db;
    RETURN_WITH_WARNING_IF(database->char_count != 1, "%d handles at the peer", database->char_count);

    const ble_gatt_db_char_t* chars = &database->charateristics[0];
    ble_uuid_t uuid = chars->characteristic.uuid;
    RETURN_WITH_WARNING_IF(uuid.uuid != RECEIVE_CHAR_UUID, "Invalid UUID 0x%04x", uuid.uuid);
    RETURN_WITH_WARNING_IF(uuid.type != baseUuidType, "Invalid type %d", uuid.type);

    connection->cccdHandle    = chars->cccd_handle;
    connection->receiveHandle = chars->characteristic.handle_value;
    connection->handle        = event->conn_handle;

    LOG_INFO("Configuring CCCD, handle %d, connection %d", connection->cccdHandle, connection->handle);

    tx_message_t message;
    uint16_t cccdValue = connection->confirm ? BLE_GATT_HVX_INDICATION : BLE_GATT_HVX_NOTIFICATION;

    message.request.gattc.params.handle   = connection->cccdHandle;
    message.request.gattc.params.len      = WRITE_MESSAGE_LENGTH;
    message.request.gattc.params.p_value  = message.request.gattc.value;
    message.request.gattc.params.offset   = 0;
    message.request.gattc.params.write_op = BLE_GATT_OP_WRITE_REQ;
    message.request.gattc.value[0]        = LSB_16(cccdValue);
    message.request.gattc.value[1]        = MSB_16(cccdValue);
    message.handle                        = connection->handle;
    message.isWrite                       = true;

    bool added = bufferInsert(&connection->buffer, message);
    PRINT_WARNING_ON(!added, "Buffer full for connection 0x%02x", connection->handle);

    ret_code_t error = processBuffer(connection);
    PRINT_WARNING_ON(error, "Connection %d buffer: %d", connection->handle, error);
}

/**@brief Adjust payload size for connection */
static void handleGattEvent(nrf_ble_gatt_t* gatt, nrf_ble_gatt_evt_t const* event) {
    switch (event->evt_id) {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED: {
            ConnectionInternal* connection = getConnectionByHandle(event->conn_handle);
            RETURN_WITH_ERROR_IF(connection == 0, "No connection");
            connection->payloadSize = event->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
            LOG_INFO("Maximum payload %d", connection->payloadSize);
            if (connection->eventHandler != 0) {
                connection->eventHandler(CONNECTION_READY);
            }
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED: {
            LOG_DEBUG("Data length: %u bytes", event->params.data_length);
        } break;
    }
}

/**@brief Set up the manager, part 1 */
static ret_code_t managerInit() {
    ret_code_t error = NRF_LOG_INIT(NULL);
    RETURN_ERROR_ON(error, "Log init failed: %d", error);
    LOG_DEBUG("Log OK");

    // Timer must be initialised before calling ble_conn_params_init()
    error = app_timer_init();
    RETURN_ERROR_ON(error, "Timer init failed: %d", error);

    // Initialize the SoftDevice handler library.
    nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);

    // Fetch the start address of the application RAM.
    uint32_t ramStart = 0;
    error = softdevice_app_ram_start_get(&ramStart);
    RETURN_ERROR_ON(error, "SD RAM get failed: %d", error);

    // Overwrite some of the default configurations for the BLE stack.
    ble_cfg_t bleConfig;

    // Configure the maximum number of connections.
    // INFO: Remember to adjust RAM start and RAM origin in multi.ld
    memset(&bleConfig, 0x00, sizeof(bleConfig));
    bleConfig.gap_cfg.role_count_cfg.periph_role_count  = MAX_CONNECTIONS_AS_PERIPHERAL;
    bleConfig.gap_cfg.role_count_cfg.central_role_count = MAX_CONNECTIONS_AS_CENTRAL;
    bleConfig.gap_cfg.role_count_cfg.central_sec_count  = MAX_CONNECTIONS_AS_CENTRAL;
    error = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &bleConfig, ramStart);
    RETURN_ERROR_ON(error, "BLE config roles failed: %d", error);

    // Configure the maximum ATT MTU.
    memset(&bleConfig, 0x00, sizeof(bleConfig));
    bleConfig.conn_cfg.conn_cfg_tag                 = CONN_CFG_TAG;
    bleConfig.conn_cfg.params.gatt_conn_cfg.att_mtu = NRF_BLE_GATT_MAX_MTU_SIZE;
    error = sd_ble_cfg_set(BLE_CONN_CFG_GATT, &bleConfig, ramStart);
    RETURN_ERROR_ON(error, "BLE config MTU size failed: %d", error);

    // Configure the maximum event length.
    memset(&bleConfig, 0x00, sizeof(bleConfig));
    bleConfig.conn_cfg.conn_cfg_tag                     = CONN_CFG_TAG;
    bleConfig.conn_cfg.params.gap_conn_cfg.event_length = BLE_GAP_CONN_EVENT_LENGTH;
    bleConfig.conn_cfg.params.gap_conn_cfg.conn_count   = MAX_CONNECTIONS;
    error = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &bleConfig, ramStart);
    RETURN_ERROR_ON(error, "BLE config event length failed: %d", error);

    // Configure the number of custom UUIDs.
    memset(&bleConfig, 0x00, sizeof(bleConfig));
    bleConfig.common_cfg.vs_uuid_cfg.vs_uuid_count = 1;
    error = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &bleConfig, ramStart);
    RETURN_ERROR_ON(error, "BLE config UUID count failed: %d", error);

    // Enable BLE stack.
    error = softdevice_enable(&ramStart);
    RETURN_ERROR_ON(error, "SD enable failed: %d", error);

    // Register a BLE event handler with the SoftDevice handler library.
    error = softdevice_ble_evt_handler_set(handleBluetoothEvent);
    RETURN_ERROR_ON(error, "SD handler set failed: %d", error);

    LOG_DEBUG("Stack OK");
    return NRF_SUCCESS;
}

/**@brief Set up the rest */
static ret_code_t managerInit2(const char* name) {

    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    ret_code_t error;
    if(strlen(name) != 3) {
        LOG_ERROR("Invalid name %s", (uint32_t) name);
        error = sd_ble_gap_device_name_set(&sec_mode, (uint8_t const *)DEVICE_NAME, strlen(DEVICE_NAME));
        RETURN_ERROR_ON(error, "Name set failed: %d", error);
    } else {
        error = sd_ble_gap_device_name_set(&sec_mode, (uint8_t const *)name, DEVICE_SHORT_NAME_LENGTH);
        RETURN_ERROR_ON(error, "Name set failed: %d", error);
    }

    error = setConnectionParameters();
    RETURN_ERROR_ON(error, "Connection parameters set failed: %d", error);

    error = nrf_ble_gatt_init(&gattModule, handleGattEvent);
    RETURN_ERROR_ON(error, "GATT init failed: %d", error);

    error = setAdvertisingData();
    RETURN_ERROR_ON(error, "Advertising set failed: %d", error);

    error = ble_db_discovery_init(handleDatabaseDiscovery);
    RETURN_ERROR_ON(error, "DB discovery init failed: %d", error);

    ble_uuid128_t baseUuid = {SERVICE_UUID_BASE};

    error = sd_ble_uuid_vs_add(&baseUuid, &baseUuidType);
    RETURN_ERROR_ON(error, "Add UUID failed: %d", error);

    ble_uuid_t bleUuid = {
        .type = baseUuidType,
        .uuid = SERVICE_UUID,
    };

    error = ble_db_discovery_evt_register(&bleUuid);
    RETURN_ERROR_ON(error, "DB event register failed: %d", error);

    // Add service.
    uint16_t serviceHandle;
    error = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &bleUuid, &serviceHandle);
    RETURN_ERROR_ON(error, "Add service failed: %d", error);

    // Add AMTS characteristic.
    ble_add_char_params_t receiveParams;
    memset(&receiveParams, 0, sizeof(receiveParams));

    receiveParams.uuid              = RECEIVE_CHAR_UUID;
    receiveParams.uuid_type         = baseUuidType;
    receiveParams.max_len           = NRF_BLE_GATT_MAX_MTU_SIZE;
    receiveParams.init_len          = 0;
    receiveParams.char_props.notify = true;
    receiveParams.char_props.indicate = true;
    receiveParams.char_props.read   = true;
    receiveParams.char_props.write  = true;
    receiveParams.cccd_write_access = SEC_OPEN;
    receiveParams.read_access       = SEC_OPEN;
    receiveParams.is_var_len        = 1;

    error = characteristic_add(serviceHandle, &receiveParams, &receiveHandles);
    RETURN_ERROR_ON(error, "Add AMT failed: %d", error);

    error = nrf_ble_gatt_att_mtu_periph_set(&gattModule, NRF_BLE_GATT_MAX_MTU_SIZE);
    RETURN_ERROR_ON(error, "ATT MTU peripheral set failed: %d", error);

    error = nrf_ble_gatt_att_mtu_central_set(&gattModule, NRF_BLE_GATT_MAX_MTU_SIZE);
    RETURN_ERROR_ON(error, "ATT MTU central set failed: %d", error);

    // A packet is MAX_MTU bytes + 4 bytes header. The payload size is MAX_MTU - 2 bytes
    uint8_t data_length = NRF_BLE_GATT_MAX_MTU_SIZE + LL_HEADER_LEN;
    error = nrf_ble_gatt_data_length_set(&gattModule, BLE_CONN_HANDLE_INVALID, data_length);
    RETURN_ERROR_ON(error, "Data length set failed: %d", error);

    ble_opt_t opt;
    memset(&opt, 0x00, sizeof(opt));
    opt.common_opt.conn_evt_ext.enable = 1;

    error = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
    RETURN_ERROR_ON(error, "Event extension set failed: %d", error);

    error = sd_ble_gap_tx_power_set(TX_POWER_LEVEL_DEFAULT);
    RETURN_ERROR_ON(error, "Power set failed: %d", error);

    ble_gap_addr_t ownAddress;
    error = sd_ble_gap_addr_get(&ownAddress);
    RETURN_ERROR_ON(error, "Address get failed: %d", error);

    LOG_DEBUG("Manager OK, Address ");
    printPeerAddress(ownAddress);
    LOG_DEBUG("\n");
    return NRF_SUCCESS;
}

/**@brief Initialise the stack and other core functions */
ret_code_t initialiseManager(const char* deviceName) {
    ret_code_t error = managerInit();
    if (error != NRF_SUCCESS) { return error; }
    return managerInit2(deviceName);
}

/**@brief Limit the connection interval to acceptable values */
static uint16_t connectionIntervalTicks(float interval) {
    uint16_t intervalInt = (uint16_t) (interval / 1.25f);
    if (intervalInt < CONN_INTERVAL_MIN) {
        return CONN_INTERVAL_MIN;
    }
    if (intervalInt > CONN_INTERVAL_MAX) {
        return CONN_INTERVAL_MAX;
    }
    return intervalInt;
}

/**@brief  Register a connection to a peripheral or a central */
uint8_t addConnection(Connection newConnection) {
    RETURN_ERROR_IF(connectionsCount == MAX_CONNECTIONS, MANAGER_CONNECTION_INVALID, "Too many connections");
    if (newConnection.asCentral) {
        RETURN_ERROR_IF(strlen(newConnection.name) != DEVICE_SHORT_NAME_LENGTH, MANAGER_CONNECTION_INVALID, "Invalid name length");
    }

    ConnectionInternal* connection = &connections[connectionsCount];
    connectionsCount += 1;
    resetConnection(connection);
    connection->received      = newConnection.received;
    connection->eventHandler  = newConnection.eventHandler;
    connection->asCentral     = newConnection.asCentral;
    connection->confirm       = newConnection.requestConfirmation;
    connection->shouldConnect = false;
    connection->mainAddress.addr_id_peer  = 0;
    connection->mainAddress.addr_type  = 1;
    memcpy(connection->mainAddress.addr, newConnection.mainAddress, 6);

    connection->redAddress.addr_id_peer  = 0;
    connection->redAddress.addr_type  = 1;
    memcpy(connection->redAddress.addr, newConnection.redAddress, 6);

    if (connection->asCentral) {
        connection->interval = connectionIntervalTicks(newConnection.interval);
    } else {
        connection->interval = 0;
    }

    memcpy(connection->name, newConnection.name, DEVICE_SHORT_NAME_LENGTH+1);
    LOG_DEBUG("Added connection %d, interval ticks %d", connectionsCount - 1, connection->interval);
    return connectionsCount - 1;
}

/**@brief request data over a connection */
ManagerStatus requestData(uint8_t connectionID) {
    RETURN_ERROR_IF(connectionID >= connectionsCount, MANAGER_INVALID_ID, "Invalid ID %d", connectionID);
    ConnectionInternal* connection = &connections[connectionID];
    RETURN_ERROR_IF(connection->handle == BLE_CONN_HANDLE_INVALID, MANAGER_NOT_CONNECTED, "No connection handle");

    tx_message_t message;
    message.request.readHandle = connection->receiveHandle;
    message.handle             = connection->handle;
    message.isWrite            = false;

    bool added = bufferInsert(&connection->buffer, message);
    PRINT_WARNING_ON(!added, "Buffer full for connection 0x%02x", connection->handle);

    ret_code_t error = processBuffer(connection);
    RETURN_INFO_IF(error != NRF_SUCCESS, MANAGER_SEND_QUEUED, "Buffer error %d", error);
    return MANAGER_SUCCESS;
}

/**@brief send data over a connection */
ManagerStatus sendData(uint8_t connectionID, Data data) {
    RETURN_ERROR_IF(connectionID >= connectionsCount, MANAGER_INVALID_ID, "Invalid ID %d", connectionID);
    ConnectionInternal* connection = &connections[connectionID];
    bool tooLong = (data.length > connection->payloadSize);
    RETURN_ERROR_IF(tooLong, MANAGER_INVALID_LENGTH, "Too many bytes (%d,%d)", data.length, connection->payloadSize);

    ble_gatts_hvx_params_t const hvxParam = {
        .type   = (connection->confirm) ? BLE_GATT_HVX_INDICATION : BLE_GATT_HVX_NOTIFICATION,
        .handle = receiveHandles.value_handle,
        .p_data = data.data,
        .p_len  = &data.length,
    };

    ret_code_t status = sd_ble_gatts_hvx(connection->handle, &hvxParam);

    if (status == NRF_ERROR_RESOURCES) {
        // Wait for BLE_GATTS_EVT_HVN_TX_COMPLETE.
        LOG_DEBUG("HVX busy");
        connection->busy = true;
        return MANAGER_BUSY;
    }
    RETURN_ERROR_IF(status != NRF_SUCCESS, MANAGER_SEND_FAILED, "send HVX failed: %d", status);
    LOG_DEBUG("Sending %d bytes", data.length);
    return MANAGER_SUCCESS;
}

/**@brief provide data over a connection for retrival by request */
ManagerStatus provideData(uint8_t connectionID, Data data) {
    RETURN_ERROR_IF(connectionID >= connectionsCount, MANAGER_INVALID_ID, "Invalid serverID %d", connectionID);
    ConnectionInternal* connection = &connections[connectionID];
    ble_gatts_value_t valueParam;
    memset(&valueParam, 0x00, sizeof(valueParam));
    valueParam.len     = data.length;
    valueParam.p_value = data.data;

    ret_code_t error = sd_ble_gatts_value_set(connection->handle, receiveHandles.value_handle, &valueParam);
    RETURN_ERROR_IF(error != NRF_SUCCESS, MANAGER_PROVIDE_FAILED, "Set GATTS value failed: %d", error);
    LOG_DEBUG("Providing %d bytes", valueParam.len);
    return MANAGER_SUCCESS;
}

/**@brief Update the connection interval (only for connection as master) */
ManagerStatus updateConnectionInterval(uint8_t connectionID, float interval) {
    RETURN_ERROR_IF(connectionID >= connectionsCount, MANAGER_INVALID_ID, "Invalid ID %d", connectionID);
    ConnectionInternal* connection = &connections[connectionID];
    RETURN_ERROR_IF(!connection->asCentral, MANAGER_NOT_MASTER, "Not connected as master %d", connectionID);

    uint16_t intervalTicks = connectionIntervalTicks(interval);
    ret_code_t status = changeIntervalToPeripheral(connection->handle, intervalTicks);
    RETURN_ERROR_ON_ERROR(status, MANAGER_UPDATE_FAILED, "Could not update connection %d", connectionID);
    connection->updateInterval = true;
    connection->interval = intervalTicks;
    return MANAGER_SUCCESS;
}

/**@brief Check if a connection is established */
bool isConnected(uint8_t ID) {
    RETURN_DEBUG_IF(ID >= connectionsCount, false, "Invalid ID %d", ID);
    ConnectionInternal* connection = &connections[ID];
    return (connection->handle != BLE_CONN_HANDLE_INVALID);
}

/**@brief Check if a connection is established */
bool isReady(uint8_t ID) {
    RETURN_DEBUG_IF(ID >= connectionsCount, false, "Invalid ID %d", ID);
    ConnectionInternal* connection = &connections[ID];

    if (connection->handle == BLE_CONN_HANDLE_INVALID) { return false; }
    if (connection->updateInterval) { return false; }
    return (connection->payloadSize != 0);
}

/**@brief Connect to the peer */
bool connect(uint8_t ID) {
    RETURN_DEBUG_IF(ID >= connectionsCount, false, "Invalid ID %d", ID);
    ConnectionInternal* connection = &connections[ID];
    if (connection->shouldConnect) { return true; }

    connection->shouldConnect = true;
    startStopScanAndAvertising();
    return true;
}

/**@brief Disconnect from the peer */
bool disconnect(uint8_t ID) {
    RETURN_DEBUG_IF(ID >= connectionsCount, false, "Invalid ID %d", ID);
    ConnectionInternal* connection = &connections[ID];
    if (connection->handle == BLE_CONN_HANDLE_INVALID) { return false; }
    ret_code_t error = sd_ble_gap_disconnect(connection->handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
    RETURN_ERROR_IF(error != NRF_SUCCESS, false, "Disconnect failed: %d", error);
    connection->shouldConnect = false;
    return true;
}

/**@brief Get the RSSI for a connection */
int8_t getRSSI(uint8_t ID) {
    RETURN_ERROR_IF(ID >= connectionsCount, 0, "Invalid ID %d", ID);
    ConnectionInternal* connection = &connections[ID];
    bool isEqual = (connection->handle == BLE_CONN_HANDLE_INVALID);
    RETURN_ERROR_IF(isEqual, 0, "No connection");

    int8_t rssi = 0;
    ret_code_t error = sd_ble_gap_rssi_get(connection->handle, &rssi);
    PRINT_ERROR_ON(error, "No client RSSI: %d", error);
    return rssi;
}

/**@brief Get temperature of the chip, in degrees (0.25 degrees precision) */
float getTemperature() {
    int32_t temp;
    ret_code_t error = sd_temp_get(&temp);
    RETURN_WARNING_IF(error != NRF_SUCCESS, 0, "No temp %d", error);
    return (float) (temp / 4.0);
}

/**@brief Start an endless loop, processing the log and sleeping the chip */
void loopAndLog() {
    LOG_DEBUG("Started loop");
    for (;;) {
        if (!NRF_LOG_PROCESS()) {
            sd_app_evt_wait();
        }
    }
}

/**@brief Call the timerCallback in the specified interval */
bool startTimer(uint16_t msInterval, void (*timerCallback) (void* context)) {
    APP_TIMER_DEF(timerID); // static timer definition

    ret_code_t error = app_timer_create(&timerID, APP_TIMER_MODE_REPEATED, timerCallback);
    RETURN_ERROR_IF(error != NRF_SUCCESS, false, "Register 0x%04d", error);

    error = app_timer_start(timerID, APP_TIMER_TICKS(msInterval), NULL);
    RETURN_ERROR_IF(error != NRF_SUCCESS, false, "Start 0x%04d", error);
    return true;
}

bool setOutputPower(OutputPower dB) {
    ret_code_t status = sd_ble_gap_tx_power_set(dB);
    RETURN_ERROR_ON_ERROR(status, false, "Power set failed: %d", status);
    return true;
}
