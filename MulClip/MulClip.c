
#include <windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include "MulClip.h"

void InitClipsList();
void *GetClipEntry(DWORD Id, BOOL bNext);
void *RemoveClipEntry(DWORD Id);
void *AddToClipList(DWORD Type, DWORD Size, BYTE *pContent);
LRESULT APIENTRY MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	WNDCLASSA wc;
	MSG msg;
	RECT r;

	memset(&r, 0x00, sizeof(r));

	hHeap = GetProcessHeap();
	InitClipsList();

	memset(&wc, 0x00, sizeof(wc));
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClassA(&wc);

	mainHwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, CLASS_NAME, NULL, WS_POPUP, 10, 10, 10, 10, NULL, NULL, hInstance, NULL);

	RegisterHotKey(mainHwnd, 1, MOD_SHIFT, 0x5A); // Shift+z
	RegisterHotKey(mainHwnd, 2, MOD_SHIFT, 0x43); // Shift+c
	RegisterHotKey(mainHwnd, 3, MOD_SHIFT, 0x56); // Shift+v
	RegisterHotKey(mainHwnd, 4, MOD_SHIFT, 0x58); // Shift+x
	RegisterHotKey(mainHwnd, 5, MOD_SHIFT, 0x20); // Shift+Space

	while (GetMessageA(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	return 0;
}

void InitClipsList()
{
	memset(&crsClipsList, 0x00, sizeof(crsClipsList));
	InitializeCriticalSection(&crsClipsList);

	pClipsList = NULL;
	clipsListSeqId = 1;
	bListShown = FALSE;

	memset(&emptyClip, 0x00, sizeof(emptyClip));
	emptyClip.hintSize = sizeof(CLIPBOARD_IS_EMPTY_MESSAGE) - 1;
	emptyClip.size = sizeof(CLIPBOARD_IS_EMPTY_MESSAGE) - 1;
	emptyClip.pContent = CLIPBOARD_IS_EMPTY_MESSAGE;
	emptyClip.type = CF_TEXT;
	emptyClip.id = 0;

	pSelEntry = &emptyClip;
}

void *AddToClipList(DWORD type, DWORD size, BYTE *pContent)
{
	__ClipEntry *pCurClip;

	if (pClipsList == NULL)
	{
		pCurClip = (__ClipEntry *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(__ClipEntry));
		pClipsList = pCurClip;
	}
	else
	{
		pCurClip = pClipsList;

		while (pCurClip->pNext != NULL)
		{
			pCurClip = pCurClip->pNext;
		}

		pCurClip->pNext = (__ClipEntry *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(__ClipEntry));
		pCurClip = pCurClip->pNext;
		pCurClip->pNext = NULL;
	}

	pCurClip->pContent = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, size);
	pCurClip->size = size;
	
	if (size > CLIP_HINT_SIZE)
	{
		pCurClip->hintSize = CLIP_HINT_SIZE;
	} 
	else
	{
		pCurClip->hintSize = size;
	}

	pCurClip->type = type;
	memcpy(pCurClip->pContent, pContent, size);

	pCurClip->id = clipsListSeqId;
	clipsListSeqId++;

	return pCurClip;
}

void *GetClipEntry(DWORD id, BOOL bNext)
{
	__ClipEntry *pCurClip;

	if (id == 0)
	{
		goto end;
	}

	pCurClip = pClipsList;
	
	while (pCurClip != NULL && pCurClip->id != id)
	{
		pCurClip = pCurClip->pNext;
	}

	if (bNext == TRUE)
	{
		if (pCurClip != NULL)
		{
			if (pCurClip->pNext == NULL)
			{
				pCurClip = pClipsList;
			}
			else
			{
				pCurClip = pCurClip->pNext;
			}
		}

	}

end:
	return pCurClip;
}

