WICED Manufacturing Bluetooth Test Tool

Overview

The WICED manufacturing Bluetooth test tool (WMBT) is used to test and verify the
RF performance of Cypress SoC Bluetooth BR/EDR/LE devices.

Each test sends an HCI or WICED HCI command to the device and then waits for an
HCI or WICED HCI Command Complete event from the device respectively.

The wmbt utility can be found in BTSDK installations in the workspace where shared
dependencies are installed, and is referred to as <WMBT_PATH> below:
mtb_shared\wiced_btsdk\tools\btsdk-utils\wmbt\Release

Device configuration:

    The Cypress Bluetooth device to be tested must expose an HCI UART and that this
    UART can be connected to a COM port or to a Serial to USB device of a PC. The
    HCI UART supports HCI Commands and Events described in this document.

    The device should be preprogrammed with an application image and should be
    reset after it has been connected to the PC and the COM port drivers are
    loaded.

    Check the device specific Kit Guide or Quick Start Guide for any DIP switch
    settings or jumper settings to configure the device to expose the HCI UART
    interface.

Environment Variables:

    MBT_BAUD_RATE: Cypress SoC Bluetooth devices support adjustable baud rates up
    to 4 Mbps via the wiced_transport_init() API included with the SDK. If this API
    is not utilized in an application to re-configure the baud rate, the default
    rate of 115.2 Kbps will be used by the device. The MBT_BAUD_RATE environment
    variable must be set to match what the device is using before running WMBT.

    As an example, to configure MBT_BAUD_RATE for 3 Mbps on a windows command line:

    mtb_shared\wiced_btsdk\tools\btsdk-utils\wmbt\Release>set MBT_BAUD_RATE=3000000

    TRANSPORT_MODE: The Bluetooth Core Specification [1] defines the Host Controller
    Interface (HCI) which provides a standardized communication protocol between the
    BT host stack and BT controller. Cypress SoC Bluetooth devices provide a high
    level of integration, e.g. BT Controller and embedded BT Host Stack in a single
    chip, to simplify BT product development for customers by not requiring them to
    be familiar with all HCI commands/events. Typically, when the embedded stack is
    utilized in the Cypress device and it interfaces to an onboard MCU, the MCU
    software would likely need to send/receive commands/events to the Cypress device.
    For such a solution, WICED HCI is defined and provided as an example, see WICED
    HCI UART Control Protocol [3]. WMBT provides support for both HCI and WICED HCI
    via the TRANSPORT_MODE environment variable. If WICED HCI is desired, your
    application must implement handlers for the
    HCI_CONTROL_TEST_COMMAND_ENCAPSULATED_HCI_COMMAND, see hci_control_test.c
    included with the watch sample application. HCI should be sufficient for most
    cases since the devices support this by default. The TRANSPORT_MODE environment
    variable must be set to the desired mode before running WMBT.

    As an example, to configure TRANSPORT_MODE for HCI on a windows command line:

    <WMBT_PATH>set TRANSPORT_MODE=0


*Reset

This command verifies that the device is correctly configured and connected to
the PC.

Usage: wmbt reset_highspeed COMx

The example below sends HCI_Reset command at the configured MBT_BAUD_RATE to the device
and processes the HCI Command Complete event (BLUETOOTH SPECIFICATION Version 4.1
[Vol 2], Section 7.3.2 for details).

<WMBT_PATH> wmbt reset_highspeed COM23
Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 03 0C 00 >
Received HCI Event:
0000 < 04 0E 04 01 03 0C 00 >
Success
Close Serial Bus

The last byte of the HCI Command Complete event is the operation status, where
0 signifies success.


*LE Receiver Test

This test configures the chip to receive reference packets at a fixed
interval. External test equipment should be used to generate the reference
packets.

The frequency on which the device listens for the packets is passed as a
parameter. BLE devices use 40 channels, each of which is 2 MHz wide. Channel
0 maps to 2402 MHz and Channel 39 maps to 2480 MHz (see BLUETOOTH
SPECIFICATION Version 4.1 [Vol 2], Section 7.8.28 for details).

Usage: wmbt le_receiver_test COMx <rx_frequency>
where:
    rx_frequency = Receive frequency in MHz ( 2402 to 2480 ).

The example below starts the LE receiver test on Channel 2 (2406 MHz).

<WMBT_PATH> wmbt le_receiver_test COM23 2406
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 1D 20 01 02 >
Received HCI Event:
0000 < 04 0E 04 01 1D 20 00 >
Success
Close Serial Bus

The last byte of the HCI event is the operation status,
where 0 signifies success.


