@echo off
setlocal enabledelayedexpansion

:: === CONFIGURATION ===
set "REPO_URL=https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS.git"
set "BRANCH=Tuna"
set "COMMIT_MSG=FORCED: Overwriting Tuna with local state"

echo.
echo === PS2 Tuna Branch FORCE-UPLOADER ===

:: === Check for Git ===
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Git is not installed or not in PATH.
    pause
    exit /b 1
)

:: === Check credential.helper ===
for /f "tokens=*" %%h in ('git config --global credential.helper 2^>nul') do set "HELPER=%%h"
if not defined HELPER (
    echo [INFO] Git credential helper not set. Enabling...
    git config --global credential.helper manager
)

:: === Init if needed ===
if not exist ".git" (
    echo Initializing Git repo...
    git init
)

:: === Set remote if missing ===
git remote get-url origin >nul 2>&1
if %errorlevel% neq 0 (
    echo Setting remote origin: %REPO_URL%
    git remote add origin %REPO_URL%
)

:: === Create or switch to local branch ===
git rev-parse --verify %BRANCH% >nul 2>&1
if %errorlevel% neq 0 (
    echo Creating local branch: %BRANCH%
    git checkout -b %BRANCH%
) else (
    echo Switching to branch: %BRANCH%
    git checkout %BRANCH%
)

:: === Stage and commit ===
echo Adding all files...
git add -A

git diff --cached --quiet
if %errorlevel% neq 0 (
    echo Committing changes...
    git commit -m "%COMMIT_MSG%"
) else (
    echo No new changes to commit.
)

:: === FORCED PUSH ===
echo Forcing push to %BRANCH%...
git push origin %BRANCH% --force

if %errorlevel% neq 0 (
    echo [ERROR] Push failed.
    pause
    exit /b 1
)

echo.
echo === FORCE PUSH COMPLETE: Remote Tuna branch now matches local ===
pause
endlocal
