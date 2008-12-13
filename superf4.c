/*
	SuperF4 - Force kill programs with Ctrl+Alt+F4
	Copyright (C) 2008  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <shlwapi.h>

//Localization
#define L10N_NAME    "SuperF4"
#define L10N_VERSION "0.9"
#ifndef L10N_FILE
#define L10N_FILE "localization/en-US/strings.h"
#endif
#include L10N_FILE
#if L10N_FILE_VERSION != 1
#error Localization not up to date!
#endif

//Messages
#define WM_ICONTRAY            WM_USER+1
#define WM_ADDTRAY             WM_USER+2 //This value has to remain constant through versions
#define SWM_TOGGLE             WM_APP+1
#define SWM_HIDE               WM_APP+2
#define SWM_AUTOSTART_ON       WM_APP+3
#define SWM_AUTOSTART_OFF      WM_APP+4
#define SWM_AUTOSTART_HIDE_ON  WM_APP+5
#define SWM_AUTOSTART_HIDE_OFF WM_APP+6
#define SWM_ABOUT              WM_APP+7
#define SWM_EXIT               WM_APP+8

//Boring stuff
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
static HICON icon[2];
static NOTIFYICONDATA traydata;
static UINT WM_TASKBARCREATED=0;
static int tray_added=0;
static int hide=0;
static char txt[100];

//Cool stuff
static HINSTANCE hinstDLL=NULL;
static HHOOK keyhook=NULL;
static HHOOK mousehook=NULL;
static int ctrl=0;
static int alt=0;
static int win=0;
static FILE *log;

//Error message handling
static int showerror=1;

LRESULT CALLBACK ErrorMsgProc(INT nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HCBT_ACTIVATE) {
		//Edit the caption of the buttons
		SetDlgItemText((HWND)wParam,IDYES,"Copy error");
		SetDlgItemText((HWND)wParam,IDNO,"OK");
	}
	return 0;
}

void Error(char *func, char *info, int errorcode, int line) {
	if (showerror) {
		//Format message
		char errormsg[100];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,errorcode,0,errormsg,sizeof(errormsg),NULL);
		errormsg[strlen(errormsg)-2]='\0'; //Remove that damn newline at the end of the formatted error message
		sprintf(txt,"%s failed in file %s, line %d.\nError: %s (%d)\n\n%s", func, TEXT(__FILE__), line, errormsg, errorcode, info);
		//Display message
		HHOOK hhk=SetWindowsHookEx(WH_CBT, &ErrorMsgProc, 0, GetCurrentThreadId());
		int response=MessageBox(NULL, txt, L10N_NAME" Error", MB_ICONERROR|MB_YESNO|MB_DEFBUTTON2);
		UnhookWindowsHookEx(hhk);
		if (response == IDYES) {
			//Copy message to clipboard
			OpenClipboard(NULL);
			EmptyClipboard();
			char *data=LocalAlloc(LMEM_FIXED,strlen(txt)+1);
			memcpy(data,txt,strlen(txt)+1);
			SetClipboardData(CF_TEXT,data);
			CloseClipboard();
		}
	}
}

//Entry point
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow) {
	//Look for previous instance
	HWND previnst;
	if ((previnst=FindWindow(L10N_NAME,NULL)) != NULL) {
		SendMessage(previnst,WM_ADDTRAY,0,0);
		return 0;
	}
	
	//Change working directory
	char path[MAX_PATH];
	if (GetModuleFileName(NULL, path, sizeof(path))) {
		PathRemoveFileSpec(path);
		SetCurrentDirectory(path);
	}
	
	//Check command line
	if (!strcmp(szCmdLine,"-hide")) {
		hide=1;
	}
	
	//Create window class
	WNDCLASSEX wnd;
	wnd.cbSize=sizeof(WNDCLASSEX);
	wnd.style=0;
	wnd.lpfnWndProc=WindowProc;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=hInst;
	wnd.hIcon=NULL;
	wnd.hIconSm=NULL;
	wnd.hCursor=LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR|LR_SHARED);
	wnd.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName=L10N_NAME;
	
	//Register class
	RegisterClassEx(&wnd);
	
	//Create window
	HWND hwnd=CreateWindowEx(0,wnd.lpszClassName, L10N_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	//Register TaskbarCreated so we can re-add the tray icon if explorer.exe crashes
	if ((WM_TASKBARCREATED=RegisterWindowMessage("TaskbarCreated")) == 0) {
		Error("RegisterWindowMessage('TaskbarCreated')","This means the tray icon won't be added if (or should I say when) explorer.exe crashes.",GetLastError(),__LINE__);
	}
	
	//Load tray icons
	if ((icon[0] = LoadImage(hInst, "tray-disabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
		Error("LoadImage('tray-disabled')","Fatal error.",GetLastError(),__LINE__);
		PostQuitMessage(1);
	}
	if ((icon[1] = LoadImage(hInst, "tray-enabled", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)) == NULL) {
		Error("LoadImage('tray-enabled')","Fatal error.",GetLastError(),__LINE__);
		PostQuitMessage(1);
	}
	
	//Create icondata
	traydata.cbSize=sizeof(NOTIFYICONDATA);
	traydata.uID=0;
	traydata.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
	traydata.hWnd=hwnd;
	traydata.uCallbackMessage=WM_ICONTRAY;
	
	//Update tray icon
	UpdateTray();
	
	//Hook keyboard
	HookKeyboard();
	
	//Add tray if hooking failed, even though -hide was supplied
	if (hide && !keyhook) {
		hide=0;
		UpdateTray();
	}
	
	//Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void ShowContextMenu(HWND hwnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu=CreatePopupMenu();
	
	//Toggle
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_TOGGLE, (keyhook?L10N_MENU_DISABLE:L10N_MENU_ENABLE));
	
	//Hide
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, L10N_MENU_HIDE);
	
	//Check autostart
	int autostart_enabled=0, autostart_hide=0;
	//Open key
	HKEY key;
	RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_QUERY_VALUE,&key);
	//Read value
	char autostart_value[MAX_PATH+10];
	DWORD len=sizeof(autostart_value);
	DWORD res=RegQueryValueEx(key,L10N_NAME,NULL,NULL,(LPBYTE)autostart_value,&len);
	//Close key
	RegCloseKey(key);
	//Get path
	char path[MAX_PATH];
	GetModuleFileName(NULL,path,MAX_PATH);
	//Compare
	char pathcmp[MAX_PATH+10];
	sprintf(pathcmp,"\"%s\"",path);
	if (!strcmp(pathcmp,autostart_value)) {
		autostart_enabled=1;
	}
	else {
		sprintf(pathcmp,"\"%s\" -hide",path);
		if (!strcmp(pathcmp,autostart_value)) {
			autostart_enabled=1;
			autostart_hide=1;
		}
	}
	//Autostart
	HMENU hAutostartMenu=CreatePopupMenu();
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_enabled?MF_CHECKED:0), (autostart_enabled?SWM_AUTOSTART_OFF:SWM_AUTOSTART_ON), L10N_MENU_AUTOSTART);
	InsertMenu(hAutostartMenu, -1, MF_BYPOSITION|(autostart_hide?MF_CHECKED:0), (autostart_hide?SWM_AUTOSTART_HIDE_OFF:SWM_AUTOSTART_HIDE_ON), L10N_MENU_HIDE);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_POPUP, (UINT)hAutostartMenu, L10N_MENU_AUTOSTART);
	InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
	
	//About
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_ABOUT, L10N_MENU_ABOUT);
	
	//Exit
	InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, L10N_MENU_EXIT);

	//Track menu
	SetForegroundWindow(hwnd);
	TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL );
	DestroyMenu(hMenu);
}

int UpdateTray() {
	strncpy(traydata.szTip,(keyhook?L10N_TRAY_ENABLED:L10N_TRAY_DISABLED),sizeof(traydata.szTip));
	traydata.hIcon=icon[keyhook?1:0];
	
	//Only add or modify if not hidden
	if (!hide) {
		if (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&traydata) == FALSE) {
			Error("Shell_NotifyIcon(NIM_ADD/NIM_MODIFY)","Failed to add tray icon.",GetLastError(),__LINE__);
			return 1;
		}
		
		//Success
		tray_added=1;
	}
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&traydata) == FALSE) {
		Error("Shell_NotifyIcon(NIM_DELETE)","Failed to remove tray icon.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Success
	tray_added=0;
	return 0;
}

void SetAutostart(int on, int hide) {
	//Open key
	HKEY key;
	int error;
	if ((error=RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&key)) != ERROR_SUCCESS) {
		Error("RegOpenKeyEx(HKEY_CURRENT_USER,'Software\\Microsoft\\Windows\\CurrentVersion\\Run')","Error opening the registry.",error,__LINE__);
		return;
	}
	if (on) {
		//Get path
		char path[MAX_PATH];
		if (GetModuleFileName(NULL,path,MAX_PATH) == 0) {
			Error("GetModuleFileName(NULL)","",GetLastError(),__LINE__);
			return;
		}
		//Add
		char value[MAX_PATH+10];
		sprintf(value,(hide?"\"%s\" -hide":"\"%s\""),path);
		if ((error=RegSetValueEx(key,L10N_NAME,0,REG_SZ,(LPBYTE)value,strlen(value)+1)) != ERROR_SUCCESS) {
			Error("RegSetValueEx('"L10N_NAME"')","",error,__LINE__);
			return;
		}
	}
	else {
		//Remove
		if ((error=RegDeleteValue(key,L10N_NAME)) != ERROR_SUCCESS) {
			Error("RegDeleteValue('"L10N_NAME"')","",error,__LINE__);
			return;
		}
	}
	//Close key
	RegCloseKey(key);
}

//Hooks
char* GetTimestamp(char *buf, size_t maxsize, char *format) {
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo=localtime(&rawtime);
	strftime(buf,maxsize,format,timeinfo);
	return buf;
}

void Kill(HWND hwnd) {
	fprintf(log,"%s ",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
	
	//Get hwnd title (for log)
	char title[100];
	GetWindowText(hwnd,title,100);
	
	//Get process id of hwnd
	DWORD pid;
	GetWindowThreadProcessId(hwnd,&pid);
	
	fprintf(log,"Killing \"%s\" (pid %d)... ",title,pid);
	
	int SeDebugPrivilege=0;
	//Get process token
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if (OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken) == 0) {
		fprintf(log,"failed to get SeDebugPrivilege.\n");
		fprintf(log,"Error: OpenProcessToken() failed (error: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
		fprintf(log,"Trying to kill without SeDebugPrivilege... ");
		fflush(log);
	}
	else {
		//Get LUID for SeDebugPrivilege
		LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
		tkp.PrivilegeCount=1;
		tkp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
		
		//Enable SeDebugPrivilege
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0); 
		if (GetLastError() != ERROR_SUCCESS) {
			fprintf(log,"failed to get SeDebugPrivilege.\n");
			fprintf(log,"Error: AdjustTokenPrivileges() failed (error: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
			fprintf(log,"Trying to kill without SeDebugPrivilege... ");
			fflush(log);
		}
		else {
			//Got it
			SeDebugPrivilege=1;
		}
	}
	
	//Open the process
	HANDLE process;
	if ((process=OpenProcess(PROCESS_TERMINATE,FALSE,pid)) == NULL) {
		fprintf(log,"failed!\n");
		fprintf(log,"Error: OpenProcess() failed (error: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
		fflush(log);
		return;
	}
	
	//Terminate process
	if (TerminateProcess(process,1) == 0) {
		fprintf(log,"failed!\n");
		fprintf(log,"Error: TerminateProcess() failed (error: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
		fflush(log);
		return;
	}
	
	//Disable SeDebugPrivilege
	if (SeDebugPrivilege) {
		tkp.Privileges[0].Attributes=0;
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
	}
	
	fprintf(log,"success!\n");
	fflush(log);
}

_declspec(dllexport) LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		int vkey=((PKBDLLHOOKSTRUCT)lParam)->vkCode;
		
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			//Check for Ctrl+Alt+F4
			if (vkey == VK_LCONTROL) {
				ctrl=1;
			}
			if (vkey == VK_LMENU) {
				alt=1;
			}
			if (ctrl && alt && vkey == VK_F4) {
				//Double check that Ctrl and Alt are being pressed
				//This prevents a faulty kill if we didn't received the keyup for these keys
				if (!(GetAsyncKeyState(VK_LCONTROL)&0x8000)) {
					ctrl=0;
				}
				else if (!(GetAsyncKeyState(VK_LMENU)&0x8000)) {
					alt=0;
				}
				else {
					//Get hwnd of foreground window
					HWND hwnd;
					if ((hwnd=GetForegroundWindow()) == NULL) {
						fprintf(log,"%s Error: GetForegroundWindow() failed in file %s, line %d.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"),__FILE__,__LINE__);
						fflush(log);
						return 0;
					}
					
					//Kill it!
					Kill(hwnd);
					
					//Prevent this keypress from being propagated
					return 1;
				}
			}
			//Check for [the windows key]+F4
			if (vkey == VK_LWIN) {
				win=1;
			}
			if (win && vkey == VK_F4) {
				//Double check that the windows button is being pressed
				if (!(GetAsyncKeyState(VK_LWIN)&0x8000)) {
					win=0;
				}
				else {
					//Hook mouse
					HookMouse();
					//Prevent this keypress from being propagated
					return 1;
				}
			}
			if (vkey == VK_ESCAPE && mousehook) {
				//Unhook mouse
				UnhookMouse();
				//Prevent this keypress from being propagated
				return 1;
			}
		}
		else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			if (vkey == VK_LCONTROL) {
				ctrl=0;
			}
			if (vkey == VK_LMENU) {
				alt=0;
			}
			if (vkey == VK_LMENU) {
				win=0;
			}
		}
	}
	
    return CallNextHookEx(NULL, nCode, wParam, lParam); 
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		if (wParam == WM_LBUTTONDOWN) {
			POINT pt=((PMSLLHOOKSTRUCT)lParam)->pt;
			
			//Get hwnd
			HWND hwnd;
			if ((hwnd=WindowFromPoint(pt)) == NULL) {
				fprintf(log,"%s Error getting mouse coordinates.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
				fprintf(log,"Error: WindowFromPoint() failed in file %s, line %d.\n",__FILE__,__LINE__);
			}
			hwnd=GetAncestor(hwnd,GA_ROOT);
			
			//Kill it!
			Kill(hwnd);
			
			//Unhook mouse
			UnhookMouse();
			
			//Prevent mousedown from propagating
			return 1;
		}
		else if (wParam == WM_RBUTTONDOWN) {
			//Unhook mouse
			UnhookMouse();
			//Prevent mousedown from propagating (this won't have any effect since the hook is removed by now)
			return 1;
		}
	}
	
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int HookMouse() {
	if (mousehook) {
		//Mouse already hooked
		return 1;
	}
	
	//Set up the mouse hook
	if ((mousehook=SetWindowsHookEx(WH_MOUSE_LL,LowLevelMouseProc,hinstDLL,0)) == NULL) {
		fprintf(log,"%s Error hooking mouse.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
		fprintf(log,"SetWindowsHookEx(WH_MOUSE_LL) failed (error code: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
		return 1;
	}
	
	//Success
	return 0;
}

int UnhookMouse() {
	if (!mousehook) {
		//Mouse not hooked
		return 1;
	}
	
	//Unhook the mouse hook
	if (UnhookWindowsHookEx(mousehook) == 0) {
		fprintf(log,"%s Error unhooking mouse.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
		fprintf(log,"UnhookWindowsHookEx() failed (error code: %d) in file %s, line %d.\n",GetLastError(),__FILE__,__LINE__);
		return 1;
	}
	
	//Success
	mousehook=NULL;
	return 0;
}


int HookKeyboard() {
	if (keyhook) {
		//Keyboard already hooked
		return 1;
	}
	
	//Open log
	log=fopen("superf4-log.txt","ab");
	
	//Load library
	char path[MAX_PATH]=L10N_NAME;
	GetModuleFileName(NULL, path, sizeof(path));
	if ((hinstDLL=LoadLibrary(path)) == NULL) {
		Error("LoadLibrary()","Check the "L10N_NAME" website if there is an update, if the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Get address to keyboard hook (beware name mangling)
	HOOKPROC procaddr;
	if ((procaddr=(HOOKPROC)GetProcAddress(hinstDLL,"LowLevelKeyboardProc@12")) == NULL) {
		Error("GetProcAddress('LowLevelKeyboardProc@12')","Check the "L10N_NAME" website if there is an update, if the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Set up the hook
	if ((keyhook=SetWindowsHookEx(WH_KEYBOARD_LL,procaddr,hinstDLL,0)) == NULL) {
		Error("SetWindowsHookEx(WH_KEYBOARD_LL)","Check the "L10N_NAME" website if there is an update, if the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Success
	fprintf(log,"\n%s New session.\n",GetTimestamp(txt,sizeof(txt),"[%Y-%m-%d %H:%M:%S]"));
	fflush(log);
	UpdateTray();
	return 0;
}

int UnhookKeyboard() {
	if (!keyhook) {
		//Keyboard not hooked
		return 1;
	}
	
	//Remove keyboard hook
	if (UnhookWindowsHookEx(keyhook) == 0) {
		Error("UnhookWindowsHookEx(keyhook)","Check the "L10N_NAME" website if there is an update, if the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Remove mouse hook (it probably isn't hooked, but just in case)
	UnhookMouse();
	
	//Unload library
	if (FreeLibrary(hinstDLL) == 0) {
		Error("FreeLibrary()","Check the "L10N_NAME" website if there is an update, if the latest version doesn't fix this, please report it.",GetLastError(),__LINE__);
		return 1;
	}
	
	//Success
	fclose(log); //Close log
	keyhook=NULL;
	UpdateTray();
	return 0;
}

void ToggleState() {
	if (keyhook) {
		UnhookKeyboard();
	}
	else {
		HookKeyboard();
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_ICONTRAY) {
		if (lParam == WM_LBUTTONDOWN) {
			ToggleState();
		}
		else if (lParam == WM_RBUTTONDOWN) {
			ShowContextMenu(hwnd);
		}
	}
	else if (msg == WM_ADDTRAY) {
		hide=0;
		UpdateTray();
	}
	else if (msg == WM_TASKBARCREATED) {
		tray_added=0;
		UpdateTray();
	}
	else if (msg == WM_COMMAND) {
		int wmId=LOWORD(wParam), wmEvent=HIWORD(wParam);
		if (wmId == SWM_TOGGLE) {
			ToggleState();
		}
		else if (wmId == SWM_HIDE) {
			hide=1;
			RemoveTray();
		}
		else if (wmId == SWM_AUTOSTART_ON) {
			SetAutostart(1,0);
		}
		else if (wmId == SWM_AUTOSTART_OFF) {
			SetAutostart(0,0);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_ON) {
			SetAutostart(1,1);
		}
		else if (wmId == SWM_AUTOSTART_HIDE_OFF) {
			SetAutostart(1,0);
		}
		else if (wmId == SWM_ABOUT) {
			MessageBox(NULL, L10N_ABOUT, L10N_ABOUT_TITLE, MB_ICONINFORMATION|MB_OK);
		}
		else if (wmId == SWM_EXIT) {
			DestroyWindow(hwnd);
		}
	}
	else if (msg == WM_DESTROY) {
		showerror=0;
		UnhookKeyboard();
		UnhookMouse();
		RemoveTray();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
