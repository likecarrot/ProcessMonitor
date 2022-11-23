#pragma	once
#include	"inc.h"

#define IOCTL_ENABLE_PROCESSMONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_THREADMONITOR CTL_CODE(FILE_DEVICE_UNKNOWN, 0x901, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_KEYBOARDFILTER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x902, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_FILEFILTER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x903, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_OPERARAM CTL_CODE(FILE_DEVICE_UNKNOWN, 0x904, METHOD_IN_DIRECT, FILE_ANY_ACCESS)



#define	PROCESSMONITOR	0x00001
#define	THREADMONITOR	0x00010
#define	KEYBOARDFILTER	0x00100
#define	FILEFILTER		0x01000
#define	OPERARAM		0x10000
struct _CommunicatePacket_
{
	DWORD64	FLAGS;
}CommunicatePacket,*PCommunicatePacket;