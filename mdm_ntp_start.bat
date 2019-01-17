@echo off
:: ver 1


set need_update=0


:: ---------------------------------------------------------
:: set HKLM\SYSTEM\CurrentControlSet\services\W32Time\TimeProviders\NtpServer\Enabled = 1

set KEY_NAME="HKLM\SYSTEM\CurrentControlSet\services\W32Time\TimeProviders\NtpServer"
set VALUE_NAME=Enabled
set REQUIRED_VALUE=0x1
:: REG QUERY %KEY_NAME% /v %VALUE_NAME% > qwe.txt


set REGVALUE=0x0
FOR /F "usebackq skip=2 tokens=1-3" %%A IN (`REG QUERY %KEY_NAME% /v %VALUE_NAME%  2^>nul`) DO (
    set ValueName=%%A
    set ValueType=%%B
    set ValueValue=%%C
)

if defined ValueName (
    echo Value Name = %ValueName%
    :: @echo Value Type = %ValueType%
    echo Value Value = %ValueValue%
	set REGVALUE=%ValueValue%
) else (
    echo %KEY_NAME%\%VALUE_NAME% not found.
)

:: echo %REGVALUE%

if %REGVALUE% NEQ %REQUIRED_VALUE% (
	:: net stop w32time
	echo set %VALUE_NAME% in registry to %REQUIRED_VALUE% ...
	REG ADD %KEY_NAME% /v %VALUE_NAME% /t REG_DWORD /d %REQUIRED_VALUE% /f
	set need_update=1
)



:: ---------------------------------------------------------
:: set HKLM\SYSTEM\CurrentControlSet\services\W32Time\Config\AnnounceFlags = 0x5
set KEY_NAME="HKLM\SYSTEM\CurrentControlSet\services\W32Time\Config"
set VALUE_NAME=AnnounceFlags
set REQUIRED_VALUE=0x5

set REGVALUE=0x0
FOR /F "usebackq skip=2 tokens=1-3" %%A IN (`REG QUERY %KEY_NAME% /v %VALUE_NAME%  2^>nul`) DO (
    set ValueName=%%A
    set ValueType=%%B
    set ValueValue=%%C
)

if defined ValueName (
    echo Value Name = %ValueName%
    :: @echo Value Type = %ValueType%
    echo Value Value = %ValueValue%
	set REGVALUE=%ValueValue%
) else (
    @echo %KEY_NAME%\%VALUE_NAME% not found.
)

:: echo %REGVALUE%

if %REGVALUE% NEQ %REQUIRED_VALUE% (
	:: net stop w32time
	echo set %VALUE_NAME% in registry to %REQUIRED_VALUE% ...
	REG ADD %KEY_NAME% /v %VALUE_NAME% /t REG_DWORD /d %REQUIRED_VALUE% /f
	set need_update=1
)


:: ---------------------------------------------------------
:: anyway, start the service

:: sc config w32time start=auto

if %need_update%==1 (
	echo restart service ...
	:: w32tm /config /update
	net stop w32time
)

sc config w32time start=auto
net start w32time
exit
