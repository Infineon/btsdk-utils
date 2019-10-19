// wmbt.cpp : Defines the entry point for the console application.
//
#ifdef _WIN32
#include <WinSock2.h>
#include <conio.h>
#include "tchar.h"
#include "wmbt_com.h"
#include "..\..\common\include\hci_control_api.h"
#else
#include <stdio.h>
#include "../../common/include/hci_control_api.h"

#include "common/wmbt_uart.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define sprintf_s(a,b,c,d)	sprintf(a,c,d)
#define sscanf_s	sscanf
#define fopen_s(a,b,c)		fopen(b,c)
#endif
#define HCI       0
#define WICED_HCI 1

#ifdef _WIN32
SOCKET log_sock = INVALID_SOCKET;
int port_num = 0;
inline void OpenPortError(int port_num) {printf("Open COM%d port Failed\n", port_num);}
#define TRANSPORT "COMx"
#else
char port_num[256];
inline void OpenPortError(const char*  port_num) {printf("Open %s port Failed\n", port_num);}
inline int _stricmp(const char* s1, const char *s2) {return strcmp(s1, s2);}
#define TRANSPORT "/dev/ttyUSBx"
#endif
typedef unsigned char UINT8;

UINT8 in_buffer[1024];
int baud_rate = 3000000;
int transport_mode = HCI; // 0:HCI, 1:WICED_HCI

//
// print hexadecimal digits of an array of bytes formatted as:
// 0000 < 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F >
// 0010 < 10 11 12 13 14 15 16 1718 19 1A 1B 1C 1D 1E 1F >
//
void HexDump(LPBYTE p, DWORD dwLen)
{
    for (DWORD i = 0; i < dwLen; ++i)
    {
        if (i % 16 == 0)
            printf("%04X <", i);
        printf(" %02X", p[i]);
        if ((i + 1) % 16 == 0)
            printf(" >\n");
    }
    printf(" >\n");
}

// mapping between wiced trace types and spy trace types (evt, cmd, rx data, tx data)
static int wiced_trace_to_spy_trace[] = { 0, 4, 3, 6, 7 };

#ifdef _WIN32
void TraceHciPkt(BYTE type, BYTE *buffer, USHORT length)
{
    SOCKADDR_IN socket_addr;
    static int initialized = FALSE;
    BYTE buf[1100];
    USHORT offset = 0;
    USHORT *p = (USHORT*)buf;

    if (!initialized)
    {
        initialized = TRUE;

        WSADATA wsaData;
        int err = WSAStartup(MAKEWORD(2, 0), &wsaData);
        if (err != 0)
            return;
        log_sock = socket(AF_INET, SOCK_DGRAM, 0);

        if (log_sock == INVALID_SOCKET)
            return;

        memset(&socket_addr, 0, sizeof(socket_addr));
        socket_addr.sin_family = AF_INET;
        socket_addr.sin_addr.s_addr = ADDR_ANY;
        socket_addr.sin_port = 0;

        err = bind(log_sock, (SOCKADDR *)&socket_addr, sizeof(socket_addr));
        if (err != 0)
        {
            closesocket(log_sock);
            log_sock = INVALID_SOCKET;
            return;
        }
    }
    if (log_sock == INVALID_SOCKET)
        return;

    if (length > 1024)
        length = 1024;

    *p++ = wiced_trace_to_spy_trace[type];
    *p++ = length;
    *p++ = 0;
    *p++ = 1;
    memcpy(p, buffer, length);

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = ntohl(0x7f000001);
    socket_addr.sin_port = 9876;

    length = sendto(log_sock, (const char *)buf, length + 8, 0, (SOCKADDR *)&socket_addr, sizeof(SOCKADDR_IN));
}
#else
extern void TraceHciPkt(BYTE type, BYTE *buffer, USHORT length);
#endif

DWORD SendWicedCommand(ComHelper *p_port, UINT16 command, LPBYTE payload, DWORD len)
{
    BYTE    data[1040];
    char    descr[30];
    int     header = 0;

    data[header++] = HCI_WICED_PKT;
    data[header++] = command & 0xff;
    data[header++] = (command >> 8) & 0xff;
    data[header++] = len & 0xff;
    data[header++] = (len >> 8) & 0xff;

    if (len)
        memcpy(&data[header], payload, len);

    sprintf_s(descr, sizeof(descr), "Xmit %3u bytes: ", len + header);
    printf("Sending WICED HCI Command:\n");
    HexDump(data, len + header);

    p_port->Write(data, len + header);

    if (command == HCI_CONTROL_COMMAND_RESET)
        return 1;

    // read HCI response payload
    // read HCI response header
    DWORD received_event = 0;
    while (received_event != HCI_CONTROL_TEST_EVENT_ENCAPSULATED_HCI_EVENT)
    {
        DWORD dwRead = p_port->Read((LPBYTE)&in_buffer[0], 5);

        if (dwRead == 5 && in_buffer[3] > 0)
            dwRead += p_port->Read((LPBYTE)&in_buffer[5], in_buffer[3]);

        received_event = in_buffer[1] + (in_buffer[2] << 8);

        DWORD len = dwRead - 5;
        LPBYTE p_data = &in_buffer[5];
        if (received_event == HCI_CONTROL_EVENT_WICED_TRACE)
        {
            if (len >= 2)
            {
                if ((len > 2) && (p_data[len - 2] == '\n'))
                {
                    p_data[len - 2] = 0;
                    len--;
                }
                TraceHciPkt(0, p_data, (USHORT)len);
            }
        }
        else if (received_event == HCI_CONTROL_EVENT_HCI_TRACE)
        {
            TraceHciPkt(p_data[0] + 1, &p_data[1], (USHORT)(len - 1));
        }
        else
        {
            printf("Received WICED HCI Event:\n");
            HexDump(in_buffer, dwRead);

            if ((in_buffer[8] == 0x1F) && (in_buffer[9] == 0x20)) //LE Test End Opcode - command complete event contains num_packets_received
            {
                printf("\nSuccess num_packets_received = %d\n\n", in_buffer[11] + (in_buffer[12] << 8));
            }
        }
    }

    return in_buffer[10] == 0;
}

