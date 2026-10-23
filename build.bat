@echo off
REM ============================================================
REM  Raiden UI - 构建脚本
REM  依赖：Visual Studio 2022 (MSVC v143) + Windows SDK 10.0
REM  产物：bin\RaidenTrainer.exe（UI 演示版，使用 mock 数据）
REM        如需接真实数据，把 hw_bridge_mock.cpp 换成你自己的实现
REM ============================================================
setlocal
cd /d "%~dp0"

REM ---- 定位 vcvars64.bat（按常见安装位置依次尝试）----
set "VCVARS="
for %%V in (
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
  if not defined VCVARS if exist %%V set "VCVARS=%%~V"
)
if not defined VCVARS goto :novs
call "%VCVARS%" >nul
goto :havevs

:novs
echo [x] 未找到 vcvars64.bat
echo     请安装 Visual Studio 2022 或 Build Tools，并勾选「使用 C++ 的桌面开发」
exit /b 1

:havevs

set IMG=third_party\imgui
if not exist obj mkdir obj
if not exist bin mkdir bin

set DEF=/DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN
set INC=/I. /I%IMG% /I%IMG%\backends
set CXXFLAGS=/nologo /utf-8 /std:c++17 /EHsc /O2 /MT /W3 /wd4996 %DEF% %INC%
set LIBS=user32.lib gdi32.lib d3d11.lib dxgi.lib dwmapi.lib shell32.lib ole32.lib

if "%1"=="clean" (
  if exist obj rmdir /s /q obj
  if exist bin rmdir /s /q bin
  echo [clean]
  exit /b 0
)

echo === [1/4] 资源（图标 + 版本信息）===
rc /nologo /fo obj\trainer.res trainer.rc || goto :fail

echo === [2/4] Dear ImGui 核心 ===
cl %CXXFLAGS% /c %IMG%\imgui.cpp         /Foobj\imgui.obj         || goto :fail
cl %CXXFLAGS% /c %IMG%\imgui_draw.cpp    /Foobj\imgui_draw.obj    || goto :fail
cl %CXXFLAGS% /c %IMG%\imgui_tables.cpp  /Foobj\imgui_tables.obj  || goto :fail
cl %CXXFLAGS% /c %IMG%\imgui_widgets.cpp /Foobj\imgui_widgets.obj || goto :fail

echo === [3/4] 后端（Win32 + D3D11）===
cl %CXXFLAGS% /c %IMG%\backends\imgui_impl_win32.cpp /Foobj\impl_win32.obj || goto :fail
cl %CXXFLAGS% /c %IMG%\backends\imgui_impl_dx11.cpp  /Foobj\impl_dx11.obj  || goto :fail

echo === [4/4] 应用 ===
cl %CXXFLAGS% /c raiden_main.cpp        /Foobj\raiden_main.obj   || goto :fail
cl %CXXFLAGS% /c hw_bridge_mock.cpp     /Foobj\hw_bridge.obj     || goto :fail

link /nologo /SUBSYSTEM:WINDOWS /OUT:bin\RaidenTrainer.exe ^
  obj\trainer.res ^
  obj\imgui.obj obj\imgui_draw.obj obj\imgui_tables.obj obj\imgui_widgets.obj ^
  obj\impl_win32.obj obj\impl_dx11.obj obj\raiden_main.obj obj\hw_bridge.obj ^
  %LIBS% || goto :fail

echo.
echo [ok] bin\RaidenTrainer.exe
exit /b 0

:fail
echo.
echo [x] 构建失败
exit /b 1
