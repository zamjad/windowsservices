#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "services.h"

#define BUFF_LEN	256

wchar_t g_szSelectedComputer[BUFF_LEN];
int g_iLen = BUFF_LEN;
QUERY_SERVICE_CONFIG* g_psc = NULL;

// main program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
				   int nCmdShow) {

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DialogProc);
	return 0;
}

// Dialog procedure
BOOL CALLBACK DialogProc(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) {

	HICON hIcon = NULL;
	static HWND hWndList = NULL;
	int iWidth = 120;

	switch (p_uMsg) {

	case WM_INITDIALOG:
		// Load Icon
		hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
		SendMessage(p_hWnd, WM_SETICON, TRUE, (LPARAM)hIcon);

		// initilize common control
		InitCommonControls();

		hWndList = GetDlgItem(p_hWnd, IDC_LIST_SERVICES);

		InsertColumn(hWndList, DISPLAY_NAME_COL, TEXT("Display Name"), iWidth);
		InsertColumn(hWndList, SERVICE_NAME_COL, TEXT("Service Name"), iWidth);
		InsertColumn(hWndList, TYPE_COL, TEXT("Type"), iWidth);
		InsertColumn(hWndList, STATE_COL, TEXT("Current State"), iWidth);
		InsertColumn(hWndList, CONTROL_COL, TEXT("Controls Accepted"), iWidth);

		// Set extended style of List control full row select
		SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM) LVS_EX_FULLROWSELECT);
		break;

	case WM_COMMAND:
		switch(LOWORD(p_wParam)) {
		
		// Exit from the program
		case IDC_BTN_EXIT:
			PostQuitMessage(0);
			break;

		case IDC_BTN_UPDATE:
			GetWindowServices(hWndList);
			break;

		case IDC_BTN_COMPUTER:
			 if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_COMPUTER), NULL, DialogProcComputer)
				 == IDOK) {
			 
				SetWindowText(GetDlgItem(p_hWnd, IDC_STATICCOMPUTER), g_szSelectedComputer);
			 }
			break;
		}

		case WM_NOTIFY:
			switch (p_wParam) {

			case IDC_LIST_SERVICES:
				if (((NMHDR*)p_lParam)->code == NM_DBLCLK) {

					TCHAR szService[BUFF_LEN];
					int iPos = SendMessage(hWndList, LVM_GETNEXTITEM, 
						-1, LVIS_SELECTED);
					LVITEM lvItem;
					ZeroMemory(&lvItem, sizeof(LVITEM));

					// get the text of second column
					lvItem.iSubItem = 1;
					lvItem.pszText = szService;
					lvItem.cchTextMax = g_iLen;
					SendMessage(hWndList, LVM_GETITEMTEXT, (WPARAM)iPos, (LPARAM)&lvItem);

					SC_HANDLE hSCM = OpenSCManager(g_szSelectedComputer, NULL, SC_MANAGER_ALL_ACCESS);

					if (hSCM == NULL)
					{
						ErrorDescription(GetLastError());
					}
					else
					{
						SC_HANDLE hService = OpenService(hSCM, szService, SERVICE_ALL_ACCESS);

						if (hService == NULL)
						{
							ErrorDescription(GetLastError());
						}
						else
						{
							QUERY_SERVICE_CONFIG sc;
							DWORD dwBytesNeeded = 0;

							// Try to get information about the query
							BOOL bRetVal = QueryServiceConfig(hService, &sc, sizeof(QUERY_SERVICE_CONFIG),
								&dwBytesNeeded);

							if (!bRetVal) {
								DWORD retVal = GetLastError();

								// buffer size is small. 
								// Required size is in dwBytesNeeded
								if (ERROR_INSUFFICIENT_BUFFER == retVal) {

									DWORD dwBytes = sizeof(QUERY_SERVICE_CONFIG) + dwBytesNeeded;
									g_psc = new QUERY_SERVICE_CONFIG[dwBytesNeeded];

									bRetVal = QueryServiceConfig(hService, g_psc, dwBytes, &dwBytesNeeded);

									if (!bRetVal) {

										ErrorDescription(GetLastError());

										delete[] g_psc;
										g_psc = NULL;
										break;
									}

									DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_SERVICE), NULL, DialogProcService);

									delete[] g_psc;
									g_psc = NULL;
								}
								else {
									ErrorDescription(retVal);
								}
							}

							CloseServiceHandle(hService);
						}

						CloseServiceHandle(hSCM);
					}
				}
			}
			break;
	}

	return FALSE;
}

