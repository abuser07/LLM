# LLM Studio

Una aplicación de escritorio nativa en C++ construida con **Qt6** para interactuar con **LLM locales y en la nube** — [Ollama](https://ollama.com) (local) y cualquier API **compatible con OpenAI** (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, etc.) — con acceso completo al sistema de archivos local y ejecución de comandos locales.

> 🌐 Disponible en **6 idiomas**: English, 中文, हिन्दी, Español, العربية y Português.

## Funcionalidades

- **Multi-proveedor**
  - **Ollama** (local, sin clave API) — listar modelos, descargar modelos (`ollama pull`).
  - APIs **compatibles con OpenAI** (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, personalizadas…) — configura URL base + clave API.
- **Importar desde OpenCode** — importa automáticamente los proveedores y claves API ya configurados en OpenCode (`~/.config/opencode/opencode.jsonc` + `auth.json`).
- **Chat en streaming** con **renderizado de Markdown** (títulos, listas, bloques de código, enlaces…) en tiempo real.
- **Acceso al sistema local**
  - Selecciona una **carpeta/proyecto** (se adjunta el árbol de archivos recursivo al contexto) o un **archivo** (se adjunta su contenido).
  - Abre la ruta seleccionada en el administrador de archivos del SO.
  - Ejecuta comandos locales (`QProcess`) y envía la salida al LLM para su análisis.
- **Controles**: `temperature`, `top_p` y **System Prompt** editable.
- **Historial por modelo**: cada modelo conserva su propia conversación.
- **Exportar** la conversación como `.txt` o `.md`.
- **Tema oscuro moderno** mediante QSS.
- **Internacionalización**: cambia de idioma en tiempo de ejecución.

## Requisitos

- **Qt 6** (Widgets + Network)
- **CMake** ≥ 3.16
- Un compilador C++17: MSVC 2019/2022, GCC o MinGW
- Opcional: [Ollama](https://ollama.com) ejecutándose en `http://localhost:11434`

## Compilación

### Windows (MSVC + Ninja)

Se incluye un script listo para usar (`build_win.bat`). Ajusta las rutas del inicio si es necesario y ejecuta:

```bat
build_win.bat
```

Configura y compila con CMake + Ninja en `build\llm_studio.exe`.

O manualmente:

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

## Empaquetado (Windows)

```powershell
# 1. Desplegar las DLL de Qt junto al ejecutable
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\llm_studio.exe

# 2. Copiar los archivos de traducción
#    - translations\llm_studio_*.qm
#    - qtbase_*.qm de Qt (en, zh_CN, es, ar, pt_BR)

# 3. Generar el instalador con Inno Setup
"C:\Program Files (x86)\Inno Setup 7\ISCC.exe" installer.iss
```

El resultado es `installer\LLM-Studio-Setup-1.0.0.exe`.

## Traducciones

Las cadenas de la interfaz se extraen con `lupdate` a archivos `.ts` y se compilan a `.qm` con `lrelease`.

```bash
lupdate main.cpp mainwindow.cpp mainwindow.h -ts translations/llm_studio_<lang>.ts
lrelease translations/*.ts
```

`translations/gen_translations.py` regenera los cinco archivos `.ts` a partir de una única tabla.

## Uso

1. Inicia Ollama (opcional) o configura un proveedor compatible con OpenAI (p. ej. DeepSeek) en **Proveedor y modelo → Administrar**.
2. (Opcional) Haz clic en **Importar proveedores de OpenCode** para reutilizar tus claves existentes.
3. Selecciona un modelo, ajusta temperature/top_p y el system prompt.
4. Chatea. Adjunta carpetas/archivos o ejecuta comandos del terminal y envía la salida al modelo.
