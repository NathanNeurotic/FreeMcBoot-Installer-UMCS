@echo off
setlocal enabledelayedexpansion

:: ==== Configuration ====
set "REPO=https://github.com/NathanNeurotic/FreeMcBoot-Installer-UMCS.git"
set "BRANCH=Tuna"
set "COMMIT_MSG=OpenTuna integration commit"

:: ==== Initialization ====
echo [INIT] Setting up Git...
git init

echo [REMOTE] Setting remote origin...
git remote remove origin 2>nul
git remote add origin "%REPO%"

:: ==== Branch Setup ====
echo [BRANCH] Switching to branch: %BRANCH%
git fetch origin
git checkout -B %BRANCH%

:: ==== Staging Files ====
echo [STAGE] Adding all files...
git add .

:: ==== Committing ====
echo [COMMIT] Committing changes...
git commit -m "%COMMIT_MSG%"

:: ==== Pushing ====
echo [PUSH] Uploading to remote: %REPO%
git push -u origin %BRANCH% --force

echo.
echo ==============================================
echo ✓ Upload complete to branch [%BRANCH%]
echo ==============================================
pause
endlocal
