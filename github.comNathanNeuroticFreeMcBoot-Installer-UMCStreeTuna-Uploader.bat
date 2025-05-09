@echo off
setlocal enabledelayedexpansion

:: === CONFIGURATION ===
set "REPO_URL=https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS.git"
set "BRANCH=Tuna"
set "COMMIT_MSG=Automated upload to Tuna branch"

echo.
echo === PS2 Tuna Branch Uploader ===

:: === Ensure Git is installed ===
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Git is not installed or not in PATH.
    pause
    exit /b 1
)

:: === Check credential.helper ===
for /f "tokens=*" %%h in ('git config --global credential.helper 2^>nul') do set "HELPER=%%h"

if not defined HELPER (
    echo [INFO] Git credential helper is not set.
    echo Enabling Git Credential Manager Core...
    git config --global credential.helper manager-core
    echo Done. You will be prompted to log in on first push.
) else (
    echo Credential helper detected: %HELPER%
)

:: === Initialize Git if needed ===
if not exist ".git" (
    echo Initializing new Git repository...
    git init
)

:: === Add remote if missing ===
git remote get-url origin >nul 2>&1
if %errorlevel% neq 0 (
    echo Adding remote origin: %REPO_URL%
    git remote add origin %REPO_URL%
) else (
    echo Remote origin already set.
)

:: === Switch to Tuna branch ===
git rev-parse --verify %BRANCH% >nul 2>&1
if %errorlevel% neq 0 (
    echo Creating branch %BRANCH%...
    git checkout -b %BRANCH%
) else (
    echo Switching to branch %BRANCH%...
    git checkout %BRANCH%
)

:: === Stage, commit, and push ===
echo Staging all changes...
git add -A

git diff --cached --quiet
if %errorlevel% neq 0 (
    echo Committing changes...
    git commit -m "%COMMIT_MSG%"
) else (
    echo No changes to commit.
)

echo Pushing to %BRANCH%...
git push -u origin %BRANCH%

echo.
echo === Upload complete ===
pause
endlocal
