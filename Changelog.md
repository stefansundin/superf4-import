Check [info.txt](http://superf4.googlecode.com/svn/trunk/localization/en-US/info.txt) if there seems to be something missing. Note that info.txt can contain changes for upcoming versions.

**1.2** - 2010-10-23:
  * Added 64-bit executable. The installer will automatically install the correct executable.
  * Added support for alternative Ctrl+Alt+F4 detection (see `SuperF4.ini`). Enable this to make SuperF4 work in StarCraft II.
  * Added xkill to the tray menu (Win+F4).
  * Added manual update check to the tray menu.
  * Fixed the `Shell_NotifyIcon` error that appeared for some when SuperF4 was on autostart.

**1.1** - 2009-08-19:
  * Fixed mouse cursor not changing when pressing Win+F4 if Aero was disabled in Vista.
  * Mouse cursor now spans multiple monitors.
  * Language can now be changed in `SuperF4.ini`.
  * A lot of small bug fixes.

**1.0** - 2009-01-12:
  * Requests `SeDebugPrivilege` in a more secure way.
  * Merged `hooks.dll` into `SuperF4.exe`.
  * The mouse cursor now changes when you press Win+F4.
  * Removed log file.
  * Added update checking.

**0.9** - 2008-10-12:
  * You can now kill a process by pressing Win+F4 and then clicking the window with your mouse. Note that the mouse cursor does not change to reflect that SuperF4 is in this mode. You can press escape or the right mouse button to exit this mode without killing a program.
  * Renamed `keyhook.dll` to `hooks.dll`.
  * Renamed log file to `superf4-log.txt`.

**0.8** - 2008-09-16:
  * Made the detection of Ctrl+Alt+F4 more secure, preventing faulty kills which could happen when SuperF4 didn't receive the keyup of the F4 key.
  * Only one instance of SuperF4 can now be run. Starting SuperF4 again will make the first instance add its tray icon if hidden.

**0.7** - 2008-05-11:
  * `keyhook` now tries to get the `SeDebugPrivilege` privilege if `OpenProcess` fails, should be able to kill more programs.

**0.6** - 2008-03-15:
  * Small fix to prevent the key pressed from being propagated to another window after terminating the process.

**0.5** - 2008-03-05:
  * Fixed a rare case where SuperF4 would kill a program when the user pressed Alt+F4.

**0.4** - 2008-02-24:
  * Option to autostart SuperF4. Can hide tray icon in current session.

**0.3** - 2008-02-14:
  * The tray icon can be hidden with -hide as a parameter.

**0.2** - 2008-02-08:
  * Tray icon is now re-added when explorer crashes.

**0.1** - 2008-02-08:
  * Initial release.