static void print_usage_set_environment_variables(bool full)
{
    printf("set environment variables: MBT_BAUD_RATE and TRANSPORT_MODE (0: HCI, 1: WICED_HCI)\n\n");
    printf("NOTE: MBT_BAUD_RATE Must match baud rate configured in embedded application\n\n");
    if (!full)
        return;
    printf("eg) on Windows,\n");
    printf("    set MBT_BAUD_RATE=4000000\n");
    printf("    set TRANSPORT_MODE=1\n");
    printf("eg) on Cygwin,\n");
    printf("    export MBT_BAUD_RATE=4000000\n");
    printf("    export TRANSPORT_MODE=1\n");
}

static int get_environment_variables()
{
    char* pbaud_rate = NULL;
    char* ptransport_mode = NULL;

    pbaud_rate = getenv("MBT_BAUD_RATE");
    if (pbaud_rate == NULL)
    {
        print_usage_set_environment_variables(true);
        return 0;
    }
    else
    {
#ifdef _WIN32
        baud_rate = stoi(pbaud_rate);
#else
        baud_rate = atoi(pbaud_rate);
#endif
        printf("MBT_BAUD_RATE:  %d\n", baud_rate);
    }

    ptransport_mode = getenv("TRANSPORT_MODE");
    if (ptransport_mode == NULL)
    {
        print_usage_set_environment_variables(true);
        return 0;
    }
    else
    {
#ifdef _WIN32
        transport_mode = stoi(ptransport_mode);
#else
        transport_mode = atoi(ptransport_mode);
#endif
        printf("TRANSPORT_MODE: %s (%s)\n\n", ptransport_mode, (transport_mode)?"WICED_HCI":"HCI");
    }

    return 1;
}

static void print_usage_reset(bool full)
{
    printf("Usage: wmbt reset %s\n", TRANSPORT);
    printf("       NOTE: Sends HCI_RESET at 115200\n");
}

static void print_usage_reset_highspeed(bool full)
{
    printf("Usage: wmbt reset_highspeed %s\n", TRANSPORT);
    printf("       NOTE: Sends HCI_RESET at the configured MBT_BAUD_RATE\n");
}

static void print_usage_wiced_reset(bool full)
{
    printf("Usage: wmbt wiced_reset %s\n", TRANSPORT);
    printf("       NOTE: Sends HCI_CONTROL_TEST_COMMAND_RX_TEST at the configured MBT_BAUD_RATE\n");
}

static BOOL send_hci_command(ComHelper *p_port, LPBYTE cmd, DWORD cmd_len, LPBYTE expected_evt, DWORD evt_len, BOOL ignore_transport_mode)
{
    // If we are using WICED_HCI, use SendWicedCommand instead
    if ((transport_mode == WICED_HCI) && (ignore_transport_mode != TRUE))
    {
        SendWicedCommand(p_port, HCI_CONTROL_TEST_COMMAND_ENCAPSULATED_HCI_COMMAND, cmd, cmd_len);
        return TRUE;
    }

    // write HCI Command
    printf("Sending HCI Command:\n");
    HexDump(cmd, cmd_len);

    p_port->Write(cmd, cmd_len);

    // read HCI response header
    DWORD dwRead = p_port->Read((LPBYTE)&in_buffer[0], 3);

    // read HCI response payload
    if (dwRead == 3 && in_buffer[2] > 0)
        dwRead += p_port->Read((LPBYTE)&in_buffer[3], in_buffer[2]);

    printf("Received HCI Event:\n");
    HexDump(in_buffer, dwRead);

    if (cmd[1] == 0x1f && cmd[2] == 0x20) //LE Test End Opcode - command complete event contains num_packets_received
    {
        if ((dwRead > sizeof(expected_evt))
            && (memcmp(in_buffer, expected_evt, sizeof(expected_evt)) == 0))
        {
            printf("\nSuccess num_packets_received = %d\n\n", in_buffer[7] + (in_buffer[8] << 8));
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    if (dwRead == evt_len )
    {
        if (memcmp(in_buffer, expected_evt, evt_len) == 0)
        {
            printf("Success\n");
            return TRUE;
        }
    }
    return FALSE;
}

static int execute_reset(ComHelper *p_port)
{
    BOOL res, opened = FALSE;

    if (p_port == NULL)
    {
        p_port = new ComHelper;
        if (!p_port->OpenPort(port_num, 115200))
        {
            OpenPortError( port_num);
            delete p_port;
            return 0;
        }
        opened = TRUE;
    }
    UINT8 hci_reset[] = {0x01, 0x03, 0x0c, 0x00};
    UINT8 hci_reset_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};

    res = send_hci_command(p_port, hci_reset, sizeof(hci_reset), hci_reset_cmd_complete_event, sizeof(hci_reset_cmd_complete_event), TRUE);
    if (opened)
        delete p_port;
    return res;
}

static int execute_reset_highspeed(ComHelper *p_port)
{
    BOOL res, opened = FALSE;

    if (p_port == NULL)
    {
        p_port = new ComHelper;
        if (!p_port->OpenPort(port_num, baud_rate))
        {
            OpenPortError( port_num);
            delete p_port;
            return 0;
        }
        opened = TRUE;
    }
    UINT8 hci_reset[] = { 0x01, 0x03, 0x0c, 0x00 };
    UINT8 hci_reset_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00 };

    res = send_hci_command(p_port, hci_reset, sizeof(hci_reset), hci_reset_cmd_complete_event, sizeof(hci_reset_cmd_complete_event), TRUE);
    if (opened)
        delete p_port;
    return res;
}

static int execute_wiced_reset(void)
{
    ComHelper SerialPort;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    SendWicedCommand(&SerialPort, HCI_CONTROL_COMMAND_RESET, NULL, 0);

    return 1;
}

