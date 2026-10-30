# LLM Studio

A native C++ desktop application built with **Qt6** to interact with **local and cloud LLMs** — [Ollama](https://ollama.com) (local) and any **OpenAI-compatible** API (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, etc.) — with full access to the local file system and local command execution.

> 🌐 Available in **6 languages**: English, 中文, हिन्दी, Español, العربية and Português.

## Features

- **Multi-provider**
  - **Ollama** (local, no API key) — list models, download models (`ollama pull`).
  - **OpenAI-compatible** APIs (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, custom…) — set base URL + API key.
- **Import from OpenCode** — automatically imports the providers and API keys already configured in OpenCode (`~/.config/opencode/opencode.jsonc` + `auth.json`).
- **Streaming chat** with **Markdown rendering** (headers, lists, code blocks, links…) in real time.
- **Local system access**
  - Select a **folder/project** (a recursive file tree is attached to the context) or a **file** (its content is attached).
  - Open the selected path in the OS file manager.
  - Run local commands (`QProcess`) and send the output to the LLM for analysis.
- **Controls**: `temperature`, `top_p`, editable **System Prompt**.
- **Per-model history**: each model keeps its own conversation.
- **Export** the conversation as `.txt` or `.md`.
- **Modern dark theme** via QSS.
- **Internationalization**: switch languages at runtime.

## Requirements

- **Qt 6** (Widgets + Network)
- **CMake** ≥ 3.16
- A C++17 compiler: MSVC 2019/2022, GCC, or MinGW
- Optional: [Ollama](https://ollama.com) running on `http://localhost:11434`

## Building

### Windows (MSVC + Ninja)

A ready-to-use script is provided (`build_win.bat`). Edit the paths at the top if needed, then run:

```bat
build_win.bat
```

It configures and builds with CMake + Ninja into `build\llm_studio.exe`.

Or manually:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\msvc2022_64
cmake --build build
```

### Windows (MinGW)

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\mingw_64
cmake --build build
```

### Linux (GCC)

```bash
sudo apt install qt6-base-dev cmake g++
cmake -S . -B build
cmake --build build -j
./build/llm_studio
```

## Packaging (Windows)

```powershell
# 1. Deploy Qt DLLs next to the executable
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\llm_studio.exe

# 2. Copy the translation files
#    - translations\llm_studio_*.qm
#    - Qt's qtbase_*.qm (en, zh_CN, es, ar, pt_BR)

# 3. Build the installer with Inno Setup
"C:\Program Files (x86)\Inno Setup 7\ISCC.exe" installer.iss
```

The result is `installer\LLM-Studio-Setup-1.0.0.exe`.

## Translations

The UI strings are extracted with `lupdate` into `.ts` files and compiled to `.qm` with `lrelease`.

```bash
lupdate main.cpp mainwindow.cpp mainwindow.h -ts translations/llm_studio_<lang>.ts
lrelease translations/*.ts
```

`translations/gen_translations.py` regenerates all five `.ts` files from a single table.

## Usage

1. Start Ollama (optional) or configure an OpenAI-compatible provider (e.g. DeepSeek) under **Provider & Model → Manage**.
2. (Optional) Click **Import providers from OpenCode** to reuse your existing keys.
3. Select a model, adjust temperature/top_p and the system prompt.
4. Chat. Attach folders/files or run terminal commands and send the output to the model.
