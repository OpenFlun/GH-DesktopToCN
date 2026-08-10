# CMake 编译指南（VS Code 与 Visual Studio）

## 一、通用前置条件

- 已安装 **Visual Studio 2022 或更高版本**（含 Build Tools），安装时需包含 **C++ 桌面开发工作负荷**。
  *脚本会自动查找 `VsDevCmd.bat`，无需手动指定路径。*
- 已安装 **Qt（64 位，MSVC 工具链）**，推荐路径 `C:\Qt\版本号\msvc*_64`（例如 `C:\Qt\6.8.0\msvc2022_64`）。
  *脚本会自动扫描 `C:\Qt` 下所有符合 `数字.数字.数字` 格式的目录（如 `5.15.2`、`6.8.0`、`7.0.0`），选择版本号最高者，并匹配其中的 `msvc*_64` 子目录。无需手动配置路径。*
- 项目根目录：`%PROJECT%`（例如 `D:\E-lang\GH-DesktopToCN`）
- **确保 `openssl-proj\lib` 下存在 64 位 OpenSSL 的导入库与动态库**：
  - `libssl.lib`
  - `libcrypto.lib`
  - `libcrypto-3-x64.dll`
  - `libssl-3-x64.dll`
- 确保根目录存在 **`build.bat`**（内容见末尾附件）

> **注意**：所有路径均已实现自动检测，用户无需手动修改脚本中的路径。

---

## 二、使用 VS Code 构建

#### 所需文件
- `.vscode` 下存在两个配置文件：
  - `settings.json`（内容见末尾附件）
  - `tasks.json`（内容见末尾附件）

#### 操作步骤
1. 在 VS Code 中按 `Ctrl+Shift+B`，选择 **`Build x64`**。
2. 等待终端输出 `生成完成！`。
3. 构建成功后，可执行文件位于：
   `out\build\x64-debug\Debug\GitHubDesktopToCN.exe`

> **特点**：一键自动完成环境检测、配置、编译、复制 OpenSSL DLL、部署 Qt 动态库。
> **注意**：编译成功时终端自动关闭，失败则保留错误信息。如需保留窗口，将 `tasks.json` 中的 `"close": true` 改为 `"close": false`。

---

## 三、使用 Visual Studio IDE 构建

#### 所需文件
- `.vs` 目录下存在配置文件：
  - `tasks.vs.json`（内容见末尾附件）

#### 操作步骤
1. 在 Visual Studio 中右键单击项目根节点（主文件夹名称），选择菜单 **“运行 Build64”**。
2. 等待任务执行完毕，输出窗口显示 `生成完成！`。
3. 构建成功后，可执行文件位于：
   `out\build\x64-debug\Debug\GitHubDesktopToCN.exe`

> **特点**：与 VS Code 任务行为一致，一键完成全部构建和部署。
> **注意事项**：如需保留窗口，将 `tasks.vs.json` 中的 `'/c'` 改为 `'/k'`。

---

## 四、文件结构速查

```
%PROJECT%\
├── build.bat                     ← 一键构建脚本（自动检测环境）
├── CMakeLists.txt
├── openssl-proj\lib\
│   ├── libssl.lib
│   ├── libcrypto.lib
│   ├── libcrypto-3-x64.dll
│   └── libssl-3-x64.dll
├── .vscode\                      ← VS Code 构建所需
│   ├── settings.json
│   └── tasks.json
├── .vs\                          ← Visual Studio IDE 构建所需
│   └── tasks.vs.json
└── out\build\x64-debug\          ← 构建输出目录
    └── Debug\GitHubDesktopToCN.exe
```

---

## 五、构建流程说明（`build.bat` 自动完成）