static void print_usage_download(bool full)
{
    printf("Usage: wmbt download %s <hcd_pathname>\n", TRANSPORT);
}

BOOL SendDownloadMinidriver(ComHelper *p_port)
{
    BYTE arHciCommandTx[] = { 0x01, 0x2E, 0xFC, 0x00 };
    BYTE arBytesExpectedRx[] = { 0x04, 0x0E, 0x04, 0x01, 0x2E, 0xFC, 0x00 };

    return (send_hci_command(p_port, arHciCommandTx, sizeof(arHciCommandTx), arBytesExpectedRx, sizeof(arBytesExpectedRx), TRUE));
}

BOOL SetBaudRate(ComHelper *p_port, int nBaudRate, bool update)
{
    p_port->ClosePort();

    p_port->OpenPort(port_num, nBaudRate);

    return (TRUE);
}

BOOL SendHcdRecord(ComHelper *p_port, ULONG nAddr, ULONG nHCDRecSize, BYTE * arHCDDataBuffer)
{
    BYTE arHciCommandTx[261] = { 0x01, 0x4C, 0xFC, 0x00 };
    BYTE arBytesExpectedRx[] = { 0x04, 0x0E, 0x04, 0x01, 0x4C, 0xFC, 0x00 };

    arHciCommandTx[3] = (BYTE)(4 + nHCDRecSize);
    arHciCommandTx[4] = (nAddr & 0xff);
    arHciCommandTx[5] = (nAddr >> 8) & 0xff;
    arHciCommandTx[6] = (nAddr >> 16) & 0xff;
    arHciCommandTx[7] = (nAddr >> 24) & 0xff;
    memcpy(&arHciCommandTx[8], arHCDDataBuffer, nHCDRecSize);

    printf("sending record at:0x%x\n", nAddr);
    return (send_hci_command(p_port, arHciCommandTx, 4 + 4 + nHCDRecSize, arBytesExpectedRx, sizeof(arBytesExpectedRx), TRUE));
}

BOOL ReadNextHCDRecord(FILE * fHCD, ULONG * nAddr, ULONG * nHCDRecSize, UINT8 * arHCDDataBuffer, BOOL * bIsLaunch)
{
    const   int HCD_LAUNCH_COMMAND = 0x4E;
    const   int HCD_WRITE_COMMAND = 0x4C;
    const   int HCD_COMMAND_BYTE2 = 0xFC;

    BYTE     arRecHeader[3];
    BYTE     byRecLen;
    BYTE     arAddress[4];

    *bIsLaunch = FALSE;

    if (fread(arRecHeader, 1, 3, fHCD) != 3)               // Unexpected EOF
        return false;

    byRecLen = arRecHeader[2];

    if ((byRecLen < 4) || (arRecHeader[1] != HCD_COMMAND_BYTE2) ||
        ((arRecHeader[0] != HCD_WRITE_COMMAND) && (arRecHeader[0] != HCD_LAUNCH_COMMAND)))
    {
        printf("Wrong HCD file format trying to read the command information\n");
        return FALSE;
    }

    if (fread(arAddress, sizeof(arAddress), 1, fHCD) != 1)      // Unexpected EOF
    {
        printf("Wrong HCD file format trying to read 32-bit address\n");
        return FALSE;
    }

    *nAddr = arAddress[0] + (arAddress[1] << 8) + (arAddress[2] << 16) + (arAddress[3] << 24);

    *bIsLaunch = (arRecHeader[0] == HCD_LAUNCH_COMMAND);

    *nHCDRecSize = byRecLen - 4;

    if (*nHCDRecSize > 0)
    {
        if (fread(arHCDDataBuffer, 1, *nHCDRecSize, fHCD) != *nHCDRecSize)   // Unexpected EOF
        {
            printf("Not enough HCD data bytes in record\n");
            return FALSE;
        }
    }

    return TRUE;
}

BOOL SendLaunchRam(ComHelper *p_port)
{
    BYTE arHciCommandTx[] = { 0x01, 0x4E, 0xFC, 0x04, 0xFF, 0xFF, 0xFF, 0xFF };
    BYTE arBytesExpectedRx[] = { 0x04, 0x0E, 0x04, 0x01, 0x4E, 0xFC, 0x00 };

    return (send_hci_command(p_port, arHciCommandTx, sizeof(arHciCommandTx), arBytesExpectedRx, sizeof(arBytesExpectedRx), TRUE));
}

BOOL SendUpdateBaudRate(ComHelper *p_port, int newBaudRate)
{
    BYTE arHciCommandTx[] = { 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA };
    BYTE arBytesExpectedRx[] = { 0x04, 0x0E, 0x04, 0x01, 0x18, 0xFC, 0x00 };

    arHciCommandTx[6] = newBaudRate & 0xff;
    arHciCommandTx[7] = (newBaudRate >> 8) & 0xff;
    arHciCommandTx[8] = (newBaudRate >> 16) & 0xff;
    arHciCommandTx[9] = (newBaudRate >> 24) & 0xff;

    return (send_hci_command(p_port, arHciCommandTx, sizeof(arHciCommandTx), arBytesExpectedRx, sizeof(arBytesExpectedRx), TRUE));
}

