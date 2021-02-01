/*
 * Copyright 2016-2021, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */
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

#ifdef __APPLE__
#include <IOKit/serial/ioss.h>
#endif

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

void init_uart(int uart_fd, int baudRate)
{
#if !defined(__APPLE__)
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

#else
    struct termios      options;
    int ret;
    int BAUDRATE = baudRate;

    // block non-root users from using this port
    ret = ioctl(uart_fd, TIOCEXCL);

    // clear the O_NONBLOCK flag, so that read() will
    //   block and wait for data.
    fcntl(uart_fd, F_SETFL, O_APPEND | O_NONBLOCK);

    // grab the options for the serial port
    tcgetattr(uart_fd, &options);

    options.c_cflag |= CREAD | CLOCAL | CS8;
    options.c_iflag = 0;
    options.c_oflag = 0;
    options.c_lflag = 0;
    options.c_cc[VMIN]=0;
    options.c_cc[VTIME]=0;
    options.c_cflag &= ~CRTSCTS;

    tcsetattr(uart_fd, TCSAFLUSH, &options);

    ret = ioctl(uart_fd, IOSSIOSPEED, &BAUDRATE);
#endif
}
