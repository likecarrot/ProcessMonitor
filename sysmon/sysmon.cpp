#include <iostream>
#include    <Windows.h>
#include	"sha.h"

int main()
{
	bool	ret = false;
	HANDLE	hDevice = CreateFile(CommunicateDeviceSym, GENERIC_ALL, NULL, NULL, OPEN_EXISTING, 0, 0);
	if (!hDevice)
	{
		printf("no.\n");
	}
	printf("ok.\n");
	ret = DeviceIoControl(hDevice, IOCTL_ENABLE_PROCESSMONITOR, 0, 0, 0, 0, 0, 0);
	
	if (ret)
	{
		printf("send.\n");
	}
	else {
		printf("send filed -%d.\n",GetLastError());
	}
	printf("ioctl-%d .\n", IOCTL_ENABLE_PROCESSMONITOR);

	CloseHandle(hDevice);


	getchar();
	return	0;
}