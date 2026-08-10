## GitHub Desktop 自定义译文映射操作指南

本指南用于在 GitHub Desktop 汉化过程中,通过直接修改源码字符串的方式完成界面翻译;核心是依据 `localization.json` 中的映射关系,对 `main.js`、`renderer.js` 等文件进行查找和替换,当前以`renderer.js`文件内容为例;

---

### 一、整体机制

- `localization.json` 的 `renderer` 数组保存了 `[原文, 译文]` 的映射对;
- 汉化时,直接扫描源码文件,将所有与原文**完全匹配**的字符串替换为对应译文;
- 对于包含变量的模板字符串,使用正则捕获方式完成替换;
- 替换后的源码直接包含中文字符串;

---

### 二、纯静态文本替换

**映射示例：**
```json
[
    "\"Start tutorial\"",
    "\"开始教程\""
]
```

**源码查找目标（`renderer.js` 中）：**
```javascript
title: "Start tutorial",
```

**执行替换后：**
```javascript
title: "开始教程",
```

**规则：** 搜索时需严格匹配完整字符串,包括引号、空格和标点;

---

### 三、被动态组件拆分的多段文本替换

当原文被动态 React 组件（如用户名、链接）分隔时,需要拆分后逐段替换,动态部分保持不动;

**替换前源码示例：**
```jsx
<div>
  {"This will create a repository on your local machine, and push it to your account "}
  <sP>@{this.props.account.login}</sP>
  {" on"}
  {" "}
  <kA uri={ir(e.endpoint)}>{e.friendlyEndpoint}</kA>
  {". This repository will only be visible to you, and not visible publicly."}
</div>
```

**映射表对应条目：**
```json
[
    "\"This will create a repository on your local machine, and push it to your account \"",
    "\"这将在你的电脑上创建一个仓库示例,并将其推送至你的账户\""
],
[
    "\" on\"",
    "\" 在\""
],
[
    "\". This repository will only be visible to you, and not visible publicly.\"",
    "\";\\n此仓库仅你个人可见,不会对外公开;\""
]
```

**替换后源码：**
```jsx
<div>
  {"这将在你的电脑上创建一个仓库示例,并将其推送至你的账户"}
  <sP>@{this.props.account.login}</sP>
  {" 在"}
  {" "}
  <kA uri={ir(e.endpoint)}>{e.friendlyEndpoint}</kA>
  {";\n此仓库仅你个人可见,不会对外公开;"}
</div>
```

**注意：**
- 动态组件（`<sP>`、`<kA>`）不能修改;
- 分段原文中的空格（如 `" on"` 前面的空格）必须完整保留在映射和查找中;
- 映射值里的 `\\n` 在写入源码时需转换为真实的换行转义符 `\n`（见换行处理部分）;

---

### 四、模板字符串中的变量捕获与替换

源码中常见 JavaScript 模板字符串内嵌变量 `${...}`,例如：
```javascript
`Fetch the latest changes from ${remote}`
```
汉化时需要保留变量位置,仅将周围固定文本替换为中文;

#### 1. 映射表格式

```json
[
    "`Fetch the latest changes from \\$\\{(.)\\}`",
    "`从远程仓库($${$1})获取最新的变化`"
]
```

- **原文模式**：`"`Fetch the latest changes from \\$\\{(.)\\}`"`
  JSON 解析后实际字符串为 `` `Fetch the latest changes from \${(.)}` ``
  其中 `\${(.)}` 是正则表达式,用于匹配 `${变量}` 部分;`(.)` 为捕获组,此处示例假设变量只含一个字符,实际可调整为 `(.+?)` 等;

- **译文模板**：`` `从远程仓库(${$1})获取最新的变化` ``
  `$1` 代表捕获到的文本,将被替换为具体变量内容;

#### 2. 查找与替换操作（使用正则）

在支持正则搜索的编辑器中：

**查找：**
```
`Fetch the latest changes from \$\{(.+?)\}`
```

**替换为：**
```
`从远程仓库($1)获取最新的变化`
```

**操作示例：**
- 源码行：`` someFunction(`Fetch the latest changes from ${remote}`) ``
- 匹配到 `remote` 并捕获为 `$1`
- 替换结果：`` someFunction(`从远程仓库(remote)获取最新的变化`) ``

运行时 JavaScript 依然会将 `remote` 作为变量求值,最终显示为 `从远程仓库(origin)获取最新的变化`;

#### 3. 多模式变体

映射表使用 `:` 分隔同一情境下的两种原文模式：

```json
[
    "`Pull \\$\\{(.)\\} with rebase`:`Pull \\$\\{(.)\\}`",
    "`从远程仓库($${$1})拉取并合并`:`从远程仓库($${$2})拉取`"
]
```

这相当于两条独立规则：

| 查找（正则）                         | 替换                             |
| ------------------------------------ | -------------------------------- |
| `` `Pull \$\{(.+?)\} with rebase` `` | `` `从远程仓库($1)拉取并合并` `` |
| `` `Pull \$\{(.+?)\}` ``             | `` `从远程仓库($1)拉取` ``       |

**替换顺序：** 务必先替换带 `with rebase` 的较长模式,再替换较短模式,避免短模式误匹配长模式的子串;

#### 4. 注意事项

- JSON 中的 `\\$\\{` 对应正则中的 `\$\{`,编辑时务必正确书写反斜杠;
- 捕获组使用 `(.+?)` 可匹配任意长度的变量名,较 `(.)` 更通用;请依据项目既有模式调整;
- 替换后源码中的 `${remote}` 结构必须原样保留,只替换其外部的英文文本;

---

### 五、换行处理

若译文需强制换行,必须同时修改字符串和 CSS;

#### 1. 映射表写法

在译文值中使用 `\\n`：
```json
"\";\\n此仓库仅你个人可见,不会对外公开;\""
```
JSON 解析后字符串为 `";\n此仓库仅你个人可见,不会对外公开;"`;

#### 2. 写入源码

将映射表中的译文写到 `renderer.js` 时,需保证字符串内包含真实的换行转义符 `\n`;即在代码中呈现为：
```javascript
{";\n此仓库仅你个人可见,不会对外公开;"}
```
（此时字符串字符序列是 `;`、`\n`、`此`……,`\n` 为一个转义字符;）

#### 3. CSS 配置

在 `renderer.css` 文件**最开头**添加：
```css
* {
    white-space: pre-wrap !important;
}
```
`pre-wrap` 使 `\n` 被渲染为换行,且保留自动换行能力;可限定作用域,例如：
```css
#create-tutorial-repository-dialog {
    white-space: pre-wrap !important;
}
```

---

### 六、操作流程总结

1. **确认字符串类型**：静态、拆分多段、模板字面量;
2. **在 `localization.json` 中添加映射**：按上文各部分格式填写原文与译文;
3. **执行源码替换**：
   - 静态文本：精确查找,直接替换;
   - 拆分文本：逐段查找替换,动态组件保留;
   - 模板字符串：使用正则查找,正则捕获 → 用 `$1`、`$2` 等写入译文,注意多模式时的替换顺序;
4. **处理换行**：映射表写 `\\n` → 源码写 `\n` → CSS 添加 `white-space: pre-wrap !important`;
5. **保存并测试**：运行 GitHub Desktop,验证界面文字、变量和换行是否正确显示;

---

按此流程操作,即可系统性地完成静态文本与动态模板字符串的自定义译文映射,保证替换准确、换行生效、变量正常运行;