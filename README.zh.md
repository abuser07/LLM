# LLM Studio

一款使用 **Qt6** 构建的原生 C++ 桌面应用，用于与**本地和云端 LLM** 交互——[Ollama](https://ollama.com)（本地）以及任何**兼容 OpenAI 的 API**（DeepSeek、OpenRouter、OpenAI、Groq、Mistral 等），并拥有完整的本地文件系统访问和本地命令执行能力。

> 🌐 支持 **6 种语言**：English、中文、हिन्दी、Español、العربية 和 Português。

## 功能

- **多提供商**
  - **Ollama**（本地，无需 API 密钥）——列出模型、下载模型（`ollama pull`）。
  - **兼容 OpenAI 的 API**（DeepSeek、OpenRouter、OpenAI、Groq、Mistral、自定义……）——设置基础 URL 和 API 密钥。
- **从 OpenCode 导入**——自动导入 OpenCode 中已配置的提供商和 API 密钥（`~/.config/opencode/opencode.jsonc` + `auth.json`）。
- **流式对话**，支持 **Markdown 实时渲染**（标题、列表、代码块、链接……）。
- **本地系统访问**
  - 选择**文件夹/项目**（将递归文件树附加到上下文）或**文件**（附加其内容）。
  - 在操作系统文件管理器中打开所选路径。
  - 执行本地命令（`QProcess`）并将输出发送给 LLM 进行分析。
- **控制项**：`temperature`、`top_p`、可编辑的 **系统提示词（System Prompt）**。
- **按模型的历史记录**：每个模型保留各自的对话。
- 将对话**导出**为 `.txt` 或 `.md`。
- 通过 QSS 实现的**现代深色主题**。
- **国际化**：运行时切换语言。

## 环境要求

- **Qt 6**（Widgets + Network）
- **CMake** ≥ 3.16
- C++17 编译器：MSVC 2019/2022、GCC 或 MinGW
- 可选：[Ollama](https://ollama.com) 运行于 `http://localhost:11434`

## 构建

### Windows（MSVC + Ninja）

已提供现成脚本（`build_win.bat`）。如有需要请先修改文件顶部的路径，然后运行：

```bat
build_win.bat
```

它会使用 CMake + Ninja 配置并构建到 `build\llm_studio.exe`。

或手动执行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\msvc2022_64
cmake --build build
```

### Windows（MinGW）

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\mingw_64
cmake --build build
```

### Linux（GCC）

```bash
sudo apt install qt6-base-dev cmake g++
cmake -S . -B build
cmake --build build -j
./build/llm_studio
```

## 打包（Windows）

```powershell
# 1. 将 Qt DLL 部署到可执行文件旁
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\llm_studio.exe

# 2. 复制翻译文件
#    - translations\llm_studio_*.qm
#    - Qt 的 qtbase_*.qm（en、zh_CN、es、ar、pt_BR）

# 3. 使用 Inno Setup 生成安装程序
"C:\Program Files (x86)\Inno Setup 7\ISCC.exe" installer.iss
```

结果位于 `installer\LLM-Studio-Setup-1.0.0.exe`。

## 翻译

界面字符串通过 `lupdate` 提取为 `.ts` 文件，并用 `lrelease` 编译为 `.qm`。

```bash
lupdate main.cpp mainwindow.cpp mainwindow.h -ts translations/llm_studio_<lang>.ts
lrelease translations/*.ts
```

`translations/gen_translations.py` 会从单一表格重新生成全部五个 `.ts` 文件。

## 使用

1. 启动 Ollama（可选），或在**提供商和模型 → 管理**中配置兼容 OpenAI 的提供商（例如 DeepSeek）。
2. （可选）点击**从 OpenCode 导入提供商**以复用现有密钥。
3. 选择模型，调整 temperature/top_p 和系统提示词。
4. 开始对话。可附加文件夹/文件，或执行终端命令并将输出发送给模型。
