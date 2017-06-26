#define NRF_LOG_MODULE_NAME "BUF"
#include "config.h"

#include "ble_gattc.h"

#include "buffer.h"
#include "error.h"

void bufferInit(buffer_t* buffer) {
    buffer->writeIndex = 0;
    buffer->readIndex = 0;
    buffer->isFull = false;
}

bool bufferInsert(buffer_t* buffer, tx_message_t message) {
    RETURN_WARNING_IF(buffer->isFull, false, "Buffer is full (%d)", TX_BUFFER_SIZE);
    buffer->data[buffer->writeIndex] = message;
    buffer->writeIndex = (buffer->writeIndex + 1) % TX_BUFFER_SIZE;
    buffer->isFull = (buffer->writeIndex == buffer->readIndex);
    return true;
}

/**@brief Function to return next element without removing it */
tx_message_t* bufferGetNext(buffer_t* buffer) {
    if ((buffer->writeIndex == buffer->readIndex) && !buffer->isFull) {
        return 0;
    }
    return &buffer->data[buffer->readIndex];
}

tx_message_t* bufferRemoveNext(buffer_t* buffer) {
    if ((buffer->writeIndex == buffer->readIndex) && !buffer->isFull) {
        return 0;
    }
    tx_message_t* message = &buffer->data[buffer->readIndex];
    buffer->readIndex = (buffer->readIndex + 1) % TX_BUFFER_SIZE;
    buffer->isFull = false;
    return message;
}

void bufferDeleteNext(buffer_t* buffer) {
    if (bufferHasElement(buffer)) {
        buffer->readIndex = (buffer->readIndex + 1) % TX_BUFFER_SIZE;
        buffer->isFull = false;
    }
}

bool bufferEmpty(buffer_t* buffer) {
    return ((buffer->writeIndex == buffer->readIndex) && !buffer->isFull);
}

bool bufferHasElement(buffer_t* buffer) {
    return (buffer->isFull || (buffer->writeIndex != buffer->readIndex));
}
