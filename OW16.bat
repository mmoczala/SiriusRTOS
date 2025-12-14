wmake -f C:\SpaceShadow\SiriusRTOS\OW16.mk -h -e C:\SpaceShadow\SiriusRTOS\OW16.exe
wdis C:\SpaceShadow\SiriusRTOS\Main.obj -l=C:\SpaceShadow\SiriusRTOS\disasm.s -s

pause

del *.obj
del *.sym
del *.lk1
del *.err

cls
debug OW16.exe

del OW16.exe
del *.map
del disasm.s
