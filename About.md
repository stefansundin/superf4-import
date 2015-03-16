# Introduction #

## What does SuperF4 do? ##

SuperF4 kills the foreground program when you press Ctrl+Alt+F4. This is different from when you press Alt+F4. When you press Alt+F4, the program can refuse to quit. Windows essentially only asks the program to quit, and lets it decide for itself what to do.

You can also kill a program by pressing Win+F4 and then clicking the window with your mouse cursor. When you press Win+F4, the cursor will change to a skull and crossbones cursor, and the next window you click will be killed. You can press escape or the right mouse button to exit this mode without killing a program.

## Why would I need SuperF4? ##

You might have encountered a game that hung because of some stupid bug, then your computer gets all slow and you just can't get out of the game since it's fullscreen. Nothing happens when you press Ctrl+Alt+Delete to bring up the task manager. You eventually give up and reboot the computer by pressing the power button.

This has happened to me quite a few times and I got tired of it, so I made SuperF4. Now I can just press Ctrl+Alt+F4 and get back to the desktop within a few seconds. :)

SuperF4 does of course work just as well with normal programs, and not only games.

## More ##

SuperF4 should be able to kill all kinds of processes, whether they are hung or not.
Use with care though, since SuperF4 effectively kills the program and doesn't give it any chance to save unsaved work.

## Video review ##
SuperF4 has been featured in [Tekzilla](http://revision3.com/tekzilla), so you can watch [this video](http://www.youtube.com/watch?v=LLtxSkoWKfc) to see how SuperF4 works before downloading it.

<wiki:gadget border="0" url="https://gist.githubusercontent.com/stefansundin/8194147/raw/youtube-iframe.xml" up\_id="LLtxSkoWKfc" up\_args="start=8&rel=0" width="640" height="390" />


# Configuration #

There are a few features that you can configure in the configuration file called `SuperF4.ini`. The simplest way to open it is through the tray menu.

## TimerCheck ##

If you enable this setting, then SuperF4 will also check if Ctrl+Alt+F4 is depressed with the help of a timer. Some applications prevent other programs from detecting keystrokes, often as a means to protect against keyloggers. Unfortunately, this will also prevents SuperF4 from detecting Ctrl+Alt+F4 with the help of its keyboard hook. If you have a program you can't kill with Ctrl+Alt+F4, try enabling this.

A game that requires this is StarCraft II.

## Language ##

This settings lets you change the language of SuperF4. You can currently choose between English, Spanish and Galician.

If you want to translate SuperF4, feel free to email me, but please only do this if you want to commit to keeping your translation up to date in the future.

## Update ##

SuperF4 will by default check if there is a newer version available when it is started. It will only notify you if it finds a new version, it will not download anything for you. If you do not like this behavior, you can disable it here. You can check for update manually in the tray menu, regardless of this setting.


# UAC #

SuperF4 does not require administrator privileges. The only case it needs administrator privileges is when you are trying to kill an elevated program. This means that, if SuperF4 is not run with administrator privileges, then you will not be able to use Ctrl+Alt+F4 or Win+F4 if the foreground window is running elevated.

If you installed SuperF4 in `C:\Program Files\`, then you need administrator privileges to edit `SuperF4.ini`. The easiest way to fix this is by right clicking on `SuperF4.ini`, click _Properties_, then go to the _Security_ tab and give the _Users_ group _Full control_ to the file.

If you want SuperF4 to launch with administrator privileges on startup, then you have to configure the task scheduler to launch SuperF4 on log in. Read [this blog post](http://botsikas.blogspot.com/2010/05/autostart-application-that-requires-uac.html) for a tutorial. There is one caveat though, you need to configure **a delay of 30 seconds** before the task is started. Otherwise the tray icon won't be added properly and SuperF4 will not work. You must also disable the autostart option in the tray menu. Successfully configuring a task like this will allow SuperF4 to be launched on startup with administrator privileges without a UAC prompt.