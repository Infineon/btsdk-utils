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
#pragma once
using namespace std;

//**************************************************************************************************
//*** Definitions for WICED Serial Port
//**************************************************************************************************

extern "C" int kbhit();
typedef unsigned char 	BYTE;
typedef unsigned char 	uchar;
typedef unsigned char*	LPBYTE;
typedef unsigned short	USHORT;
typedef unsigned short	UINT16;
typedef unsigned long		DWORD;
typedef unsigned long		ULONG;
typedef unsigned long		UINT32;
typedef long					LONG;
typedef	bool	BOOL;
#define	FALSE	false
#define	TRUE	true

//
// Serial Bus class, use this class to read/write from/to the serial port
//
class ComHelper
{
public:
    ComHelper();
    virtual ~ComHelper();

    // oopen serialbus driver to access device
    BOOL OpenPort( const char* port, int baudRate );
    void ClosePort( );

    // read data from device
    DWORD Read(LPBYTE b, DWORD dwLen);

    // write data to device
    DWORD Write(LPBYTE b, DWORD dwLen);

    //BOOL IsOpened( );

    void Flush(DWORD dwFlags);
private:

    int m_handle;
};
