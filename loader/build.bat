@echo off
rem FATAL FRAME II: Crimson Butterfly REMAKE - MixedNuts Mod Loader
rem Builds dist\ containing the files users drop into the game root.
setlocal
set "VS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VS%" (echo [NG] vcvars64.bat not found: %VS% & exit /b 1)
call "%VS%" >nul
if errorlevel 1 exit /b 1

set "ROOT=%~dp0"
set "REPO=%ROOT%.."
set "OUT=%ROOT%dist"
set "OBJ=%ROOT%obj"
set "INC=/I"%REPO%\common" /I"%REPO%\api""
if not exist "%OUT%\MixedNuts\Mods" mkdir "%OUT%\MixedNuts\Mods"
if not exist "%OBJ%" mkdir "%OBJ%"

echo === proxy (dinput8.dll) ===
cl /nologo /LD /O2 /EHsc /MT /W3 /std:c++17 /utf-8 /DNDEBUG %INC% /Fo"%OBJ%\x_" /Fe"%OUT%\dinput8.dll" "%ROOT%proxy\proxy.cpp" /link /DEF:"%ROOT%proxy\proxy.def" /OPT:REF /OPT:ICF
if errorlevel 1 exit /b 1

echo === loader (MixedNutsLoader.dll) ===
cl /nologo /LD /O2 /EHsc /MT /W3 /std:c++17 /utf-8 /DNDEBUG %INC% /Fo"%OBJ%\c_" /Fe"%OUT%\MixedNuts\MixedNutsLoader.dll" "%ROOT%core\loader.cpp" /link /OPT:REF /OPT:ICF
if errorlevel 1 exit /b 1

echo === copying package files ===
copy /y "%ROOT%package\MixedNuts\loader.ini" "%OUT%\MixedNuts\" >nul
copy /y "%REPO%\LICENSE" "%OUT%\MixedNuts\LICENSE.txt" >nul

rem import library / export file are build by-products
for %%f in ("%OUT%\dinput8.lib" "%OUT%\dinput8.exp" "%OUT%\MixedNuts\MixedNutsLoader.lib" "%OUT%\MixedNuts\MixedNutsLoader.exp") do if exist %%f del %%f

echo.
echo === done: %OUT% ===
dir /b /s "%OUT%"
endlocal
