
#ifndef _SERVICES_H
#define _SERVICES_H

// for service column
#define DISPLAY_NAME_COL	0
#define SERVICE_NAME_COL	1
#define TYPE_COL			2
#define STATE_COL			3
#define CONTROL_COL			4

// for computer column
#define	PLATEFORM_COLUMN	0
#define NAME_COLUMN			1
#define VERSION_COLUMN		2
#define TYPE_COLUMN			3
#define COMMENT_COLUMN		4

// header files for network api and common controls
#include <lm.h>
#include <commctrl.h>

// library files for network api and common controls
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "netapi32.lib")

// prototype of function
INT_PTR CALLBACK DialogProc(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);
INT_PTR CALLBACK DialogProcComputer(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);
INT_PTR CALLBACK	DialogProcService(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam);
int InsertColumn(HWND p_hWnd, int p_iCol, LPCTSTR p_lpszHeading, int p_iWidth);
int InsertItem(HWND p_hWnd, int p_iRow, LPCTSTR p_lpszText);
int InsertSubItem(HWND p_hWnd, int p_iRow, LPCTSTR p_lpszText, int p_iSubItem);
BOOL GetWindowServices(HWND p_hWnd);
void GetTypeOfService(HWND p_hWnd, DWORD p_dwType, int p_iIndex);
void GetCurrentStatus(HWND p_hWnd, DWORD p_dwType, int p_iIndex);
void GetControlCode(HWND p_hWnd, DWORD p_dwType, int p_iIndex);
void GetComputerInfo(HWND p_hWnd);
void ErrorDescription(DWORD p_dwError);
void SetServiceType(HWND p_hWnd, DWORD p_dwType);
void SetStartType(HWND p_hWnd, DWORD p_dwType);
void SetErrorControl(HWND p_hWnd, DWORD p_dwErrro);

#endif
