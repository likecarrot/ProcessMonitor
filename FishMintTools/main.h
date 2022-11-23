#pragma once
#include	"inc.h"
#include	"AutoLock.h"
#define	CommunicateDeviceName		L"\\Device\\FishMintMonitor"
#define	CommunicateDeviceSym		L"\\??\\FishMintMonitor"

#define	RAM_PROCESSNOTIFY		0x1111


#define	KD_CRUSH	"FISHMINT_TOOLS:"
typedef	struct _CommunicateDeviceExtension_
{
	//���̹��˿���
	BOOLEAN	KeyBoardFilter = FALSE;
	//�ļ����˿���
	BOOLEAN	FileSystemFilter = FALSE;
	//�����߳�֪ͨ����
	BOOLEAN	ProcessMonitor = FALSE;
	BOOLEAN	ThreadMonitor = FALSE;
	FastMutex	Mutex;
	LIST_ENTRY	head;
	//�����ڴ濪��
	BOOLEAN	OperRam = FALSE;

	PUNICODE_STRING	SymName = NULL;
	PUNICODE_STRING	DeviceName = NULL;

	PDEVICE_OBJECT	pCommunicateDevice = NULL;
}CommunicateDeviceExtension,*PCommunicateDeviceExtension;


VOID	DriverUnload(PDRIVER_OBJECT	pDriver);
NTSTATUS	GeneralDispatch(PDEVICE_OBJECT	Device, PIRP	irp);
NTSTATUS	CreateCloseDispatch(PDEVICE_OBJECT	Device, PIRP	irp);
NTSTATUS	DeviceControlDispatch(PDEVICE_OBJECT	Device, PIRP	irp);
NTSTATUS	ReadDevice(PDEVICE_OBJECT	Device, PIRP	irp);

void	OnProcessNotify(PEPROCESS	Process, HANDLE	ProcessId, PPS_CREATE_NOTIFY_INFO	CreateInfo);
void	OnThreadNotify(HANDLE ProcessId,HANDLE ThreadId,BOOLEAN Create);


void	PushItem(LIST_ENTRY* entry);
char* EPROCESS_GET_NAME(PEPROCESS	process);