@echo off
setlocal
cd /d "%~dp0"

REM ================= 可改配置 =================
set "MQTT_BROKER=127.0.0.1"
set "MQTT_PORT=1883"
set "WEB_PORT=5000"
set "ADMIN_PORT=5174"
set "DEVICE_ID=board1"
set "SERIAL_PORT=COM7"
set "PYTHON=D:\Anaconda_envs\envs\pytorch\python.exe"
set "ENABLE_ASR_PRACTICE=1"
REM ===========================================

echo ======================================
echo   手语翻译手套 - 一键启动
echo ======================================
echo.

REM ---------- 1/4 MQTT Broker (只检测, 不代启动) ----------
echo [1/4] 检查 MQTT Broker (%MQTT_BROKER%:%MQTT_PORT%) ...
netstat -ano | findstr /c:":%MQTT_PORT%" | findstr /i "LISTENING" >nul 2>&1
if errorlevel 1 (
  echo       [警告] %MQTT_PORT% 端口无监听, 板子和后端都连不上.
  echo              以管理员身份执行一次:  net start mosquitto
) else (
  netstat -ano | findstr /c:"0.0.0.0:%MQTT_PORT%" | findstr /i "LISTENING" >nul 2>&1
  if errorlevel 1 (
    echo       [警告] 只监听 127.0.0.1, 局域网里的板子连不进来.
    echo              检查 F:\mosquitto\mosquitto.conf 里是否有 listener 1883 0.0.0.0
  ) else (
    echo       Broker 正常, 已监听全部网卡.
  )
)

REM ---------- 2/4 学习主站 (Flask :5000) ----------
netstat -ano | findstr /c:":%WEB_PORT%" | findstr /i "LISTENING" >nul 2>&1
if not errorlevel 1 (
  echo [2/4] 学习主站已在运行, 端口 %WEB_PORT% 被占用, 跳过.
) else (
  echo [2/4] 启动学习主站, 端口 %WEB_PORT% ...
  start "Glove-MQTT-Bridge" /D "%~dp0backend" cmd /k python mqtt_bridge.py
)
timeout /t 4 /nobreak >nul

REM ---------- 3/4 管理看板 (静态 :5174) ----------
netstat -ano | findstr /c:":%ADMIN_PORT%" | findstr /i "LISTENING" >nul 2>&1
if not errorlevel 1 (
  echo [3/4] 管理看板已在运行, 端口 %ADMIN_PORT% 被占用, 跳过.
) else (
  echo [3/4] 启动管理看板, 端口 %ADMIN_PORT% ...
  start "Glove-Admin-Dashboard" /D "%~dp0frontend" cmd /k python -m http.server %ADMIN_PORT% --bind 127.0.0.1
)
timeout /t 2 /nobreak >nul
echo [3.5/4] PC inference on %SERIAL_PORT% ...
start "Glove-PC-Inference" /D "%~dp0backend" cmd /k %PYTHON% pc_inference.py --port %SERIAL_PORT%
timeout /t 2 /nobreak >nul

REM ---------- 3.6/4 语音练习 (麦克风 -> 前端) ----------
REM 注意: 与 pc_inference 的语音模式(等 F4 发 0xFE)会抢麦克风,
REM       串口通了改用 F4 触发时, 把下面这行改成 ENABLE_ASR_PRACTICE=0
if "%ENABLE_ASR_PRACTICE%"=="1" (
  echo [3.6/4] 启动语音练习, 麦克风转文字送前端 ...
  start "Glove-ASR-Practice" /D "%~dp0backend" cmd /k %PYTHON% asr_practice.py
) else (
  echo [3.6/4] 语音练习已跳过 ^(ENABLE_ASR_PRACTICE=0^)
)
timeout /t 2 /nobreak >nul

REM ---------- 4/4 打开浏览器 ----------
echo [4/4] 打开浏览器 ...
start "" "http://localhost:%WEB_PORT%"
start "" "http://localhost:%ADMIN_PORT%"

REM ---------- 探测本机局域网 IP, 给板子填 ----------
REM 这台机器上装了 ZeroTier/Radmin/WSL/Hyper-V/代理虚拟网卡, 且它们的默认
REM 路由 metric 可能和真实网卡并列. 所以这里按"接口名 + 保留网段"排除虚拟网卡,
REM 再优先取 DHCP 地址(真实 WLAN/以太网), 避免打印出 198.18.x.x 这类代理地址.
set "LANIP="
set "IPFILE=%TEMP%\glove_lanip.txt"
powershell -NoProfile -Command "Get-NetIPAddress -AddressFamily IPv4 | Where-Object { $_.IPAddress -notmatch '^(127\.|169\.254\.|198\.18\.)' -and $_.InterfaceAlias -notmatch 'ZeroTier|Meta|Radmin|WSL|Hyper-V|Loopback|vEthernet' } | Sort-Object { if ($_.PrefixOrigin -eq 'Dhcp') { 0 } else { 1 } } | Select-Object -First 1 -ExpandProperty IPAddress" > "%IPFILE%" 2>nul
if exist "%IPFILE%" for /f "usebackq delims=" %%i in ("%IPFILE%") do set "LANIP=%%i"
del "%IPFILE%" >nul 2>&1
if not defined LANIP set "LANIP="
if "%LANIP%"=="" (
  echo        [提示] 没探测到局域网 IP, 下面会让你手动查.
)

echo.
echo ======================================
echo  学习主站 : http://localhost:%WEB_PORT%
echo  管理看板 : http://localhost:%ADMIN_PORT%
echo  Broker   : %MQTT_BROKER%:%MQTT_PORT%
echo  设备ID   : %DEVICE_ID%
echo --------------------------------------
if defined LANIP (
  echo  [板子填这个] %LANIP%:%MQTT_PORT%
) else (
  echo  [板子填这个] 用 ipconfig 查本机 IP, 再拼 :%MQTT_PORT%
  echo               注意别用 ZeroTier / Radmin / 198.18.x.x 这类虚拟网卡地址
)
echo --------------------------------------
if defined LLM_MODEL (
  echo  AI 模型  : %LLM_MODEL%  (已配置)
) else (
  echo  AI 模型  : 未配置 - 报告与对话只能用本地摘要
  echo             要开 AI 请改用 F:\glove\start_glove_ai.bat
)
echo ======================================
echo.
echo  停止: 关掉两个黑色窗口即可; Broker 是 Windows 服务, 不用管.
echo.
pause
