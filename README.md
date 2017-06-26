# Intra-satellite communication based on Bluetooth Low Energy

## Overview

This repository provides the code that has been developed for the master thesis *Intra-satellite communication based on Bluetooth Low Energy* at Aalto university, Finland. The code is used with the nRF52832 chip from Nordic Semiconductor and handles the Bluetooth Low Energy connections between multiple devices to provide an easy-to-use system for the wireless communication between several satellite subsystems.

## Usage

The files provided here should be self-sufficient, since the complete SDK from Nordic Semiconductor is included in this repository. To use the code, follow these steps:

1. Download the repository
2. Connect the chip through a J-Link interface with the computer, or connect the nRF52 Development Kit via USB
3. Use a terminal and cd into the `make` folder of this repository
4. Run `make flash_softdevice` to load the Bluetooth Stack onto the chip
5. Run `make flash` to load the user code to the chip
6. The chip should be up and running

After this basic setup is working, use any code editor to modify the application code (for example [Atom](https://atom.io)).

## Features

The basic use of the code is best understood by looking at the example code in `main.c`. The overall process is fairly simple:

1. Initialise the core functionality through `initialiseManager(const char* deviceName)`.
2. Add the devices that should be connected to by using `addConnection(Connection newConnection)`.
3. Establish the connection by calling `connect(uint8_t ID)`.
4. Use the functions `sendData`,`provideData` and requestData` to exchange data.
5. Handle received data through the callback function `receivedData`.
6. Act on events for the connection through the event handler `handleEvents`.

## Other notes

Refer to the thesis paper in this repository for in-depth information about the protocols, hardware implementation and capabilities.

The nRF SDK is provided by Nordic Semiconductor [here](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF52832/Development-tools-and-Software).

This makefile uses the updated SoftDevice v4.0.3 (not 4.0.2 provided with the SDK), available [here](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF52832/Development-tools-and-Software).
