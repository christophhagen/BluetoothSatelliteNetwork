#include "config.h"
#include "logging.h"

#include "util.h"

/**@brief Compares two addresses for equality */
bool peerAddressEqual(const ble_gap_addr_t* a1, ble_gap_addr_t* a2) {
    if (a1->addr_type != a2->addr_type) { return false; }
    if (a1->addr_id_peer != a2->addr_id_peer) { return false; }
    for (uint8_t i = 0; i < BLE_GAP_ADDR_LEN; i += 1) {
        if (a1->addr[i] != a2->addr[i]) { return false; }
    }
    return true;
}

// TODO: Change into macro
/**@brief Prints the full device address */
void printPeerAddressFull(ble_gap_addr_t a) {
    LOG_INFO_RAW("Address %d,%d: %02X:", a.addr_type, a.addr_id_peer, a.addr[0]);
    LOG_INFO_RAW("%02X:%02X:%02X:%02X:%02X", a.addr[1], a.addr[2], a.addr[3], a.addr[4], a.addr[5]);
}

/**@brief Prints the device address */
void printPeerAddress(ble_gap_addr_t a) {
    LOG_INFO_RAW("%02X:%02X:%02X:", a.addr[0], a.addr[1], a.addr[2]);
    LOG_INFO_RAW("%02X:%02X:%02X", a.addr[3], a.addr[4], a.addr[5]);
}