1. **自动查找 Visual Studio 环境**：递归搜索 `VsDevCmd.bat`，取最新版本并加载 amd64 环境。
2. **自动查找 Qt 安装目录**：扫描 `C:\Qt` 下所有 `数字.数字.数字` 格式的目录，选择版本号最高者，并匹配其中的 `msvc*_64` 子目录。
3. **检查并清理无效编译器缓存**：若 CMake 缓存中的 `cl.exe` 路径已失效，自动删除缓存并重新配置。
4. **增量编译尝试**：若缓存有效，尝试增量编译；若失败或路径变更则自动清理并全新配置。
5. **配置与构建**：使用 Ninja 生成器（避免 Visual Studio 版本字符串硬编码），构建 Debug 版本。
6. **部署 Qt 动态库**：
   - 关闭可能占用文件的 `GitHubDesktopToCN.exe` 进程。
   - 备份生成的 exe → 清空 Debug 目录 → 恢复 exe（解决 DLL 被占用问题）。
   - 复制 OpenSSL DLL。
   - 调用 `windeployqt` 部署所有 Qt 依赖。
7. 输出 `生成完成！`

---

## 六、常见问题

| 现象                                   | 原因                               | 解决                                                                                                                         |
| -------------------------------------- | ---------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| 提示“找不到 VsDevCmd.bat”              | Visual Studio 未安装或未在默认路径 | 确保已安装 Visual Studio 或 Build Tools，且位于 `C:\Program Files` 下                                                        |
| 提示“找不到合适的 Qt 安装目录”         | Qt 未安装或不符合 `msvc*_64` 结构  | 安装任意版本 Qt（如 5.15.2、6.8.0 等），确保路径为 `C:\Qt\版本号\msvc*_64`，且包含 `msvc*_64` 子目录（例如 `msvc2022_64`）。 |
| 配置阶段报“找不到 cli11_proj-src”      | CLI11 下载失败                     | 手动下载 v2.4.1 源码解压到 `out/build/x64-debug/_deps/cli11_proj-src`                                                        |
| 运行时提示“找不到 libcrypto-3-x64.dll” | OpenSSL DLL 未复制                 | 检查 `openssl-proj\lib` 下是否存在该 DLL，或手动复制到 exe 目录                                                              |
| VS Code 终端一闪而过                   | 成功时按配置自动关闭               | 临时修改 `tasks.json` 中 `"close": true` 为 `"close": false` 后重试                                                          |
| Visual Studio 终端一闪而过             | 成功时按配置自动关闭               | 临时修改 `tasks.vs.json` 中 `'\c'` 为 `'\k'` 后重试                                                                          |

---

## 七、重要提醒

- **不要手动修改 `build.bat` 以及 `.vscode`、`.vs` 下的配置文件**，除非明确知道后果。
- 脚本已实现 Visual Studio 和 Qt 的自动适配，**无需因版本升级而手动修改路径**。
- 项目仅支持 **64 位构建**。
- `.vscode/settings.json` 中已禁用 CMake Tools 自动配置，请勿开启。

---

## 附件：关键文件内容

### 1. `build.bat`（位于项目根目录）

