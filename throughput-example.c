#define NRF_LOG_MODULE_NAME "APP"
#include "config.h"

#include "manager.h"

#include "boards.h" // Only needed for LEDs
#include "counter.h"

#define CONNECTION_INTERVAL     7.5
#define BYTES_TO_MEASURE        500000
#define SEND_PACKAGE_LENGTH     PAYLOAD_LENGTH

#define BE_SENDER

#ifdef BE_SENDER
void testTransferSpeed();

static uint8_t peerID = MANAGER_CONNECTION_INVALID;

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED: {
            bsp_board_led_on(1);
        } break;
        case DISCONNECTED: {
            bsp_board_led_off(1);
            // counter_stop();
        } break;
        case CONNECTION_UPDATED:
        case CONNECTION_READY: {
            bsp_board_led_off(2);
            testTransferSpeed();
        } break;
        case DATA_SENT_TO_PEER: {
            bsp_board_led_invert(2);
            testTransferSpeed();
        } break;
        default: break;
    }
}

void setupConnection() {
    Connection toPeer = {
        .name         = "BOB",
        .asCentral    = true,
        .mainAddress  = {0x48,0x69,0x59,0x92,0x5D,0xE6},
        .interval     = CONNECTION_INTERVAL,
        .requestConfirmation = false,
        .received     = 0,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

uint8_t data[PAYLOAD_LENGTH];
Data dataToSend = {
    .data = data,
    .length = SEND_PACKAGE_LENGTH,
};

void testTransferSpeed() {
    while (true) {
        uint8_t result = sendData(peerID, dataToSend);
        if (result == MANAGER_BUSY) {
            return; // Wait for transfer complete event
        }
        if (result != MANAGER_SUCCESS) {
            NRF_LOG_INFO("Error while sending, %d\n", result);
            bsp_board_led_off(2);
            return;
        }
        bsp_board_led_invert(2);
    }
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

static uint32_t receivedBytes = 0;
static uint8_t peerID = MANAGER_CONNECTION_INVALID;

void receivedData(ConstantData data) {
    bsp_board_led_invert(2);

    if (receivedBytes == 0) {
        counter_start();
    }

    receivedBytes += data.length;
    if (receivedBytes >= BYTES_TO_MEASURE) {
        counter_stop();
        uint32_t time_ms      = counter_get();
        uint32_t bit_count    = (receivedBytes * 8);
        float throughput_kbps = ((bit_count / (time_ms / 1000.f)) / 1000.f);

        NRF_LOG_ERROR_PLAIN("%3d; " NRF_LOG_FLOAT_MARKER "\n", data.length, NRF_LOG_FLOAT(throughput_kbps));
        receivedBytes = 0;
        bsp_board_led_off(2);
    }
}

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
            receivedBytes = 0;
        } break;
        default: break;
    }
}

void setupConnection() {
    Connection toPeer = {
        .name         = "ALI",
        .asCentral    = false,
        .mainAddress  = {0x7C,0xA7,0xDF,0x89,0x26,0xDB},
        .requestConfirmation = false,
        .received     = receivedData,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

int main(void) {
    counter_init();
    initialiseManager("BOB");
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}
#endif
