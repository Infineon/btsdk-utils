WICED Manufacturing Bluetooth Test Tool

Overview

The WICED manufacturing Bluetooth test tool (WMBT) is used to test and verify the
RF performance of the CYW20706 family of SoC Bluetooth Low Energy (BLE) devices.
Each test sends a WICED HCI command to the device and then waits for the appro-
priate WICED HCI event from the device.

To run the tests:
  1. Configure the CYW20706 to run in the WICED HCI mode. The user MUST download
     an appropriate application that supports the WICED HCI Test Embedded HCI
     Command comand, please refer to the WICED HCI Protocol doc for a description
     on the Embedded HCI command.

  2. Plug the CYW20706 into the computer and note the COM port assigned to
     the HCI UART. The COM port is used to send WICED HCI commands and receive
     WICED HCI events from the device.

  3. Set the environment variable MBT_BAUD_RATE which must match the application
     baud rate. For example, in the hci_av_source_plus sample app the baud rate
     is set to 4M. Therefore, to set the environment variable in a Windows
     enviroment, run "set MBT_BAUD_RATE=4000000" on a Windows command line.

  4. Set the environment variable TRANSPORT_MODE (0:HCI, 1:WICED_HCI). wmbt can
     support sending/receiving HCI or WICED HCI commands/events. For the examples
     provided below, we will use the WICED HCI mode.

Note: All examples in this document use COM4 as the port assigned to the
CYW20706 by the PC. Replace “COM4” in each test example with the actual port
assigned to the CYW20706 under test).

Reset

This command verifies that the device is correctly configured and connected to
the PC.

Usage: wmbt reset COMx

The example below sends HCI_Reset command to the device at a default bard rate
of 115200 bps and processes the HCI Command Complete event (BLUETOOTH SPECIFICA-
TION Version 4.1 [Vol 2], Section 7.3.2 for details).

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt reset COM4
Sending HCI Command:
0000 < 01 03 0C 00 >
Received HCI Event:
0000 < 04 0E 04 01 03 0C 00 >

The last byte of the HCI Command Complete event is the operation status, where
0 signifies success.

WICED Reset

This command sends the WICED HCI command HCI_CONTROL_COMMAND_RESET to the CYW20706.

Usage: wmbt wiced_reset COMx

The example below sends HCI_CONTROL_COMMAND_RESET command to the device.
No Command Status is processed since this command will force a watchdog reset in
the CYW20706.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt wiced_reset COM4
Sending WICED HCI Command:
0000 < 19 01 00 00 00 >

LE Receiver Test

This test configures the CYW20706 to receive reference packets at a fixed
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

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt le_receiver_test COM4 2406
Sending WICED HCI Command:
0000 < 19 10 08 05 00 01 1D 20 01 02 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 1D 20 00 >
Success
LE Receiver Test running, to stop execute wmbt le_test_end COMx

The last byte of the WICED HCI event is the operation status,
where 0 signifies success.


LE Transmitter Test

The LE Transmitter Test configures the CYW20706 to send test packets at a
fixed interval. External test equipment may be used to receive and analyze
the reference packets.

The frequency on which the CYW20706 transmits the packets  is passed as a
parameter. BLE devices use 40 channels, each of which is 2 MHz wide. Channel 0
maps to 2402 MHz and Channel 39 maps to 2480 MHz.

The other two parameters specify the length of the test data and the data
pattern to be used (see BLUETOOTH SPECIFICATION Version 4.1 [Vol 2], Section
7.8.29 for details).

Usage: wmbt le_transmitter_test COMx <tx_frequency> <data_length> <packet_payload>
where:
    tx_frequency = transmit frequency in MHz ( 2402 to 2480 ).

    data_length = 0–37

    data_pattern = 0–7
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

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt le_transmitter_test COM4 2406 10 4
Sending WICED HCI Command:
0000 < 19 10 08 07 00 01 1E 20 03 02 0A 04 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 1E 20 00 >

LE Transmitter Test running, to stop execute wmbt le_test_end COMx

The last byte of the WICED HCI event is the status of the operation,
where 0 signifies the success.


LE Test End

This command stops the LE Transmitter or LE Receiver Test that is in progress
on the CYW20706.

Usage: wmbt le_test_end COMx

The example below stops the active test.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt le_test_end COM4
Sending WICED HCI Command:
0000 < 19 10 08 04 00 01 1F 20 00 >
Received WICED HCI Event:
0000 < 19 01 08 08 00 0E 06 01 1F 20 00 33 39 >
Success num_packets_received = 14643


Continuous Transmit Test

Note: Unlike the LE tests, this test uses 79 frequencies, each 1 MHz wide.

This test configures the CYW20706 to turn the carrier ON or OFF. When the
carrier is ON the device transmits an unmodulated pattern on the specified
frequency at a specified power level.