*LE Transmitter Test

The LE Transmitter Test configures the Cypress SoC BT device to send test
packets at a fixed interval. External test equipment may be used to receive
and analyze the reference packets.

The frequency on which the device transmits the packets  is passed as a
parameter. BLE devices use 40 channels, each of which is 2 MHz wide. Channel 0
maps to 2402 MHz and Channel 39 maps to 2480 MHz.

The other two parameters specify the length of the test data and the data
pattern to be used (see BLUETOOTH SPECIFICATION Version 4.1 [Vol 2], Section
7.8.29 for details).

Usage: wmbt le_transmitter_test COMx <tx_frequency> <data_length> <data_pattern>
where:
    tx_frequency = transmit frequency in MHz ( 2402 to 2480 ).

    data_length = 0・7

    data_pattern = 0・
        0 = Pseudo-random bit sequence 9
        1 = Pattern of alternating bits: 11110000
        2 = Pattern of alternating bits: 10101010
        3 = Pseudo-random bit sequence 15
        4 = Pattern of all 1s
        5 = Pattern of all 0s
        6 = Pattern of alternating bits: 00001111
        7 = Pattern of alternating bits: 0101

The example below starts the test and instructs the device to transmit packets
on Channel 2 (2406 MHz), with a 10-byte payload of all ones (1s).

<WMBT_PATH> wmbt le_transmitter_test COM23 2406 10 4
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 1E 20 03 02 0A 04 >
Received HCI Event:
0000 < 04 0E 04 01 1E 20 00 >
Success
Close Serial Bus

The last byte of the HCI event is the status of the operation,
where 0 signifies the success.


*LE Test End

This command stops the LE Transmitter or LE Receiver Test that is in progress.

Usage: wmbt le_test_end COMx

The example below stops the active test.

<WMBT_PATH> wmbt le_test_end COM23
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 1F 20 00 >
Received HCI Event:
0000 < 04 0E 06 01 1F 20 00 00 00 >

Success num_packets_received = 0

Close Serial Bus


*Continuous Transmit Test

Note: Unlike the LE tests, this test uses 79 frequencies, each 1 MHz wide.

This test configures the Cypress SoC BT device to turn the carrier ON or OFF.
When the carrier is ON the device transmitsaccording to the specified transmit
mode, modulation type, frequency, and power level.

Usage: wmbt tx_frequency_arm COMx <carrier on/off> <tx_frequency> <mode>
<modulation_type> <tx_power>
where:

    carrier on/off:
        1 = carrier ON
        0 = carrier OFF
    tx_frequency = (2402 ・2480) transmit frequency, in MHz
    mode: (0 - 9)
        0 = Unmodulated
        1 = PRBS9
        2 = PRBS15
        3 = All Zeros
        4 = All Ones
                5 = Incrementing Symbols
        modulation_type: (0 - 3)
                0 = GFSK
                1 = QPSK
                2 = 8PSK
                3 = LE
        tx_power = Power Table Index with [0] being max power. Number of
                   indexes will depend on the SoC BT device used.

The example below turns the carrier ON and instructs the device to transmit an
unmodulated pattern on 2402 MHz at max tx power.

<WMBT_PATH> wmbt tx_frequency_arm COM23 1 2402 1 2 0
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 14 FC 07 00 00 01 02 09 00 00 >
Received HCI Event:
0000 < 04 0E 04 01 14 FC 00 >
Success
Close Serial Bus

To stop the test, send the command a second time to the same COM port with the
carrier on/off parameter set to zero (0).

<WMBT_PATH> wmbt tx_frequency_arm COM23 0 2402 1 2 3
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 14 FC 07 01 02 00 00 00 00 00 >
Received HCI Event:
0000 < 04 0E 04 01 14 FC 00 >
Success
Close Serial Bus


*Radio Tx Test

Note: Connectionless transmit test to send Bluetooth packets

The test configures the Cypress SoC BT device to transmit the selected data pattern
which is governed by a specified frequency and a specified logical channel at a
specified power level.

The frequency, modulation_type, logical channel, bb_packet_type, packet_length, and power level to be used by
the device are passed as parameters.

