build:
	cl /TC /LD main.c /link user32.lib /out:bin\x86\gvimtogglefullscreen.dll
	del *.obj
	del *.exp
	del *.lib