```batch
@echo off
setlocal enabledelayedexpansion

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

:: ===== 自动查找 Visual Studio 的 VsDevCmd.bat =====
set "VSDEVCMD="
for /f "delims=" %%i in ('dir /s /b /o-d "C:\Program Files (x86)\Microsoft Visual Studio\VsDevCmd.bat" 2^>nul') do set "VSDEVCMD=%%i" & goto :found_vs
for /f "delims=" %%i in ('dir /s /b /o-d "C:\Program Files\Microsoft Visual Studio\VsDevCmd.bat" 2^>nul') do set "VSDEVCMD=%%i" & goto :found_vs
:found_vs
if not defined VSDEVCMD (
    echo 错误: 找不到 VsDevCmd.bat
    exit /b 1
)
echo 使用 Visual Studio 环境: %VSDEVCMD%
call "%VSDEVCMD%" -arch=amd64
if errorlevel 1 ( echo 环境加载失败！ & exit /b 1 )

:: ===== 自动查找 Qt 安装目录 =====
set "QT_DIR="
for /f "delims=" %%i in ('powershell -Command "$dirs=Get-ChildItem 'C:\Qt' -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' }; $latest=$dirs | Sort-Object { [version]($_.Name) } -Descending | Select-Object -First 1; if($latest){$latest.FullName}"') do set "QT_BASE=%%i"
if defined QT_BASE (
    for /f "delims=" %%j in ('dir /b /ad "%QT_BASE%\msvc*_64" 2^>nul ^| sort /r') do (
        if not defined QT_DIR set "QT_DIR=%QT_BASE%\%%j"
    )
)
if not defined QT_DIR (
    echo 错误: 找不到合适的 Qt 安装目录（需包含 msvc*_64 子目录）
    exit /b 1
)
echo 使用 Qt 目录: %QT_DIR%

:: ===== OpenSSL DLL 目录（项目内）=====
set "OPENSSL_DLL_DIR=%PROJECT_DIR%\openssl-proj\lib"
if not exist "%OPENSSL_DLL_DIR%" echo 警告: OpenSSL DLL 目录不存在: %OPENSSL_DLL_DIR%

:: ===== 构建配置 =====
set "BUILD_DIR=out\build\x64-debug"
set "CACHE_FILE=%PROJECT_DIR%\%BUILD_DIR%\CMakeCache.txt"
:: ★ 修改1：exe 实际在构建根目录，不是 Debug 子目录
set "EXE_FILE=%PROJECT_DIR%\%BUILD_DIR%\GitHubDesktopToCN.exe"

:: 检测无效编译器缓存
if exist "%CACHE_FILE%" (
    set "NEED_CLEAN=0"
    for /f "tokens=*" %%i in ('type "%CACHE_FILE%" ^| findstr /c:"CMAKE_C_COMPILER:FILEPATH=" 2^>nul') do (
        set "CACHE_COMPILER=%%i"
        set "CACHE_COMPILER=!CACHE_COMPILER:*CMAKE_C_COMPILER:FILEPATH=!"
        set "CACHE_COMPILER=!CACHE_COMPILER:~1!"
        if defined CACHE_COMPILER if not exist "!CACHE_COMPILER!" set "NEED_CLEAN=1"
    )
    if "!NEED_CLEAN!"=="1" (
        echo 检测到无效编译器缓存，正在清理...
        del /f /q "%CACHE_FILE%" 2>nul
        if exist "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles" rmdir /s /q "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles" 2>nul
        goto :configure
    )
)

if not exist "%CACHE_FILE%" goto :configure

:: 增量编译尝试
echo 检测到缓存，尝试增量编译...
set "BUILD_LOG=%TEMP%\gh_build_%RANDOM%.log"
cmake --build "%PROJECT_DIR%\%BUILD_DIR%" --config Debug > "%BUILD_LOG%" 2>&1
set "BUILD_ERR=%ERRORLEVEL%"

findstr /C:"different than the directory" "%BUILD_LOG%" >nul 2>&1
if %ERRORLEVEL% equ 0 set "PATH_CHANGED=1"

if defined PATH_CHANGED (
    del /q "%BUILD_LOG%" 2>nul
    echo 项目路径已变更，重新配置...
    goto :clean_and_configure
)

type "%BUILD_LOG%"
del /q "%BUILD_LOG%" 2>nul

if %BUILD_ERR% neq 0 goto :clean_and_configure
if not exist "%EXE_FILE%" goto :clean_and_configure
goto :deploy

:clean_and_configure
if exist "%CACHE_FILE%" del /q "%CACHE_FILE%" 2>nul
rmdir /s /q "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles" 2>nul
echo 已清除旧缓存，开始全新配置...

:configure
cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\%BUILD_DIR%" ^
      -G Ninja ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TLS_VERIFY=0 ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%"
if errorlevel 1 ( echo 配置失败！ & exit /b 1 )

cmake --build "%PROJECT_DIR%\%BUILD_DIR%" --config Debug
if errorlevel 1 ( echo 构建失败！ & exit /b 1 )

:deploy
echo 开始部署 Qt 动态库...
:: 部署目标仍是 Debug 子目录（用于放置 DLL 和依赖）
set "DEPLOY_TARGET_DIR=%PROJECT_DIR%\%BUILD_DIR%\Debug"
if not exist "%EXE_FILE%" ( echo 错误：未找到可执行文件 & exit /b 1 )

:: 关闭可能占用文件的进程
taskkill /f /im GitHubDesktopToCN.exe >nul 2>&1
timeout /t 1 /nobreak >nul

cd /d "%PROJECT_DIR%"

:: 备份 exe 文件（从根目录）
set "EXE_BACKUP=%TEMP%\GitHubDesktopToCN_backup.exe"
copy /y "%EXE_FILE%" "%EXE_BACKUP%" >nul
if errorlevel 1 ( echo 备份 exe 失败 & exit /b 1 )

:: 清空整个 Debug 目录（删除所有文件）
echo 清空部署目录...
rmdir /s /q "%DEPLOY_TARGET_DIR%" 2>nul
mkdir "%DEPLOY_TARGET_DIR%"

:: 重新设置 EXE_FILE 为 Debug 子目录下的路径
set "EXE_FILE=%DEPLOY_TARGET_DIR%\GitHubDesktopToCN.exe"

:: 恢复 exe 到 Debug 目录
move /y "%EXE_BACKUP%" "%EXE_FILE%" >nul
if not exist "%EXE_FILE%" ( echo 恢复 exe 失败 & exit /b 1 )

:: 切换到 Debug 目录
cd /d "%DEPLOY_TARGET_DIR%"

:: 复制项目内的 OpenSSL DLL
if exist "%OPENSSL_DLL_DIR%" (
    for %%F in ("%OPENSSL_DLL_DIR%\libcrypto*.dll" "%OPENSSL_DLL_DIR%\libssl*.dll") do (
        if exist "%%F" ( echo 复制 %%~nxF ... & copy /y "%%F" . >nul )
    )
) else ( echo 警告: 未找到 OpenSSL DLL 目录 )

:: 运行 windeployqt
call "%QT_DIR%\bin\windeployqt.exe" "GitHubDesktopToCN.exe" --no-translations --no-system-d3d-compiler --compiler-runtime --force-openssl
if errorlevel 1 ( echo windeployqt 执行失败！ & cd /d "%PROJECT_DIR%" & exit /b 1 )

cd /d "%PROJECT_DIR%"
echo 生成完成！
exit /b 0
```

