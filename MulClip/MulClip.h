
#include <windows.h>

#define CLIP_HINT_SIZE 128

static DWORD SUPPORTED_CLIP_TYPES[] =
{
	CF_TEXT,
	CF_HDROP,
};

struct _ClipEntry
{
	DWORD id;
	DWORD type;
	DWORD size;
	DWORD hintSize;
	BYTE *pContent;

	struct _ClipEntry *pNext;
};

typedef struct _ClipEntry __ClipEntry;

CRITICAL_SECTION crsClipsList;
__ClipEntry emptyClip;
__ClipEntry *pClipsList, *pSelEntry;
DWORD clipsListSeqId;
BOOL bListShown;
HANDLE hHeap;

HWND mainHwnd, hwndNextViewer; 

BYTE CLASS_NAME[]  = "MulClipWindowClassName";
BYTE CLIPBOARD_IS_EMPTY_MESSAGE[] = "Clipboard history is empty.";

