## 一、前置条件（已完成可跳过）
- 已安装 **Visual Studio 2026 Build Tools**，路径：
  `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`
- 项目根目录：`D:\E-lang\GH-DesktopToCN`（以下用 `%PROJECT%` 代指）
- **确保 `openssl-proj\lib` 下有 64 位 OpenSSL 的导入库与动态库**：
  - `libssl.lib`
  - `libcrypto.lib`
  - `libcrypto-3-x64.dll`
  - `libssl-3-x64.dll`
- 确保 `.vscode` 下有两个配置文件：`settings.json` 和 `tasks.json`
- 确保根目录存在 **`build.bat`**（内容见附件）
*** 请根据自己的电脑配置替换成自己的相应文件路径 ***
---

## 二、文件结构速查
```
D:\E-lang\GH-DesktopToCN\
├── build.bat                  ← 一键构建脚本（加载环境、配置、编译、部署 Qt、复制 OpenSSL DLL）
├── CMakeLists.txt
├── CMakePresets.json
├── openssl-proj\
│   └── lib\
│       ├── libssl.lib         (64位导入库)
│       ├── libcrypto.lib      (64位导入库)
│       ├── libcrypto-3-x64.dll
│       └── libssl-3-x64.dll
└── .vscode\
    ├── settings.json
    └── tasks.json
```

---

## 三、首次配置（只需一次）
1. **CLI11 依赖**（自动处理）
   - CLI11 会在 CMake 配置阶段通过 `FetchContent` 自动从 GitHub 下载，无需手动操作。
   - 如果你的网络无法直接访问 GitHub，CMake 配置可能会失败。此时可以：
     - 设置代理环境变量 `HTTP_PROXY` / `HTTPS_PROXY`，或
     - 手动下载 CLI11 源码（v2.4.1）并解压到 `out/build/x64-debug/_deps/cli11_proj-src` 目录。
2. 在 VS Code 中按 `Ctrl+Shift+B`，选择 **`Build x64`**，等待配置和编译通过。
3. 构建成功后，可执行文件位于：
   `out\build\x64-debug\Debug\GitHubDesktopToCN.exe`

---

## 四、日常构建（一键构建）
1. 在 VS Code 中按 ** `Ctrl+Shift+B` 或 菜单中点击 ... ->终端 ->点击 运行生成任务**。
2. 在弹出的任务列表中选择 **`Build x64`**（当前唯一任务）。
3. 等待终端输出 `=== 生成完成 ===`，exe 位置同上。

> **注意**：
> - 编译成功 → 终端自动关闭，不留痕。
> - 编译失败 → 终端保留错误信息，按任意键关闭。

---

## 五、构建流程说明
`build.bat` 会自动完成以下步骤：
1. 加载 Visual Studio 2026 编译环境（amd64）。
2. 检查 CMake 缓存，若源目录变化则自动清理旧缓存。
3. 调用 CMake 配置（生成 Visual Studio 工程，架构固定为 x64）。
4. 执行构建（Debug 模式）。
5. **自动从 `openssl-proj\lib` 复制 OpenSSL 动态库**。
6. 自动调用 `windeployqt` 部署所需的 Qt 动态库。

**不需要手动切换库文件、不需要清除缓存。**

---

## 六、常见问题
| 现象                                         | 原因                 | 解决                                                                                                                  |
| -------------------------------------------- | -------------------- | --------------------------------------------------------------------------------------------------------------------- |
| 构建时提示“无法解析的外部符号 OPENSSL_...”   | 导入库文件缺失或损坏 | 检查 `openssl-proj\lib\libssl.lib` 和 `libcrypto.lib` 是否存在且为有效 64 位导入库。                                  |
| 运行时提示“找不到 libcrypto-3-x64.dll”       | 动态库未正确复制     | 检查 `build.bat` 中复制 DLL 的步骤是否执行；或手动将 DLL 复制到 exe 所在目录。                                        |
| 配置阶段报“找不到 cli11_proj-src”            | CLI11 下载失败       | 手动从 `https://github.com/CLIUtils/CLI11` 下载 v2.4.1 源码，解压到 `out\build\x64-debug\_deps\cli11_proj-src` 目录。 |
| 终端一闪而过，没看到错误                     | 成功时终端自动关闭   | 在 `tasks.json` 中临时将 `"close": true` 改为 `false`，复现后查看输出。                                               |
| 按 `Ctrl+Shift+B` 没反应或直接执行不弹出选择 | 默认任务被锁定       | 检查 `tasks.json` 中 `Build x64` 是否误设 `"isDefault": true`，删掉该行即可。                                         |

---

## 七、重要提醒
- **永远不要手动修改 `build.bat` 和 `tasks.json`**，除非你确切知道后果。
- **若重装系统或移动项目**，请重新确认 Visual Studio Build Tools 路径，并相应修改 `build.bat` 中的 `VsDevCmd.bat` 路径。
- **`.vscode/settings.json` 中已禁用 CMake Tools 自动配置**，请勿开启，否则可能重现卡死。
- **项目现在仅支持 64 位构建**，不再保留 32 位库和备份文件。

