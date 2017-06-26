#ifndef UTIL_H__
#define UTIL_H__

#include <stdint.h>
#include <stdbool.h>

#include <ble_gap.h>

/**@brief Variable length data encapsulation in terms of length and pointer to data. */
typedef struct {
    uint8_t* data;      /**< Pointer to data. */
    uint16_t length;    /**< Length of data in bytes. */
} Data;

/**@brief Variable length data encapsulation in terms of length and pointer to data. */
typedef struct {
    const uint8_t* data;      /**< Pointer to data. */
    const uint16_t length;    /**< Length of data in bytes. */
} ConstantData;

/** Compares two addresses for equality */
bool peerAddressEqual(const ble_gap_addr_t* a1, ble_gap_addr_t* a2);

void printPeerAddress(ble_gap_addr_t a);

void printPeerAddressFull(ble_gap_addr_t a);


#endif
