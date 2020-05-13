#include "wmbt_uart.h"
extern "C"
{
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
};

extern 	int debug;

extern "C"
{
	void init_uart(int uart_fd, int baudRate);
	void dump(uchar *out, int len);
};


ComHelper::ComHelper() :
	m_handle(-1)
{
}

ComHelper::~ComHelper()
{
	if (m_handle != -1)
		ClosePort();
}

BOOL ComHelper::OpenPort( const char*  port, int baudRate )
{
	if( (m_handle = open(port, O_RDWR | O_NOCTTY)) == -1 )
	{
		printf("port %s could not be opened, error %d\n", port, errno);
		exit(EXIT_FAILURE);
	}
	else
	{
		printf( "Opened port %s\n", port);
	}

	init_uart(m_handle, baudRate);
	return true;
}

void ComHelper::ClosePort( )
{
	close(m_handle);
	m_handle = -1;
}

DWORD ComHelper::Read(LPBYTE buffer, DWORD dwLen)
{
    int i = 0;
    int len = dwLen;
    DWORD count = 0;

    while ((count = read(m_handle, &buffer[i], len)) < len)
    {
        i += count;
        len -= count;
		count = 0;
    }

    return i+count;
}

DWORD ComHelper::Write(LPBYTE b, DWORD dwLen)
{
	DWORD ret = 0;
	ret =  write(m_handle, b, dwLen);
	return ret;
}

void ComHelper::Flush(DWORD dwFlags)
{
}