static int execute_download(const char *pathname)
{
    ComHelper SerialPort;

    FILE *          fHCD = NULL;
    LONG            nVeryFirstAddress = 0;

    fopen_s(&fHCD, pathname, "rb");
    if (!fHCD)
    {
        printf("Failed to open HCD file %s", pathname);
        return 0;
    }

    if (!SerialPort.OpenPort(port_num, 115200))
    {
        OpenPortError( port_num);
        return 0;
    }
    if (!execute_reset(&SerialPort))
    {
        SerialPort.ClosePort();

        printf("Failed to HCI Reset\n");
        fclose(fHCD);
        return 0;
    }
    printf("HCI Reset success\n");
    if (baud_rate != 115200)
    {
        if (!SendUpdateBaudRate(&SerialPort, baud_rate))
        {
            printf("Failed to send update baud rate\n");
            fclose(fHCD);
            return 0;
        }
        SerialPort.ClosePort();
        SerialPort.OpenPort(port_num, baud_rate);
        printf("Set Baud Rate success\n");
    }
    if (!SendDownloadMinidriver(&SerialPort))
    {
        printf("Failed to send download minidriver\n");
        fclose(fHCD);
        return 0;
    }
    printf("Download minidriver success, downloading configuration...\n");
    ULONG   nAddr, nHCDRecSize;
    BYTE    arHCDDataBuffer[256];
    BOOL    bIsLaunch = FALSE;
    while (ReadNextHCDRecord(fHCD, &nAddr, &nHCDRecSize, arHCDDataBuffer, &bIsLaunch))
    {
        if (!SendHcdRecord(&SerialPort, nAddr, nHCDRecSize, arHCDDataBuffer))
        {
            printf("Failed to send hcd portion at %x\n", nAddr);
            if (fHCD)
                fclose(fHCD);
            return 0;
        }
        if (bIsLaunch)
            break;
    }
    printf("Download configuration success\n");

    if (!SendLaunchRam(&SerialPort))
    {
        printf("Failed to send launch RAM\n");
    }
    printf("Launch RAM success\n");
    fclose(fHCD);


    DWORD received_event = 0;
    while (received_event != HCI_CONTROL_EVENT_DEVICE_STARTED)
    {
        DWORD dwRead = SerialPort.Read((LPBYTE)&in_buffer[0], 5);

        if (dwRead == 5 && in_buffer[3] > 0)
            dwRead += SerialPort.Read((LPBYTE)&in_buffer[5], in_buffer[3]);

        received_event = in_buffer[1] + (in_buffer[2] << 8);

        DWORD len = dwRead - 5;
        LPBYTE p_data = &in_buffer[5];
        if (received_event == HCI_CONTROL_EVENT_WICED_TRACE)
        {
            if (len >= 2)
            {
                if ((len > 2) && (p_data[len - 2] == '\n'))
                {
                    p_data[len - 2] = 0;
                    len--;
                }
                TraceHciPkt(0, p_data, (USHORT)len);
            }
        }
        else if (received_event == HCI_CONTROL_EVENT_HCI_TRACE)
        {
            TraceHciPkt(p_data[0] + 1, &p_data[1], (USHORT)(len - 1));
        }
        else
        {
            printf("Received WICED HCI Event:\n");
            HexDump(in_buffer, dwRead);
        }
    }
    printf("Device Started successfully!\n");

    if (transport_mode == HCI)
    {
        /* Send HCI Reset after downloading .hcd file (FW patch and Application) */
        printf("Issuing HCI Reset after downloading Application\n");
        if (!execute_reset_highspeed(&SerialPort))
        {
            printf("Failed to send HCI Reset...attempting to resend\n");
            SerialPort.ClosePort();

            if (!execute_reset_highspeed(NULL))
            {
                printf("Failed to send HCI Reset after LaunchRam, please try download again\n");
                return 0;
            }
        }
    }

    return 1;
}


static void print_usage_le_receiver_test(bool full)
{
    printf ("Usage: wmbt le_receiver_test %s <rx_frequency>\n", TRANSPORT);
    if (!full)
        return;
    printf ("                rx_frequency: (2402 - 2480) receive frequency, in MHz\n");
}

