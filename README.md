# 🥮 GitHubDesktopToCN

[![GitHub Repo](https://img.shields.io/badge/GitHub-仓库-blue?logo=github&style=flat-square)](https://github.com/OpenFlun/GH-DesktopToCN)
[![GitHub release](https://img.shields.io/github/v/release/OpenFlun/GH-DesktopToCN?style=flat-square)](https://github.com/OpenFlun/GH-DesktopToCN/releases)
[![GitHub stars](https://img.shields.io/github/stars/OpenFlun/GH-DesktopToCN?style=flat-square)](https://github.com/OpenFlun/GH-DesktopToCN/stargazers)

> 这是一个自动替换 GitHub Desktop 中文本为目标语言文本的程序;
> 优点是对 GitHub Desktop 频繁更新的版本变化兼容性比较高,即便只有一两条条目失去了翻译,也只需手动修改添加即可,项目维护成本较低;

---

## 💻 系统要求

- **操作系统**：Windows 10 / 11（64 位）
- **架构**：仅支持 **x64（64 位）**,不支持 32 位系统
- **GitHub Desktop**：官方最新版本（会自动检测安装路径）
- **运行时依赖**：无需额外安装,单文件 exe 已包含所有依赖（VC++ 运行库已打包）

> ⚠️ **注意**：本项目仅针对 64 位 Windows 开发,32 位系统无法运行;

---

## 📦 怎么使用它（最终用户）

1. 前往 [GitHub Releases 页面](https://github.com/OpenFlun/GH-DesktopToCN/releases/latest) 下载：
   - `GitHubDesktopToCN.exe`（单文件版,已包含所有依赖）
   - `localization.json`（汉化映射文件,如不需要自定义翻译可忽略）

2. **⚠️ 重要：汉化前请先关闭 GitHub Desktop 程序**（否则替换可能不生效或需要重启）;

3. 将 `GitHubDesktopToCN.exe` 和 `localization.json`（如有）放在同一个文件夹中,双击运行程序;

4. 按照界面提示选择汉化方式即可;

> 💡 **提示**：程序会自动检测 GitHub Desktop 的安装路径,如果未检测到,请手动选择 `resources/app` 目录;

---

## 🌐 网络要求

- 程序启动时会自动检查更新（需要访问 GitHub API）;如果您处于需要代理的网络环境,程序会自动使用系统代理设置,无需额外配置;
- 如果网络访问受限导致检查更新失败,不影响本地汉化功能;

---

## 🧩 自定义汉化或补充翻译

请先阅读项目中的 `些注意事项.txt` 和 `localization.md`,了解编写规则;
然后在 `localization.json` 中参照已有格式添加或修改翻译条目,并将该文件放在程序同目录下即可;
（对于已汉化过的 GitHub Desktop,需先恢复为原始英文,再应用新的自定义汉化;）

---

## 📄 映射文件：localization.json

此文件存储所有 GitHub Desktop 中英文文本到本地化（中文）文本之间的映射,使用正则匹配方式进行替换;

- 路径：`./localization.json`
- 主节点 `version` (int)：json 文件的格式版本（仅在未来格式更新时变化）
- 主节点 `minversion` (string)：需要的最低加载器（GitHubDesktopToCN.exe）版本
- 主节点 `tip` (array[string])：在加载器中显示的通知信息
- 主节点 `select` (JSON)：汉化时可选的增强功能
  - `replaceFile` (string)：要替换的文件（main.js 或 renderer.js）
  - `tooltip` (string)：提示信息
  - `enable` (bool)：默认是否启用
  - `replace` (array[array])：启用时执行的替换规则
- 主节点 `main` / `renderer` (array)：主替换规则,分别作用于 `main.js` 和 `renderer.js`

---

## 🛠️ 从源码构建（开发者）

如果你想自行修改程序或参与开发,请参考以下文档：

| 文档                                                           | 用途                                           |
| -------------------------------------------------------------- | ---------------------------------------------- |
| [`CMAKE编译指南.md`](./CMAKE编译指南.md)                       | 完整的编译环境配置、依赖说明、常见问题         |
| [`项目构建与发布操作指南.md`](./项目构建与发布操作指南.md)     | Release 构建、封包、发布到 GitHub 的自动化流程 |
| [`VS2026自定义外部工具指南.md`](./VS2026自定义外部工具指南.md) | 在 Visual Studio 中集成一键构建按钮            |

**快速开始**：
1. 确保系统为 **64 位 Windows**;
2. 安装 Visual Studio 2026 Build Tools（含 MSVC v143）和 Qt 6.11.0 (msvc2022_64);
3. 确保 `openssl-proj\lib` 目录下存在 OpenSSL 的导入库（`libssl.lib`、`libcrypto.lib`）和动态库（`libcrypto-3-x64.dll`、`libssl-3-x64.dll`）;
4. 在项目根目录运行 `build.bat`（或使用 VS Code 任务 `Build x64`）;
5. 构建产物位于 `out\build\x64-debug\Debug\GitHubDesktopToCN.exe`;

---

## 📚 第三方库

感谢以下开源项目：

- [CLI11](https://github.com/CLIUtils/CLI11) – 命令行解析
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) – HTTP 客户端/服务器库
- [nlohmann/json](https://github.com/nlohmann/json) – JSON 解析与生成
- [WinReg](https://github.com/GiovanniDicanio/WinReg) – Windows 注册表访问

---

## ✅ TODO

- [x] json 文件格式版本控制与最低加载器版本检查
- [x] 加载器支持正则替换的第三个参数（捕获组替换）
- [x] 自动检测更新并可选择一键更新
- [x] 显示通知信息与参与者列表
- [x] 汉化异常恢复机制（备份 .bak）
- [x] 支持选择性汉化（`select` 节点）
- [x] 支持系统 HTTP 代理（环境变量 + 注册表）
- [x] 下载更新的断点续传
- [x] 检测 GitHub Desktop 版本并用颜色提示更新

---

## 📄 许可证

本项目采用 [MIT License](LICENSE) 开源协议;