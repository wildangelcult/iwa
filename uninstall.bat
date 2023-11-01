@echo off

sc stop iwaclient
sc delete iwa
sc delete iwaclient

pushd "%systemroot%\system32"
echo @echo off > deleteIWA.bat
echo del %systemroot%\system32\drivers\iwa.sys >> deleteIWA.bat
echo del %systemroot%\system32\iwa.exe >> deleteIWA.bat
echo del %systemroot%\system32\iwa.log >> deleteIWA.bat
echo (goto) 2^>nul ^& del "%CD%\deleteIWA.bat" >> deleteIWA.bat

reg add HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce /v deleteIWA /t REG_SZ /d "%CD%\deleteIWA.bat"
popd

echo This script will now restart the computer.
pause

shutdown -r -f -t 0