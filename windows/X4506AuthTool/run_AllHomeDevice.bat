
copy /y Release\HomeDevice.exe .

set _argc=0
for %%i in (%*) do set /A _argc+=1

echo argc is %_argc%!

if %_argc% == 0 (
	.\HomeDevice.exe ..\..\data SystemAircon Light GasValve Curtain RemoteInspector DoorLock Vantilator Breaker PreventCrimeExt Boiler TemperatureController PowerGate
) else (
	.\HomeDevice.exe ..\..\data %1
)
