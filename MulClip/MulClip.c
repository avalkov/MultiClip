
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include "MulClip.h"

void initClipsList();
void *GetClipEntry(DWORD Id,BOOL bNext);
void *RemoveClipEntry(DWORD Id);
void *AddToClipList(DWORD Type,DWORD Size,BYTE *pContent);
LRESULT APIENTRY MainWndProc(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {

	WNDCLASS wc;
	MSG msg;
	RECT r = { 0, 0, 0, 0 };

	hHeap = GetProcessHeap();
	initClipsList();

	memset(&wc,0x00,sizeof(wc));
	wc.lpfnWndProc   = MainWndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = ClassName;

	RegisterClass(&wc);

	MainHwnd = CreateWindowExA(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,ClassName,NULL,WS_POPUP,10,10,10,10,NULL,NULL,hInstance,NULL);

	RegisterHotKey(MainHwnd,1,MOD_SHIFT,0x5A); //Shift+z
	RegisterHotKey(MainHwnd,2,MOD_SHIFT,0x43); //Shift+c
	RegisterHotKey(MainHwnd,3,MOD_SHIFT,0x56); //Shift+v
	RegisterHotKey(MainHwnd,4,MOD_SHIFT,0x58); //Shift+x
	RegisterHotKey(MainHwnd,5,MOD_SHIFT,0x20); //Shift+Space

	while(GetMessage(&msg, NULL, 0, 0)) {

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


void initClipsList() {

	memset(&crsClipsList,0x00,sizeof(crsClipsList));
	InitializeCriticalSection(&crsClipsList);

	pClipsList = NULL;
	ClipsListSeqId = 1;
	bListShown = FALSE;

	memset(&EmptyClip,0x00,sizeof(EmptyClip));
	EmptyClip.HintSize = sizeof(sEmpty)-1;
	EmptyClip.Size = sizeof(sEmpty)-1;
	EmptyClip.pContent = sEmpty;
	EmptyClip.Type = CF_TEXT;
	EmptyClip.Id = 0;

	pSelEntry = &EmptyClip;
}


void *AddToClipList(DWORD Type,DWORD Size,BYTE *pContent) {

	__ClipEntry *pCurClip;


	if(pClipsList == NULL) {

		pCurClip = (__ClipEntry *) HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(__ClipEntry));
		pClipsList = pCurClip;

	} else {

		pCurClip = pClipsList;

		while(pCurClip->pNext!=NULL) {

			pCurClip = pCurClip->pNext;
		}

		pCurClip->pNext = (__ClipEntry *) HeapAlloc(hHeap,HEAP_ZERO_MEMORY,sizeof(__ClipEntry));
		pCurClip = pCurClip->pNext;
		pCurClip->pNext = NULL;
	}

	pCurClip->pContent = (BYTE *) HeapAlloc(hHeap,HEAP_ZERO_MEMORY,Size);
	pCurClip->Size = Size;
	if(Size>ClipHintSize) { pCurClip->HintSize = ClipHintSize; } else { pCurClip->HintSize = Size; }
	pCurClip->Type = Type;
	memcpy(pCurClip->pContent,pContent,Size);

	pCurClip->Id = ClipsListSeqId;
	ClipsListSeqId++;


	return pCurClip;
}

void *GetClipEntry(DWORD Id,BOOL bNext) {

	__ClipEntry *pCurClip;

	if(Id==0) { goto end; }

	pCurClip = pClipsList;
	while(pCurClip!=NULL && pCurClip->Id!=Id) {

		pCurClip = pCurClip->pNext;
	}

	if(bNext==TRUE) {

		if(pCurClip!=NULL) {

			if(pCurClip->pNext==NULL) {

				pCurClip = pClipsList;

			} else {

				pCurClip = pCurClip->pNext;
			}
		}

	}

end:
	return pCurClip;
}

void *RemoveClipEntry(DWORD Id) {

	__ClipEntry *pCurClip,*pPrevClip,*pNext;


	pNext = NULL;

	if(Id==0) { goto end; }

	pCurClip = pClipsList;
	pPrevClip = NULL;
	while(pCurClip!=NULL && pCurClip->Id!=Id) {

		pPrevClip = pCurClip;
		pCurClip = pCurClip->pNext;
	}

	if(pCurClip!=NULL) {

		if(pPrevClip!=NULL) { //Its not first entry

			pPrevClip->pNext = pCurClip->pNext;

		} else {

			pClipsList = pCurClip->pNext;
		}

		pNext = pCurClip->pNext;

		HeapFree(hHeap,0,pCurClip->pContent);
		HeapFree(hHeap,0,pCurClip);
	}

end:
	if(pNext==NULL) { //Id not found or last element removed or only element removed

		if(pClipsList!=NULL) { //Last element removed

			pNext = pClipsList;

		} else { //only left element was removed

			pNext = &EmptyClip;
		}
	}

	return pNext;
}

void CreateClipDisplay(__ClipEntry *pClip) {

	POINT CursorPos;
	RECT rect;
	HDC hdc;


	hdc = GetDC(MainHwnd);

	memset(&rect,0x00,sizeof(rect));

	if(pClip->Type==CF_TEXT) {

	DrawText(hdc, pClip->pContent, pClip->HintSize, &rect, DT_CALCRECT|DT_NOPREFIX|DT_EXPANDTABS);

	} else {

	DrawTextW(hdc, (LPCWSTR)(pClip->pContent+sizeof(DROPFILES)), -1, &rect, DT_CALCRECT|DT_NOPREFIX|DT_EXPANDTABS);
	}

	ReleaseDC(MainHwnd,hdc);

	GetCursorPos(&CursorPos);
	
	ShowWindow(MainHwnd,SW_HIDE);
	SetWindowPos(MainHwnd,NULL,CursorPos.x,CursorPos.y,(rect.right-rect.left)+20,(rect.bottom-rect.top)+20,SWP_SHOWWINDOW);
}

LRESULT APIENTRY MainWndProc(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam) {

	HDC hdc; 
	PAINTSTRUCT ps; 
	RECT rc; 
	HGLOBAL hgMainClip;
	BYTE *pClipContent,*pGlobalCopy;
	HBRUSH hBrush;
	COLORREF BrushColor;
	DWORD ClipFormat,numFiles,i,userClipSize;
	HGLOBAL hGlobalCopy;

	switch (uMsg) {

    //HOT KEY NOTIFICATIONS
	case WM_HOTKEY:

	switch(wParam) {

	case 1: //Show the clip list

		//if(bListShown==FALSE) {

			EnterCriticalSection(&crsClipsList);

			if(pSelEntry==&EmptyClip) {

				if(pClipsList!=NULL) {

					pSelEntry = pClipsList;

				} else {

					pSelEntry = &EmptyClip;
				}
			}

			CreateClipDisplay(pSelEntry);

			bListShown = TRUE;

			LeaveCriticalSection(&crsClipsList);
		//}
		
		break;

	case 2: //Close the clip list

		if(bListShown==TRUE) {

			EnterCriticalSection(&crsClipsList);

			ShowWindow(MainHwnd,SW_HIDE);
			bListShown = FALSE;

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 3: //Select clip and set it

		if(bListShown==TRUE) {

			EnterCriticalSection(&crsClipsList);

			if(pSelEntry!=NULL && pSelEntry!=&EmptyClip) {

				OpenClipboard(hwnd);

				EmptyClipboard();

				hGlobalCopy = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,(pSelEntry->Size + 1));
				pGlobalCopy = GlobalLock(hGlobalCopy);

				memcpy(pGlobalCopy,pSelEntry->pContent,pSelEntry->Size);

				GlobalUnlock(hGlobalCopy);

				SetClipboardData(pSelEntry->Type,hGlobalCopy);

				CloseClipboard();
			}

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 4: //Delete clip entry

		if(bListShown==TRUE) {

			EnterCriticalSection(&crsClipsList);

			pSelEntry = RemoveClipEntry(pSelEntry->Id);

			CreateClipDisplay(pSelEntry);

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 5: //Go to next clip entry

		if(bListShown==TRUE) {

			EnterCriticalSection(&crsClipsList);

			pSelEntry = GetClipEntry(pSelEntry->Id,TRUE);

			CreateClipDisplay(pSelEntry);

			LeaveCriticalSection(&crsClipsList);

		}

		break;

	}
		break;
   /////////////////////////////////////////////////////////

	case WM_PAINT:

		hdc = BeginPaint(hwnd, &ps); 

		SetBkMode(hdc,TRANSPARENT);

		if(bListShown==TRUE) {

			EnterCriticalSection(&crsClipsList);

			if(pSelEntry!=NULL) {

			BrushColor = RGB(250,240,220);
			hBrush = CreateSolidBrush(BrushColor);

			SelectObject(hdc,hBrush);

			GetClientRect(hwnd, &rc); 
			RoundRect(hdc,rc.left,rc.top,rc.right,rc.bottom,10,10);
				
			rc.left += 10;
			rc.top += 10;

			if(pSelEntry->Type==CF_TEXT) {

			DrawTextA(hdc, pSelEntry->pContent, pSelEntry->HintSize, &rc, DT_LEFT|DT_EXPANDTABS); 

			} else {

			DrawTextW(hdc, (LPCWSTR)(pSelEntry->pContent+sizeof(DROPFILES)), -1, &rc, DT_LEFT|DT_EXPANDTABS); 
			}

			DeleteObject(hBrush);

			}

			LeaveCriticalSection(&crsClipsList);
		}

		EndPaint(hwnd, &ps); 

		break; 

	case WM_CREATE: 

		hwndNextViewer = SetClipboardViewer(hwnd); 
		break; 

	case WM_CHANGECBCHAIN: 

		if((HWND)wParam == hwndNextViewer) {

			hwndNextViewer = (HWND) lParam;

		} else if (hwndNextViewer != NULL) {

			SendMessage(hwndNextViewer, uMsg, wParam, lParam); 
		}

		break; 

	case WM_DESTROY: 

		ChangeClipboardChain(hwnd, hwndNextViewer); 
		PostQuitMessage(0); 
		break; 

    //CLIPBOARD UPDATE NOTIFICATION
	case WM_DRAWCLIPBOARD:  // clipboard contents changed. 

		ClipFormat = GetPriorityClipboardFormat(SupportedClipTypes, 2); 

		if(ClipFormat==CF_TEXT || ClipFormat==CF_HDROP) {

			EnterCriticalSection(&crsClipsList);

			if(OpenClipboard(hwnd)) {

				hgMainClip = GetClipboardData(ClipFormat); 
				if((pClipContent = GlobalLock(hgMainClip))!=NULL) { 

				if(ClipFormat==CF_HDROP) {

					numFiles = DragQueryFileA((HDROP)pClipContent,0xFFFFFFFF,NULL,0);

					if(numFiles>0) {

					//Get the entire structure and file paths size - file paths are wide-char
					userClipSize = sizeof(DROPFILES);
					i = 0;
					while(i<numFiles) {

						userClipSize += (sizeof(WCHAR)*DragQueryFileA((HDROP)pClipContent,i,NULL,0))+sizeof(WCHAR); //For null termination
						i++;
					}
					//////////////////////////////////////////////////////////

					}

				} else {

					userClipSize = lstrlenA(pClipContent);
				}

				AddToClipList(ClipFormat,userClipSize,pClipContent);

				GlobalUnlock(hgMainClip); 
				}

				CloseHandle(hgMainClip);

				CloseClipboard(); 
			}

			LeaveCriticalSection(&crsClipsList);
		}

		//Notify next window in clipboard viewer chain
		SendMessage(hwndNextViewer, uMsg, wParam, lParam); 
		break; 
    //////////////////////////////////////////////

	default: 
		return DefWindowProc(hwnd, uMsg, wParam, lParam); 
	} 

	return (LRESULT) NULL; 
}

