@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

:: --- Usage -------------------------------------------------------------------
:: build.bat [compiler] [mode] [options]
::   compiler: msvc (default) | clang
::   mode:     debug (default) | release
::   options:  asan   enable address sanitizer
:: Compiles only (no link): produces out\main.obj
:: Examples:  build.bat | build.bat clang release | build.bat msvc debug

for %%a in (%*) do set "%%~a=1"
if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
if not "%release%"=="1" set debug=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0   && echo [release mode]
if "%msvc%"=="1"    set clang=0   && echo [msvc compile]
if "%clang%"=="1"   set msvc=0    && echo [clang compile]

set auto_compile_flags=
if "%asan%"=="1" set auto_compile_flags=-fsanitize=address && echo [asan enabled]

:: --- Compile/Link Lines ------------------------------------------------------
set cl_common=     /I..\..\src /nologo /FC /Z7 /Zc:preprocessor /W3 /wd4090 /wd4244 /wd4267 /wd4996 /wd4018 /wd4146
set cl_debug=      call cl /c /Od /Ob1 /DBUILD_DEBUG=1 %cl_common% %auto_compile_flags%
set cl_release=    call cl /c /O2 /DBUILD_DEBUG=0 %cl_common% %auto_compile_flags%
set cl_out=        /Fo:
set clang_common=  -mcx16 -I..\..\src -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-unused-parameter -Wno-writable-strings -Wno-missing-field-initializers -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf -ferror-limit=10000 -gcodeview
set clang_debug=   call clang -c -g -O0 -DBUILD_DEBUG=1 -D_DEBUG %clang_common% %auto_compile_flags%
set clang_release= call clang -c -g -O2 -DBUILD_DEBUG=0 -DNDEBUG %clang_common% %auto_compile_flags%
set clang_out=     -o

if "%msvc%"=="1"    set out=%cl_out%
if "%clang%"=="1"   set out=%clang_out%
if "%msvc%"=="1"  if "%debug%"=="1"   set compile=%cl_debug%
if "%msvc%"=="1"  if "%release%"=="1" set compile=%cl_release%
if "%clang%"=="1" if "%debug%"=="1"   set compile=%clang_debug%
if "%clang%"=="1" if "%release%"=="1" set compile=%clang_release%

:: --- MSVC Environment --------------------------------------------------------
if not "%msvc%"=="1" goto skip_msvc_setup
where cl >nul 2>nul && goto skip_msvc_setup
set "vcvars="
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "vcvars=%%i\VC\Auxiliary\Build\vcvars64.bat"
if not defined vcvars for /d %%d in ("%ProgramFiles%\Microsoft Visual Studio\*") do for %%e in (Community Professional Enterprise BuildTools) do if exist "%%d\%%e\VC\Auxiliary\Build\vcvars64.bat" set "vcvars=%%d\%%e\VC\Auxiliary\Build\vcvars64.bat"
if not defined vcvars goto skip_msvc_setup
echo [setting up msvc environment]
call "%vcvars%" >nul
:skip_msvc_setup

:: --- Build -------------------------------------------------------------------
if not exist out mkdir out
if "%msvc%"=="1"  where cl >nul 2>nul    || (echo [error] cl.exe not found - run from a Developer Command Prompt & exit /b 1)
if "%clang%"=="1" where clang >nul 2>nul || (echo [error] clang not found in PATH & exit /b 1)

pushd out
%compile% ../../src/main.c %out%main.obj
if errorlevel 1 (
  echo [build failed]
  popd
  exit /b 1
)
popd
echo [build ok: out\main.obj]
