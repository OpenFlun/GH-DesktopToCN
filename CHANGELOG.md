# Changelog

所有本项目的显著更改都将记录在此文件中;

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/),
并且本项目遵循 [语义化版本](https://semver.org/lang/zh-CN);

## [1.0.22] - 2026-08-01 16:04
### 更新
- 增加对GitHub桌面版3.6.3的汉化适配;
### 修复
- 修复了部分场景没有准确自动资源目录的问题;
- 修复了'build.bat'构建时报找不到可执行文件的问题;

## [1.0.21] - 2026-06-01 20:24
### 更新
- 增加对GitHub桌面版3.5.11的汉化适配;

## [1.0.20] - 2026-06-01 08:58
### 优化
- 优化了构建和发布说明文档,以适应上一版本所做的修改;

## [1.0.19] - 2026-05-31 22:11
### 修复
- 修复因 spdlog 版本过旧导致与 Visual Studio 2026 不兼容的编译错误（`stdext::checked_array_iterator` 缺失）。现已强制更新 spdlog 至最新 v1.x 分支，或指导用户手动替换;

### 优化
- **构建脚本 (`build.bat`) 和发布脚本 (`release.bat`) 全面自动化**：
  - 自动查找 Visual Studio 的 `VsDevCmd.bat`（支持未来版本升级，无需硬编码路径）。
  - 自动查找 Qt 安装目录（支持任意 6.x.x 版本，自动选择最高版本号，并适配 `msvc*_64` 编译器子目录）。
  - 改用 Ninja 生成器，彻底移除对 Visual Studio 版本字符串的依赖（避免因 VS 2027 等升级导致生成器名称失效）。
  - 增加编译器缓存有效性检测：当缓存中的 `cl.exe` 路径无效时自动清理并重新配置。
  - 部署阶段采用“备份 exe → 清空 Debug 目录 → 恢复 exe”的策略，解决因 DLL 被占用导致的 `windeployqt` 失败问题。
  - 统一错误处理，失败时自动恢复 `version.json` 备份。

[unreleased]: https://github.com/OpenFlun/GH-DesktopToCN/compare/v1.0.21...HEAD
[1.0.20]: https://github.com/OpenFlun/GH-DesktopToCN/compare/v1.0.19...v1.0.20
[1.0.19]: https://github.com/OpenFlun/GH-DesktopToCN/compare/v1.0.18...v1.0.19