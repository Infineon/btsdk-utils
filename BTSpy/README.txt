About BTSpy
===========

BTSpy is a trace utility that can be used in the WICED BT platforms to
view protocol and generic trace messages from the embedded device.  The
tool can run on Windows, Linux or OSX systems. It listens on the UDP
port 9876 and can receive specially formatted message from another
application on the same or different system.

To start BTSpy execute wiced_btsdk\tools\btsdk-utils\BTSpy\<OS>, BTSpy.exe on
Windows or bt_spy.dmg on MacOS or use RunBtSpy.sh on Linux.  Note that only a
single instance of the application can be executed on a machine.

To read traces over the UART interface the Client Control application
(wiced_btsdk\tools\btsdk-host-apps-bt-ble\client_control), should open the
serial port associated with the HCI UART port of the CYW20xxx, which should be
configured to use the same baud rate as configured in the embedded application.

How to send application traces to BTSpy
=======================================

The WICED_BT_TRACE macro can be used by the embedded application to
generate a trace.  The output can be routed to peripheral UART (PUART)
or to the WICED HCI UART.  To route traces to the BTSpy the application
should use WICED UART.

The application should configure the transport for appropriate type,
mode and baud rate.

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

The application should initialize the transport during application
initialization:
wiced_transport_init(&transport_cfg);

Application can then configure system to route traces to WICED HCI:
wiced_set_debug_uart(WICED_ROUTE_DEBUG_TO_WICED_UART);

If the application needs to send trace messages or HCI packets
(see section below) longer than 240 bytes it should also have a separate
transmit transport pool.  If HCI traces are routed over UART, the
size of the buffers in the pool should be enough to handle any command,
event or data packet (1024 bytes).

// create special pool for sending data to the MCU
host_trans_pool = wiced_transport_create_buffer_pool(TRANS_UART_BUFFER_SIZE, TRANS_MAX_BUFFERS);

How to send HCI traces to BTSpy
===============================

The firmware which is executed on a CYW207xx device consists of the
embedded BT stack (in ROM) and the BT controller. The embedded
application can register with the stack to receive copies of all
commands, events and data packets exchanged between the stack and
the controller and route them to the BTSpy.

To register a callback which will be called whenever HCI packet is
exchanged between the upper layer stack and the controller, the
application can call the API wiced_bt_dev_register_hci_trace.
For example:
wiced_bt_dev_register_hci_trace(hello_sensor_hci_trace_cback);

When the callback is executed, the application can just forward
the packet to the transport:
wiced_transport_send_hci_trace(host_trans_pool, type, length, p_data);