// insert column in the list control
int InsertColumn(HWND p_hWnd, int p_iCol, LPCTSTR p_lpszHeading, int p_iWidth) {

	LVCOLUMN column;

	column.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	column.fmt = LVCFMT_LEFT;
	column.cx = p_iWidth;
	column.pszText = (LPTSTR)p_lpszHeading;
	column.iSubItem = -1;
	column.iImage = -1;
	column.iOrder = 0;
	
	return (int) SendMessage(p_hWnd, LVM_INSERTCOLUMN, p_iCol, (LPARAM)&column);
}

// insert rows in the list control
int InsertItem(HWND p_hWnd, int p_iRow, LPCTSTR p_lpszText) {

	LVITEM lvItem;

	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = p_iRow;
	lvItem.iSubItem = 0;
	lvItem.state = 0;
	lvItem.stateMask = 0;
	lvItem.pszText = (LPTSTR)p_lpszText;	
	lvItem.iImage = 0;
	lvItem.lParam = 0;
	lvItem.iIndent = 0;

	return (int) SendMessage(p_hWnd, LVM_INSERTITEM, 0, (LPARAM)&lvItem);
}

// insert the item in the other columns of the list control
int InsertSubItem(HWND p_hWnd, int p_iRow, LPCTSTR p_lpszText, int p_iSubItem) {

	LVITEM lvItem;

	lvItem.iSubItem = p_iSubItem;
	lvItem.pszText = (LPTSTR)p_lpszText;

	return (int) SendMessage(p_hWnd, LVM_SETITEMTEXT, p_iRow, (LPARAM)&lvItem);
}

// get all the services of the window
BOOL GetWindowServices(HWND p_hWnd) {

	// first delete all item
	SendMessage(p_hWnd, LVM_DELETEALLITEMS, 0, 0);

	// open service manager
	SC_HANDLE hHandle = OpenSCManager(g_szSelectedComputer, NULL, SC_MANAGER_ENUMERATE_SERVICE);

	if (!hHandle) {
	
		ErrorDescription(GetLastError());
		return FALSE;
	}

	ENUM_SERVICE_STATUS service;

	DWORD dwBytesNeeded = 0;
	DWORD dwServicesReturned = 0;
	DWORD dwResumedHandle = 0;

	// Query services
	BOOL retVal = EnumServicesStatus(hHandle, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, 
		&service, sizeof(ENUM_SERVICE_STATUS), &dwBytesNeeded, &dwServicesReturned,
		&dwResumedHandle);

	if (!retVal) {
	
		// Need big buffer
		if (ERROR_MORE_DATA == GetLastError()) {
		
			// Set the buffer
			DWORD dwBytes = sizeof(ENUM_SERVICE_STATUS) + dwBytesNeeded;
			ENUM_SERVICE_STATUS* pServices = NULL;
			pServices = new ENUM_SERVICE_STATUS [dwBytes];

			// Now query again for services
			EnumServicesStatus(hHandle, SERVICE_WIN32 | SERVICE_DRIVER, SERVICE_STATE_ALL, 
				pServices, dwBytes, &dwBytesNeeded, &dwServicesReturned, &dwResumedHandle);

			// now traverse each service to get information
			for (unsigned iIndex = 0; iIndex < dwServicesReturned; iIndex++) {
			
				InsertItem(p_hWnd, iIndex, (pServices + iIndex)->lpDisplayName);
				InsertSubItem(p_hWnd, iIndex, (pServices + iIndex)->lpServiceName, SERVICE_NAME_COL);

				// get type of the service
				GetTypeOfService(p_hWnd, (pServices + iIndex)->ServiceStatus.dwServiceType, iIndex);

				// get current status of the services
				GetCurrentStatus(p_hWnd, (pServices + iIndex)->ServiceStatus.dwCurrentState, iIndex);

				// check the control code which service can accept
				GetControlCode(p_hWnd, (pServices + iIndex)->ServiceStatus.dwControlsAccepted, iIndex);
			}

			delete [] pServices;
			pServices = NULL;
		}
		else
			return FALSE;
	}

	CloseServiceHandle(hHandle);

	return TRUE;
}
	