### 2. `.vscode/tasks.json`

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build x64",
            "type": "shell",
            "command": "${workspaceFolder}/build.bat",
            "args": ["x64"],
            "group": "build",
            "problemMatcher": "$msCompile",
            "presentation": {
                "reveal": "always",
                "panel": "shared",
                "showReuseMessage": false,
                "close": true
            }
        }
    ]
}
```

### 3. `.vscode/settings.json`

```json
{
    "cmake.configureOnOpen": false,
    "cmake.configureOnEdit": false,
    "cmake.allowDebugger": false,
    "cmake.configureEnvironment": {
        "CMAKE_TLS_VERIFY": "0"
    }
}
```

### 4. `.vs/tasks.vs.json`

```json
{
    "version": "0.2.1",
    "tasks": [
        {
            "taskLabel": "Build64",
            "appliesTo": "/",
            "type": "default",
            "command": "pwsh.exe",
            "args": [
                "-Command",
                "Start-Process cmd.exe -ArgumentList '/c', 'build.bat' -WindowStyle Normal -WorkingDirectory '${workspaceRoot}'"
            ],
            "workingDirectory": "${workspaceRoot}"
        }
    ]
}
```

---

**文档版本**：2026-06-01
**对应脚本版本**：build.bat v2（支持 VS/Qt 自动检测、Ninja 生成器、编译器缓存自动修复）