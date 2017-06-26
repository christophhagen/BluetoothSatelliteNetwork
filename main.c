#include "manager.h"

#include "boards.h"

// {0x7C,0xA7,0xDF,0x89,0x26,0xDB},
// {0x48,0x69,0x59,0x92,0x5D,0xE6},

void receivedData(ConstantData data) {
    /** Handle the received data */
}

void handleEvents(ManagerEvent event) {
    switch (event) {
        case CONNECTED:             /** Do something */ break;
        case CONNECTION_READY:      /** Do something */ break;
        case DISCONNECTED:          /** Do something */ break;
        case DATA_ARRIVED_AT_PEER:  /** Do something */ break;
        case DATA_SENT_TO_PEER:     /** Do something */ break;
    }
}

uint8_t peerID = MANAGER_CONNECTION_INVALID;

void setupConnection() {
    Connection toPeer = {
        .name         = "BOT",
        .asCentral    = false,
        .mainAddress  = {0x48,0x69,0x59,0x92,0x5D,0xE6},
        .interval     = 7.5,
        .requestConfirmation = false,
        .received     = receivedData,
        .eventHandler = handleEvents,
    };
    peerID = addConnection(toPeer);
}

void doSomethingPeriodic(void* context) {
    uint8_t data[1];
    Data dataToSend = {
        .data = data,
        .length = 1,
    };
    if (isConnected(peerID)) {
        sendData(peerID, dataToSend);    /* OR */
        provideData(peerID, dataToSend); /* OR */
        requestData(peerID);
    }
}

int main(void) {
    initialiseManager("MID");
    bsp_board_leds_init();
    setupConnection();
    //connect(peerID);
    //startTimer(1000, doSomethingPeriodic);
    bsp_board_led_on(0);
    loopAndLog();
}