// get type of the service
void GetTypeOfService(HWND p_hWnd, DWORD p_dwType, int p_iIndex) {

	switch (p_dwType) {
	
	case SERVICE_WIN32_OWN_PROCESS:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Run its own process"), TYPE_COL);
		break;

	case SERVICE_WIN32_SHARE_PROCESS:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Share a process with other application"), TYPE_COL);
		break;

	case SERVICE_KERNEL_DRIVER:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Device driver"), TYPE_COL);
		break;

	case SERVICE_FILE_SYSTEM_DRIVER:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("File system driver"), TYPE_COL);
		break;

	case SERVICE_INTERACTIVE_PROCESS:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Service can interactive with desktop"), TYPE_COL);
		break;

	case SERVICE_USER_SHARE_PROCESS:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("The service shares a process with one or more other services"), TYPE_COL);
		break;
	}
}

// get current status of the services
void GetCurrentStatus(HWND p_hWnd, DWORD p_dwType, int p_iIndex) {

	switch (p_dwType) {
	
	case SERVICE_STOPPED:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Not running"), STATE_COL);
		break;

	case SERVICE_START_PENDING:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Starting"), STATE_COL);
		break;

	case SERVICE_STOP_PENDING:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Stopping"), STATE_COL);
		break;

	case SERVICE_RUNNING:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Running"), STATE_COL);
		break;

	case SERVICE_CONTINUE_PENDING:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Continue is pending"), STATE_COL);
		break;

	case SERVICE_PAUSE_PENDING:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Pause is pending"), STATE_COL);
		break;

	case SERVICE_PAUSED:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Paused"), STATE_COL);
		break;
	}
}

// check the control code which service can accept
void GetControlCode(HWND p_hWnd, DWORD p_dwType, int p_iIndex) {

	switch (p_dwType) {
	
	case SERVICE_ACCEPT_STOP:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Can Stop"), CONTROL_COL);
		break;

	case SERVICE_ACCEPT_PAUSE_CONTINUE:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Can Pause and continue"), CONTROL_COL);
		break;

	case SERVICE_ACCEPT_SHUTDOWN:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Notified when shutdown"), CONTROL_COL);
		break;

	// win 2000 and above
	case SERVICE_ACCEPT_PARAMCHANGE:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Reread startup paramater"), CONTROL_COL);
		break;

	// win 2000 and above
	case SERVICE_ACCEPT_NETBINDCHANGE:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Can change network binding"), CONTROL_COL);
		break;

	// win 2003 or Win XP and above
	case SERVICE_ACCEPT_PRESHUTDOWN:
		InsertSubItem(p_hWnd, p_iIndex, TEXT("Can perform preshutdown task"), CONTROL_COL);
		break;
	}
}

// dialog procedure for computer dialog
BOOL CALLBACK DialogProcComputer(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) {

	HICON hIcon = NULL;
	static HWND hWndList = NULL;
	int iWidth = 85;
	int iPos = 0;

	switch (p_uMsg) {
	
	case WM_INITDIALOG:
		// Load Icon
		hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
		SendMessage(p_hWnd, WM_SETICON, TRUE, (LPARAM)hIcon);

		hWndList = GetDlgItem(p_hWnd, IDC_LIST_COMPUTER);

		InsertColumn(hWndList, PLATEFORM_COLUMN, TEXT("Plateform"), iWidth);
		InsertColumn(hWndList, NAME_COLUMN, TEXT("Name"), iWidth);
		InsertColumn(hWndList, VERSION_COLUMN, TEXT("Version"), iWidth);
		InsertColumn(hWndList, TYPE_COLUMN, TEXT("Type"), iWidth);
		InsertColumn(hWndList, COMMENT_COLUMN, TEXT("Comment"), iWidth);

		// Set extended style of List control full row select
		SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM) LVS_EX_FULLROWSELECT);
		GetComputerInfo(hWndList);

		break;

	case WM_COMMAND:
		switch(LOWORD(p_wParam)) {
		
		case IDOK:
			iPos = SendMessage(hWndList, LVM_GETNEXTITEM, -1, LVIS_SELECTED);
			LVITEM lvItem;
			ZeroMemory(&lvItem, sizeof(LVITEM));

			// get the text of second column
			lvItem.iSubItem = 1;
			lvItem.pszText = g_szSelectedComputer;
			lvItem.cchTextMax = g_iLen;
			SendMessage(hWndList, LVM_GETITEMTEXT, (WPARAM)iPos, (LPARAM)&lvItem);

			EndDialog(p_hWnd, IDOK);
			break;

		case IDCANCEL:
			wcscpy(g_szSelectedComputer, TEXT(""));
			EndDialog(p_hWnd, IDCANCEL);
			break;
		}

	case WM_NOTIFY:
		switch (p_wParam) {
		
		case IDC_LIST_COMPUTER:
			if (((NMHDR*)p_lParam)->code == NM_DBLCLK) {

				SendMessage(p_hWnd, WM_COMMAND, IDOK, NULL);
			}
			break;
		}

		break;
	}

	return FALSE;
}

