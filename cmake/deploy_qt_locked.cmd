@echo off
rem ============================================================================
rem deploy_qt_locked.cmd — 带文件锁的 windeployqt 串行化包装
rem ============================================================================
rem 根因: 多个测试 target 的 windeployqt 在 -jN 并行构建时, 同时向同一目录
rem       (build/tests/) 复制 Qt6Core.dll / Qt6Network.dll 等共享 DLL, 产生
rem       "Cannot create ... for output" 写竞争 (Build System Defect)。
rem
rem 修复: 用一个原子目录锁 (.deploy.lock) 串行化 windeployqt 调用。
rem       Windows 上 mkdir 是原子操作, 可作为自旋锁。
rem
rem 用法: deploy_qt_locked.cmd <lock_dir> <windeployqt> [windeployqt args...]
rem ============================================================================

setlocal

set "LOCK_DIR=%~1"
set "WINDEPLOYQT=%~2"
shift
shift

rem --- 自旋获取锁 (最多 60 秒) ---
set /a TRY=0
:acquire_lock
mkdir "%LOCK_DIR%" 2>nul
if not errorlevel 1 goto lock_acquired
set /a TRY+=1
if %TRY% GEQ 60 (
    echo [deploy_qt_locked] ERROR: timeout waiting for lock %LOCK_DIR% 1>&2
    exit /b 1
)
rem 约 1 秒等待 (ping 本机回环不依赖 stdin, 可安全用于非交互式构建)
ping -n 2 127.0.0.1 >nul
goto acquire_lock

:lock_acquired
rem --- 执行 windeployqt (先捕获退出码再释放锁) ---
rem 注意: 批处理的 %* 不受 shift 影响, 必须显式使用 shift 后的 %1..%9
rem windeployqt 参数固定为: --debug --no-translations --no-system-d3d-compiler [--test] <exe>
"%WINDEPLOYQT%" %1 %2 %3 %4 %5 %6 %7 %8 %9
set "DEPLOY_EXIT=%ERRORLEVEL%"

rem --- 释放锁 ---
rmdir "%LOCK_DIR%" 2>nul

exit /b %DEPLOY_EXIT%