Usage: wmbt radio_tx_test COMx <bdaddr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <tx_power>
where:
    bd_addr: BD_ADDR of Tx device (6 bytes)
    frequency: 0 or transmit frequency (2402 ・2480) in MHz
        0: normal Bluetooth hopping sequence (79 channels)
        2402 - 2480: single frequency without hopping
    modulation_type:
        0: 0x00 8-bit Pattern
        1: 0xFF 8-bit Pattern
        2: 0xAA 8-bit Pattern
        3: 0xF0 8-bit Pattern
        4: PRBS9 Pattern
    logical_channel:
        0: EDR
        1: BR
    bb_packet_type:
        3: DM1
        4: DH1 / 2-DH1
        8: 3-DH1
        10: DM3 / 2-DH3
        11: DH3 / 3-DH3
        14: DM5 / 2-DH5
        15: DH5 / 3-DH5
    packet_length: 0 ・65535. Device will limit the length to the max for the baseband packet type.
        eg) if DM1 packets are sent, the maximum packet size is 17 bytes.
    tx_power = Power Table Index with [0] being max power. Number of
               indexes will depend on the SoC BT device used.

The example below instructs the device to transmit 0xAA 8-bit Pattern on the 2402 MHz and ACL Basic
with DM1 packet (17 bytes) type at max tx power

<WMBT_PATH> wmbt radio_tx_test COM23 112233445566 2402 2 1 3 17 0
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 51 FC 10 66 55 44 33 22 11 01 00 03 01 03 11 >
0010 < 00 09 00 00 >
Received HCI Event:
0000 < 04 0E 04 01 51 FC 00 >
Success
Close Serial Bus

The last byte of the HCI event is the operation status,
where 0 signifies that operation was successful and test started to run.
The test continues to run until device is reset.


*Radio Rx Test

Note: Connectionless receive test for Bluetooth packets

This test issues a command to the Cypress SoC BT device to set the radio to camp on a specified
frequency. While the test is running, the BT device periodically sends reports about received
packets.

Usage: wmbt radio_rx_test COMx <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length>
where:
    bd_addr: BD_ADDR for the remote Tx device (6 bytes)
    frequency = receive frequency (2402 ・2480) in MHz
    modulation_type:
        0: 0x00 8-bit Pattern
        1: 0xFF 8-bit Pattern
        2: 0xAA 8-bit Pattern
        3: 0xF0 8-bit Pattern
        4: PRBS9 Pattern
    logical_channel:
        0: EDR
        1: BR
    bb_packet_type:
        3: DM1
        4: DH1 / 2-DH1
        8: 3-DH1
        10: DM3 / 2-DH3
        11: DH3 / 3-DH3
        14: DM5 / 2-DH5
        15: DH5 / 3-DH5
    packet_length: 0 ・65535.
        Device will compare length of the received packets with the specified packet_length.

The Cypress SoC BT device will generate the statistics report of the Rx Test every second.

The example below instructs the device to receive 0xAA 8-bit Pattern on the 2402 MHz and ACL Basic with DM1 packet type.

<WMBT_PATH> wmbt radio_rx_test COM23 112233445566 2402 2 1 3 17
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 52 FC 0E 66 55 44 33 22 11 E8 03 00 03 01 03 >
0010 < 11 00 >
Received HCI Event:
0000 < 04 0E 04 01 52 FC 00 >
Success

Radio RX Test is running. Press the Enter key to stop the test.

WMBT reports connectionless Rx Test statistics every second.

The example below shows the Rx Test Statistics report -

Statistics Report received:
  [Rx Test statistics]
    Sync_Timeout_Count:     0x0
    HEC_Error_Count:        0x0
    Total_Received_Packets: 0x31f
    Good_Packets:           0x31f
    CRC_Error_Packets:      0x0
    Total_Received_Bits:    0x1a878
    Good_Bits:              0x1a878
    Error_Bits:             0x0

*TELEC Reduced Hopping Transmit Test
--------------------------------
!!!! Important Note !!!!
Sometime frequency hopping pattern cannot set to specified frequency.
User needs to re-execute this command when such frequency pattern was seen.

TELEC the Japanese government regulatory testing requires 20-ch reduced hopping pattern
at each lower, middle, upper 20-channels. The DUT will transmit a specific packet at
specific hopping channels. It works standalone, does not require peer device.

Note: Connectionless transmit test to send Bluetooth packets

The test configures the Cypress SoC BT device to transmit the selected data pattern
which is governed by a specified hopping pattern and a specified logical channel at a
specified power level.

The hopping_pattern, modulation_type, logical channel, bb_packet_type, packet_length, and power level to be used by
the device are passed as parameters.

