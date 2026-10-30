# LLM Studio

تطبيق سطح مكتب أصلي مكتوب بلغة C++ ومبني على **Qt6** للتفاعل مع **نماذج اللغة المحلية والسحابية** — [Ollama](https://ollama.com) (محلي) وأي واجهة برمجة **متوافقة مع OpenAI** (DeepSeek وOpenRouter وOpenAI وGroq وMistral وغيرها) — مع وصول كامل إلى نظام الملفات المحلي وتنفيذ الأوامر المحلية.

> 🌐 متوفر بـ **6 لغات**: English و中文 وहिन्दी وEspañol والعربية وPortuguês.

## المميزات

- **متعدد الموفّرين**
  - **Ollama** (محلي، بدون مفتاح API) — سرد النماذج وتنزيلها (`ollama pull`).
  - واجهات برمجية **متوافقة مع OpenAI** (DeepSeek وOpenRouter وOpenAI وGroq وMistral ومخصصة…) — عيّن عنوان URL الأساسي ومفتاح API.
- **الاستيراد من OpenCode** — يستورد تلقائيًا الموفّرين ومفاتيح API المكوّنة مسبقًا في OpenCode (`~/.config/opencode/opencode.jsonc` + `auth.json`).
- **محادثة متدفقة (streaming)** مع **عرض Markdown** (عناوين وقوائم وكتل أكواد وروابط…) في الوقت الفعلي.
- **الوصول إلى النظام المحلي**
  - اختر **مجلدًا/مشروعًا** (تُرفق شجرة الملفات التكرارية بالسياق) أو **ملفًا** (يُرفق محتواه).
  - افتح المسار المحدد في مدير ملفات النظام.
  - نفّذ أوامر محلية (`QProcess`) وأرسل المخرجات إلى النموذج لتحليلها.
- **عناصر التحكم**: `temperature` و`top_p` و**موجّه النظام** القابل للتحرير.
- **سجل لكل نموذج**: يحتفظ كل نموذج بمحادثته الخاصة.
- **تصدير** المحادثة بصيغة `.txt` أو `.md`.
- **مظهر داكن حديث** عبر QSS.
- **التدويل**: تبديل اللغة في وقت التشغيل.

## المتطلبات

- **Qt 6** (Widgets + Network)
- **CMake** ≥ 3.16
- مترجم C++17: MSVC 2019/2022 أو GCC أو MinGW
- اختياري: [Ollama](https://ollama.com) يعمل على `http://localhost:11434`

## البناء

### ويندوز (MSVC + Ninja)

يتوفر سكربت جاهز (`build_win.bat`). عدّل المسارات أعلى الملف عند الحاجة ثم شغّل:

```bat
build_win.bat
```

يُهيّئ ويبني عبر CMake + Ninja إلى `build\llm_studio.exe`.

أو يدويًا:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\msvc2022_64
cmake --build build
```

### ويندوز (MinGW)

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.9.3\mingw_64
cmake --build build
```

### لينكس (GCC)

```bash
sudo apt install qt6-base-dev cmake g++
cmake -S . -B build
cmake --build build -j
./build/llm_studio
```

## التغليف (ويندوز)

```powershell
# 1. انشر مكتبات Qt بجوار الملف التنفيذي
C:\Qt\6.9.3\msvc2022_64\bin\windeployqt.exe --release --no-translations dist\llm_studio.exe

# 2. انسخ ملفات الترجمة
#    - translations\llm_studio_*.qm
#    - ملفات Qt مثل qtbase_*.qm (en وzh_CN وes وar وpt_BR)

# 3. أنشئ المثبّت باستخدام Inno Setup
"C:\Program Files (x86)\Inno Setup 7\ISCC.exe" installer.iss
```

النتيجة في `installer\LLM-Studio-Setup-1.0.0.exe`.

## الترجمات

تُستخرج نصوص الواجهة عبر `lupdate` إلى ملفات `.ts` وتُجمَّع إلى `.qm` عبر `lrelease`.

```bash
lupdate main.cpp mainwindow.cpp mainwindow.h -ts translations/llm_studio_<lang>.ts
lrelease translations/*.ts
```

يعيد `translations/gen_translations.py` توليد ملفات `.ts` الخمسة من جدول واحد.

## الاستخدام

1. شغّل Ollama (اختياري) أو كوّن موفرًا متوافقًا مع OpenAI (مثل DeepSeek) من **الموفر والنموذج → إدارة**.
2. (اختياري) انقر **استيراد الموفّرين من OpenCode** لإعادة استخدام مفاتيحك الحالية.
3. اختر نموذجًا وعدّل temperature/top_p وموجّه النظام.
4. تحدّث. أرفق مجلدات/ملفات أو نفّذ أوامر الطرفية وأرسل المخرجات إلى النموذج.