void *RemoveClipEntry(DWORD id)
{
	__ClipEntry *pCurClip, *pPrevClip, *pNext;

	pNext = NULL;

	if (id == 0)
	{
		goto end; 
	}

	pCurClip = pClipsList;
	pPrevClip = NULL;
	
	while (pCurClip != NULL && pCurClip->id != id)
	{
		pPrevClip = pCurClip;
		pCurClip = pCurClip->pNext;
	}

	if (pCurClip != NULL)
	{
		if (pPrevClip != NULL)
		{ 
			// Its not first entry

			pPrevClip->pNext = pCurClip->pNext;
		} 
		else
		{
			pClipsList = pCurClip->pNext;
		}

		pNext = pCurClip->pNext;

		HeapFree(hHeap, 0, pCurClip->pContent);
		HeapFree(hHeap, 0, pCurClip);
	}

end:
	if (pNext == NULL)
	{ 
		// Id not found or last element removed or only element removed

		if (pClipsList != NULL)
		{ 
			// Last element removed

			pNext = pClipsList;
		}
		else
		{ 
			// only left element was removed

			pNext = &emptyClip;
		}
	}

	return pNext;
}

void CreateClipDisplay(__ClipEntry *pClip)
{
	POINT cursorPos;
	RECT rect;
	HDC hdc;

	hdc = GetDC(mainHwnd);

	memset(&rect, 0x00, sizeof(rect));

	if (pClip->type == CF_TEXT)
	{
		DrawTextA(hdc, pClip->pContent, pClip->hintSize, &rect, DT_CALCRECT | DT_NOPREFIX | DT_EXPANDTABS);

	} 
	else 
	{
		DrawTextW(hdc, (LPCWSTR)(pClip->pContent + sizeof(DROPFILES)), -1, &rect, DT_CALCRECT | DT_NOPREFIX | DT_EXPANDTABS);
	}

	ReleaseDC(mainHwnd, hdc);

	GetCursorPos(&cursorPos);
	
	ShowWindow(mainHwnd, SW_HIDE);
	SetWindowPos(mainHwnd, NULL, cursorPos.x, cursorPos.y, (rect.right - rect.left) + 20, (rect.bottom - rect.top) + 20, SWP_SHOWWINDOW);
}

