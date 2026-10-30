# LLM Studio

**Qt6** से निर्मित एक नेटिव C++ डेस्कटॉप एप्लिकेशन, जो **स्थानीय और क्लाउड LLM** — [Ollama](https://ollama.com) (स्थानीय) तथा कोई भी **OpenAI-संगत** API (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, आदि) — के साथ बातचीत करता है, और स्थानीय फ़ाइल सिस्टम तक पूर्ण पहुँच तथा स्थानीय कमांड निष्पादन प्रदान करता है।

> 🌐 **6 भाषाओं** में उपलब्ध: English, 中文, हिन्दी, Español, العربية और Português।

## विशेषताएँ

- **बहु-प्रदाता**
  - **Ollama** (स्थानीय, कोई API कुंजी नहीं) — मॉडल सूचीबद्ध करें, मॉडल डाउनलोड करें (`ollama pull`)।
  - **OpenAI-संगत** API (DeepSeek, OpenRouter, OpenAI, Groq, Mistral, कस्टम…) — बेस URL + API कुंजी सेट करें।
- **OpenCode से आयात** — OpenCode में पहले से कॉन्फ़िगर किए गए प्रदाताओं और API कुंजियों को स्वतः आयात करता है (`~/.config/opencode/opencode.jsonc` + `auth.json`)।
- **स्ट्रीमिंग चैट** और वास्तविक समय में **Markdown रेंडरिंग** (शीर्षक, सूचियाँ, कोड ब्लॉक, लिंक…)।
- **स्थानीय सिस्टम एक्सेस**
  - **फ़ोल्डर/प्रोजेक्ट** चुनें (संदर्भ में पुनरावर्ती फ़ाइल ट्री जुड़ता है) या **फ़ाइल** चुनें (उसकी सामग्री जुड़ती है)।
  - चयनित पथ को OS फ़ाइल प्रबंधक में खोलें।
  - स्थानीय कमांड चलाएँ (`QProcess`) और आउटपुट को विश्लेषण हेतु LLM को भेजें।
- **नियंत्रण**: `temperature`, `top_p`, संपादन-योग्य **सिस्टम प्रॉम्प्ट**।
- **प्रति-मॉडल इतिहास**: प्रत्येक मॉडल की अपनी वार्तालाप सुरक्षित रहती है।
- वार्तालाप को `.txt` या `.md` के रूप में **निर्यात** करें।
- QSS द्वारा **आधुनिक डार्क थीम**।
- **अंतर्राष्ट्रीयकरण**: रनटाइम पर भाषा बदलें।

## आवश्यकताएँ

- **Qt 6** (Widgets + Network)
- **CMake** ≥ 3.16
- C++17 कंपाइलर: MSVC 2019/2022, GCC या MinGW
- वैकल्पिक: [Ollama](https://ollama.com) `http://localhost:11434` पर चल रहा हो

## निर्माण (Build)

### Windows (MSVC + Ninja)

तैयार स्क्रिप्ट (`build_win.bat`) दी गई है। यदि आवश्यक हो तो शीर्ष पर पथ बदलें, फिर चलाएँ:

```bat
build_win.bat
```

यह CMake + Ninja से `build\llm_studio.exe` में कॉन्फ़िगर और बिल्ड करता है।

या मैन्युअल रूप से:

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

## पैकेजिंग (Windows)

```powershell
# 1. निष्पादन-योग्य के पास Qt DLL तैनात करें
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\llm_studio.exe

# 2. अनुवाद फ़ाइलें कॉपी करें
#    - translations\llm_studio_*.qm
#    - Qt के qtbase_*.qm (en, zh_CN, es, ar, pt_BR)

# 3. Inno Setup से इंस्टॉलर बनाएँ
"C:\Program Files (x86)\Inno Setup 7\ISCC.exe" installer.iss
```

परिणाम `installer\LLM-Studio-Setup-1.0.0.exe` होगा।

## अनुवाद

UI स्ट्रिंग्स को `lupdate` से `.ts` फ़ाइलों में निकाला जाता है और `lrelease` से `.qm` में संकलित किया जाता है।

```bash
lupdate main.cpp mainwindow.cpp mainwindow.h -ts translations/llm_studio_<lang>.ts
lrelease translations/*.ts
```

`translations/gen_translations.py` एक ही तालिका से पाँचों `.ts` फ़ाइलें पुनः उत्पन्न करता है।

## उपयोग

1. Ollama प्रारंभ करें (वैकल्पिक) या **प्रदाता और मॉडल → प्रबंधित करें** में OpenAI-संगत प्रदाता (जैसे DeepSeek) कॉन्फ़िगर करें।
2. (वैकल्पिक) मौजूदा कुंजियाँ पुनः उपयोग करने के लिए **OpenCode से प्रदाता आयात करें** पर क्लिक करें।
3. मॉडल चुनें, temperature/top_p और सिस्टम प्रॉम्प्ट समायोजित करें।
4. चैट करें। फ़ोल्डर/फ़ाइलें संलग्न करें या टर्मिनल कमांड चलाएँ और आउटपुट मॉडल को भेजें।
