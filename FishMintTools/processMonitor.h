#pragma	once
#include	"inc.h"

enum class ItemType:short
{
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit
};
struct ItemHeader
{
	ItemType	Type;
	USHORT		Size;
	LARGE_INTEGER	Time;
};
struct ProcessExitInfo:ItemHeader
{
	ULONG	ProcessId;
};
struct ProcessCreateInfo:ItemHeader
{
	ULONG	ProcessId;
	int		NameLength;
	CHAR	ProcessName;
	
};
template	<typename	T>
struct FullItem
{
	LIST_ENTRY	Entry;
	T	Data;
};

