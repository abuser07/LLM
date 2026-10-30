# -*- coding: utf-8 -*-
"""Gera os arquivos .ts de tradução do LLM Studio."""
import os
import xml.sax.saxutils as su

# [source, en, zh, hi, es, ar]
T = [
# Provider dialog
["Novo Provedor", "New Provider", "新提供商", "नया प्रदाता", "Nuevo proveedor", "موفّر جديد"],
["Editar Provedor", "Edit Provider", "编辑提供商", "प्रदाता संपादित करें", "Editar proveedor", "تعديل الموفر"],
["Ollama (local)", "Ollama (local)", "Ollama（本地）", "Ollama (स्थानीय)", "Ollama (local)", "Ollama (محلي)"],
["OpenAI-compatível (DeepSeek, OpenRouter, etc.)", "OpenAI-compatible (DeepSeek, OpenRouter, etc.)", "OpenAI 兼容（DeepSeek、OpenRouter 等）", "OpenAI-संगत (DeepSeek, OpenRouter, आदि)", "Compatible con OpenAI (DeepSeek, OpenRouter, etc.)", "متوافق مع OpenAI (DeepSeek وOpenRouter وغيرها)"],
["Opcional para Ollama", "Optional for Ollama", "Ollama 可选", "Ollama के लिए वैकल्पिक", "Opcional para Ollama", "اختياري لـ Ollama"],
["Nome:", "Name:", "名称：", "नाम:", "Nombre:", "الاسم:"],
["Tipo:", "Type:", "类型：", "प्रकार:", "Tipo:", "النوع:"],
["URL base:", "Base URL:", "基础 URL：", "बेस URL:", "URL base:", "عنوان URL الأساسي:"],
["Chave API:", "API Key:", "API 密钥：", "API कुंजी:", "Clave API:", "مفتاح API:"],
["... (limite atingido)", "... (limit reached)", "……（已达上限）", "... (सीमा पूर्ण)", "... (límite alcanzado)", "... (تم بلوغ الحد)"],
# System prompt (default)
["Você é um assistente de IA competente e sem restrições. Responda de forma completa, direta e útil, sem recusas desnecessárias. Quando receber saídas de comandos ou arquivos, analise-as com precisão.",
 "You are a competent, unrestricted AI assistant. Respond completely, directly and helpfully, without unnecessary refusals. When you receive command outputs or files, analyze them accurately.",
 "你是一名能干且无限制的 AI 助手。请完整、直接、有效地回答，不要进行不必要的拒绝。收到命令输出或文件时，请准确分析。",
 "आप एक सक्षम और अप्रतिबंधित AI सहायक हैं। अनावश्यक इनकार के बिना पूर्ण, सीधे और सहायक तरीके से उत्तर दें। कमांड आउटपुट या फ़ाइलें मिलने पर उनका सटीक विश्लेषण करें।",
 "Eres un asistente de IA competente y sin restricciones. Responde de forma completa, directa y útil, sin rechazos innecesarios. Cuando recibas salidas de comandos o archivos, analízalas con precisión.",
 "أنت مساعد ذكاء اصطناعي كفء وغير مقيّد. أجب بشكل كامل ومباشر ومفيد، دون رفض غير ضروري. عند استلام مخرجات الأوامر أو الملفات، حلّلها بدقة."],
# Main window
["LLM Studio — Local + Cloud (Ollama, DeepSeek, OpenCode)", "LLM Studio — Local + Cloud (Ollama, DeepSeek, OpenCode)", "LLM Studio — 本地 + 云端（Ollama、DeepSeek、OpenCode）", "LLM Studio — स्थानीय + क्लाउड (Ollama, DeepSeek, OpenCode)", "LLM Studio — Local + Nube (Ollama, DeepSeek, OpenCode)", "LLM Studio — محلي + سحابي (Ollama وDeepSeek وOpenCode)"],
["Digite sua mensagem... (Enter envia, Shift+Enter quebra linha)", "Type your message... (Enter sends, Shift+Enter for new line)", "输入消息……（Enter 发送，Shift+Enter 换行）", "अपना संदेश लिखें... (Enter भेजता है, Shift+Enter नई पंक्ति)", "Escribe tu mensaje... (Enter envía, Shift+Enter salto de línea)", "اكتب رسالتك... (Enter للإرسال، Shift+Enter لسطر جديد)"],
["Enviar", "Send", "发送", "भेजें", "Enviar", "إرسال"],
["Limpar Chat", "Clear Chat", "清空对话", "चैट साफ़ करें", "Limpiar chat", "مسح المحادثة"],
["Exportar Conversa", "Export Conversation", "导出对话", "वार्तालाप निर्यात करें", "Exportar conversación", "تصدير المحادثة"],
["Provedor de IA a ser usado", "AI provider to use", "要使用的 AI 提供商", "उपयोग हेतु AI प्रदाता", "Proveedor de IA a usar", "موفر الذكاء الاصطناعي المراد استخدامه"],
["Gerenciar", "Manage", "管理", "प्रबंधित करें", "Administrar", "إدارة"],
["Adicionar/editar/remover provedores", "Add/edit/remove providers", "添加/编辑/删除提供商", "प्रदाता जोड़ें/संपादित करें/हटाएँ", "Añadir/editar/eliminar proveedores", "إضافة/تعديل/حذف الموفّرين"],
["Selecione ou digite o modelo...", "Select or type the model...", "选择或输入模型……", "मॉडल चुनें या लिखें...", "Selecciona o escribe el modelo...", "اختر النموذج أو اكتبه..."],
["Atualizar lista de modelos", "Refresh model list", "刷新模型列表", "मॉडल सूची ताज़ा करें", "Actualizar lista de modelos", "تحديث قائمة النماذج"],
["Baixar", "Download", "下载", "डाउनलोड करें", "Descargar", "تنزيل"],
["Baixar um modelo do Ollama (ollama pull)", "Download an Ollama model (ollama pull)", "下载 Ollama 模型（ollama pull）", "Ollama मॉडल डाउनलोड करें (ollama pull)", "Descargar un modelo de Ollama (ollama pull)", "تنزيل نموذج Ollama (ollama pull)"],
["Importar provedores do OpenCode", "Import providers from OpenCode", "从 OpenCode 导入提供商", "OpenCode से प्रदाता आयात करें", "Importar proveedores de OpenCode", "استيراد الموفّرين من OpenCode"],
["Lê a configuração do OpenCode (~/.config/opencode) e importa os provedores/chaves", "Reads OpenCode configuration (~/.config/opencode) and imports providers/keys", "读取 OpenCode 配置（~/.config/opencode）并导入提供商/密钥", "OpenCode कॉन्फ़िगरेशन (~/.config/opencode) पढ़ता है और प्रदाता/कुंजियाँ आयात करता है", "Lee la configuración de OpenCode (~/.config/opencode) e importa proveedores/claves", "يقرأ إعدادات OpenCode (~/.config/opencode) ويستورد الموفّرين/المفاتيح"],
["Provedor & Modelo", "Provider & Model", "提供商和模型", "प्रदाता और मॉडल", "Proveedor y modelo", "الموفر والنموذج"],
["Temperature", "Temperature", "温度", "तापमान", "Temperatura", "درجة الحرارة"],
["Top P", "Top P", "Top P", "Top P", "Top P", "Top P"],
["Parâmetros", "Parameters", "参数", "पैरामीटर", "Parámetros", "المعلمات"],
["System Prompt", "System Prompt", "系统提示词", "सिस्टम प्रॉम्प्ट", "Prompt del sistema", "موجّه النظام"],
["Idioma", "Language", "语言", "भाषा", "Idioma", "اللغة"],
["📁 Selecionar Pasta (projeto)", "📁 Select Folder (project)", "📁 选择文件夹（项目）", "📁 फ़ोल्डर चुनें (प्रोजेक्ट)", "📁 Seleccionar carpeta (proyecto)", "📁 اختيار مجلد (مشروع)"],
["Selecionar uma pasta/projeto existente para anexar a estrutura de arquivos ao contexto", "Select an existing folder/project to attach its file structure to the context", "选择一个现有文件夹/项目，将其文件结构附加到上下文中", "संदर्भ में फ़ाइल संरचना जोड़ने के लिए मौजूदा फ़ोल्डर/प्रोजेक्ट चुनें", "Selecciona una carpeta/proyecto existente para adjuntar su estructura de archivos al contexto", "اختر مجلدًا/مشروعًا موجودًا لإرفاق بنية الملفات بالسياق"],
["📄 Selecionar Arquivo", "📄 Select File", "📄 选择文件", "📄 फ़ाइल चुनें", "📄 Seleccionar archivo", "📄 اختيار ملف"],
["Selecionar um arquivo para anexar o conteúdo ao contexto", "Select a file to attach its content to the context", "选择一个文件，将其内容附加到上下文中", "संदर्भ में सामग्री जोड़ने के लिए फ़ाइल चुनें", "Selecciona un archivo para adjuntar su contenido al contexto", "اختر ملفًا لإرفاق محتواه بالسياق"],
["Abrir no Explorer", "Open in Explorer", "在资源管理器中打开", "एक्सप्लोरर में खोलें", "Abrir en el Explorador", "فتح في المستكشف"],
["Abrir o caminho selecionado no gerenciador de arquivos do SO", "Open the selected path in the OS file manager", "在操作系统的文件管理器中打开所选路径", "चयनित पथ को OS फ़ाइल प्रबंधक में खोलें", "Abrir la ruta seleccionada en el administrador de archivos del SO", "فتح المسار المحدد في مدير ملفات النظام"],
["Acesso ao Sistema Local", "Local System Access", "本地系统访问", "स्थानीय सिस्टम एक्सेस", "Acceso al sistema local", "الوصول إلى النظام المحلي"],
["Comando a executar (ex.: dir, ls -la, ping ...)", "Command to run (e.g. dir, ls -la, ping ...)", "要执行的命令（例如 dir、ls -la、ping……）", "चलाने हेतु कमांड (जैसे dir, ls -la, ping ...)", "Comando a ejecutar (ej.: dir, ls -la, ping ...)", "الأمر المطلوب تنفيذه (مثال: dir وls -la وping ...)"],
["Executar", "Run", "执行", "चलाएँ", "Ejecutar", "تنفيذ"],
["Saída dos comandos aparecerá aqui...", "Command output will appear here...", "命令输出将显示在此处……", "कमांड आउटपुट यहाँ दिखेगा...", "La salida de los comandos aparecerá aquí...", "سيظهر مخرجات الأوامر هنا..."],
["Enviar saída para a LLM", "Send output to the LLM", "将输出发送给 LLM", "आउटपुट LLM को भेजें", "Enviar salida a la LLM", "إرسال المخرجات إلى LLM"],
["Modelo & Ajustes", "Model & Settings", "模型和设置", "मॉडल और सेटिंग्स", "Modelo y ajustes", "النموذج والإعدادات"],
["Terminal", "Terminal", "终端", "टर्मिनल", "Terminal", "الطرفية"],
["Nenhum provedor configurado.", "No provider configured.", "未配置提供商。", "कोई प्रदाता कॉन्फ़िगर नहीं है।", "Ningún proveedor configurado.", "لم يتم تكوين أي موفر."],
["sem chave", "no key", "无密钥", "कोई कुंजी नहीं", "sin clave", "بدون مفتاح"],
["chave definida", "key set", "已设置密钥", "कुंजी सेट है", "clave establecida", "تم تعيين المفتاح"],
["Servidor: ", "Server: ", "服务器：", "सर्वर: ", "Servidor: ", "الخادم: "],
["Gerenciar Provedores", "Manage Providers", "管理提供商", "प्रदाता प्रबंधित करें", "Administrar proveedores", "إدارة الموفّرين"],
["Adicionar", "Add", "添加", "जोड़ें", "Añadir", "إضافة"],
["Editar", "Edit", "编辑", "संपादित करें", "Editar", "تعديل"],
["Remover", "Remove", "删除", "हटाएँ", "Eliminar", "حذف"],
["Importar do OpenCode", "Import from OpenCode", "从 OpenCode 导入", "OpenCode से आयात करें", "Importar de OpenCode", "استيراد من OpenCode"],
["Fechar", "Close", "关闭", "बंद करें", "Cerrar", "إغلاق"],
["É necessário manter ao menos um provedor.", "At least one provider must be kept.", "必须保留至少一个提供商。", "कम से कम एक प्रदाता रखना आवश्यक है।", "Es necesario mantener al menos un proveedor.", "يجب الاحتفاظ بموفر واحد على الأقل."],
["OpenCode", "OpenCode", "OpenCode", "OpenCode", "OpenCode", "OpenCode"],
["Importado(s) %1 provedor(es) do OpenCode.", "Imported %1 provider(s) from OpenCode.", "已从 OpenCode 导入 %1 个提供商。", "OpenCode से %1 प्रदाता आयात किए गए।", "Importado(s) %1 proveedor(es) de OpenCode.", "تم استيراد %1 موفر(ين) من OpenCode."],
["Nenhum provedor novo encontrado no OpenCode.", "No new provider found in OpenCode.", "在 OpenCode 中未找到新的提供商。", "OpenCode में कोई नया प्रदाता नहीं मिला।", "No se encontró ningún proveedor nuevo en OpenCode.", "لم يتم العثور على موفر جديد في OpenCode."],
["Buscando modelos...", "Fetching models...", "正在获取模型……", "मॉडल प्राप्त कर रहे हैं...", "Buscando modelos...", "جارٍ جلب النماذج..."],
["Erro ao listar modelos: ", "Error listing models: ", "列出模型时出错：", "मॉडल सूचीबद्ध करने में त्रुटि: ", "Error al listar modelos: ", "خطأ في إدراج النماذج: "],
["Nenhum modelo encontrado. Verifique URL base e chave.", "No model found. Check base URL and key.", "未找到模型。请检查基础 URL 和密钥。", "कोई मॉडल नहीं मिला। बेस URL और कुंजी जाँचें।", "No se encontró ningún modelo. Verifica la URL base y la clave.", "لم يتم العثور على نموذج. تحقق من عنوان URL الأساسي والمفتاح."],
["%1 modelo(s) encontrado(s).", "%1 model(s) found.", "已找到 %1 个模型。", "%1 मॉडल मिले।", "%1 modelo(s) encontrado(s).", "تم العثور على %1 نموذج(نماذج)."],
["Baixar Modelo", "Download Model", "下载模型", "मॉडल डाउनलोड करें", "Descargar modelo", "تنزيل نموذج"],
["Nome do modelo a baixar (ex.: llama3.2, dolphin-mistral):", "Model name to download (e.g. llama3.2, dolphin-mistral):", "要下载的模型名称（例如 llama3.2、dolphin-mistral）：", "डाउनलोड हेतु मॉडल का नाम (जैसे llama3.2, dolphin-mistral):", "Nombre del modelo a descargar (ej.: llama3.2, dolphin-mistral):", "اسم النموذج المطلوب تنزيله (مثال: llama3.2 وdolphin-mistral):"],
["Baixando modelo ", "Downloading model ", "正在下载模型 ", "मॉडल डाउनलोड हो रहा है ", "Descargando modelo ", "جارٍ تنزيل النموذج "],
["Baixando... %1 (%2%)", "Downloading... %1 (%2%)", "正在下载……%1（%2%）", "डाउनलोड हो रहा है... %1 (%2%)", "Descargando... %1 (%2%)", "جارٍ التنزيل... %1 (%2%)"],
["Baixando... ", "Downloading... ", "正在下载……", "डाउनलोड हो रहा है... ", "Descargando... ", "جارٍ التنزيل... "],
["Download concluído. Atualizando modelos...", "Download finished. Refreshing models...", "下载完成。正在刷新模型……", "डाउनलोड पूर्ण। मॉडल ताज़ा कर रहे हैं...", "Descarga completada. Actualizando modelos...", "اكتمل التنزيل. جارٍ تحديث النماذج..."],
["📁 %1 item(ns) de contexto anexado(s) a esta conversa.", "📁 %1 context item(s) attached to this conversation.", "📁 已向此对话附加 %1 个上下文项。", "📁 इस वार्तालाप में %1 संदर्भ आइटम जुड़े हैं।", "📁 %1 elemento(s) de contexto adjunto(s) a esta conversación.", "📁 تم إرفاق %1 عنصر(عناصر) سياق بهذه المحادثة."],
["Aguarde", "Wait", "请稍候", "प्रतीक्षा करें", "Espere", "انتظر"],
["Já existe uma resposta sendo gerada.", "A response is already being generated.", "已有正在生成的回复。", "एक उत्तर पहले से उत्पन्न हो रहा है।", "Ya se está generando una respuesta.", "يتم بالفعل إنشاء رد."],
["Sem provedor", "No provider", "没有提供商", "कोई प्रदाता नहीं", "Sin proveedor", "لا يوجد موفر"],
["Configure um provedor antes de enviar.", "Configure a provider before sending.", "请先配置提供商再发送。", "भेजने से पहले एक प्रदाता कॉन्फ़िगर करें।", "Configura un proveedor antes de enviar.", "قم بتكوين موفر قبل الإرسال."],
["Sem modelo", "No model", "没有模型", "कोई मॉडल नहीं", "Sin modelo", "لا يوجد نموذج"],
["Selecione (ou digite) um modelo antes de enviar.", "Select (or type) a model before sending.", "请先选择（或输入）模型再发送。", "भेजने से पहले एक मॉडल चुनें (या लिखें)।", "Selecciona (o escribe) un modelo antes de enviar.", "اختر (أو اكتب) نموذجًا قبل الإرسال."],
["Contexto do sistema de arquivos fornecido pelo usuário:\n", "User-provided file system context:\n", "用户提供的文件系统上下文：\n", "उपयोगकर्ता द्वारा प्रदत्त फ़ाइल सिस्टम संदर्भ:\n", "Contexto del sistema de archivos proporcionado por el usuario:\n", "سياق نظام الملفات المقدم من المستخدم:\n"],
["Gerando resposta...", "Generating response...", "正在生成回复……", "उत्तर उत्पन्न हो रहा है...", "Generando respuesta...", "جارٍ إنشاء الرد..."],
["Pronto.", "Done.", "完成。", "पूर्ण।", "Listo.", "تم."],
["⚠ Erro de conexão: ", "⚠ Connection error: ", "⚠ 连接错误：", "⚠ कनेक्शन त्रुटि: ", "⚠ Error de conexión: ", "⚠ خطأ في الاتصال: "],
["Erro na conexão.", "Connection error.", "连接错误。", "कनेक्शन त्रुटि।", "Error de conexión.", "خطأ في الاتصال."],
["Selecionar Arquivo", "Select File", "选择文件", "फ़ाइल चुनें", "Seleccionar archivo", "اختيار ملف"],
["Arquivo: ", "File: ", "文件：", "फ़ाइल: ", "Archivo: ", "الملف: "],
["\n--- conteúdo ---\n", "\n--- content ---\n", "\n—— 内容 ——\n", "\n--- सामग्री ---\n", "\n--- contenido ---\n", "\n--- المحتوى ---\n"],
["📎 Contexto anexado: ", "📎 Context attached: ", "📎 已附加上下文：", "📎 संदर्भ जुड़ा: ", "📎 Contexto adjunto: ", "📎 السياق المرفق: "],
["Arquivo anexado ao contexto.", "File attached to context.", "文件已附加到上下文。", "फ़ाइल संदर्भ में जुड़ गई।", "Archivo adjunto al contexto.", "تم إرفاق الملف بالسياق."],
["Selecionar Pasta do Projeto", "Select Project Folder", "选择项目文件夹", "प्रोजेक्ट फ़ोल्डर चुनें", "Seleccionar carpeta del proyecto", "اختيار مجلد المشروع"],
["Projeto (pasta): ", "Project (folder): ", "项目（文件夹）：", "प्रोजेक्ट (फ़ोल्डर): ", "Proyecto (carpeta): ", "المشروع (المجلد): "],
["\nEstrutura de arquivos:\n", "\nFile structure:\n", "\n文件结构：\n", "\nफ़ाइल संरचना:\n", "\nEstructura de archivos:\n", "\nبنية الملفات:\n"],
["📁 Projeto anexado: ", "📁 Project attached: ", "📁 已附加项目：", "📁 प्रोजेक्ट जुड़ा: ", "📁 Proyecto adjunto: ", "📁 المشروع المرفق: "],
["Projeto anexado ao contexto.", "Project attached to context.", "项目已附加到上下文。", "प्रोजेक्ट संदर्भ में जुड़ गया।", "Proyecto adjunto al contexto.", "تم إرفاق المشروع بالسياق."],
["Escolha uma pasta para abrir", "Choose a folder to open", "选择要打开的文件夹", "खोलने हेतु फ़ोल्डर चुनें", "Elige una carpeta para abrir", "اختر مجلدًا لفتحه"],
["Erro", "Error", "错误", "त्रुटि", "Error", "خطأ"],
["Não foi possível abrir: ", "Could not open: ", "无法打开：", "खोला नहीं जा सका: ", "No se pudo abrir: ", "تعذر الفتح: "],
["Um comando ainda está em execução.", "A command is still running.", "仍有命令正在执行。", "एक कमांड अभी भी चल रहा है।", "Un comando aún está en ejecución.", "لا يزال هناك أمر قيد التنفيذ."],
["\n[Processo encerrado — código %1]\n", "\n[Process ended — code %1]\n", "\n[进程已结束 —— 代码 %1]\n", "\n[प्रक्रिया समाप्त — कोड %1]\n", "\n[Proceso terminado — código %1]\n", "\n[انتهت العملية — الرمز %1]\n"],
["Analise a seguinte saída de comando executado no terminal:\n```\n", "Analyze the following command output from the terminal:\n```\n", "请分析以下在终端执行的命令输出：\n```\n", "टर्मिनल में चलाए गए निम्न कमांड आउटपुट का विश्लेषण करें:\n```\n", "Analiza la siguiente salida de comando ejecutado en la terminal:\n```\n", "حلّل مخرجات الأمر التالي المنفذ في الطرفية:\n```\n"],
["⌨ Saída do terminal enviada para a LLM.", "⌨ Terminal output sent to the LLM.", "⌨ 终端输出已发送给 LLM。", "⌨ टर्मिनल आउटपुट LLM को भेजा गया।", "⌨ Salida del terminal enviada a la LLM.", "⌨ تم إرسال مخرجات الطرفية إلى LLM."],
["Limpar chat", "Clear chat", "清空对话", "चैट साफ़ करें", "Limpiar chat", "مسح المحادثة"],
["Deseja apagar todo o histórico da conversa e o contexto anexado?", "Do you want to erase the entire conversation history and attached context?", "是否要清除全部对话历史和已附加的上下文？", "क्या आप पूरी वार्तालाप इतिहास और जुड़ा संदर्भ मिटाना चाहते हैं?", "¿Deseas borrar todo el historial de la conversación y el contexto adjunto?", "هل تريد مسح سجل المحادثة بالكامل والسياق المرفق؟"],
["Chat limpo.", "Chat cleared.", "对话已清空。", "चैट साफ़ हो गई।", "Chat limpiado.", "تم مسح المحادثة."],
["Exportar", "Export", "导出", "निर्यात करें", "Exportar", "تصدير"],
["Não há conversa para exportar.", "There is no conversation to export.", "没有可导出的对话。", "निर्यात हेतु कोई वार्तालाप नहीं है।", "No hay conversación para exportar.", "لا توجد محادثة للتصدير."],
["Markdown (*.md);;Texto puro (*.txt)", "Markdown (*.md);;Plain text (*.txt)", "Markdown (*.md)；；纯文本 (*.txt)", "Markdown (*.md);;सादा पाठ (*.txt)", "Markdown (*.md);;Texto plano (*.txt)", "Markdown (*.md);;نص عادي (*.txt)"],
["# Conversa LLM Studio\n\n", "# LLM Studio Conversation\n\n", "# LLM Studio 对话\n\n", "# LLM Studio वार्तालाप\n\n", "# Conversación de LLM Studio\n\n", "# محادثة LLM Studio\n\n"],
["**System Prompt:**\n\n", "**System Prompt:**\n\n", "**系统提示词：**\n\n", "**सिस्टम प्रॉम्प्ट:**\n\n", "**Prompt del sistema:**\n\n", "**موجّه النظام:**\n\n"],
["System Prompt:\n", "System Prompt:\n", "系统提示词：\n", "सिस्टम प्रॉम्प्ट:\n", "Prompt del sistema:\n", "موجّه النظام:\n"],
["## 👤 Usuário", "## 👤 User", "## 👤 用户", "## 👤 उपयोगकर्ता", "## 👤 Usuario", "## 👤 المستخدم"],
["## 🤖 Assistente — ", "## 🤖 Assistant — ", "## 🤖 助手 —— ", "## 🤖 सहायक — ", "## 🤖 Asistente — ", "## 🤖 المساعد — "],
["Usuário:", "User:", "用户：", "उपयोगकर्ता:", "Usuario:", "المستخدم:"],
["Assistente — ", "Assistant — ", "助手 —— ", "सहायक — ", "Asistente — ", "المساعد — "],
["Não foi possível gravar o arquivo:\n", "Could not write the file:\n", "无法写入文件：\n", "फ़ाइल लिखी नहीं जा सकी:\n", "No se pudo escribir el archivo:\n", "تعذر كتابة الملف:\n"],
["Conversa exportada para ", "Conversation exported to ", "对话已导出到 ", "वार्तालाप निर्यात किया गया: ", "Conversación exportada a ", "تم تصدير المحادثة إلى "],
["👤 Você", "👤 You", "👤 你", "👤 आप", "👤 Tú", "👤 أنت"],
["🤖 Assistente", "🤖 Assistant", "🤖 助手", "🤖 सहायक", "🤖 Asistente", "🤖 المساعد"],
]

LANGS = [
    ("en", "en_US"),
    ("zh", "zh_CN"),
    ("hi", "hi_IN"),
    ("es", "es_ES"),
    ("ar", "ar_SA"),
]

COL = {"en": 1, "zh": 2, "hi": 3, "es": 4, "ar": 5}

outdir = os.path.dirname(os.path.abspath(__file__))
os.makedirs(outdir, exist_ok=True)

for code, locale in LANGS:
    col = COL[code]
    lines = []
    lines.append('<?xml version="1.0" encoding="utf-8"?>')
    lines.append('<!DOCTYPE TS>')
    lines.append('<TS version="2.1" language="%s">' % locale)
    lines.append('<context>')
    lines.append('    <name>MainWindow</name>')
    for row in T:
        src = row[0]
        trans = row[col]
        lines.append('    <message>')
        lines.append('        <source>%s</source>' % su.escape(src))
        lines.append('        <translation>%s</translation>' % su.escape(trans))
        lines.append('    </message>')
    lines.append('</context>')
    lines.append('</TS>')
    path = os.path.join(outdir, "llm_studio_%s.ts" % code)
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("Gerado:", path)