// get the information about the computer
void GetComputerInfo(HWND p_hWnd) {

	NET_API_STATUS nStatus;
	LPSERVER_INFO_101 pBuff = NULL;
	DWORD dwEntriesRead = NULL;
	DWORD dwTotalEntries = NULL;
	DWORD dwResumeHandle = NULL;
	wchar_t buff[BUFF_LEN];
	DWORD dwPrefMaxLen = -1;

	// get information
	nStatus = NetServerEnum(NULL, 101, (LPBYTE*)&pBuff, MAX_PREFERRED_LENGTH, 
		&dwEntriesRead,	&dwTotalEntries, SV_TYPE_SERVER, NULL, &dwResumeHandle);

	if ((NERR_Success == nStatus) || (ERROR_MORE_DATA == nStatus)) {
	
		// first delete all item
		SendMessage(p_hWnd, LVM_DELETEALLITEMS, 0, 0);

		for (unsigned int iIndex = 0; iIndex < dwEntriesRead; iIndex++) {
		
			if ((pBuff+iIndex)->sv101_platform_id == PLATFORM_ID_DOS) {

				wsprintf(buff, TEXT("%s"), TEXT("DOS"));
			}
			else if ((pBuff+iIndex)->sv101_platform_id == PLATFORM_ID_OS2) {

				wsprintf(buff, TEXT("%s"), TEXT("OS/2 or Win9x"));
			}
			else if ((pBuff+iIndex)->sv101_platform_id == PLATFORM_ID_NT) {

				wsprintf(buff, TEXT("%s"), TEXT("Win NT/2000"));
			}
			else if ((pBuff+iIndex)->sv101_platform_id == PLATFORM_ID_VMS) {
				wsprintf(buff, TEXT("%s"), TEXT("VMS"));
			}

			InsertItem(p_hWnd, iIndex, buff);

			// Name
			// convert UNICODE to ANSI
			wsprintf(buff, TEXT("%S"), (pBuff+iIndex)->sv101_name);
			InsertSubItem(p_hWnd, iIndex, buff, NAME_COLUMN);

			// version
			wsprintf(buff, TEXT("%d.%d"), (pBuff+iIndex)->sv101_version_major, 
				(pBuff+iIndex)->sv101_version_minor);
			InsertSubItem(p_hWnd, iIndex, buff, VERSION_COLUMN);

			// type
			if ((pBuff+iIndex)->sv101_type & SV_TYPE_DOMAIN_CTRL) {			
				wsprintf(buff, TEXT("%s"), TEXT("PDC"));
			}
			else if ((pBuff+iIndex)->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) {			
				wsprintf(buff, TEXT("%s"), TEXT("BDC"));
			}
			else if ((pBuff+iIndex)->sv101_type & SV_TYPE_WORKSTATION){ 			
				wsprintf(buff, TEXT("%s"), TEXT("WorkStation"));
			}

			InsertSubItem(p_hWnd, iIndex, buff, TYPE_COLUMN);

			// comment
			// convert UNICODE to ANSI
			wsprintf(buff, TEXT("%S"), (pBuff+iIndex)->sv101_comment);
			InsertSubItem(p_hWnd, iIndex, buff, COMMENT_COLUMN);
		}
	}
	else {
	
		ErrorDescription(GetLastError());
	}

	if (pBuff != NULL) {
	
		NetApiBufferFree(pBuff);
	}
}

// get the description of the error
void ErrorDescription(DWORD p_dwError) {

	HLOCAL hLocal = NULL;

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, p_dwError, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),(LPTSTR)&hLocal, 
		0, NULL);

	MessageBox(NULL, (LPCTSTR)LocalLock(hLocal), TEXT("Error"), MB_OK | MB_ICONERROR);
	LocalFree(hLocal);
}

