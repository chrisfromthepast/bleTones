@echo off
REM bleTones Standalone App Launcher for Windows
REM This script installs dependencies and runs the Electron app

echo.
echo 🎵 bleTones - Starting Standalone App
echo.

REM Check if Node.js is installed
where node >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ❌ Node.js is not installed.
    echo Please install Node.js from https://nodejs.org/
    echo.
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('node --version') do set NODE_VERSION=%%i
echo ✅ Node.js found: %NODE_VERSION%

REM Check if npm is installed
where npm >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ❌ npm is not installed.
    echo Please install npm (usually comes with Node.js)
    echo.
    pause
    exit /b 1
)

for /f "tokens=*" %%i in ('npm --version') do set NPM_VERSION=%%i
echo ✅ npm found: %NPM_VERSION%
echo.

REM Check if node_modules exists
if not exist "node_modules" (
    echo 📦 Installing dependencies (this may take a few minutes)...
    call npm install
    if %ERRORLEVEL% neq 0 (
        echo.
        echo ❌ Failed to install dependencies
        pause
        exit /b 1
    )
    echo ✅ Dependencies installed successfully
    echo.
)

echo 🚀 Launching bleTones...
echo.
call npm start
