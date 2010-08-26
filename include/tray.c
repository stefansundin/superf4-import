/*
	Tray functions.
	Copyright (C) 2010  Stefan Sundin (recover89@gmail.com)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
*/

NOTIFYICONDATA tray;
HICON icon[2];
int tray_added = 0;
int hide = 0;

int InitTray(int xmas) {
	//Load icons
	icon[0] = LoadImage(g_hinst, (xmas?L"tray_xmas_disabled":L"tray_disabled"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	icon[1] = LoadImage(g_hinst, (xmas?L"tray_xmas_enabled":L"tray_enabled"), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	if (icon[0] == NULL || icon[1] == NULL) {
		Error(L"LoadImage('tray_*')", L"Could not load tray icons.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Create icondata
	tray.cbSize = sizeof(NOTIFYICONDATA);
	tray.uID = 0;
	tray.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tray.hWnd = g_hwnd;
	tray.uCallbackMessage = WM_TRAY;
	//Balloon tooltip
	tray.uTimeout = 10000;
	wcsncpy(tray.szInfoTitle, APP_NAME, sizeof(tray.szInfoTitle)/sizeof(wchar_t));
	tray.dwInfoFlags = NIIF_USER;
	
	//Register TaskbarCreated so we can re-add the tray icon if (when) explorer.exe crashes
	WM_TASKBARCREATED = RegisterWindowMessage(L"TaskbarCreated");
	
	return 0;
}

int UpdateTray() {
	wcsncpy(tray.szTip, (enabled()?l10n->tray_enabled:l10n->tray_disabled), sizeof(tray.szTip)/sizeof(wchar_t));
	tray.hIcon = icon[enabled()?1:0];
	
	//Only add or modify if not hidden or if balloon will be displayed
	if (!hide || tray.uFlags&NIF_INFO) {
		//Try until it succeeds, sleep 100 ms between each attempt
		while (Shell_NotifyIcon((tray_added?NIM_MODIFY:NIM_ADD),&tray) == FALSE) {
			Sleep(100);
		}
		
		//Success
		tray_added = 1;
	}
	return 0;
}

int RemoveTray() {
	if (!tray_added) {
		//Tray not added
		return 1;
	}
	
	if (Shell_NotifyIcon(NIM_DELETE,&tray) == FALSE) {
		Error(L"Shell_NotifyIcon(NIM_DELETE)", L"Failed to remove tray icon.", GetLastError(), TEXT(__FILE__), __LINE__);
		return 1;
	}
	
	//Success
	tray_added = 0;
	return 0;
}
