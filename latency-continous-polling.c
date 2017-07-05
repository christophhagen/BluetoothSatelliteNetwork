#define NRF_LOG_MODULE_NAME "APP"
#include "config.h"

#include "manager.h"

#include "boards.h" // Only needed for LEDs
#include "counter.h"

#define CONNECTION_INTERVAL     7.5
#define CHANGE_INTERVAL         3000


//#define BE_SENDER

#ifdef BE_SENDER
void testTransferSpeed();

static uint8_t peerID = MANAGER_CONNECTION_INVALID;

uint8_t oldLength = PAYLOAD_LENGTH;

void receivedData(ConstantData data) {
    counter_stop();
    uint32_t time_ms = counter_get();
    if (data.length == oldLength) {
        NRF_LOG_ERROR_PLAIN("%d;", time_ms);
    } else {
        oldLength = data.length;
        NRF_LOG_ERROR_PLAIN("\nLength %d", oldLength);
    }
    counter_start();
    requestData(peerID);
}

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED: {
            bsp_board_led_on(1);
        } break;
        case DISCONNECTED: {
            bsp_board_led_off(1);
            counter_stop();
        } break;
        case CONNECTION_UPDATED:
        case CONNECTION_READY: {
            counter_stop();
            oldLength = PAYLOAD_LENGTH;
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
    .length = PAYLOAD_LENGTH,
};

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED: {
            bsp_board_led_on(1);
        } break;
        case DISCONNECTED: {
            bsp_board_led_off(1);
        } break;
        case CONNECTION_READY: {
            dataToSend.length = PAYLOAD_LENGTH;
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

void timerCallback (void* context) {
    dataToSend.length = dataToSend.length - 1;
    if (dataToSend.length == 0) {
        dataToSend.length = PAYLOAD_LENGTH;
    }
    provideData(peerID, dataToSend);
}

int main(void) {
    counter_init();
    initialiseManager("BOB");
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    startTimer(CHANGE_INTERVAL, timerCallback);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}
#endif
