#include	"inc.h"


PCommunicateDeviceExtension	DevExt = NULL;
EXTERN_C	NTSTATUS	
DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath) {
	
	PDEVICE_OBJECT	pCommunicateDevice;
	UNICODE_STRING	DeviceName;
	UNICODE_STRING DeviceSym;
	
	NTSTATUS	status;
	do
	{
		RtlInitUnicodeString(&DeviceName, CommunicateDeviceName);
		//创建一个设备
		status = IoCreateDevice(pDriver, sizeof(CommunicateDeviceExtension), &DeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &pCommunicateDevice);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		pCommunicateDevice->Flags |= DO_DIRECT_IO;

		//操作扩展
		DevExt = (PCommunicateDeviceExtension)pCommunicateDevice->DeviceExtension;
		DevExt->DeviceName = &DeviceName;
		DevExt->pCommunicateDevice = pCommunicateDevice;
		DevExt->SymName = &DeviceSym;


		//创建Symlink
		RtlInitUnicodeString(&DeviceSym, CommunicateDeviceSym);
		status = IoCreateSymbolicLink(&DeviceSym, &DeviceName);
		if (!NT_SUCCESS(status))
		{
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pCommunicateDevice);
		IoDeleteSymbolicLink(&DeviceSym);
		PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	}

	//分发函数
	for (size_t i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriver->MajorFunction[i] = GeneralDispatch;
	}
	pDriver->MajorFunction[IRP_MJ_CREATE] = \
		pDriver->MajorFunction[IRP_MJ_CLOSE] = \
		CreateCloseDispatch;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControlDispatch;
	pDriver->DriverUnload = DriverUnload;
	return	status;
}

VOID	DriverUnload(PDRIVER_OBJECT	pDriver) {
	DevExt =(PCommunicateDeviceExtension)pDriver->DeviceObject->DeviceExtension;
	//做删除操作
	IoDeleteDevice(DevExt->pCommunicateDevice);
	IoDeleteSymbolicLink(DevExt->SymName);
	if (DevExt->ProcessMonitor)
	{
		PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	}
	if (DevExt->ThreadMonitor)
	{
		PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
	}
	if (DevExt->ProcessMonitor)
	{

	}
	if (DevExt->OperRam)
	{
		
	}
	
}


NTSTATUS	GeneralDispatch(PDEVICE_OBJECT	Device, PIRP	irp) {
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	STATUS_SUCCESS;
}
NTSTATUS	CreateCloseDispatch(PDEVICE_OBJECT	Device, PIRP	irp) {
	PIO_STACK_LOCATION	irpsp = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS	status = STATUS_SUCCESS;
	if (irpsp->MajorFunction==IRP_MJ_CREATE)
	{

	}
	if (irpsp->MajorFunction == IRP_MJ_CLOSE)
	{

	}

	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}
NTSTATUS	ReadDevice(PDEVICE_OBJECT	Device, PIRP	irp) {
	PIO_STACK_LOCATION	irpsp = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS	status = STATUS_SUCCESS;
	auto	mdl = irp->MdlAddress;
	auto	len = irpsp->Parameters.Read.Length;
	NT_ASSERT(irp->MdlAddress);
	auto	buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	if (!buffer)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else {
		AutoLock<FastMutex>	lock(DevExt->Mutex);
		while (true)
		{
			if (IsListEmpty(&DevExt->head))
			{
				break;
			}
			auto	entry = RemoveHeadList(&DevExt->head);
			auto	info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);
			auto	size = info->Data.Size;
			if (len<size)
			{
				InsertHeadList(&DevExt->head, entry);
				break;
			}
			memcpy(buffer, &info->Data, size);
			len -= size;
			buffer += size;
			ExFreePool(info);
		}
	}
	KdPrint((KD_CRUSH"ReadDevice.\n"));
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = len;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}
NTSTATUS	DeviceControlDispatch(PDEVICE_OBJECT	Device, PIRP	irp) {
	PIO_STACK_LOCATION	irpsp = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS	status = STATUS_SUCCESS;
	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case	IOCTL_ENABLE_PROCESSMONITOR:
		if (DevExt->ProcessMonitor)
		{
			status=PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		}
		else {
			//初始化锁和链表
			DevExt->Mutex.Init();
			InitializeListHead(&DevExt->head);
			status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		}
		if (status==STATUS_SUCCESS)
		{
			DevExt->ProcessMonitor = !DevExt->ProcessMonitor;
		}
		break;
	case	IOCTL_ENABLE_THREADMONITOR:
		if (DevExt->ThreadMonitor)
		{
			status= PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
		}
		else {
			status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		}
		if (status == STATUS_SUCCESS)
		{
			DevExt->ThreadMonitor = !DevExt->ThreadMonitor;
		}
		break;

	default:
		break;
	}
	KdPrint((KD_CRUSH"IOCTL-%d.\n", irpsp->Parameters.DeviceIoControl.IoControlCode));
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}



void	OnProcessNotify(PEPROCESS	Process, HANDLE	ProcessId,PPS_CREATE_NOTIFY_INFO	CreateInfo) {
	if (CreateInfo)
	{//process	create
		auto	item = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessCreateInfo>), RAM_PROCESSNOTIFY);
		if (item== nullptr)
		{
			KdPrint((KD_CRUSH"OnProcessNotify--CreateInfo--item--allocate filed.\n"));
			return;
		}
		KeQuerySystemTimePrecise(&item->Data.Time);
		item->Data.Type = ItemType::ProcessCreate;
		item->Data.ProcessId = HandleToULong(ProcessId);
		item->Data.NameLength = strlen(EPROCESS_GET_NAME(Process));
		strcpy(&item->Data.ProcessName, EPROCESS_GET_NAME(Process));
		PushItem(&item->Entry);
		KdPrint((KD_CRUSH"PROCESSID-%d -%s.\n", item->Data.ProcessId, item->Data.ProcessName));
	}
	else {//proccess exit
		auto	item = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessExitInfo>), RAM_PROCESSNOTIFY);
		if (item == nullptr)
		{
			KdPrint((KD_CRUSH"OnProcessNotify--ExitInfo--item--allocate filed.\n"));
			return;
		}
		KeQuerySystemTimePrecise(&item->Data.Time);
		item->Data.Type = ItemType::ProcessCreate;
		item->Data.ProcessId = HandleToULong(ProcessId);
		PushItem(&item->Entry);
		KdPrint((KD_CRUSH"PROCESSID-%d -%s.\n", item->Data.ProcessId));
	}
}
void	OnThreadNotify(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) {
	if (Create)
	{//thread create

	}
	else {//thread exit

	}
}

void	PushItem(LIST_ENTRY* entry) {
	AutoLock<FastMutex>	lock(DevExt->Mutex);
	InsertTailList(&DevExt->head, entry);
}

char* EPROCESS_GET_NAME(PEPROCESS	process) {
	DWORD64	addr = (DWORD64)&(*process);
	addr += 0x450;
	char* ret = (char*)addr;
	return	ret;
}