Usage: wmbt telec_tx_test COMx <bdaddr> <hopping_pattern> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <tx_power>
where:
    bd_addr: BD_ADDR of Tx device (6 bytes)
    hopping_pattern:
        0: lower 20-ch hopping channels are used
        1: middle 20-ch hopping channels are used
        2: upper 20-ch hopping channels are used
    modulation_type:
        0: 0x00 8-bit Pattern
        1: 0xFF 8-bit Pattern
        2: 0xAA 8-bit Pattern
        3: 0xF0 8-bit Pattern
        4: PRBS9 Pattern
    logical_channel:
        0: EDR
        1: BR
    bb_packet_type:
        3: DM1
        4: DH1 / 2-DH1
        8: 3-DH1
        10: DM3 / 2-DH3
        11: DH3 / 3-DH3
        14: DM5 / 2-DH5
        15: DH5 / 3-DH5
    packet_length: 0 ・65535. Device will limit the length to the max for the baseband packet type.
        eg) if DM1 packets are sent, the maximum packet size is 17 bytes.
    tx_power = (・5 to +3) transmit power, in dBm.

The example below instructs the device to transmit 0xAA 8-bit Pattern on the lower 20-channels and ACL Basic
with DM1 packet (17 bytes) type at -3 dBm.

<WMBT_PATH> wmbt telec_tx_test COM4 112233445566 2 4 0 15 2000 0
MBT_BAUD_RATE:  115200
TRANSPORT_MODE: 0 (HCI)

Opened COM4 at speed: 115200
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 51 FC 10 66 55 44 33 22 11 00 00 04 00 0F D0 >
0010 < 07 08 00 00 >
Received HCI Event:
0000 < 04 0E 04 01 51 FC 00 >
Success
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 B0 86 31 00 00 00 00 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success
Close Serial Bus

The last byte of the HCI event is the operation status,
where 0 signifies that operation was successful and test started to run.
The test continues to run until device is reset.

Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 B4 86 31 00 00 00 00 F8 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 B8 86 31 00 FF 7F 00 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 FF >
Success
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 05 04 28 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 B3 >
Success, read data = B3
Sending HCI Command:
0000 < 01 0A FC 05 04 29 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 8E >
Success, read data = 8E
Sending HCI Command:
0000 < 01 0A FC 05 04 2A 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 04 >
Success, read data = 04
Sending HCI Command:
0000 < 01 0A FC 05 04 2B 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success, read data = 00
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 28 86 31 00 B2 8E 04 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 B2 >
Success
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 38 86 31 00 14 00 00 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 14 >
Success
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 05 04 20 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 60 >
Success, read data = 60
Sending HCI Command:
0000 < 01 0A FC 05 04 21 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success, read data = 00
Sending HCI Command:
0000 < 01 0A FC 05 04 22 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success, read data = 00
Sending HCI Command:
0000 < 01 0A FC 05 04 23 86 31 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 00 >
Success, read data = 00
Close Serial Bus
Opened COM4 at speed: 115200
Sending HCI Command:
0000 < 01 0A FC 09 05 20 86 31 00 61 00 00 00 >
Received HCI Event:
0000 < 04 0E 05 01 0A FC 00 61 >
Success
Close Serial Bus

*Read BD ADDR

This command reads the BD ADDR that is currently programmed for the DUT.

Usage: wmbt read_bd_addr COMx

<WMBT_PATH> wmbt read_bd_addr COM23
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 09 10 00 >
Received HCI Event:
0000 < 04 0E 0A 01 09 10 00 66 55 44 33 22 11 >

Success BD_ADDR = 112233445566

Close Serial Bus

*Factory Commit BD ADDR

This command writes the BD_ADDR to the Static Section (SS) area of flash.
To utilize this command, the BD_ADDR MUST be initially set to all FFs.
To set the initial BD_ADDR to all FFs, include the BT_DEVICE_ADDRESS directive
in your make target, for example:
    demo.hello_sensor-BCM920706_P49 download BT_DEVICE_ADDRESS=FFFFFFFFFFFF

Usage: wmbt factory_commit_bd_addr COMx <bd_addr>

The example below sets the BD ADDR to 112233445566

<WMBT_PATH> wmbt factory_commit_bd_addr COM23 112233445566
MBT_BAUD_RATE:  3000000
TRANSPORT_MODE: 0 (HCI)

Opened COM23 at speed: 3000000
Sending HCI Command:
0000 < 01 10 FC 07 66 55 44 33 22 11 00 >
Received HCI Event:
0000 < 04 0E 04 01 10 FC 00 >
Success
Close Serial Bus

NOTE: The factory_commit_bd_addr command is not supported by all devices, e.g.
those with no internal or external nvram interface. When the device returns the
below status code, it means “BT_ERROR_CODE_UNKNOWN_HCI_COMMAND”.

Received HCI Event:
0000 < 04 0E 04 01 10 FC "01" >
