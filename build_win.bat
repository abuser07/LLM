@echo off
setlocal
REM ============================================================
REM  Build do LLM Studio (Windows / MSVC + Qt6)
REM  Ajuste as variaveis abaixo para o seu ambiente.
REM ============================================================
set "VSVCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set "QT_DIR=C:\Qt\6.9.3\msvc2022_64"
set "CMAKE_BIN=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not exist "%QT_DIR%" (
    echo [ERRO] Qt nao encontrado em %QT_DIR%
    exit /b 1
)

cd /d "%~dp0"

echo [1/3] Configurando ambiente MSVC...
call "%VSVCVARS%" >nul 2>&1
if errorlevel 1 (
    echo [ERRO] Falha ao executar vcvars64.bat
    exit /b 1
)

echo [2/3] Configurando com CMake (Ninja, Release)...
"%CMAKE_BIN%" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
if errorlevel 1 exit /b 1

echo [3/3] Compilando...
"%CMAKE_BIN%" --build build
if errorlevel 1 exit /b 1

echo.
echo Build concluido: %CD%\build\llm_studio.exe
endlocal
