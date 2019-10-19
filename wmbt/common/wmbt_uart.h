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
