/*****************************************************************************
 *
 *  Name:          wmbt_linux.c
 *
 *  Description:   The WICED Manufacturing Bluetooth test tool (WMBT) is used to test and verify the RF
 *                 performance of the Cypress family of SoC Bluetooth BR/EDR/BLE standalone and
 *                 combo devices. Each test sends an WICED HCI command to the device and then waits
 *                 for an WICED HCI Command Complete event from the device.
 *
 *                 For usage description, execute:
 *
 *                 ./wmbt help
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <stdbool.h>

#include <sys/ioctl.h>


int kbhit()
{
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized)
	{
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

void init_uart(int uart_fd)
{
    struct termios termios;
    tcflush(uart_fd, TCIOFLUSH);
    tcgetattr(uart_fd, &termios);

    termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                | INLCR | IGNCR | ICRNL | IXON);
    termios.c_oflag &= ~OPOST;
    termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios.c_cflag &= ~(CSIZE | PARENB);
    termios.c_cflag |= CS8;
    termios.c_cflag &= ~CRTSCTS;

    tcsetattr(uart_fd, TCSANOW, &termios);
    tcflush(uart_fd, TCIOFLUSH);
    tcsetattr(uart_fd, TCSANOW, &termios);
    tcflush(uart_fd, TCIOFLUSH);
    tcflush(uart_fd, TCIOFLUSH);
    cfsetospeed(&termios, B115200);
    cfsetispeed(&termios, B115200);
    tcsetattr(uart_fd, TCSANOW, &termios);
}
