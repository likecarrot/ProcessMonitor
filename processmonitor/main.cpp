#include	"main.h"


GLOBALS_EXTENSION	Global = { 0 };
EXTERN_C
NTSTATUS	DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath) {
	NTSTATUS	status = 0;
	UNICODE_STRING	DeviceName = { 0 };
	UNICODE_STRING	DeviceSym = { 0 };
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&DeviceSym, DEVICE_SYM);

	do
	{
		status = IoCreateDevice(pDriver, NULL, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &Global.pCommunicateDevice);
		if (!NT_SUCCESS(status))
		{
			KdPrint((CURSH_PRINT"IoCreateDevice Failed.\n"));
			break;
		}
		Global.pCommunicateDevice->Flags |= DO_DIRECT_IO;
		status=IoCreateSymbolicLink(&DeviceSym, &DeviceName);
		if (!NT_SUCCESS(status))
		{
			KdPrint((CURSH_PRINT"IoCreateSymbolicLink Failed.\n"));
			break;
		}
		Global.DeviceName = &DeviceName;
		Global.DeviceSym = &DeviceSym;
		ExInitializeFastMutex(&Global.mutex);
		InitializeListHead(&Global.ItemHeads);

		//这里还必须说明的是，调用PsSetCreateProcessNotifyRoutineEx函数时，
		//需要对强制完整性签名进行破解。破解的方法也很简单，只需要把驱动对象中的
		//DriverSection中的Flags包含0x20就行，代码如下
		auto pLdrData = (PKLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
		pLdrData->Flags = pLdrData->Flags | 0x20;
		status = PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, Global.ProcessNotifyFlag);
		if (!NT_SUCCESS(status))
		{
			KdPrint((CURSH_PRINT"PsSetCreateProcessNotifyRoutineEx Failed.\n"));
			break;
		}
		Global.ProcessNotifyFlag = !Global.ProcessNotifyFlag;
	} while (false);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(Global.pCommunicateDevice);
		IoDeleteSymbolicLink(&DeviceSym);
		PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, TRUE);
	}
	for (size_t i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriver->MajorFunction[i] = GenerialDispatch;
	}
	pDriver->DriverUnload = DriverUnload;

	KdPrint((CURSH_PRINT"Initialize DriverEntry successed.\n"));

	return	status;
}

NTSTATUS    GenerialDispatch(PDEVICE_OBJECT pDevice, PIRP irp) {
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	STATUS_SUCCESS;
}

NTSTATUS    ReadDispatch(PDEVICE_OBJECT pDevice, PIRP    irp) {
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION	irpsp = IoGetCurrentIrpStackLocation(irp);
	LONG64	len = irpsp->Parameters.Read.Length;
	LONG64	len2 = 0;
	auto	mdl = irp->MdlAddress;
	NT_ASSERT(mdl);
	auto	buffer = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
	if (buffer)
	{
		ExAcquireFastMutex(&Global.mutex);
		while (true)
		{
			if (IsListEmpty(&Global.ItemHeads))
				break;
			auto	entry = RemoveEntryList(&Global.ItemHeads);
			auto	info = CONTAINING_RECORD(entry, PROCESS_INFO, List);
			if (len>=(len2+sizeof(PROCESS_INFO)))
			{
				memcpy((VOID*)((ULONG)buffer+len2), info, sizeof(PROCESS_INFO));
				len2 += sizeof(PROCESS_INFO);
				Global.ItemCount--;
				ExFreePool(info);
			}
			else
			{
				break;
			}
		}
	}
	ExReleaseFastMutex(&Global.mutex);
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = len2;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}
NTSTATUS    WriteDispatch(PDEVICE_OBJECT pDevice, PIRP   irp) {
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION	irpsp = IoGetCurrentIrpStackLocation(irp);
	auto	len = irpsp->Parameters.Write.Length;
	NT_ASSERT(irp->MdlAddress);
	if (len>=sizeof(WRITE_PACKET))
	{
		PWRITE_PACKET buffer = (PWRITE_PACKET)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
		ExAcquireFastMutex(&Global.mutex);
		if (buffer->flag== OPEN_CLOSE::OPEN)
		{
			Global.NoProcessFlag = OPEN_CLOSE::OPEN;
			strcpy(Global.NoProcessName, buffer->processname);
		}else
			Global.NoProcessFlag = OPEN_CLOSE::CLOSE;
		ExReleaseFastMutex(&Global.mutex);
		status = STATUS_SUCCESS;
	}
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = len;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}
NTSTATUS    CreataCloseDispatch(PDEVICE_OBJECT  pDevice, PIRP    irp) {
	NTSTATUS	status = STATUS_UNSUCCESSFUL;
	if (pDevice==Global.pCommunicateDevice)
	{
		status = STATUS_SUCCESS;
	}
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return	status;
}
void    DriverUnload(PDRIVER_OBJECT pDriver) {
	IoDeleteDevice(Global.pCommunicateDevice);
	IoDeleteSymbolicLink(Global.DeviceSym);
	PsSetCreateProcessNotifyRoutineEx(ProcessNotifyRoutine, TRUE);
	KdPrint((CURSH_PRINT" DriverUnload.\n"));
}

void ProcessNotifyRoutine(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) {
	if (CreateInfo)
	{
		auto	info =(PPROCESS_INFO)ExAllocatePoolWithTag(PagedPool, sizeof(PROCESS_INFO), DRIVER_TAG);
		memcpy(info->ImageName, (char*)PsGetProcessImageFileName(Process),strlen((char*)PsGetProcessImageFileName(Process)));
		info->PROCESSID = HandleToULong(ProcessId);
		info->PARENT_PROCESSID = HandleToULong(CreateInfo->ParentProcessId);
		info->Type = ProcessType::Create;
		if (Global.NoProcessFlag== OPEN_CLOSE::OPEN)
		{
			if (!strcmp(Global.NoProcessName,info->ImageName)) {
				KdPrint((CURSH_PRINT"ProcessNotifyRoutine: no process-processid-%x processName-%s.\n", info->PROCESSID,info->ImageName));
				CreateInfo->CreationStatus = STATUS_UNSUCCESSFUL;
				return;
			}
		}
		KdPrint((CURSH_PRINT"ProcessNotifyRoutine Create pid-%x name-%s.\n",info->PROCESSID, info->ImageName));
		ExAcquireFastMutex(&Global.mutex);
		InsertTailList(&Global.ItemHeads, &info->List);
		Global.ItemCount++;
		ExReleaseFastMutex(&Global.mutex);
	}
	else {
		auto	info = (PPROCESS_INFO)ExAllocatePoolWithTag(PagedPool, sizeof(PROCESS_INFO), DRIVER_TAG);
		memcpy(info->ImageName, (char*)PsGetProcessImageFileName(Process), strlen((char*)PsGetProcessImageFileName(Process)));
		info->PROCESSID = HandleToULong(ProcessId);
		info->Type = ProcessType::Exit;
		KdPrint((CURSH_PRINT"ProcessNotifyRoutine Exit pid-%x name-%s.\n", info->PROCESSID, info->ImageName));

		ExAcquireFastMutex(&Global.mutex);
		InsertTailList(&Global.ItemHeads, &info->List);
		Global.ItemCount++;
		ExReleaseFastMutex(&Global.mutex);
	}
}