---

## 附件：关键文件内容

### `build.bat`（位于项目根目录）
```bat
@echo off
setlocal enabledelayedexpansion

set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

:: ===== 加载编译环境 =====
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64
if errorlevel 1 (
    echo 环境加载失败！
    exit /b 1
)

:: ===== 固定 64 位架构 =====
set "ARCH=x64"
set "BUILD_DIR=out\build\x64-debug"
set "QT_DIR=C:\Qt\6.11.0\msvc2022_64"
set "OPENSSL_DLL_DIR=%PROJECT_DIR%\openssl-proj\lib"

if not exist "%QT_DIR%" (
    echo 错误: 未找到 64 位 Qt 安装目录: %QT_DIR%
    echo 请安装 Qt 6.11.0 的 x64 版本，或修改脚本中的路径。
    exit /b 1
)

:: ===== 自动检测并清理不一致的 CMake 缓存 =====
set "CACHE_FILE=%PROJECT_DIR%\%BUILD_DIR%\CMakeCache.txt"
if exist "%CACHE_FILE%" (
    echo 检测到已有 CMake 缓存，正在验证源目录一致性...

    set "CACHED_SRC="
    for /f "usebackq tokens=1* delims==" %%a in (`cmake -LA -N "%PROJECT_DIR%\%BUILD_DIR%" 2^>nul ^| findstr /b "CMAKE_SOURCE_DIR"`) do (
        set "CACHED_SRC=%%b"
    )

    if defined CACHED_SRC (
        set "CACHED_SRC=!CACHED_SRC:"=!"
        set "CACHED_SRC=!CACHED_SRC:/=\!"
        if "!CACHED_SRC:~-1!"=="\" set "CACHED_SRC=!CACHED_SRC:~0,-1!"

        if /i not "!CACHED_SRC!"=="%PROJECT_DIR%" (
            echo [警告] 缓存记录的源目录 !CACHED_SRC! 与当前目录 %PROJECT_DIR% 不一致。
            echo 正在清除旧的 CMake 缓存文件（保留 _deps 等已下载内容）...
            if exist "%CACHE_FILE%" del /q "%CACHE_FILE%"
            if exist "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles" rmdir /s /q "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles"
        ) else (
            echo 缓存目录一致，继续使用现有缓存。
        )
    ) else (
        echo [警告] 无法从缓存读取源目录（缓存可能已损坏），将清除缓存以确保正常...
        if exist "%CACHE_FILE%" del /q "%CACHE_FILE%"
        if exist "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles" rmdir /s /q "%PROJECT_DIR%\%BUILD_DIR%\CMakeFiles"
    )
)

:: ===== CMake 配置 =====
echo === 开始配置 x64 项目 ===
cmake -S "%PROJECT_DIR%" -B "%PROJECT_DIR%\%BUILD_DIR%" ^
      -G "Visual Studio 18 2026" -A %ARCH% ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TLS_VERIFY=0 ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%"
if errorlevel 1 (
    echo 配置失败！
    exit /b 1
)

:: ===== 构建 =====
echo === 开始构建 ===
cmake --build "%PROJECT_DIR%\%BUILD_DIR%" --config Debug
if errorlevel 1 (
    echo 构建失败！
    exit /b 1
)

:: ===== 复制 OpenSSL 动态库 =====
echo === 复制 OpenSSL DLL ===
set "DEPLOY_TARGET_DIR=%PROJECT_DIR%\%BUILD_DIR%\Debug"
if exist "%OPENSSL_DLL_DIR%" (
    for %%F in ("%OPENSSL_DLL_DIR%\libcrypto*.dll" "%OPENSSL_DLL_DIR%\libssl*.dll") do (
        if exist "%%F" (
            echo 复制 %%~nxF ...
            copy /y "%%F" "%DEPLOY_TARGET_DIR%" >nul
        )
    )
) else (
    echo 警告: 未找到 OpenSSL DLL 目录 %OPENSSL_DLL_DIR%，运行时可能无法使用 HTTPS。
)

:: ===== 自动部署 Qt 动态库 =====
echo === 开始部署 Qt 动态库 ===
set "EXE_FILE=%DEPLOY_TARGET_DIR%\GitHubDesktopToCN.exe"

if not exist "%EXE_FILE%" (
    echo 错误：未找到可执行文件 %EXE_FILE%
    exit /b 1
)

cd /d "%DEPLOY_TARGET_DIR%"
call "%QT_DIR%\bin\windeployqt.exe" "GitHubDesktopToCN.exe" --no-translations --no-system-d3d-compiler --compiler-runtime
if errorlevel 1 (
    echo windeployqt 执行失败，请检查 Qt 安装路径是否正确。
    cd /d "%PROJECT_DIR%"
    exit /b 1
)

cd /d "%PROJECT_DIR%"

echo === 生成完成 ===
exit /b 0
```

### `.vscode/tasks.json`
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

### `.vscode/settings.json`
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