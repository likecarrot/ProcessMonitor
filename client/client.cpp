#include    "main.h"


int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}
std::map< ProcessType, std::string> ProcessType_to_String = {
	{ProcessType::Create,"Create"},
	{ProcessType::Exit,"Exit"}
};

void	DisplayInfo(char* buffer, int  bytes) {
	PROCESS_INFO* info = (PPROCESS_INFO)buffer;
	int	len = 0;
	while (bytes>=(len+sizeof(PROCESS_INFO)))
	{
		std::cout << "process id-" << info->PROCESSID << " ProcessName-" << info->ImageName << " ParentId-" << info->PROCESSID << " ProcessType-" << ProcessType_to_String.at(info->Type) << std::endl;	
		len += sizeof(PROCESS_INFO);
		info = (PPROCESS_INFO)(buffer + len);
	}
}
int main()
{
	HANDLE	hDevice =
	CreateFile(DEVICE_SYM, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
		return Error("Failed to open file");
	int	len = (sizeof(PROCESS_INFO)) * 40;
	char	*buffer = (char*)malloc(len);
	
	while (true) {
		DWORD bytes;
		ZeroMemory(buffer, len);
		if (!::ReadFile(hDevice, buffer, len, &bytes, nullptr))
			return Error("Failed to read");

		if (bytes != 0)
			DisplayInfo(buffer, bytes);

		::Sleep(200);
	}

	return	0;
}

