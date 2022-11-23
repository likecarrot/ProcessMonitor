#pragma once

#include	<ntddk.h>

#define	CURSH_PRINT	"FishMintNotify: "

#define	DEVICE_NAME	L"\\Device\\FishMintNotify"
#define	DEVICE_SYM	L"\\??\\FishMintNotify"

#define DRIVER_TAG  1111
enum OPEN_CLOSE
{
    OPEN = 1, CLOSE = 2
};
typedef	struct GLOBALS_EXTENSION
{
	char NoProcessName[32];		//这个是禁止创建的程序名称
    OPEN_CLOSE  NoProcessFlag = OPEN_CLOSE::CLOSE;
	PDEVICE_OBJECT	pCommunicateDevice = NULL;
	BOOLEAN	ProcessNotifyFlag = FALSE;
	BOOLEAN	ThreadNotifyFlag = FALSE;
	PUNICODE_STRING	DeviceName=NULL;
	PUNICODE_STRING	DeviceSym=NULL;
	FAST_MUTEX	mutex;
	LIST_ENTRY	ItemHeads;
	int	ItemCount=0;
}GLOBALS_EXTENSION,*PGLOBALS_EXTENSION;

enum ProcessType
{
    Create = 0x10,Exit=0x11
};
typedef	struct PROCESS_INFO
{
	ULONG	PROCESSID;		//process id
	ULONG	PARENT_PROCESSID;		//parent process id
	char	ImageName[32];	//eprocess+0x450
    LIST_ENTRY  List;
    ProcessType Type;
}PROCESS_INFO,*PPROCESS_INFO;



typedef struct _KLDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    UINT32 SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    UINT32 Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    UINT32 CheckSum;
    UINT32 TimeDateStamp;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} KLDR_DATA_TABLE_ENTRY, * PKLDR_DATA_TABLE_ENTRY;

typedef struct Write_packet
{
    OPEN_CLOSE flag;
    char    processname[32];
}WRITE_PACKET,*PWRITE_PACKET;

EXTERN_C
UCHAR* PsGetProcessImageFileName(PEPROCESS EProcess);

EXTERN_C NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath);
void    DriverUnload(PDRIVER_OBJECT pDriver);
NTSTATUS    GenerialDispatch(PDEVICE_OBJECT pDevice, PIRP irp);
NTSTATUS    CreataCloseDispatch(PDEVICE_OBJECT  pDevice, PIRP    irp);
NTSTATUS    ReadDispatch(PDEVICE_OBJECT pDevice, PIRP    irp);
NTSTATUS    WriteDispatch(PDEVICE_OBJECT pDevice, PIRP   irp);
void ProcessNotifyRoutine(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);



