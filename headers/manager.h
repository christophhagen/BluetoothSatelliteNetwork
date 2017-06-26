#ifndef MANAGER_H__
#define MANAGER_H__

#include <stdint.h>
#include <stdbool.h>

#include "ble.h"
#include "ble_db_discovery.h"
#include "nrf_ble_gatt.h"
#include "nrf_temp.h"  // Temperature measurements

#include "util.h"
#include "buffer.h"

/**@cond To Make Doxygen skip documentation generation for this file.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**< Length of opcode inside a notification. */
#define OPCODE_LENGTH 1
/**< Length of handle inside a notification. */
#define HANDLE_LENGTH 2

/** Maximum number of bytes that can be transmitted in a single packet */
#define PAYLOAD_LENGTH      (NRF_BLE_GATT_MAX_MTU_SIZE-OPCODE_LENGTH-HANDLE_LENGTH)

#define MANAGER_CONNECTION_INVALID    0xFF

typedef enum {
    MANAGER_SUCCESS = 0x00,
    MANAGER_INVALID_LENGTH = 0x01,
    MANAGER_BUSY = 0x02,
    MANAGER_SEND_QUEUED = 0x03,
    MANAGER_INVALID_ID = 0x04,
    MANAGER_SERVER_NOT_READY = 0x05,
    MANAGER_NOT_CONNECTED = 0x06,
    MANAGER_PROVIDE_FAILED = 0x07,
    MANAGER_SEND_FAILED = 0x08,
    MANAGER_NO_NOTIFICATIONS = 0x09,
    MANAGER_NO_INDICATIONS = 0x10,
} ManagerStatus;

typedef enum {
    CONNECTED = 1,
    CONNECTION_READY = 2,
    DISCONNECTED = 3,
    DATA_ARRIVED_AT_PEER = 4,
    DATA_SENT_TO_PEER = 5,
} ManagerEvent;

/** A callback function for requested data */
typedef void (*DataHandler) (ConstantData data);
typedef void (*EventHandler) (ManagerEvent event);

/**@brief Init struct to register a new connection */
typedef struct {
    bool         asCentral;
    DataHandler  received;
    EventHandler eventHandler;
    const char*  name;
    uint8_t      mainAddress[6];
    uint8_t      redAddress[6];
    float        interval;
    bool         requestConfirmation;
} Connection;

/**@brief Initialise the stack and other core functions */
ret_code_t initialiseManager(const char* deviceName);

/**@brief  Register a connection to a peripheral or a central */
uint8_t addConnection(Connection newConnection);

/**@brief Check if a connection is established */
bool isConnected(uint8_t ID);

/**@brief Check if a connection is ready */
bool isReady(uint8_t ID);

/**@brief Connect to the peer */
bool connect(uint8_t ID);

/**@brief Disconnect from the peer */
bool disconnect(uint8_t ID);

/**@brief provide data over a connection for retrival by request */
ManagerStatus provideData(uint8_t ID, Data data);

/**@brief send data over a connection */
ManagerStatus sendData(uint8_t ID, Data data);

/**@brief request data over a connection */
ManagerStatus requestData(uint8_t ID);

/**@brief Get the RSSI for a connection */
int8_t getRSSI(uint8_t ID);

/**@brief Get temperature of the chip, in degrees (0.25 degrees precision) */
float getTemperature();

/**@brief Start an endless loop, processing the log and sleeping the chip */
void loopAndLog();

/**@brief Call the timerCallback in the specified interval */
bool startTimer(uint16_t msInterval, void (*timerCallback) (void* context));

#endif
