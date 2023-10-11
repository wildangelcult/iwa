sc stop iwaclient
sc stop iwa
sc delete iwa
sc delete iwaclient
del %systemroot%\system32\drivers\iwa.sys
del %systemroot%\system32\iwa.exe
del %systemroot%\system32\iwa.log