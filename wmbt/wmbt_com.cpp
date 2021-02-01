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
#include <stdio.h>
#include <windows.h>
#include <winioctl.h>
#include <conio.h>
#include "wmbt_com.h"

static char _parityChar[] = "NOEMS";
static char* _stopBits[] = { "1", "1.5", "2" };

//
//Class ComHelper Implementation
//
ComHelper::ComHelper() :
    m_handle(INVALID_HANDLE_VALUE)
{
    memset(&m_OverlapRead, 0, sizeof(m_OverlapRead));
    memset(&m_OverlapWrite, 0, sizeof(m_OverlapWrite));
}

ComHelper::~ComHelper()
{
    ClosePort();
}

//
//Open Serial Bus driver
//
BOOL ComHelper::OpenPort(int port, int baudRate)
{
    char lpStr[20];
    sprintf_s(lpStr, 20, "\\\\.\\COM%d", port);

    // open once only
    if (m_handle != NULL&& m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
    }
    m_handle = CreateFileA(lpStr,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (m_handle != NULL&& m_handle != INVALID_HANDLE_VALUE)
    {
        // setup serial bus device
        BOOL bResult;
        DWORD dwError = 0;
        COMMTIMEOUTS commTimeout;
        COMMPROP commProp;
        COMSTAT comStat;
        DCB serial_config;

        PurgeComm(m_handle, PURGE_RXABORT | PURGE_RXCLEAR |PURGE_TXABORT | PURGE_TXCLEAR);

        // create events for Overlapped IO
        m_OverlapRead.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        m_OverlapWrite.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        // set comm timeout
        memset(&commTimeout, 0, sizeof(COMMTIMEOUTS));
        commTimeout.ReadIntervalTimeout = 1;
        commTimeout.ReadTotalTimeoutConstant = 1000;
        commTimeout.ReadTotalTimeoutMultiplier = 10;
        commTimeout.WriteTotalTimeoutConstant = 1000;
        commTimeout.WriteTotalTimeoutMultiplier = 1;
        bResult = SetCommTimeouts(m_handle, &commTimeout);

        // set comm configuration
        memset(&serial_config, 0, sizeof(serial_config));
        serial_config.DCBlength = sizeof (DCB);
        bResult = GetCommState(m_handle, &serial_config);

        serial_config.BaudRate = baudRate;
        serial_config.ByteSize = 8;
        serial_config.Parity = NOPARITY;
        serial_config.StopBits = ONESTOPBIT;
        serial_config.fBinary = TRUE;
        serial_config.fOutxCtsFlow = TRUE; // TRUE;
        serial_config.fRtsControl = RTS_CONTROL_HANDSHAKE;
        serial_config.fOutxDsrFlow = FALSE; // TRUE;
        serial_config.fDtrControl = FALSE;

        serial_config.fOutX = FALSE;
        serial_config.fInX = FALSE;
        serial_config.fErrorChar = FALSE;
        serial_config.fNull = FALSE;
        serial_config.fParity = FALSE;
        serial_config.XonChar = 0;
        serial_config.XoffChar = 0;
        serial_config.ErrorChar = 0;
        serial_config.EofChar = 0;
        serial_config.EvtChar = 0;
        bResult = SetCommState(m_handle, &serial_config);

        if (!bResult)
            printf("OpenPort SetCommState failed %d\n", GetLastError());
        else
        {
            // verify CommState
            memset(&serial_config, 0, sizeof(serial_config));
            serial_config.DCBlength = sizeof (DCB);
            bResult = GetCommState(m_handle, &serial_config);
        }

        // set IO buffer size
        memset(&commProp, 0, sizeof(commProp));
        bResult = GetCommProperties(m_handle, &commProp);

        if (!bResult)
            printf("OpenPort GetCommProperties failed %d\n", GetLastError());
        else
        {
            // use 4096 byte as preferred buffer size, adjust to fit within allowed Max
            commProp.dwCurrentTxQueue = 4096;
            commProp.dwCurrentRxQueue = 4096;
            if (commProp.dwCurrentTxQueue > commProp.dwMaxTxQueue)
                commProp.dwCurrentTxQueue = commProp.dwMaxTxQueue;
            if (commProp.dwCurrentRxQueue > commProp.dwMaxRxQueue)
                commProp.dwCurrentRxQueue = commProp.dwMaxRxQueue;
            bResult = SetupComm(m_handle, commProp.dwCurrentRxQueue, commProp.dwCurrentTxQueue);

            if (!bResult)
                printf("OpenPort SetupComm failed %d\n", GetLastError());
            else
            {
                memset(&commProp, 0, sizeof(commProp));
                bResult = GetCommProperties(m_handle, &commProp);

                if (!bResult)
                    printf("OpenPort GetCommProperties failed %d\n", GetLastError());
            }
        }

        // clear comm error
        memset(&comStat, 0, sizeof(comStat));
        ClearCommError(m_handle, &dwError, &comStat);
    }
    printf("Opened COM%d at speed: %u\n", port, baudRate);
    return m_handle != NULL && m_handle != INVALID_HANDLE_VALUE;
}

void ComHelper::ClosePort()
{
    printf("Close Serial Bus\n");
    if (m_OverlapRead.hEvent != NULL)
    {
        CloseHandle(m_OverlapRead.hEvent);
        m_OverlapRead.hEvent = NULL;
    }

    if (m_OverlapWrite.hEvent != NULL)
    {
        CloseHandle(m_OverlapWrite.hEvent);
        m_OverlapWrite.hEvent = NULL;
    }
    if (m_handle != NULL&& m_handle != INVALID_HANDLE_VALUE)
    {
        // drop DTR
        EscapeCommFunction(m_handle, CLRDTR);
        // purge any outstanding reads/writes and close device handle
        PurgeComm(m_handle, PURGE_RXABORT | PURGE_RXCLEAR |PURGE_TXABORT | PURGE_TXCLEAR);
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

BOOL ComHelper::IsOpened()
{
    return (m_handle != NULL && m_handle != INVALID_HANDLE_VALUE);
}

// read a number of bytes from Serial Bus Device
// Parameters:
//	lpBytes - Pointer to the buffer
//	dwLen   - number of bytes to read
// Return:	Number of byte read from the device.
//
DWORD ComHelper::Read(LPBYTE lpBytes, DWORD dwLen)
{
    LPBYTE p = lpBytes;
    DWORD Length = dwLen;
    DWORD dwRead = 0;
    DWORD dwTotalRead = 0;

//    printf("ComHelper::Read Prepare to read %ld bytes\n", dwLen);

    // Loop here until request is fulfilled
    while (Length)
    {
        DWORD dwRet = WAIT_TIMEOUT;
        dwRead = 0;
        ResetEvent(m_OverlapRead.hEvent);
        m_OverlapRead.Internal = ERROR_SUCCESS;
        m_OverlapRead.InternalHigh = 0;
        if (!ReadFile(m_handle, (LPVOID)p, Length, &dwRead, &m_OverlapRead))
        {
//			printf("ComHelper::ReadFile returned with %ld\n", GetLastError());

            // Overlapped IO returns FALSE with ERROR_IO_PENDING
            if (GetLastError() != ERROR_IO_PENDING)
            {
                printf("ComHelper::ReadFile failed with %ld\n", GetLastError());
                break;
            }

            //Clear the LastError and wait for the IO to Complete
            SetLastError(ERROR_SUCCESS);
            dwRet = WaitForSingleObject(m_OverlapRead.hEvent, 10000);
//			printf("ComHelper::WaitForSingleObject returned with %ld\n", dwRet);

            // IO completed, retrieve Overlapped result
            GetOverlappedResult(m_handle, &m_OverlapRead, &dwRead, TRUE);

            // if dwRead is not updated, retrieve it from OVERLAPPED structure
            if (dwRead == 0)
                dwRead = (DWORD)m_OverlapRead.InternalHigh;
        }

        if (dwRead > Length)
            break;
        p += dwRead;
        Length -= dwRead;
        dwTotalRead += dwRead;

        if (kbhit())  //press any key to stop the Read
            return 0;
    }

//	printf("dwLen = %d TotalRead = %d\n", dwLen, dwTotalRead);
    return dwTotalRead;
}

// Write a number of bytes to Serial Bus Device
// Parameters:
//	lpBytes - Pointer to the buffer
//	dwLen   - number of bytes to write
// Return:	Number of byte Written to the device.
//
DWORD ComHelper::Write(LPBYTE lpBytes, DWORD dwLen)
{
    LPBYTE p = lpBytes;
    DWORD Length = dwLen;
    DWORD dwWritten = 0;
    DWORD dwTotalWritten = 0;

//	printf("ComHelper::Write Prepare to Write %ld bytes\n", dwLen);
    while (Length)
    {
        dwWritten = 0;
        SetLastError(ERROR_SUCCESS);
        ResetEvent(m_OverlapWrite.hEvent);
        if (!WriteFile(m_handle, p, Length, &dwWritten, &m_OverlapWrite))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                printf("ComHelper::WriteFile failed with %ld\n", GetLastError());
                break;
            }
            DWORD dwRet = WaitForSingleObject(m_OverlapWrite.hEvent, 5000);
            if (dwRet != WAIT_OBJECT_0)
            {
                printf("ComHelper::Write WaitForSingleObject failed with %ld\n", GetLastError());
                break;
            }
            GetOverlappedResult(m_handle, &m_OverlapWrite, &dwWritten, FALSE);
        }
        if (dwWritten > Length)
            break;
        p += dwWritten;
        Length -= dwWritten;
        dwTotalWritten += dwWritten;
    }

//    printf("dwLen = %d TotalWritten = %d\n", dwLen, dwWritten);
    return dwTotalWritten;
}

void ComHelper::Flush(DWORD dwFlags)
{
    PurgeComm(m_handle, dwFlags); //PURGE_RXABORT | PURGE_RXCLEAR, 0x0002 | 0x0008
}
