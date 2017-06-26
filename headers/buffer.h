#ifndef BUFFER_H__
#define BUFFER_H__

#include "ble_srv_common.h" // BLE_CCCD_VALUE_LEN
#include <ble_gattc.h>

#define TX_BUFFER_SIZE          8  /**< Size of send buffer */

#define WRITE_MESSAGE_LENGTH    BLE_CCCD_VALUE_LEN    /**< Length of the write message for the CCCD. */

/**@brief Structure for holding data to be transmitted to the connected central. */
typedef struct {
    uint16_t handle;  /**< Connection handle to be used when transmitting this message. */
    bool isWrite;         /**< Type of this message, i.e. read or write message. */
    union {
        uint16_t readHandle;  /**< Read request message. */
        struct {
            uint8_t                  value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
            ble_gattc_write_params_t params;                       /**< GATTC parameters for this message. */
        } gattc;    /**< Write request message. */
    } request;
} tx_message_t;

typedef struct {
    tx_message_t data[TX_BUFFER_SIZE]; //!<  Transmit buffer for messages to be transmitted to the central. */
    uint8_t      writeIndex;   //!<  Current insert index in the transmit buffer. */
    uint8_t      readIndex;   //!<  Current read index in the transmit buffer. */
    bool         isFull;
} buffer_t;

void bufferInit(buffer_t* buffer);

bool bufferInsert(buffer_t* buffer, tx_message_t message);

tx_message_t* bufferGetNext(buffer_t* buffer);

tx_message_t* bufferRemoveNext(buffer_t* buffer);

void bufferDeleteNext(buffer_t* buffer);

bool bufferEmpty(buffer_t* buffer);

bool bufferHasElement(buffer_t* buffer);

#endif
