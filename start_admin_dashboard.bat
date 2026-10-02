@echo off
setlocal
start "Glove B Dashboard" /D "%~dp0frontend" cmd /k python -m http.server 5174 --bind 127.0.0.1
timeout /t 2 /nobreak >nul
start "" "http://localhost:5174"