The frequency to be used by the CYW20706 is passed as a parameter.

Usage: wmbt tx_frequency_arm COMx <carrier on/off> <tx_frequency> <mode>
<modulation_type> <tx_power>
where:

    carrier on/off:
        1 = carrier ON
        0 = carrier OFF
    tx_frequency = (2402 – 2480) transmit frequency, in MHz
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
        tx_power = (–25 to +3) transmit power, in dBm

The example below turns the carrier ON and instructs the CYW20706 to transmit an
unmodulated pattern on 2402 MHz at 3 dBm.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt tx_frequency_arm COM4 1 2402 0 0 3
Sending WICED HCI Command:
0000 < 19 10 08 0B 00 01 14 FC 07 00 00 00 00 08 03 00 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 14 FC 00 >

To stop the test, send the command a second time to the same COM port with the
carrier on/off parameter set to zero (0). No other parameters are used.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt tx_frequency_arm COM4 0 0 0 0 0
Sending HCI Command:
0000 < 19 10 08 0B 00 01 14 FC 07 01 02 00 00 00 00 00 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 14 FC 00 >


Radio Tx Test

Note: Connectionless transmit test to send Bluetooth packets

This test configures the CYW20706 to transmit the selected data pattern which is on the specified frequency
and specified logical channel at a specified power level.

The frequency, modulation_type, logical channel, bb_packet_type, packet_length, and power level to be used by
the CYW20706 are passed as parameters.

Usage: wmbt radio_tx_test COMx <bdaddr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <tx_power>
where:
    bd_addr: BD_ADDR of Tx device (6 bytes)
    frequency: 0 or transmit frequency (2402 – 2480) in MHz
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
    packet_length: 0 – 65535. Device will limit the length to the max for the baseband packet type.
        eg) if DM1 packets are sent, the maximum packet size is 17 bytes.
    tx_power = (–25 to +3) transmit power, in dBm.

The example below instructs the CYW20706 to transmit 0xAA 8-bit Pattern on the 2402 MHz and ACL Basic
with DM1 packet (17 bytes) type at -3 dBm.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt radio_tx_test COM4 20703A012345 2402 2 1 3 17 -3
Sending WICED HCI Command:
0000 < 19 10 08 14 00 01 51 FC 10 45 23 01 3A 70 20 01 >
0010 < 00 02 01 03 11 00 08 FD 00 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 51 FC 00 >
Success

The last byte of the WICED HCI event is the operation status,
where 0 signifies that operation was successful and test started to run.
The test continues to run until device is reset (wmbt wiced_reset).

Radio Rx Test

Note: Connectionless receive test for Bluetooth packets

This test issues a command to the CYW20706 to set radio to camp on a specified frequency.
While test is running the CYW20706 periodically sends reports about received packets.

Usage: wmbt radio_rx_test COMx <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length>
where:
    bd_addr: BD_ADDR for the remote Tx device (6 bytes)
    frequency = receive frequency (2402 – 2480) in MHz
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
    packet_length: 0 – 65535.
        Device will compare length of the received packets with the specified packet_length.

CYW20706 will generate the statistics report of the Rx Test every second.

The example below instructs the CYW20706 to receive 0xAA 8-bit Pattern on the 2402 MHz and ACL Basic with DM1 packet type.

WICED-SmartReady-SDK\Tools\wmbt\Release>wmbt radio_rx_test COM4 20703A012345 2402 2 1 3 17
Sending WICED HCI Command:
0000 < 19 10 08 12 00 01 52 FC 0E 45 23 01 3A 70 20 E8 >
0010 < 03 00 02 01 03 11 00 >
Received WICED HCI Event:
0000 < 19 01 08 06 00 0E 04 01 52 FC 00 >
Success

Radio RX Test is running. Press any key to stop the test.

WMBT reports connectionless Rx Test statistics every second.

The example below shows the Rx Test Statistics report -
Statistics Report received:
0000 < 19 01 08 23 00 FF 21 07 00 00 00 00 00 00 00 00 >
0010 < 1F 03 00 00 1F 03 00 00 00 00 00 00 78 A8 01 00 >
0020 < 78 A8 01 00 00 00 00 00 >
  [Rx Test statistics]
    Sync_Timeout_Count:     0x0
    HEC_Error_Count:        0x0
    Total_Received_Packets: 0x31f
    Good_Packets:           0x31f
    CRC_Error_Packets:      0x0
    Total_Received_Bits:    0x1a878
    Good_Bits:              0x1a878
    Error_Bits:             0x0

Upong exiting the Radio RX Test, wmbt will issue a WICED HCI Reset command forcing a watchdog reset which is
necessary to bring the CYW20706 out of the test mode. To execute another test, you must download the application
image again.
