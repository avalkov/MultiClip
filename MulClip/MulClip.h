
#include <windows.h>

#define ClipHintSize 128

static DWORD SupportedClipTypes[] = {
	CF_TEXT,
	CF_HDROP,
};

struct _ClipEntry{

	DWORD Id;
	DWORD Type;
	DWORD Size;
	DWORD HintSize;
	BYTE *pContent;

	struct _ClipEntry *pNext;
};

typedef struct _ClipEntry __ClipEntry;

__ClipEntry EmptyClip;

CRITICAL_SECTION crsClipsList;
__ClipEntry *pClipsList,*pSelEntry;
DWORD ClipsListSeqId;
BOOL bListShown;
HANDLE hHeap;

HWND MainHwnd;

HWND hwndNextViewer; 

BYTE ClassName[]  = "MulClipWindowClassName";

BYTE sEmpty[] = "Clipboard history is empty.";

