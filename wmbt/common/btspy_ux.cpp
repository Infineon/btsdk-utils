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

/*
 * Sample MCU application for sending trace to BTSpy application on Linux/Mac OS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define INVALID_SOCKET  -1
typedef unsigned char BYTE;
typedef unsigned short USHORT;

static int log_sock = INVALID_SOCKET;
static int wiced_trace_to_spy_trace[] = { 0, 4, 3, 6, 7 };

void TraceHciPkt(BYTE type, BYTE *buffer, USHORT length)
{
    struct sockaddr_in socket_addr;
    BYTE buf[1100];
    USHORT offset = 0;
    USHORT *p = (USHORT*)buf;

    if (log_sock == -1)
    {
        log_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (log_sock == INVALID_SOCKET)
            return;

        memset(&socket_addr, 0, sizeof(socket_addr));
        socket_addr.sin_family = AF_INET;
        socket_addr.sin_addr.s_addr = 0;
        socket_addr.sin_port = 0;

        int err = bind(log_sock, (sockaddr *)&socket_addr, sizeof(socket_addr));
        if (err != 0)
        {
            close(log_sock);
            log_sock = INVALID_SOCKET;
            return;
        }
    }

    if (length > 1024)
        length = 1024;

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = ntohl(0x7f000001);
    socket_addr.sin_port = 9876;
    int r = -1;
    if ((char)type == -1)
    {
        r = sendto(log_sock, (const char *)buffer, length, 0, (sockaddr *)&socket_addr, sizeof(socket_addr));
    }
    else
    {
        *p++ = wiced_trace_to_spy_trace[type];
        *p++ = length;
        *p++ = 0;
        *p++ = 0;
        memcpy(p, buffer, length);
        r = sendto(log_sock, (const char *)buf, length + 8, 0, (sockaddr *)&socket_addr, sizeof(socket_addr));
    }
}