LRESULT APIENTRY MainWndProc(HWND hwnd,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc; 
	PAINTSTRUCT ps; 
	RECT rc; 
	HGLOBAL hgMainClip;
	BYTE *pClipContent, *pGlobalCopy;
	HBRUSH hBrush;
	COLORREF brushColor;
	DWORD clipFormat, numFiles, i, userClipSize;
	HGLOBAL hGlobalCopy;

	switch (uMsg) 
	{
    
	// Hot key notifications
	
	case WM_HOTKEY:

	switch(wParam)
	{

	case 1:

		// Show the clip list

		EnterCriticalSection(&crsClipsList);

		if (pSelEntry == &emptyClip)
		{
			if (pClipsList != NULL)
			{
				pSelEntry = pClipsList;
			}
			else
			{
				pSelEntry = &emptyClip;
			}
		}

		CreateClipDisplay(pSelEntry);

		bListShown = TRUE;

		LeaveCriticalSection(&crsClipsList);

		break;

	case 2: 
		
		// Close the clip list

		if (bListShown == TRUE)
		{
			EnterCriticalSection(&crsClipsList);

			ShowWindow(mainHwnd, SW_HIDE);
			bListShown = FALSE;

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 3: 
		
		// Select clip and set it

		if (bListShown == TRUE)
		{
			EnterCriticalSection(&crsClipsList);

			if (pSelEntry != NULL && pSelEntry != &emptyClip)
			{
				OpenClipboard(hwnd);

				EmptyClipboard();

				hGlobalCopy = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, pSelEntry->size + 1);
				pGlobalCopy = GlobalLock(hGlobalCopy);

				memcpy(pGlobalCopy, pSelEntry->pContent, pSelEntry->size);

				GlobalUnlock(hGlobalCopy);

				SetClipboardData(pSelEntry->type, hGlobalCopy);

				CloseClipboard();
			}

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 4: 
		
		// Delete clip entry

		if (bListShown == TRUE)
		{
			EnterCriticalSection(&crsClipsList);

			pSelEntry = RemoveClipEntry(pSelEntry->id);

			CreateClipDisplay(pSelEntry);

			LeaveCriticalSection(&crsClipsList);
		}

		break;

	case 5: 
		
		// Go to next clip entry

		if (bListShown == TRUE)
		{
			EnterCriticalSection(&crsClipsList);

			pSelEntry = GetClipEntry(pSelEntry->id, TRUE);

			CreateClipDisplay(pSelEntry);

			LeaveCriticalSection(&crsClipsList);
		}

		break;
	}
	
	break;
   
	case WM_PAINT:

		hdc = BeginPaint(hwnd, &ps); 

		SetBkMode(hdc, TRANSPARENT);

		if (bListShown == TRUE)
		{
			EnterCriticalSection(&crsClipsList);

			if (pSelEntry != NULL)
			{
				brushColor = RGB(250, 240, 220);
				hBrush = CreateSolidBrush(brushColor);

				SelectObject(hdc, hBrush);

				GetClientRect(hwnd, &rc);
				RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);
					
				rc.left += 10;
				rc.top += 10;

				if (pSelEntry->type == CF_TEXT)
				{
					DrawTextA(hdc, pSelEntry->pContent, pSelEntry->hintSize, &rc, DT_LEFT | DT_EXPANDTABS); 
				} 
				else
				{
					DrawTextW(hdc, (LPCWSTR)(pSelEntry->pContent + sizeof(DROPFILES)), -1, &rc, DT_LEFT | DT_EXPANDTABS); 
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

		if ((HWND)wParam == hwndNextViewer)
		{
			hwndNextViewer = (HWND) lParam;

		} else if (hwndNextViewer != NULL)
		{
			SendMessageA(hwndNextViewer, uMsg, wParam, lParam); 
		}

		break; 

	case WM_DESTROY: 

		ChangeClipboardChain(hwnd, hwndNextViewer); 
		PostQuitMessage(0); 
		break; 

    // Clipboard update notification

	case WM_DRAWCLIPBOARD: 
		
		// Clipboard contents changed. 

		clipFormat = GetPriorityClipboardFormat(SUPPORTED_CLIP_TYPES, 2); 

		if (clipFormat == CF_TEXT || clipFormat == CF_HDROP)
		{
			EnterCriticalSection(&crsClipsList);

			if (OpenClipboard(hwnd))
			{
				hgMainClip = GetClipboardData(clipFormat); 
				
				if((pClipContent = GlobalLock(hgMainClip)) != NULL)
				{ 
					if (clipFormat == CF_HDROP)
					{
						numFiles = DragQueryFileA((HDROP)pClipContent, 0xFFFFFFFF, NULL, 0);

						if (numFiles > 0)
						{
							// Get the entire structure and file paths size - file paths are wide-char
						
							userClipSize = sizeof(DROPFILES);
							i = 0;
							
							while(i < numFiles)
							{
								userClipSize += (sizeof(WCHAR) * DragQueryFileA((HDROP)pClipContent, i, NULL, 0)) + sizeof(WCHAR); // For null termination
								i++;
							}
					
						}

					} 
					else 
					{
						userClipSize = lstrlenA(pClipContent);
					}

					AddToClipList(clipFormat, userClipSize, pClipContent);

					GlobalUnlock(hgMainClip); 
				}

				CloseHandle(hgMainClip);
				CloseClipboard(); 
			}

			LeaveCriticalSection(&crsClipsList);
		}

		// Notify next window in clipboard viewer chain
		
		SendMessageA(hwndNextViewer, uMsg, wParam, lParam); 
		break; 
    
	default: 
		return DefWindowProcA(hwnd, uMsg, wParam, lParam); 
	} 

	return (LRESULT)NULL;
}

