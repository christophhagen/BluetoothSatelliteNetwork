#define NRF_LOG_MODULE_NAME "APP"
#include "config.h"

#include "manager.h"

#include "boards.h" // Only needed for LEDs
#include "counter.h"

#define CONNECTION_INTERVAL     7.5
#define CHANGE_INTERVAL         3000

static uint8_t peerID = MANAGER_CONNECTION_INVALID;

#define BE_SENDER

#ifdef BE_SENDER

#define OWN_NAME                "ALI"
#define CONNECT_TO_NAME         "BOB"
#define CONNECT_AS_CENTRAL      true
#define CONNECT_TO_ADDRESS      {0x7C,0xA7,0xDF,0x89,0x26,0xDB}

uint8_t oldLength = PAYLOAD_LENGTH;

void receivedData(ConstantData data) {
    counter_stop();
    uint32_t time_ms = counter_get();
    if (data.length == oldLength) {
        LOG_INFO_RAW("%d\n", time_ms);
    } else {
        oldLength = data.length;
        LOG_INFO_RAW("Length %d\n", oldLength);
    }
    counter_start();
    requestData(peerID);
}

void connectionReady() {
    counter_stop();
    oldLength = PAYLOAD_LENGTH;
    counter_start();
    requestData(peerID);
}


void additionalSetup() { }

#else

#define OWN_NAME                "BOB"
#define CONNECT_TO_NAME         "ALI"
#define CONNECT_AS_CENTRAL      false
#define CONNECT_TO_ADDRESS      {0x48,0x69,0x59,0x92,0x5D,0xE6}

uint8_t data[PAYLOAD_LENGTH];
Data dataToSend = {
    .data = data,
    .length = PAYLOAD_LENGTH,
};

void receivedData(ConstantData data) { }

void connectionReady() {
    dataToSend.length = PAYLOAD_LENGTH;
    provideData(peerID, dataToSend);
}

void timerCallback (void* context) {
    dataToSend.length = dataToSend.length - 1;
    if (dataToSend.length == 0) {
        dataToSend.length = PAYLOAD_LENGTH;
    }
    provideData(peerID, dataToSend);
}

void additionalSetup() {
    startTimer(CHANGE_INTERVAL, timerCallback);
}

#endif

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
            connectionReady();
        } break;
        default: break;
    }
}

void setupConnection() {
    Connection toPeer = {
        .name         = CONNECT_TO_NAME,
        .asCentral    = CONNECT_AS_CENTRAL,
        .mainAddress  = CONNECT_TO_ADDRESS,
        .requestConfirmation = false,
        .received     = receivedData,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

int main(void) {
    counter_init();
    initialiseManager(OWN_NAME);
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    additionalSetup();
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}