// call back function for service dialog
BOOL CALLBACK DialogProcService(HWND p_hWnd, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam) {

	HWND hWndEdit = NULL;
	HICON hIcon = NULL;
	TCHAR szBuff[BUFF_LEN];

	switch(p_uMsg) {
	
	case WM_INITDIALOG:
		// Load Icon
		hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
		SendMessage(p_hWnd, WM_SETICON, TRUE, (LPARAM)hIcon);

		// display information
		SetDlgItemText(p_hWnd, IDC_EDIT_PATH_NAME, g_psc->lpBinaryPathName);
		SetDlgItemText(p_hWnd, IDC_EDIT_DEPENDENCIES, g_psc->lpDependencies);
		SetDlgItemText(p_hWnd, IDC_EDIT_START_NAME, g_psc->lpServiceStartName);
		SetDlgItemText(p_hWnd, IDC_EDIT_DISPLAY_NAME, g_psc->lpDisplayName);
		SetDlgItemText(p_hWnd, IDC_EDIT_ORDER_GROUP, g_psc->lpLoadOrderGroup);

		SetServiceType(p_hWnd, g_psc->dwServiceType);
		SetStartType(p_hWnd, g_psc->dwStartType);
		SetErrorControl(p_hWnd, g_psc->dwErrorControl);

		wsprintf(szBuff, TEXT("%d"), g_psc->dwTagId);
		SetDlgItemText(p_hWnd, IDC_EDIT_TAG_ID, szBuff);
		break;

	case WM_COMMAND:
		switch(LOWORD(p_wParam)) {
		
		case IDOK:
			EndDialog(p_hWnd, IDOK);
			break;
		}
	}

	return FALSE;
}

// display the service type
void SetServiceType(HWND p_hWnd, DWORD p_dwType) {

	wchar_t szBuff[BUFF_LEN];

	// service type
	switch(p_dwType) {

	case SERVICE_WIN32_OWN_PROCESS:
		wsprintf(szBuff, TEXT("%s"), TEXT("Runs in its own process"));
		break;

	case SERVICE_WIN32_SHARE_PROCESS:
		wsprintf(szBuff, TEXT("%s"), TEXT("Service shares a process with other services"));
		break;

	case SERVICE_KERNEL_DRIVER:
		wsprintf(szBuff, TEXT("%s"), TEXT("Service is device driver"));
		break;

	case SERVICE_FILE_SYSTEM_DRIVER:
		wsprintf(szBuff, TEXT("%s"), TEXT("Service is file system driver"));
		break;

	case SERVICE_INTERACTIVE_PROCESS:
		wsprintf(szBuff, TEXT("%s"), TEXT("Service can interact with desktop"));
		break;
	}

	SetDlgItemText(p_hWnd, IDC_EDIT_SERVICE_TYPE, szBuff);
}

// dispalay the start type
void SetStartType(HWND p_hWnd, DWORD p_dwType) {

	wchar_t szBuff[BUFF_LEN];

	// service type
	switch(p_dwType) {
	
	case SERVICE_BOOT_START:
		wsprintf(szBuff, TEXT("%s"), TEXT("Start by System Loader"));
		break;

	case SERVICE_SYSTEM_START:
		wsprintf(szBuff, TEXT("%s"), TEXT("Started by IoInitSystem function"));
		break;

	case SERVICE_AUTO_START:
		wsprintf(szBuff, TEXT("%s"), TEXT("Started by Service Control Manager"));
		break;

	case SERVICE_DEMAND_START:
		wsprintf(szBuff, TEXT("%s"), TEXT("Start by StartService function"));
		break;

	case SERVICE_DISABLED:
		wsprintf(szBuff, TEXT("%s"), TEXT("No Longer be started"));
		break;
	}

	SetDlgItemText(p_hWnd, IDC_EDIT_START_TYPE, szBuff);
}

// set error control
void SetErrorControl(HWND p_hWnd, DWORD p_dwErrro) {

	wchar_t szBuff[BUFF_LEN];

	// service type
	switch(p_dwErrro) {
	
	case SERVICE_ERROR_IGNORE:
		wsprintf(szBuff, TEXT("%s"), TEXT("Logs error but continue operation"));
		break;

	case SERVICE_ERROR_NORMAL:
		wsprintf(szBuff, TEXT("%s"), TEXT("Logs error and display message box"));
		break;

	case SERVICE_ERROR_SEVERE:
		wsprintf(szBuff, TEXT("%s"), TEXT("Logs error and restarted with Last Known Good Configuration"));
		break;

	case SERVICE_ERROR_CRITICAL:
		wsprintf(szBuff, TEXT("%s"), TEXT("Log error if possible"));
		break;
	}

	SetDlgItemText(p_hWnd, IDC_EDIT_ERROR_CONTROL, szBuff);
}
