/**
Code used to test the effect of packet size on throughput
for different connection intervals */

#define NRF_LOG_MODULE_NAME "APP"
#include "config.h"

#include "manager.h"

#include "boards.h" // Only needed for LEDs
#include "counter.h"

#define CONNECTION_INTERVAL     7.5
#define NR_OF_PACKETS           500

//#define BE_SENDER

#ifdef BE_SENDER
void testTransferSpeed();

static uint8_t peerID = MANAGER_CONNECTION_INVALID;
uint16_t packetsSent = 0;
uint8_t currentConnIntervalTicks = 6;

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
            packetsSent = 0;
            // counter_start();
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
    .length = PAYLOAD_LENGTH,
};

void testTransferSpeed() {
    while (true) {
        if (packetsSent == NR_OF_PACKETS) {
            packetsSent = 0;
            dataToSend.length = (dataToSend.length - 1);
            if (dataToSend.length == 9) {
                dataToSend.length = PAYLOAD_LENGTH;
                currentConnIntervalTicks += 1;
                if (currentConnIntervalTicks == 41) {
                    currentConnIntervalTicks = 6;
                }
                NRF_LOG_ERROR_PLAIN("Conn interval ticks %d\n", currentConnIntervalTicks);
                updateConnectionInterval(peerID, currentConnIntervalTicks * 1.25f);
                return; // Wait for connection updated event
            }
        }
        dataToSend.data[0] = (uint8_t) ((packetsSent & 0xFF00) >> 8);
        dataToSend.data[1] = (uint8_t) (packetsSent & 0x00FF);
        uint8_t result = sendData(peerID, dataToSend);
        if (result == MANAGER_BUSY) {
            return; // Wait for transfer complete event
        }
        if (result != MANAGER_SUCCESS) {
            NRF_LOG_INFO("Error while sending, %d\n", result);
            bsp_board_led_off(2);
            return;
        }
        packetsSent += 1;
        bsp_board_led_invert(2);
    }
}

int main(void) {
    counter_init();
    initialiseManager("ALI");
    bsp_board_leds_init(); // Only for debugging
    setupConnection();
    connect(peerID);
    NRF_LOG_ERROR_PLAIN("Conn interval ticks %d\n", currentConnIntervalTicks);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}

#else

static uint32_t receivedPackets = 0;
static uint8_t peerID = MANAGER_CONNECTION_INVALID;
static uint8_t packetSize = 0;

void receivedData(ConstantData data) {
    bsp_board_led_invert(2);

    uint16_t packet = (((uint16_t) data.data[0]) << 8) | data.data[1];
    if (packet == 0) {
        packetSize = data.length;
        counter_start();
        receivedPackets = 0;
    }
    if (packet != receivedPackets) {
        NRF_LOG_ERROR_PLAIN("Is %d, expected %d\n", packet, receivedPackets);
    }
    receivedPackets = packet + 1;
    if (receivedPackets == NR_OF_PACKETS) {
        counter_stop();
        uint32_t time_ms      = counter_get();
        uint32_t bit_count    = (NR_OF_PACKETS * 8) * packetSize;
        float throughput_kbps = ((bit_count / (time_ms / 1000.f)) / 1000.f);

        NRF_LOG_ERROR_PLAIN("%3d; " NRF_LOG_FLOAT_MARKER "\n", data.length, NRF_LOG_FLOAT(throughput_kbps));
        receivedPackets = 0;
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
            receivedPackets = 0;
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
    NRF_LOG_ERROR_PLAIN("Conn interval ticks %d\n", 6);
    bsp_board_led_on(0); // Indicate that the device is running
    loopAndLog();
}
#endif
