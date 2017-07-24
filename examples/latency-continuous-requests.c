#include "config.h"

#include "manager.h"

#include "boards.h" // Only needed for LEDs
#include "counter.h"

#define CONNECTION_INTERVAL     7.5
#define SEND_PACKAGE_LENGTH     100

#define BE_SENDER

#ifdef BE_SENDER

static uint8_t peerID = MANAGER_CONNECTION_INVALID;

uint8_t nrOfRequests = 0;
float connectionInterval = CONNECTION_INTERVAL;

void receivedData(ConstantData data) {
    counter_stop();
    uint32_t time_ms = counter_get();
    LOG_ERROR_RAW("%d\n", time_ms);
    if (nrOfRequests == 100) {
        nrOfRequests = 0;
        connectionInterval += 1.25;
        updateConnectionInterval(peerID, connectionInterval);
        LOG_ERROR_RAW("CI " NRF_LOG_FLOAT_MARKER " ms\n", NRF_LOG_FLOAT(connectionInterval));
    } else {
        nrOfRequests += 1;
        counter_start();
        requestData(peerID);
    }

}

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED: {
            bsp_board_led_on(1);
        } break;
        case DISCONNECTED: {
            bsp_board_led_off(1);
        } break;
        case CONNECTION_UPDATED:
        case CONNECTION_READY: {
            counter_start();
            requestData(peerID);
        } break;
        default: break;
    }
}

void setupConnection() {
    Connection toPeer = {
        .name         = "BOB",
        .asCentral    = true,
        .mainAddress  = {0x7C,0xA7,0xDF,0x89,0x26,0xDB},
        .interval     = CONNECTION_INTERVAL,
        .requestConfirmation = false,
        .received     = receivedData,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

int main(void) {
    counter_init();
    initialiseManager("ALI");
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}

#else

static uint8_t peerID = MANAGER_CONNECTION_INVALID;

uint8_t data[PAYLOAD_LENGTH];
Data dataToSend = {
    .data = data,
    .length = SEND_PACKAGE_LENGTH,
};

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED: {
            bsp_board_led_on(1);
        } break;
        case DISCONNECTED: {
            counter_stop();
            bsp_board_led_off(1);
            bsp_board_led_off(2);
        } break;
        case CONNECTION_READY: {
            provideData(peerID, dataToSend);
        } break;
        default: break;
    }
}

void setupConnection() {
    Connection toPeer = {
        .name         = "ALI",
        .asCentral    = false,
        .mainAddress  = {0x48,0x69,0x59,0x92,0x5D,0xE6},
        .requestConfirmation = false,
        .received     = 0,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

int main(void) {
    initialiseManager("BOB");
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}
#endif