static int execute_le_receiver_test(UINT16 rx_frequency)
{
    ComHelper SerialPort;
    BOOL res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    UINT8 rx_channel = (rx_frequency - 2402) / 2;;
    UINT8 hci_le_receiver_test[] = { 0x01, 0x01D, 0x20, 0x01, 0x00 };
    UINT8 hci_le_receiver_test_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x01D, 0x20, 0x00 };

    hci_le_receiver_test[4] = rx_channel;

    res = send_hci_command(&SerialPort, hci_le_receiver_test, sizeof(hci_le_receiver_test), hci_le_receiver_test_cmd_complete_event, sizeof(hci_le_receiver_test_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_le_receiver_test\n");
    return res;
}

static void print_usage_le_transmitter_test(bool full)
{
    printf ("Usage: wmbt le_transmitter_test %s <tx_frequency> <data_length> <data_pattern>\n", TRANSPORT);
    if (!full)
        return;
    printf ("                tx_frequency: (2402 - 2480) transmit frequency, in MHz\n");
    printf ("                data_length: (0 - 37)\n");
    printf ("                data_pattern: (0 - 7)\n");
    printf ("                    0 - Pseudo-Random bit sequence 9\n");
    printf ("                    1 Pattern of alternating bits '11110000'\n");
    printf ("                    2 Pattern of alternating bits '10101010'\n");
    printf ("                    3 Pseudo-Random bit sequence 15\n");
    printf ("                    4 Pattern of All '1' bits\n");
    printf ("                    5 Pattern of All '0' bits\n");
    printf ("                    6 Pattern of alternating bits '00001111'\n");
    printf ("                    7 Pattern of alternating bits '0101'\n");
}

static int execute_le_transmitter_test(UINT16 tx_frequency, UINT8 length, UINT8 pattern)
{
    ComHelper SerialPort;
    BOOL res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    UINT8 tx_channel = (tx_frequency - 2402) / 2;
    UINT8 hci_le_transmitter_test[] = { 0x01, 0x01E, 0x20, 0x03, 0x00, 0x00, 0x00 };
    UINT8 hci_le_transmitter_test_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x01E, 0x20, 0x00 };

    hci_le_transmitter_test[4] = tx_channel;
    hci_le_transmitter_test[5] = length;
    hci_le_transmitter_test[6] = pattern;

    res = send_hci_command(&SerialPort, hci_le_transmitter_test, sizeof(hci_le_transmitter_test), hci_le_transmitter_test_cmd_complete_event, sizeof(hci_le_transmitter_test_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_le_transmitter_test\n");
    return res;
}

static void print_usage_le_test_end(bool full)
{
    printf ("Usage: wmbt le_test_end %s\n", TRANSPORT);
}

static int execute_le_test_end()
{
    ComHelper SerialPort;
    BOOL res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    UINT8 hci_le_test_end[] = { 0x01, 0x1f, 0x20, 0x00 };
    UINT8 hci_le_test_end_cmd_complete_event[] = { 0x04, 0x0e, 0x06, 0x01, 0x1f, 0x20, 0x00 };

    res = send_hci_command(&SerialPort, hci_le_test_end, sizeof(hci_le_test_end), hci_le_test_end_cmd_complete_event, sizeof(hci_le_test_end_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_le_transmitter_test\n");
    return res;
}

static void print_usage_tx_frequency_arm(bool full)
{
    printf ("Usage: wmbt tx_frequency_arm %s <carrier on/off> <tx_frequency>\n", TRANSPORT);
    printf ("                                 <mode> <modulation_type> <tx_power>\n");
    if (!full)
        return;
    printf ("                carrier on/off: 1 - carrier on, 0 - carrier_off\n");
    printf ("                tx_frequency: (2402 - 2480) transmit frequency, in MHz\n");
    printf ("                mode: (0 - 9)\n");
    printf ("                       0 = Unmodulated\n");
    printf ("                       1 = PRBS9\n");
    printf ("                       2 = PRBS15\n");
    printf ("                       3 = All Zeros\n");
    printf ("                       4 = All Ones\n");
    printf ("                       5 = Incrementing Symbols\n");
    printf ("                modulation_type: (0 - 3)\n");
    printf ("                       0 = GFSK\n");
    printf ("                       1 = QPSK\n");
    printf ("                       2 = 8PSK\n");
    printf ("                       3 = LE\n");
    printf ("                tx_power = (-25 - 13) transmit power in dbm\n");
}

static void print_usage_receive_only_test(bool full)
{
    printf ("Usage: wmbt receive_only %s <rx_frequency>\n", TRANSPORT);
    if (!full)
        return;
    printf ("                rx_frequency = (2402 - 2480) receiver frequency, in MHz\n");
}

static int execute_tx_frequency_arm(UINT8 carrier_on, UINT16 tx_frequency, UINT8 mode, UINT8 modulation_type, int tx_power)
{
    ComHelper SerialPort;
    BOOL res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    UINT8 hci_set_tx_frequency_arm[] = { 0x01, 0x014, 0xfc, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    UINT8 hci_set_tx_frequency_arm_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x014, 0xfc, 0x00 };

    hci_set_tx_frequency_arm[4] = (carrier_on == 0) ? 1 : 0;
    hci_set_tx_frequency_arm[5] = (carrier_on == 1) ? (tx_frequency - 2402) : 2;
    hci_set_tx_frequency_arm[6] = mode;
    hci_set_tx_frequency_arm[7] = modulation_type;
    hci_set_tx_frequency_arm[8] = (carrier_on == 1) ? 8 : 0;
    hci_set_tx_frequency_arm[9] = tx_power;

    res = send_hci_command(&SerialPort, hci_set_tx_frequency_arm, sizeof(hci_set_tx_frequency_arm), hci_set_tx_frequency_arm_cmd_complete_event, sizeof(hci_set_tx_frequency_arm_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_set_tx_frequency_arm\n");
    return res;
}

static int execute_receive_only(UINT16 rx_frequency)
{
    ComHelper SerialPort;
    BOOL res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    UINT8 chan_num = rx_frequency - 2402;
    UINT8 hci_write_receive_only[] = { 0x01, 0x02b, 0xfc, 0x01, 0x00 };
    UINT8 hci_write_receive_only_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x02b, 0xfc, 0x00 };

    hci_write_receive_only[4] = chan_num;

    res = send_hci_command(&SerialPort, hci_write_receive_only, sizeof(hci_write_receive_only), hci_write_receive_only_cmd_complete_event, sizeof(hci_write_receive_only_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_receive_only\n");
    return res;
}

static void print_usage_radio_tx_test(bool full)
{
    printf("Usage: wmbt radio_tx_test %s <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length> <tx_power>\n", TRANSPORT);
    if (!full)
        return;
    printf("                bd_addr: BD_ADDR of Tx device (6 bytes)\n");
    printf("                frequency: 0 for hopping or (2402 - 2480) transmit frequency in MHz\n");
    printf("                    0: normal Bluetooth hopping sequence (79 channels)\n");
    printf("                    2402 - 2480: single frequency without hopping\n");
    printf("                modulation_type: sets the data pattern\n");
    printf("                    0: 0x00 8-bit Pattern\n");
    printf("                    1: 0xFF 8-bit Pattern\n");
    printf("                    2: 0xAA 8-bit Pattern\n");
    printf("                    3: 0xF0 8-bit Pattern\n");
    printf("                    4: PRBS9 Pattern\n");
    printf("                logical_channel: sets the logical channel to Basic Rate (BR) or Enhanced Data Rate (EDR) for ACL packets\n");
    printf("                    0: EDR\n");
    printf("                    1: BR\n");
    printf("                bb_packet_type: baseband packet type to use\n");
    printf("                    3: DM1\n");
    printf("                    4: DH1 / 2-DH1\n");
    printf("                    8: 3-DH1\n");
    printf("                    10: DM3 / 2-DH3\n");
    printf("                    11: DH3 / 3-DH3\n");
    printf("                    12: EV4 / 2-EV5\n");
    printf("                    13: EV5 / 3-EV5\n");
    printf("                    14: DM5 / 2-DH5\n");
    printf("                    15: DH5 / 3-DH5\n");
    printf("                packet_length: 0 - 65535. Device will limit the length to the max for the baseband packet type\n");
    printf("                tx_power = (-25 - +3) transmit power in dbm\n");
}

static int execute_radio_tx_test(const char *bdaddr, int frequency, int modulation_type, int logical_channel, int bb_packet_type, int packet_length, int tx_power)
{
    ComHelper SerialPort;
    UINT32     params[6];
    BOOL      res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    sscanf_s(bdaddr, "%02x%02x%02x%02x%02x%02x", &params[0], &params[1], &params[2], &params[3], &params[4], &params[5]);

    UINT8 chan_num = frequency - 2402;
    UINT8 hci_radio_tx_test[20] = { 0x01, 0x051, 0xfc, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    UINT8 hci_radio_tx_test_cmd_complete_event[] = { 0x04, 0x0e, 0x04, 0x01, 0x051, 0xfc, 0x00 };
    for (int i = 0; i < 6; i++)
    {
        hci_radio_tx_test[i + 4] = params[5 - i];    //bd address
    }
    hci_radio_tx_test[10] = (frequency == 0) ? 0 : 1;        //0: hopping, 1: single frequency
    hci_radio_tx_test[11] = (frequency == 0) ? 0 : (frequency - 2402);  //0: hopping 0-79:channel number (0: 2402 MHz)
    hci_radio_tx_test[12] = modulation_type;               //data pattern (3: 0xAA  8-bit Pattern)
    hci_radio_tx_test[13] = logical_channel;               //logical_Channel (0:ACL EDR, 1:ACL Basic)
    hci_radio_tx_test[14] = bb_packet_type;                //modulation type (BB_Packet_Type. 3:DM1, 4: DH1 / 2-DH1)
    hci_radio_tx_test[15] = packet_length & 0xff;          //low byte of packet_length
    hci_radio_tx_test[16] = (packet_length >> 8) & 0xff;     //high byte of packet_length
    hci_radio_tx_test[17] = 8;                             //power in dBm
    hci_radio_tx_test[18] = tx_power;                      //dBm
    hci_radio_tx_test[19] = 0;                             //power table index

    res = send_hci_command(&SerialPort, hci_radio_tx_test, sizeof(hci_radio_tx_test), hci_radio_tx_test_cmd_complete_event, sizeof(hci_radio_tx_test_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_set_tx_frequency_arm\n");
    return res;
}

static void print_usage_radio_rx_test(bool full)
{
    printf("Usage: wmbt radio_rx_test %s <bd_addr> <frequency> <modulation_type> <logical_channel> <bb_packet_type> <packet_length>\n", TRANSPORT);
    if (!full)
        return;
    printf("                bd_addr: BD_ADDR of Tx device (6 bytes)\n");
    printf("                frequency = (2402 - 2480) receive frequency in MHz\n");
    printf("                modulation_type: sets the data pattern\n");
    printf("                    0: 0x00 8-bit Pattern\n");
    printf("                    1: 0xFF 8-bit Pattern\n");
    printf("                    2: 0xAA 8-bit Pattern\n");
    printf("                    3: 0xF0 8-bit Pattern\n");
    printf("                    4: PRBS9 Pattern\n");
    printf("                logical_channel: sets the logical channel to Basic Rate (BR) or Enhanced Data Rate (EDR) for ACL packets\n");
    printf("                    0: EDR\n");
    printf("                    1: BR\n");
    printf("                bb_packet_type: baseband packet type to use\n");
    printf("                    3: DM1\n");
    printf("                    4: DH1 / 2-DH1\n");
    printf("                    8: 3-DH1\n");
    printf("                    10: DM3 / 2-DH3\n");
    printf("                    11: DH3 / 3-DH3\n");
    printf("                    12: EV4 / 2-EV5\n");
    printf("                    13: EV5 / 3-EV5\n");
    printf("                    14: DM5 / 2-DH5\n");
    printf("                    15: DH5 / 3-DH5\n");
    printf("                packet_length: 0 - 65535. Device will limit the length to the max for the baseband packet type\n");
}

static int execute_radio_rx_test(const char *bdaddr, int frequency, int modulation_type, int logical_channel, int bb_packet_type, int packet_length)
{
    ComHelper SerialPort;
    UINT32     params[6];
    BOOL      res;

    if (!SerialPort.OpenPort(port_num, baud_rate))
    {
        OpenPortError( port_num);
        return 0;
    }

    sscanf_s(bdaddr, "%02x%02x%02x%02x%02x%02x", &params[0], &params[1], &params[2], &params[3], &params[4], &params[5]);

    UINT8 hci_radio_rx_test[] = {0x01, 0x52, 0xfc, 0x0e, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    UINT8 hci_radio_rx_test_cmd_complete_event[] = {0x04, 0x0e, 0x04, 0x01, 0x52, 0xfc, 0x00};
    for( int i = 0; i < 6; i++ )
    {
        hci_radio_rx_test[i+4] = params[5-i];
    }
    hci_radio_rx_test[10] = 0xe8;                          //low byte of report perioe in ms (1sec = 1000ms, 0x03e8)
    hci_radio_rx_test[11] = 0x03;                          //high byte
    hci_radio_rx_test[12] = frequency - 2402;
    hci_radio_rx_test[13] = modulation_type;               //data pattern (3: 0xAA 8-bit Pattern)
    hci_radio_rx_test[14] = logical_channel;               //logical_Channel (0:ACL EDR, 1:ACL Basic)
    hci_radio_rx_test[15] = bb_packet_type;                //modulation type (BB_Packet_Type. 3:DM1, 4: DH1 / 2-DH1)
    hci_radio_rx_test[16] = packet_length & 0xff;          //low byte of packet_length
    hci_radio_rx_test[17] = (packet_length>>8) & 0xff;     //high byte of packet_length

    res = send_hci_command(&SerialPort, hci_radio_rx_test, sizeof(hci_radio_rx_test), hci_radio_rx_test_cmd_complete_event, sizeof(hci_radio_rx_test_cmd_complete_event), FALSE);
    if (!res)
        printf("Failed execute_radio_rx_test\n");

    printf("\nRadio RX Test is running. Press the Enter key to stop the test.\n");

    /*loop and handle the Rx Test statistics report until the 'q' key pressed*/
    UINT8 receive_length    = (transport_mode) ? 40 : 36;
    UINT8 wiced_hci_offset  = (transport_mode) ? 4 : 0;
    UINT8 event_header_size = (transport_mode) ? 5 : 4;

    while (TRUE)
    {
        /* read statistics report*/
        DWORD dwRead = SerialPort.Read((LPBYTE)&in_buffer[0], event_header_size);

        //printf("Read header: %d bytes\n", dwRead);
        //HexDump(in_buffer, dwRead);

        if ((dwRead == 5) && (in_buffer[0] == 0x19) && (in_buffer[1] == 0x03) && (in_buffer[2] == 0x00))
        {
            //Received HCI Trace Event message. Need to read the rest of the message out of the rx queue.
            SerialPort.Read((LPBYTE)&in_buffer[0], (in_buffer[3] | (in_buffer[4] >> 8)));
            continue;
        }
        else
        {
            dwRead += SerialPort.Read((LPBYTE)&in_buffer[event_header_size], receive_length - event_header_size);
        }

        if ((dwRead == receive_length) && (in_buffer[1 + wiced_hci_offset] == 0xFF && in_buffer[2 + wiced_hci_offset] == 0x21 && in_buffer[3 + wiced_hci_offset] == 0x07))
        {
            printf("Statistics Report received:\n");
            HexDump(in_buffer, dwRead);

            printf("  [Rx Test statistics]\n");
            printf("    Sync_Timeout_Count:     0x%x\n", in_buffer[4 + wiced_hci_offset] | in_buffer[5 + wiced_hci_offset] << 8 | in_buffer[6 + wiced_hci_offset] << 16 | in_buffer[7 + wiced_hci_offset] << 24);
            printf("    HEC_Error_Count:        0x%x\n", in_buffer[8 + wiced_hci_offset] | in_buffer[9 + wiced_hci_offset] << 8 | in_buffer[10 + wiced_hci_offset] << 16 | in_buffer[11 + wiced_hci_offset] << 24);
            printf("    Total_Received_Packets: 0x%x\n", in_buffer[12 + wiced_hci_offset] | in_buffer[13 + wiced_hci_offset] << 8 | in_buffer[14 + wiced_hci_offset] << 16 | in_buffer[15 + wiced_hci_offset] << 24);
            printf("    Good_Packets:           0x%x\n", in_buffer[16 + wiced_hci_offset] | in_buffer[17 + wiced_hci_offset] << 8 | in_buffer[18 + wiced_hci_offset] << 16 | in_buffer[19 + wiced_hci_offset] << 24);
            printf("    CRC_Error_Packets:      0x%x\n", in_buffer[20 + wiced_hci_offset] | in_buffer[21 + wiced_hci_offset] << 8 | in_buffer[22 + wiced_hci_offset] << 16 | in_buffer[23 + wiced_hci_offset] << 24);
            printf("    Total_Received_Bits:    0x%x\n", in_buffer[24 + wiced_hci_offset] | in_buffer[25 + wiced_hci_offset] << 8 | in_buffer[26 + wiced_hci_offset] << 16 | in_buffer[27 + wiced_hci_offset] << 24);
            printf("    Good_Bits:              0x%x\n", in_buffer[28 + wiced_hci_offset] | in_buffer[29 + wiced_hci_offset] << 8 | in_buffer[30 + wiced_hci_offset] << 16 | in_buffer[31 + wiced_hci_offset] << 24);
            printf("    Error_Bits:             0x%x\n", in_buffer[32 + wiced_hci_offset] | in_buffer[33 + wiced_hci_offset] << 8 | in_buffer[34 + wiced_hci_offset] << 16 | in_buffer[35 + wiced_hci_offset] << 24);
        }
        else
            SerialPort.Flush(0x0008); //flush data in fifo (PURGE_RXCLEAR = 0x0008)

        if (kbhit() /* && getchar() == 'q'*/)  //press any key to stop the test.
        {
            printf("A key pressed. Stop the Radio Rx Test.\n");
            SerialPort.Flush(0x0008); //flush data in fifo (PURGE_RXCLEAR = 0x0008)

            if (transport_mode)
            {
                res = SendWicedCommand(&SerialPort, HCI_CONTROL_COMMAND_RESET, NULL, 0);
            }
            else
            {
                res = execute_reset_highspeed(&SerialPort);
            }
            return res;
        }
    }

    return res;
}
#ifdef _WIN32
int _tmain(int argc, const char* argv[])
#else
int main (int argc, char **argv)
#endif
{
    int rx_frequency = 0;
    int tx_frequency = 0;
    int pattern = 0;
    int length = 0;

    unsigned char modulation_type_mapping[] = { 0x1, //  0x00 8-bit Pattern
                                                0x2, // 0xFF 8-bit Pattern
                                                0x3, // 0xAA 8-bit Pattern
                                                0x9, // 0xF0 8-bit Pattern
                                                0x4  // PRBS9 Pattern
                                               };

    if (!get_environment_variables())
        return 0;

    if (argc >= 3)
#ifdef _WIN32
        sscanf_s(argv[2], "COM%d", &port_num);
#else
		strcpy(port_num, argv[2]);
#endif

    if ((argc >= 2) && (_stricmp(argv[1], "reset") == 0))
    {
        if (argc == 3)
        {
            return (execute_reset(NULL));
        }
        print_usage_reset(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "reset_highspeed") == 0))
    {
        if (argc == 3)
        {
            return (execute_reset_highspeed(NULL));
        }
        print_usage_reset_highspeed(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "wiced_reset") == 0))
    {
        if (argc == 3)
        {
            if (transport_mode == WICED_HCI)
                return (execute_wiced_reset());
            else
                printf("Error: TRANSPORT_MODE Must be set to WICED_HCI to use this command\n");
            return 0;
        }
        print_usage_wiced_reset(true);
        return 0;
    }
    else if ((argc >= 3) && (_stricmp(argv[1], "download") == 0))
    {
        if (argc == 4)
        {
            const char* pathname;
            pathname = (argv[3]);
            return (execute_download(pathname));
        }
        print_usage_download(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "le_receiver_test") == 0))
    {
        if (argc == 4)
        {
            rx_frequency = atoi(argv[3]);
            if ((rx_frequency >= 2402) && (rx_frequency <= 2480))
            {
                return (execute_le_receiver_test(rx_frequency));
            }
        }
        print_usage_le_receiver_test(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "le_test_end") == 0))
    {
        if (argc == 3)
        {
            return (execute_le_test_end());
        }
        print_usage_le_test_end(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "le_transmitter_test") == 0))
    {
        if (argc == 6)
        {
            tx_frequency = atoi(argv[3]);
            if ((tx_frequency >= 2402) && (tx_frequency <= 2480))
            {
                length = atoi(argv[4]);
                if ((length > 0) && (length <= 37))
                {
                    pattern = atoi(argv[5]);
                    if ((pattern >= 0) && (pattern < 7))
                    {
                        return (execute_le_transmitter_test(tx_frequency, length, pattern));
                    }
                }
            }
        }
        print_usage_le_transmitter_test(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "tx_frequency_arm") == 0))
    {
        if (argc >= 5)
        {
            int carrier_on;
            carrier_on = atoi(argv[3]);
            if ((carrier_on == 0) || (carrier_on == 1))
            {
                if (carrier_on == 0)
                {
                    return execute_tx_frequency_arm(carrier_on, 2402, 0, 0, 0);
                }
                else if (argc == 8)
                {
                    int tx_frequency;
                    tx_frequency = atoi(argv[4]);
                    if ((tx_frequency >= 2402) && (tx_frequency <= 2480))
                    {
                        int mode;
                        mode = atoi(argv[5]);
                        if ((mode >= 0) && (mode <= 5))
                        {
                            int modulation_type;
                            modulation_type = atoi(argv[6]);
                            if ((modulation_type >= 0) && (modulation_type <= 3))
                            {
                                int tx_power;
                                tx_power = atoi(argv[7]);
                                if ((tx_power >= -25) && (tx_power <= 13))
                                {
                                    return execute_tx_frequency_arm(carrier_on, tx_frequency, mode, modulation_type, tx_power);
                                }
                            }
                        }
                    }
                }
            }
        }
        print_usage_tx_frequency_arm(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "receive_only") == 0))
    {
        if (argc == 4)
        {
            rx_frequency = atoi(argv[3]);
            if ((rx_frequency >= 2402) && (rx_frequency <= 2480))
            {
                return (execute_receive_only(rx_frequency));
            }
        }
        print_usage_receive_only_test(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "radio_tx_test") == 0))
    {
        if (argc == 10)
        {
            if (strlen(argv[3]) == 12)
            {
                int frequency;
                frequency = atoi(argv[4]);
                if ((frequency == 0) || (frequency >= 2402) && (frequency <= 2480))
                {
                    int modulation_type;
                    modulation_type = atoi(argv[5]);
                    if ((modulation_type >= 0) && (modulation_type <= 4))
                    {
                        int logical_channel;
                        logical_channel = atoi(argv[6]);
                        if ((logical_channel >= 0) && (logical_channel <= 1))
                        {
                            int bb_packet_type;
                            bb_packet_type = atoi(argv[7]);
                            if ((bb_packet_type >= 3) && (bb_packet_type <= 15))
                            {
                                int packet_length;
                                packet_length = atoi(argv[8]);
                                if ((packet_length >= 0) && (packet_length <= 0xffff))
                                {
                                    int tx_power;
                                    tx_power = atoi(argv[9]);

                                    if ((tx_power >= -25) && (tx_power <= 3))
                                    {
                                        return execute_radio_tx_test(argv[3], frequency, modulation_type_mapping[modulation_type], logical_channel, bb_packet_type, packet_length, tx_power);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        print_usage_radio_tx_test(true);
        return 0;
    }
    else if ((argc >= 2) && (_stricmp(argv[1], "radio_rx_test") == 0))
    {
        if (argc == 9)
        {
            if (strlen(argv[3]) == 12)
            {
                int frequency;
                frequency = atoi(argv[4]);
                if ((frequency >= 2402) && (frequency <= 2480))
                {
                    int modulation_type;
                    modulation_type = atoi(argv[5]);
                    if ((modulation_type >= 0) && (modulation_type <= 4))
                    {
                        int logical_channel;
                        logical_channel = atoi(argv[6]);
                        if ((logical_channel >= 0) && (logical_channel <= 1))
                        {
                            int bb_packet_type;
                            bb_packet_type = atoi(argv[7]);
                            if ((bb_packet_type >= 3) && (bb_packet_type <= 15))
                            {
                                int packet_length;
                                packet_length = atoi(argv[8]);
                                if ((packet_length >= 0) && (packet_length <= 0xffff))
                                {
                                    return execute_radio_rx_test(argv[3], frequency, modulation_type_mapping[modulation_type], logical_channel, bb_packet_type, packet_length);
                                }
                            }
                        }
                    }
                }
            }
        }
        print_usage_radio_rx_test(true);
        return 0;
    }
    else
    {
        printf("Usage: wmbt help\n");
        print_usage_download(false);
        print_usage_reset(false);
        print_usage_reset_highspeed(false);
        print_usage_wiced_reset(false);
        print_usage_le_receiver_test(false);
        print_usage_le_transmitter_test(false);
        print_usage_le_test_end(false);
        print_usage_tx_frequency_arm(false);
        print_usage_receive_only_test(false);
        print_usage_radio_tx_test(false);
        print_usage_radio_rx_test(false);
        printf("\nCheck Bluetooth Core 4.1 spec vol. 2 Sections 7.8.28-7.2.30\nfor details of LE Transmitter and Receiver tests\n");
    }
    return 0;
}
