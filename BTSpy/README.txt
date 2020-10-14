BTSpy
=====

Overview
========
BTSpy is a trace utility that can be used in the WICED BT platforms to
view protocol and generic trace messages from the embedded device.

This application can also generate Bluetooth Snoop logs by saving the
logs from menu Tools -> File Logging Options -> Generate snoop log file
To view the snoop log, use a utility such as FrontLine Viewer.

The tool can run on Windows, Linux, or macOS systems. It listens on
the UDP port 9876 and can receive a specially formatted message
from another application on the same or different system.

To start BTSpy execute wiced_btsdk\tools\btsdk-utils\BTSpy\<OS>:
- Windows: BTSpy.exe
- macOS: bt_spy.dmg
- Linux: RunBtSpy.sh

Note that only a single instance of the application can be executed
on a machine.

To read traces over the UART interface, the Client Control application
(wiced_btsdk\tools\btsdk-host-apps-bt-ble\client_control) should open
the serial port associated with the HCI UART port of the WICED BT device,
which should be configured to use the same baud rate configured
in the embedded application.

Send application traces to BTSpy
================================
The WICED_BT_TRACE macro can be used by the embedded application to
generate a trace.  The output can be routed to Peripheral UART (PUART)
or to the WICED HCI UART.  To route traces to BTSpy, the application
should use WICED UART.

The application should configure the transport for the appropriate type,
mode, and baud rate.

(The below snippets are from RFCOMM Serial Port application).

const wiced_transport_cfg_t transport_cfg =
{
    .type = WICED_TRANSPORT_UART,
    .cfg.uart_cfg =
    {
        .mode         = WICED_TRANSPORT_UART_HCI_MODE,
        .baud_rate    = HCI_UART_DEFAULT_BAUD
    },
    .rx_buff_pool_cfg =
    {
        .buffer_size  = TRANS_UART_BUFFER_SIZE,
        .buffer_count = 1
    },
    .p_status_handler    = NULL,
    .p_data_handler      = NULL,
    .p_tx_complete_cback = NULL
};

The default UART speed is configured for 3mbps, which is typically
available on all WICED boards and on all operating systems.

#define HCI_UART_DEFAULT_BAUD   3000000

The application should initialize the transport during the application
initialization. The application can then configure the system to
route traces to WICED HCI.

APPLICATION_START( )
{
    ...
    wiced_transport_init(&transport_cfg);
    ...
    wiced_set_debug_uart(WICED_ROUTE_DEBUG_TO_WICED_UART);
    ....
}

If the application needs to send trace messages or HCI packets
(see the section below) longer than 240 bytes it should also have a separate
transmit transport pool.  If HCI traces are routed over UART, the
size of the buffers in the pool should be enough to handle any command,
event or data packet (1024 bytes).

wiced_transport_buffer_pool_t*  host_trans_pool;

APPLICATION_START()
{
    ....
    // create special pool for sending data to the MCU
    host_trans_pool = wiced_transport_create_buffer_pool(TRANS_UART_BUFFER_SIZE, TRANS_MAX_BUFFERS);
    .....
}

Send HCI traces to BTSpy
========================
The firmware which is executed on a BT WICED device consists of the
embedded BT stack (in ROM) and the BT controller. The embedded application
can register with the stack to receive copies of all
commands, events, and data packets exchanged between the stack and
the controller and route them to the BTSpy.

To register a callback which will be called whenever the HCI packet is
exchanged between the upper layer stack and the controller, the
application can call the API wiced_bt_dev_register_hci_trace.
For example:

void application_init( void )
{
    ....
    wiced_bt_dev_register_hci_trace(app_trace_callback);
    ....
}

When the callback is executed, the application can just forward
the packet to the transport. For example:

void app_trace_callback( wiced_bt_hci_trace_type_t type, uint16_t length, uint8_t* p_data )
{
    wiced_transport_send_hci_trace( host_trans_pool, type, length, p_data);
}
