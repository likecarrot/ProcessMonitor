#pragma	once	
#include    <Windows.h>
#include <stdio.h>
#include    <iostream>
#include    <map>

#define	DEVICE_SYM	L"\\\\.\\FishMintNotify"


enum OPEN_CLOSE
{
    OPEN = 1, CLOSE = 2
};

enum ProcessType
{
    Create = 0x10, Exit = 0x11
};


typedef	struct PROCESS_INFO
{
    ULONG	PROCESSID;		//process id
    ULONG	PARENT_PROCESSID;		//parent process id
    char	ImageName[32];	//eprocess+0x450
    LIST_ENTRY  List;
    ProcessType Type;
}PROCESS_INFO, * PPROCESS_INFO;


typedef struct Write_packet
{
    OPEN_CLOSE flag;
    char    processname[32];
}WRITE_PACKET, * PWRITE_PACKET;