#include "mainwindow.h"

#include <QComboBox>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QSplitter>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextCursor>
#include <QTextDocument>
#include <QScrollBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QApplication>
#include <QStatusBar>
#include <QSettings>
#include <QTimer>
#include <QSignalBlocker>
#include <QRegularExpression>
#include <QSet>

// =====================================================================
// Funções utilitárias (escopo de arquivo)
// =====================================================================
namespace {

const int kMaxFileContextBytes = 40 * 1024;

QString joinUrl(const QString &base, const QString &path)
{
    QString b = base;
    while (b.endsWith(QLatin1Char('/')))
        b.chop(1);
    return b + path;
}

// Normaliza a URL base do Ollama: remove barra final e o sufixo "/api".
QString normalizeOllamaBase(QString base)
{
    base = base.trimmed();
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    if (base.endsWith(QStringLiteral("/api"), Qt::CaseInsensitive))
        base = base.left(base.size() - 4);
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    return base;
}

// Remove comentários de um arquivo JSONC (// e /* */), respeitando strings.
QString stripJsonComments(const QString &src)
{
    QString out;
    out.reserve(src.size());
    bool inString = false;
    bool lineComment = false;
    bool blockComment = false;

    for (int i = 0; i < src.size(); ++i) {
        const QChar c = src.at(i);
        const QChar n = (i + 1 < src.size()) ? src.at(i + 1) : QChar();

        if (lineComment) {
            if (c == QLatin1Char('\n')) {
                lineComment = false;
                out += c;
            }
            continue;
        }
        if (blockComment) {
            if (c == QLatin1Char('*') && n == QLatin1Char('/')) {
                blockComment = false;
                ++i;
            }
            continue;
        }
        if (inString) {
            out += c;
            if (c == QLatin1Char('\\') && i + 1 < src.size()) {
                out += src.at(i + 1);
                ++i;
                continue;
            }
            if (c == QLatin1Char('"'))
                inString = false;
            continue;
        }
        if (c == QLatin1Char('"')) {
            inString = true;
            out += c;
            continue;
        }
        if (c == QLatin1Char('/') && n == QLatin1Char('/')) {
            lineComment = true;
            ++i;
            continue;
        }
        if (c == QLatin1Char('/') && n == QLatin1Char('*')) {
            blockComment = true;
            ++i;
            continue;
        }
        out += c;
    }
    return out;
}

// Formatação inline de Markdown (código, negrito, itálico, links).
QString formatInline(const QString &input)
{
    static const QRegularExpression boldRe(QStringLiteral(R"(\*\*(.+?)\*\*)"));
    static const QRegularExpression italicRe(QStringLiteral(R"(?<!\*)\*([^*]+?)\*(?!\*))"));
    static const QRegularExpression linkRe(QStringLiteral(R"(\[([^\]]+)\]\(([^)\s]+)\)"));

    // Protege trechos de código inline (entre crases).
    const QStringList parts = input.split(QLatin1Char('`'));
    QString out;
    for (int i = 0; i < parts.size(); ++i) {
        if (i % 2 == 1) {
            out += QStringLiteral("<code>") + parts.at(i).toHtmlEscaped() + QStringLiteral("</code>");
        } else {
            QString t = parts.at(i).toHtmlEscaped();
            t.replace(boldRe, QStringLiteral("<b>\\1</b>"));
            t.replace(italicRe, QStringLiteral("<i>\\1</i>"));
            t.replace(linkRe, QStringLiteral("<a href=\"\\2\">\\1</a>"));
            out += t;
        }
    }
    return out;
}

// Converte Markdown (blocos + inline) em HTML simples.
QString markdownToHtml(const QString &md)
{
    static const QRegularExpression hRe(QStringLiteral(R"(^(#{1,6})\s+(.*)$)"));
    static const QRegularExpression olRe(QStringLiteral(R"(^\d+\.\s+(.*)$)"));

    const QStringList lines = md.split(QLatin1Char('\n'));
    QStringList html;
    bool inCode = false;
    QStringList codeLines;
    QStringList para;
    QStringList list;
    int listKind = 0; // 0 = nenhum, 1 = <ul>, 2 = <ol>

    auto flushPara = [&]() {
        if (para.isEmpty())
            return;
        html << QStringLiteral("<p>") + formatInline(para.join(QStringLiteral("<br>"))) + QStringLiteral("</p>");
        para.clear();
    };
    auto flushList = [&]() {
        if (list.isEmpty())
            return;
        const QString tag = (listKind == 2) ? QStringLiteral("ol") : QStringLiteral("ul");
        html << QStringLiteral("<") + tag + QStringLiteral(">");
        for (const QString &item : list)
            html << QStringLiteral("<li>") + formatInline(item) + QStringLiteral("</li>");
        html << QStringLiteral("</") + tag + QStringLiteral(">");
        list.clear();
        listKind = 0;
    };

    for (const QString &raw : lines) {
        const QString t = raw.trimmed();

        // Bloco de código delimitado por ```.
        if (t.startsWith(QStringLiteral("```"))) {
            flushPara();
            flushList();
            if (!inCode) {
                inCode = true;
                codeLines.clear();
            } else {
                html << QStringLiteral("<pre><code>")
                        + codeLines.join(QLatin1Char('\n')).toHtmlEscaped()
                        + QStringLiteral("</code></pre>");
                inCode = false;
                codeLines.clear();
            }
            continue;
        }
        if (inCode) {
            codeLines << raw;
            continue;
        }

        if (t.isEmpty()) {
            flushPara();
            flushList();
            continue;
        }

        if (t == QStringLiteral("---") || t == QStringLiteral("***") || t == QStringLiteral("___")) {
            flushPara();
            flushList();
            html << QStringLiteral("<hr>");
            continue;
        }

        // Cabeçalhos.
        const auto hm = hRe.match(raw);
        if (hm.hasMatch()) {
            flushPara();
            flushList();
            const QString tag = QStringLiteral("h") + QString::number(hm.captured(1).size());
            html << QStringLiteral("<") + tag + QStringLiteral(">")
                    + formatInline(hm.captured(2).trimmed())
                    + QStringLiteral("</") + tag + QStringLiteral(">");
            continue;
        }

        // Citação.
        if (t.startsWith(QLatin1Char('>'))) {
            flushPara();
            flushList();
            html << QStringLiteral("<blockquote>") + formatInline(t.mid(1).trimmed()) + QStringLiteral("</blockquote>");
            continue;
        }

        // Lista não ordenada.
        if (t.startsWith(QStringLiteral("- ")) || t.startsWith(QStringLiteral("* "))
            || t.startsWith(QStringLiteral("+ "))) {
            if (listKind != 1) {
                flushPara();
                flushList();
                listKind = 1;
            }
            list << t.mid(2).trimmed();
            continue;
        }

        // Lista ordenada.
        const auto om = olRe.match(t);
        if (om.hasMatch()) {
            if (listKind != 2) {
                flushPara();
                flushList();
                listKind = 2;
            }
            list << om.captured(1).trimmed();
            continue;
        }

        flushList();
        para << t;
    }

    flushPara();
    flushList();
    if (inCode)
        html << QStringLiteral("<pre><code>")
                + codeLines.join(QLatin1Char('\n')).toHtmlEscaped()
                + QStringLiteral("</code></pre>");

    return html.join(QLatin1Char('\n'));
}

// Diálogo de adição/edição de um provedor.
bool editProviderDialog(ProviderConfig &cfg, QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(cfg.name.isEmpty() ? QStringLiteral("Novo Provedor") : QStringLiteral("Editar Provedor"));
    dlg.resize(440, 200);

    auto *form = new QFormLayout(&dlg);

    auto *name = new QLineEdit(cfg.name);
    auto *kind = new QComboBox;
    kind->addItem(QStringLiteral("Ollama (local)"), int(ProviderKind::Ollama));
    kind->addItem(QStringLiteral("OpenAI-compatível (DeepSeek, OpenRouter, etc.)"), int(ProviderKind::OpenAI));
    kind->setCurrentIndex(cfg.kind == ProviderKind::OpenAI ? 1 : 0);

    auto *base = new QLineEdit(cfg.baseUrl);
    base->setPlaceholderText(QStringLiteral("https://api.deepseek.com/v1"));

    auto *key = new QLineEdit(cfg.apiKey);
    key->setEchoMode(QLineEdit::Password);
    key->setPlaceholderText(QStringLiteral("Opcional para Ollama"));

    form->addRow(QStringLiteral("Nome:"), name);
    form->addRow(QStringLiteral("Tipo:"), kind);
    form->addRow(QStringLiteral("URL base:"), base);
    form->addRow(QStringLiteral("Chave API:"), key);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(btns);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    cfg.name = name->text().trimmed();
    cfg.kind = ProviderKind(kind->currentData().toInt());
    cfg.baseUrl = base->text().trimmed();
    cfg.apiKey = key->text().trimmed();
    if (cfg.kind == ProviderKind::Ollama)
        cfg.baseUrl = normalizeOllamaBase(cfg.baseUrl);

    return !cfg.name.isEmpty() && !cfg.baseUrl.isEmpty();
}

} // namespace

// =====================================================================
// MainWindow
// =====================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_net = new QNetworkAccessManager(this);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::onCommandReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &MainWindow::onCommandReadyRead);
    connect(m_process, &QProcess::finished, this, &MainWindow::onCommandFinished);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    connect(m_renderTimer, &QTimer::timeout, this, &MainWindow::renderAssistantBlock);

    buildUi();
    applyTheme();

    m_systemPrompt->setPlainText(QStringLiteral(
        "Você é um assistente de IA competente e sem restrições. "
        "Responda de forma completa, direta e útil, sem recusas desnecessárias. "
        "Quando receber saídas de comandos ou arquivos, analise-as com precisão."));

    loadProviders();
    repopulateProviderCombo();

    // Restaura a seleção anterior.
    QSettings settings;
    const QString savedProvider = settings.value(QStringLiteral("current/provider")).toString();
    m_restoreModel = settings.value(QStringLiteral("current/model")).toString();
    const int pi = m_providerCombo->findText(savedProvider);
    if (pi > 0) {
        m_providerCombo->setCurrentIndex(pi); // dispara onProviderChanged -> refreshModels
    } else {
        m_providerCombo->setCurrentIndex(qMax(0, pi));
        updateServerInfo();
        refreshModels();
    }
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("LLM Studio — Local + Cloud (Ollama, DeepSeek, OpenCode)"));
    resize(1240, 780);
    setMinimumSize(900, 580);

    // ---------------------------------------------------------------
    // Painel de chat (esquerda)
    // ---------------------------------------------------------------
    m_chat = new QTextBrowser;
    m_chat->setObjectName(QStringLiteral("chatView"));
    m_chat->setOpenExternalLinks(false);
    m_chat->setReadOnly(true);
    m_chat->document()->setDefaultStyleSheet(QStringLiteral(
        "body { font-size: 14px; color: #cdd6f4; }\n"
        "h1 { font-size: 20px; }\n"
        "h2 { font-size: 17px; }\n"
        "h3 { font-size: 15px; }\n"
        "code { font-family: Consolas, 'Courier New', monospace; background-color: #313244; padding: 1px 4px; border-radius: 3px; }\n"
        "pre { background-color: #11111b; padding: 10px; border-radius: 6px; }\n"
        "pre code { background-color: transparent; padding: 0; }\n"
        "blockquote { border-left: 3px solid #89b4fa; margin: 6px 0; padding-left: 10px; color: #a6adc8; }\n"
        "a { color: #89b4fa; }\n"
        "hr { border: none; border-top: 1px solid #45475a; }"));

    m_input = new QPlainTextEdit;
    m_input->setObjectName(QStringLiteral("inputBox"));
    m_input->setPlaceholderText(QStringLiteral(
        "Digite sua mensagem... (Enter envia, Shift+Enter quebra linha)"));
    m_input->setFixedHeight(92);
    m_input->installEventFilter(this);

    m_sendBtn = new QPushButton(QStringLiteral("Enviar"));
    m_sendBtn->setObjectName(QStringLiteral("sendBtn"));
    m_sendBtn->setFixedHeight(92);
    m_sendBtn->setMinimumWidth(120);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::sendMessage);

    m_clearBtn = new QPushButton(QStringLiteral("Limpar Chat"));
    m_clearBtn->setObjectName(QStringLiteral("clearBtn"));
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::clearChat);

    m_exportBtn = new QPushButton(QStringLiteral("Exportar Conversa"));
    m_exportBtn->setObjectName(QStringLiteral("exportBtn"));
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportConversation);

    auto *chatButtons = new QHBoxLayout;
    chatButtons->addWidget(m_clearBtn);
    chatButtons->addWidget(m_exportBtn);
    chatButtons->addStretch();

    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_input, 1);
    inputRow->addWidget(m_sendBtn);

    auto *chatPanel = new QWidget;
    auto *chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setContentsMargins(8, 8, 8, 8);
    chatLayout->addWidget(m_chat, 1);
    chatLayout->addLayout(chatButtons);
    chatLayout->addLayout(inputRow);

    // ---------------------------------------------------------------
    // Painel lateral (direita)
    // ---------------------------------------------------------------
    // --- Provedor & Modelo ---
    m_providerCombo = new QComboBox;
    m_providerCombo->setToolTip(QStringLiteral("Provedor de IA a ser usado"));
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProviderChanged);

    auto *manageBtn = new QPushButton(QStringLiteral("Gerenciar"));
    manageBtn->setToolTip(QStringLiteral("Adicionar/editar/remover provedores"));
    connect(manageBtn, &QPushButton::clicked, this, &MainWindow::manageProviders);

    auto *providerRow = new QHBoxLayout;
    providerRow->addWidget(m_providerCombo, 1);
    providerRow->addWidget(manageBtn);

    m_modelCombo = new QComboBox;
    m_modelCombo->setEditable(true);
    m_modelCombo->setPlaceholderText(QStringLiteral("Selecione ou digite o modelo..."));
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelChanged);
    if (m_modelCombo->lineEdit())
        connect(m_modelCombo->lineEdit(), &QLineEdit::editingFinished, this, &MainWindow::onModelChanged);

    auto *refreshBtn = new QPushButton(QStringLiteral("↻"));
    refreshBtn->setObjectName(QStringLiteral("refreshBtn"));
    refreshBtn->setToolTip(QStringLiteral("Atualizar lista de modelos"));
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshModels);

    m_pullBtn = new QPushButton(QStringLiteral("Baixar"));
    m_pullBtn->setObjectName(QStringLiteral("pullBtn"));
    m_pullBtn->setToolTip(QStringLiteral("Baixar um modelo do Ollama (ollama pull)"));
    connect(m_pullBtn, &QPushButton::clicked, this, &MainWindow::pullModel);

    auto *modelRow = new QHBoxLayout;
    modelRow->addWidget(m_modelCombo, 1);
    modelRow->addWidget(refreshBtn);
    modelRow->addWidget(m_pullBtn);

    auto *importBtn = new QPushButton(QStringLiteral("Importar provedores do OpenCode"));
    importBtn->setToolTip(QStringLiteral("Lê a configuração do OpenCode (~/.config/opencode) e importa os provedores/chaves"));
    connect(importBtn, &QPushButton::clicked, this, &MainWindow::importFromOpenCode);

    m_serverInfo = new QLabel;
    m_serverInfo->setObjectName(QStringLiteral("serverInfo"));
    m_serverInfo->setWordWrap(true);

    auto *providerGroup = new QGroupBox(QStringLiteral("Provedor & Modelo"));
    auto *providerLayout = new QVBoxLayout(providerGroup);
    providerLayout->addLayout(providerRow);
    providerLayout->addLayout(modelRow);
    providerLayout->addWidget(importBtn);
    providerLayout->addWidget(m_serverInfo);

    // --- Parâmetros ---
    m_tempSpin = new QDoubleSpinBox;
    m_tempSpin->setRange(0.0, 2.0);
    m_tempSpin->setSingleStep(0.1);
    m_tempSpin->setDecimals(2);
    m_tempSpin->setValue(0.7);

    m_topPSpin = new QDoubleSpinBox;
    m_topPSpin->setRange(0.0, 1.0);
    m_topPSpin->setSingleStep(0.05);
    m_topPSpin->setDecimals(2);
    m_topPSpin->setValue(0.9);

    auto *paramsGroup = new QGroupBox(QStringLiteral("Parâmetros"));
    auto *paramsForm = new QFormLayout(paramsGroup);
    paramsForm->addRow(QStringLiteral("Temperature"), m_tempSpin);
    paramsForm->addRow(QStringLiteral("Top P"), m_topPSpin);

    // --- System Prompt ---
    m_systemPrompt = new QPlainTextEdit;
    m_systemPrompt->setObjectName(QStringLiteral("systemPrompt"));
    m_systemPrompt->setFixedHeight(130);

    auto *promptGroup = new QGroupBox(QStringLiteral("System Prompt"));
    auto *promptLayout = new QVBoxLayout(promptGroup);
    promptLayout->addWidget(m_systemPrompt);

    // --- Acesso ao sistema local ---
    m_pathBtn = new QToolButton;
    m_pathBtn->setText(QStringLiteral("Selecionar Pasta/Arquivo"));
    m_pathBtn->setObjectName(QStringLiteral("pathBtn"));
    m_pathBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    m_pathBtn->setPopupMode(QToolButton::MenuButtonPopup);
    m_pathBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *pathMenu = new QMenu(m_pathBtn);
    pathMenu->addAction(QStringLiteral("Selecionar Arquivo..."), this, &MainWindow::selectFile);
    pathMenu->addAction(QStringLiteral("Selecionar Pasta..."), this, &MainWindow::selectFolder);
    m_pathBtn->setMenu(pathMenu);
    connect(m_pathBtn, &QToolButton::clicked, this, &MainWindow::selectFile);

    m_explorerBtn = new QPushButton(QStringLiteral("Abrir no Explorer"));
    m_explorerBtn->setObjectName(QStringLiteral("explorerBtn"));
    m_explorerBtn->setToolTip(QStringLiteral("Abrir o caminho selecionado no gerenciador de arquivos do SO"));
    connect(m_explorerBtn, &QPushButton::clicked, this, &MainWindow::openInExplorer);

    auto *systemGroup = new QGroupBox(QStringLiteral("Acesso ao Sistema Local"));
    auto *systemLayout = new QVBoxLayout(systemGroup);
    systemLayout->addWidget(m_pathBtn);
    systemLayout->addWidget(m_explorerBtn);

    auto *settingsTab = new QWidget;
    auto *settingsLayout = new QVBoxLayout(settingsTab);
    settingsLayout->setContentsMargins(8, 8, 8, 8);
    settingsLayout->addWidget(providerGroup);
    settingsLayout->addWidget(paramsGroup);
    settingsLayout->addWidget(promptGroup);
    settingsLayout->addWidget(systemGroup);
    settingsLayout->addStretch();

    // --- Terminal ---
    m_termInput = new QLineEdit;
    m_termInput->setPlaceholderText(QStringLiteral("Comando a executar (ex.: dir, ls -la, ping ...)"));

    m_runBtn = new QPushButton(QStringLiteral("Executar"));
    m_runBtn->setObjectName(QStringLiteral("runBtn"));
    connect(m_runBtn, &QPushButton::clicked, this, &MainWindow::runCommand);

    auto *termRow = new QHBoxLayout;
    termRow->addWidget(m_termInput, 1);
    termRow->addWidget(m_runBtn);

    m_termOutput = new QPlainTextEdit;
    m_termOutput->setObjectName(QStringLiteral("termOutput"));
    m_termOutput->setReadOnly(true);
    m_termOutput->setPlaceholderText(QStringLiteral("Saída dos comandos aparecerá aqui..."));

    m_sendOutputBtn = new QPushButton(QStringLiteral("Enviar saída para a LLM"));
    m_sendOutputBtn->setObjectName(QStringLiteral("sendOutputBtn"));
    m_sendOutputBtn->setEnabled(false);
    connect(m_sendOutputBtn, &QPushButton::clicked, this, &MainWindow::sendTerminalOutputToPrompt);

    auto *termTab = new QWidget;
    auto *termLayout = new QVBoxLayout(termTab);
    termLayout->setContentsMargins(8, 8, 8, 8);
    termLayout->addLayout(termRow);
    termLayout->addWidget(m_termOutput, 1);
    termLayout->addWidget(m_sendOutputBtn);

    auto *tabs = new QTabWidget;
    tabs->addTab(settingsTab, QStringLiteral("Modelo & Ajustes"));
    tabs->addTab(termTab, QStringLiteral("Terminal"));

    auto *sidePanel = new QWidget;
    sidePanel->setObjectName(QStringLiteral("sidePanel"));
    sidePanel->setMinimumWidth(310);
    sidePanel->setMaximumWidth(440);
    auto *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->addWidget(tabs);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(chatPanel);
    splitter->addWidget(sidePanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({ 830, 360 });

    m_statusLabel = new QLabel(QStringLiteral("Conectando..."));
    m_statusLabel->setObjectName(QStringLiteral("statusLabel"));
    statusBar()->addWidget(m_statusLabel, 1);

    auto *central = new QWidget;
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(splitter);
    setCentralWidget(central);
}

void MainWindow::applyTheme()
{
    setStyleSheet(QStringLiteral(R"QSS(
        QWidget {
            background-color: #1e1e2e;
            color: #cdd6f4;
            font-family: 'Segoe UI', 'Noto Sans', 'DejaVu Sans', sans-serif;
            font-size: 13px;
        }
        QMainWindow, QStatusBar {
            background-color: #181825;
        }
        QStatusBar QLabel#statusLabel {
            color: #a6adc8;
            padding: 2px 6px;
        }
        QLabel#serverInfo {
            color: #a6adc8;
            font-size: 12px;
        }
        QSplitter::handle {
            background-color: #313244;
            width: 2px;
        }
        QTabWidget::pane {
            border: 1px solid #313244;
            border-radius: 6px;
            top: -1px;
        }
        QTabBar::tab {
            background: #1e1e2e;
            color: #a6adc8;
            padding: 8px 14px;
            border: 1px solid #313244;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QTabBar::tab:selected {
            background: #313244;
            color: #cdd6f4;
        }
        QGroupBox {
            border: 1px solid #313244;
            border-radius: 6px;
            margin-top: 12px;
            padding-top: 8px;
            font-weight: bold;
            color: #b4befe;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QComboBox, QLineEdit, QDoubleSpinBox, QPlainTextEdit, QTextBrowser {
            background-color: #181825;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 6px;
            selection-background-color: #585b70;
        }
        QComboBox:hover, QLineEdit:hover, QDoubleSpinBox:hover {
            border-color: #585b70;
        }
        QComboBox:focus, QLineEdit:focus, QDoubleSpinBox:focus,
        QPlainTextEdit:focus, QTextBrowser:focus {
            border-color: #89b4fa;
        }
        QComboBox QAbstractItemView {
            background-color: #181825;
            border: 1px solid #45475a;
            selection-background-color: #585b70;
        }
        QPushButton {
            background-color: #313244;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 8px 12px;
            color: #cdd6f4;
        }
        QPushButton:hover {
            background-color: #45475a;
        }
        QPushButton:pressed {
            background-color: #585b70;
        }
        QPushButton:disabled {
            color: #6c7086;
            background-color: #26263a;
        }
        QPushButton#sendBtn, QPushButton#runBtn, QPushButton#sendOutputBtn {
            background-color: #89b4fa;
            color: #11111b;
            font-weight: bold;
        }
        QPushButton#sendBtn:hover, QPushButton#runBtn:hover, QPushButton#sendOutputBtn:hover {
            background-color: #a6c8ff;
        }
        QPushButton#sendBtn:disabled {
            background-color: #585b70;
            color: #313244;
        }
        QPushButton#pullBtn {
            background-color: #cba6f7;
            color: #11111b;
            font-weight: bold;
        }
        QPushButton#pullBtn:hover {
            background-color: #d9bcff;
        }
        QPushButton#pullBtn:disabled {
            background-color: #585b70;
            color: #313244;
        }
        QToolButton#pathBtn {
            background-color: #a6e3a1;
            color: #11111b;
            font-weight: bold;
            padding: 8px 12px;
            border-radius: 6px;
            border: none;
        }
        QToolButton#pathBtn:hover {
            background-color: #bdeab8;
        }
        QToolButton#pathBtn::menu-indicator {
            subcontrol-position: right center;
            subcontrol-origin: padding;
            right: 8px;
        }
        QScrollBar:vertical {
            background: #181825;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #45475a;
            border-radius: 6px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #585b70;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QTextBrowser#chatView {
            background-color: #181825;
        }
    )QSS"));
}

void MainWindow::setStatus(const QString &text, bool isError)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(isError
        ? QStringLiteral("color: #f38ba8;")
        : QStringLiteral("color: #a6adc8;"));
}

// =====================================================================
// Provedores
// =====================================================================

ProviderConfig *MainWindow::currentProvider()
{
    const int idx = m_providerCombo->currentIndex();
    if (idx < 0 || idx >= m_providers.size())
        return nullptr;
    return &m_providers[idx];
}

const ProviderConfig *MainWindow::currentProvider() const
{
    const int idx = m_providerCombo->currentIndex();
    if (idx < 0 || idx >= m_providers.size())
        return nullptr;
    return &m_providers.at(idx);
}

QString MainWindow::ollamaBaseUrl() const
{
    return normalizeOllamaBase(currentProvider() ? currentProvider()->baseUrl
                                                 : QStringLiteral("http://localhost:11434"));
}

void MainWindow::loadProviders()
{
    QSettings settings;
    const int n = settings.value(QStringLiteral("providers/size"), 0).toInt();
    m_providers.clear();
    for (int i = 0; i < n; ++i) {
        ProviderConfig pc;
        pc.name = settings.value(QStringLiteral("providers/%1/name").arg(i)).toString();
        pc.kind = ProviderKind(settings.value(QStringLiteral("providers/%1/kind").arg(i), 0).toInt());
        pc.baseUrl = settings.value(QStringLiteral("providers/%1/baseUrl").arg(i)).toString();
        pc.apiKey = settings.value(QStringLiteral("providers/%1/apiKey").arg(i)).toString();
        if (!pc.name.isEmpty())
            m_providers.append(pc);
    }
    if (m_providers.isEmpty()) {
        ProviderConfig ollama;
        ollama.name = QStringLiteral("Ollama (local)");
        ollama.kind = ProviderKind::Ollama;
        ollama.baseUrl = QStringLiteral("http://localhost:11434");
        m_providers.append(ollama);
    }
}

void MainWindow::saveProviders()
{
    QSettings settings;
    settings.setValue(QStringLiteral("providers/size"), m_providers.size());
    for (int i = 0; i < m_providers.size(); ++i) {
        settings.setValue(QStringLiteral("providers/%1/name").arg(i), m_providers[i].name);
        settings.setValue(QStringLiteral("providers/%1/kind").arg(i), int(m_providers[i].kind));
        settings.setValue(QStringLiteral("providers/%1/baseUrl").arg(i), m_providers[i].baseUrl);
        settings.setValue(QStringLiteral("providers/%1/apiKey").arg(i), m_providers[i].apiKey);
    }
}

void MainWindow::repopulateProviderCombo()
{
    const QSignalBlocker blocker(m_providerCombo);
    const QString prev = m_providerCombo->currentText();
    m_providerCombo->clear();
    for (const auto &p : m_providers)
        m_providerCombo->addItem(p.name);
    const int idx = m_providerCombo->findText(prev);
    m_providerCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MainWindow::saveCurrentSelection()
{
    QSettings settings;
    settings.setValue(QStringLiteral("current/provider"), m_providerCombo->currentText());
    settings.setValue(QStringLiteral("current/model"), m_modelCombo->currentText());
}

void MainWindow::updateServerInfo()
{
    const ProviderConfig *p = currentProvider();
    if (!p) {
        m_serverInfo->setText(QStringLiteral("Nenhum provedor configurado."));
        m_pullBtn->setEnabled(false);
        return;
    }
    m_pullBtn->setEnabled(p->kind == ProviderKind::Ollama);
    const QString keyInfo = p->apiKey.isEmpty() ? QStringLiteral("sem chave") : QStringLiteral("chave definida");
    m_serverInfo->setText(QStringLiteral("Servidor: ") + p->baseUrl
                          + QStringLiteral("\n(") + keyInfo + QStringLiteral(")"));
}

void MainWindow::onProviderChanged()
{
    saveCurrentConversation();
    updateServerInfo();
    refreshModels();
    saveCurrentSelection();
}

void MainWindow::manageProviders()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Gerenciar Provedores"));
    dlg.resize(520, 400);

    auto *v = new QVBoxLayout(&dlg);

    auto *list = new QListWidget;
    auto refreshList = [list, this]() {
        list->clear();
        for (const auto &p : m_providers) {
            const QString kind = (p.kind == ProviderKind::OpenAI) ? QStringLiteral("API") : QStringLiteral("Ollama");
            list->addItem(p.name + QStringLiteral("   [") + kind + QStringLiteral("]"));
        }
    };
    refreshList();
    v->addWidget(list);

    auto *addBtn = new QPushButton(QStringLiteral("Adicionar"));
    auto *editBtn = new QPushButton(QStringLiteral("Editar"));
    auto *delBtn = new QPushButton(QStringLiteral("Remover"));
    auto *importBtn = new QPushButton(QStringLiteral("Importar do OpenCode"));
    auto *row = new QHBoxLayout;
    row->addWidget(addBtn);
    row->addWidget(editBtn);
    row->addWidget(delBtn);
    row->addWidget(importBtn);
    v->addLayout(row);

    auto *closeBtn = new QPushButton(QStringLiteral("Fechar"));
    v->addWidget(closeBtn);

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(addBtn, &QPushButton::clicked, &dlg, [&]() {
        ProviderConfig pc;
        pc.baseUrl = QStringLiteral("http://localhost:11434");
        if (editProviderDialog(pc, &dlg)) {
            m_providers.append(pc);
            refreshList();
        }
    });
    connect(editBtn, &QPushButton::clicked, &dlg, [&]() {
        const int i = list->currentRow();
        if (i >= 0 && i < m_providers.size()) {
            ProviderConfig pc = m_providers[i];
            if (editProviderDialog(pc, &dlg)) {
                m_providers[i] = pc;
                refreshList();
            }
        }
    });
    connect(delBtn, &QPushButton::clicked, &dlg, [&]() {
        const int i = list->currentRow();
        if (m_providers.size() <= 1) {
            QMessageBox::information(&dlg, QStringLiteral("Remover"),
                QStringLiteral("É necessário manter ao menos um provedor."));
            return;
        }
        if (i >= 0 && i < m_providers.size()) {
            m_providers.removeAt(i);
            refreshList();
        }
    });
    connect(importBtn, &QPushButton::clicked, &dlg, [&]() {
        importFromOpenCode();
        refreshList();
    });

    dlg.exec(); // ao fechar, persiste e recarrega o combo
    saveProviders();
    repopulateProviderCombo();
    updateServerInfo();
    if (m_providerCombo->currentIndex() >= 0)
        refreshModels();
}

void MainWindow::importFromOpenCode()
{
    const QString home = QDir::homePath();

    QString configPath = home + QStringLiteral("/.config/opencode/opencode.jsonc");
    if (!QFile::exists(configPath))
        configPath = home + QStringLiteral("/.config/opencode/opencode.json");
    const QString authPath = home + QStringLiteral("/.local/share/opencode/auth.json");

    QJsonObject providersObj;
    if (QFile::exists(configPath)) {
        QFile f(configPath);
        if (f.open(QIODevice::ReadOnly)) {
            const QString raw = stripJsonComments(QString::fromUtf8(f.readAll()));
            providersObj = QJsonDocument::fromJson(raw.toUtf8()).object()
                               .value(QStringLiteral("provider")).toObject();
            f.close();
        }
    }

    QJsonObject authObj;
    if (QFile::exists(authPath)) {
        QFile f(authPath);
        if (f.open(QIODevice::ReadOnly)) {
            authObj = QJsonDocument::fromJson(f.readAll()).object();
            f.close();
        }
    }

    QSet<QString> names;
    for (auto it = providersObj.begin(); it != providersObj.end(); ++it)
        names.insert(it.key());
    for (auto it = authObj.begin(); it != authObj.end(); ++it)
        names.insert(it.key());

    QHash<QString, QString> defaults;
    defaults.insert(QStringLiteral("openai"), QStringLiteral("https://api.openai.com/v1"));
    defaults.insert(QStringLiteral("deepseek"), QStringLiteral("https://api.deepseek.com/v1"));
    defaults.insert(QStringLiteral("openrouter"), QStringLiteral("https://openrouter.ai/api/v1"));
    defaults.insert(QStringLiteral("groq"), QStringLiteral("https://api.groq.com/openai/v1"));
    defaults.insert(QStringLiteral("anthropic"), QStringLiteral("https://api.anthropic.com/v1"));
    defaults.insert(QStringLiteral("mistral"), QStringLiteral("https://api.mistral.ai/v1"));
    defaults.insert(QStringLiteral("together"), QStringLiteral("https://api.together.xyz/v1"));
    defaults.insert(QStringLiteral("xai"), QStringLiteral("https://api.x.ai/v1"));
    defaults.insert(QStringLiteral("google"), QStringLiteral("https://generativelanguage.googleapis.com/v1beta"));

    int added = 0;
    for (const QString &name : names) {
        const QJsonObject cfg = providersObj.value(name).toObject();
        const QJsonObject opts = cfg.value(QStringLiteral("options")).toObject();
        const QString npm = cfg.value(QStringLiteral("npm")).toString();

        QString base = opts.value(QStringLiteral("baseURL")).toString();
        QString key = authObj.value(name).toObject().value(QStringLiteral("key")).toString();
        if (key.isEmpty())
            key = opts.value(QStringLiteral("apiKey")).toString();

        const bool isOllama = name.contains(QStringLiteral("ollama"), Qt::CaseInsensitive)
                              || npm.contains(QStringLiteral("ollama"), Qt::CaseInsensitive);
        const ProviderKind kind = isOllama ? ProviderKind::Ollama : ProviderKind::OpenAI;

        if (base.isEmpty())
            base = defaults.value(name.toLower());
        if (base.isEmpty())
            continue; // provedor desconhecido sem URL

        if (kind == ProviderKind::Ollama)
            base = normalizeOllamaBase(base);

        bool exists = false;
        for (const auto &p : m_providers) {
            if (p.name.compare(name, Qt::CaseInsensitive) == 0) {
                exists = true;
                break;
            }
            // Evita duplicar o provedor Ollama (mesmo servidor local).
            if (kind == ProviderKind::Ollama && p.kind == ProviderKind::Ollama) {
                exists = true;
                break;
            }
        }
        if (exists)
            continue;

        ProviderConfig pc;
        pc.name = name;
        pc.kind = kind;
        pc.baseUrl = base;
        pc.apiKey = key;
        m_providers.append(pc);
        ++added;
    }

    saveProviders();
    repopulateProviderCombo();
    updateServerInfo();

    QMessageBox::information(this, QStringLiteral("OpenCode"),
        added > 0
            ? QStringLiteral("Importado(s) %1 provedor(es) do OpenCode.").arg(added)
            : QStringLiteral("Nenhum provedor novo encontrado no OpenCode."));
}

// =====================================================================
// Modelos
// =====================================================================

void MainWindow::refreshModels()
{
    const ProviderConfig *p = currentProvider();
    if (!p) {
        setStatus(QStringLiteral("Nenhum provedor configurado."), true);
        return;
    }

    setStatus(QStringLiteral("Buscando modelos..."));

    QNetworkRequest request;
    if (p->kind == ProviderKind::Ollama) {
        request.setUrl(QUrl(joinUrl(ollamaBaseUrl(), QStringLiteral("/api/tags"))));
    } else {
        request.setUrl(QUrl(joinUrl(p->baseUrl, QStringLiteral("/models"))));
        if (!p->apiKey.isEmpty())
            request.setRawHeader("Authorization", (QStringLiteral("Bearer ") + p->apiKey).toUtf8());
    }

    if (m_modelsReply)
        m_modelsReply->abort();

    QNetworkReply *reply = m_net->get(request);
    m_modelsReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onModelsLoaded(reply);
        reply->deleteLater();
    });
}

void MainWindow::onModelsLoaded(QNetworkReply *reply)
{
    if (!reply || reply != m_modelsReply)
        return; // resposta antiga/fora de ordem: ignora
    m_modelsReply = nullptr;

    const ProviderConfig *p = currentProvider();
    if (!p)
        return;

    const QSignalBlocker blocker(m_modelCombo);
    const QString previous = m_modelCombo->currentText();
    m_modelCombo->clear();

    if (reply->error() != QNetworkReply::NoError) {
        setStatus(QStringLiteral("Erro ao listar modelos: ") + reply->errorString(), true);
        if (!previous.isEmpty())
            m_modelCombo->setEditText(previous);
        return;
    }

    QStringList modelNames;
    const QByteArray payload = reply->readAll();

    if (p->kind == ProviderKind::Ollama) {
        const QJsonArray models = QJsonDocument::fromJson(payload).object().value(QStringLiteral("models")).toArray();
        for (const QJsonValue &v : models)
            modelNames << v.toObject().value(QStringLiteral("name")).toString();
    } else {
        const QJsonArray models = QJsonDocument::fromJson(payload).object().value(QStringLiteral("data")).toArray();
        for (const QJsonValue &v : models)
            modelNames << v.toObject().value(QStringLiteral("id")).toString();
    }
    modelNames.removeAll(QString());

    for (const QString &n : modelNames)
        m_modelCombo->addItem(n);

    if (modelNames.isEmpty()) {
        m_modelCombo->setEditText(QString());
        setStatus(QStringLiteral("Nenhum modelo encontrado. Verifique URL base e chave."), true);
        return;
    }

    int idx = -1;
    if (!m_restoreModel.isEmpty()) {
        idx = m_modelCombo->findText(m_restoreModel);
        m_restoreModel.clear();
    }
    if (idx < 0)
        idx = m_modelCombo->findText(previous);
    m_modelCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    setStatus(QStringLiteral("%1 modelo(s) encontrado(s).").arg(modelNames.size()));
    onModelChanged();
}

void MainWindow::pullModel()
{
    const ProviderConfig *p = currentProvider();
    if (!p || p->kind != ProviderKind::Ollama)
        return;

    bool ok = false;
    const QString name = QInputDialog::getText(this, QStringLiteral("Baixar Modelo"),
        QStringLiteral("Nome do modelo a baixar (ex.: llama3.2, dolphin-mistral):"),
        QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok || name.isEmpty())
        return;

    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    body.insert(QStringLiteral("stream"), true);

    QNetworkRequest request(QUrl(joinUrl(ollamaBaseUrl(), QStringLiteral("/api/pull"))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    m_pullReply = m_net->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_pullBuffer.clear();
    setStatus(QStringLiteral("Baixando modelo ") + name + QStringLiteral("..."));

    connect(m_pullReply, &QNetworkReply::readyRead, this, &MainWindow::onPullReadyRead);
    connect(m_pullReply, &QNetworkReply::finished, this, &MainWindow::onPullFinished);
}

void MainWindow::onPullReadyRead()
{
    if (!m_pullReply)
        return;

    m_pullBuffer += m_pullReply->readAll();
    int idx = -1;
    while ((idx = m_pullBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_pullBuffer.left(idx).trimmed();
        m_pullBuffer.remove(0, idx + 1);
        if (line.isEmpty())
            continue;

        const QJsonObject obj = QJsonDocument::fromJson(line).object();
        const QString status = obj.value(QStringLiteral("status")).toString();
        if (obj.contains(QStringLiteral("total")) && obj.contains(QStringLiteral("completed"))) {
            const qint64 total = qint64(obj.value(QStringLiteral("total")).toDouble());
            const qint64 done = qint64(obj.value(QStringLiteral("completed")).toDouble());
            const int pct = total > 0 ? int(done * 100 / total) : 0;
            setStatus(QStringLiteral("Baixando... %1 (%2%)").arg(status).arg(pct));
        } else if (!status.isEmpty()) {
            setStatus(QStringLiteral("Baixando... ") + status);
        }
    }
}

void MainWindow::onPullFinished()
{
    if (!m_pullReply)
        return;
    m_pullReply->deleteLater();
    m_pullReply = nullptr;
    setStatus(QStringLiteral("Download concluído. Atualizando modelos..."));
    refreshModels();
}

// =====================================================================
// Conversa por modelo
// =====================================================================

QString MainWindow::currentConversationKey() const
{
    return m_providerCombo->currentText() + QStringLiteral("||")
           + m_modelCombo->currentText().trimmed();
}

void MainWindow::onModelChanged()
{
    if (m_modelCombo->currentText().trimmed().isEmpty())
        return;
    const QString key = currentConversationKey();
    if (key == m_currentConvKey)
        return;
    saveCurrentConversation();
    loadConversation(key);
    saveCurrentSelection();
}

void MainWindow::saveCurrentConversation()
{
    if (m_currentConvKey.isEmpty())
        return;
    Conversation c;
    c.messages = m_history;
    c.contextBlocks = m_contextBlocks;
    m_conversations.insert(m_currentConvKey, c);
}

void MainWindow::loadConversation(const QString &key)
{
    m_currentConvKey = key;
    const auto it = m_conversations.constFind(key);
    if (it != m_conversations.constEnd()) {
        m_history = it->messages;
        m_contextBlocks = it->contextBlocks;
    } else {
        m_history.clear();
        m_contextBlocks.clear();
    }
    applyConversationToView();
}

void MainWindow::applyConversationToView()
{
    m_chat->clear();
    m_assistantBlockStart = -1;
    m_assistantAccum.clear();

    if (!m_contextBlocks.isEmpty())
        appendSystemBlock(QStringLiteral("📁 %1 item(ns) de contexto anexado(s) a esta conversa.")
                              .arg(m_contextBlocks.size()));

    for (const auto &msg : m_history) {
        if (msg.first == QStringLiteral("user"))
            appendUserBlock(msg.second);
        else
            appendAssistantBlock(msg.second);
    }
}

// =====================================================================
// Chat
// =====================================================================

void MainWindow::sendMessage()
{
    const QString text = m_input->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    if (m_streamReply) {
        QMessageBox::information(this, QStringLiteral("Aguarde"),
            QStringLiteral("Já existe uma resposta sendo gerada."));
        return;
    }
    if (!currentProvider()) {
        QMessageBox::warning(this, QStringLiteral("Sem provedor"),
            QStringLiteral("Configure um provedor antes de enviar."));
        return;
    }
    if (m_modelCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Sem modelo"),
            QStringLiteral("Selecione (ou digite) um modelo antes de enviar."));
        return;
    }

    m_input->clear();
    sendUserMessage(text);
}

void MainWindow::sendUserMessage(const QString &text)
{
    appendUserBlock(text);
    m_history.append(qMakePair(QStringLiteral("user"), text));
    startChatRequest();
}

QJsonArray MainWindow::buildMessagesArray() const
{
    QJsonArray messages;

    const QString sysPrompt = m_systemPrompt->toPlainText().trimmed();
    if (!sysPrompt.isEmpty()) {
        QJsonObject sys;
        sys.insert(QStringLiteral("role"), QStringLiteral("system"));
        sys.insert(QStringLiteral("content"), sysPrompt);
        messages.append(sys);
    }

    if (!m_contextBlocks.isEmpty()) {
        QJsonObject ctx;
        ctx.insert(QStringLiteral("role"), QStringLiteral("system"));
        ctx.insert(QStringLiteral("content"),
            QStringLiteral("Contexto do sistema de arquivos fornecido pelo usuário:\n")
                + m_contextBlocks.join(QLatin1Char('\n')));
        messages.append(ctx);
    }

    for (const auto &entry : m_history) {
        QJsonObject msg;
        msg.insert(QStringLiteral("role"), entry.first);
        msg.insert(QStringLiteral("content"), entry.second);
        messages.append(msg);
    }

    return messages;
}

QByteArray MainWindow::buildOllamaPayload() const
{
    QJsonObject options;
    options.insert(QStringLiteral("temperature"), m_tempSpin->value());
    options.insert(QStringLiteral("top_p"), m_topPSpin->value());

    QJsonObject body;
    body.insert(QStringLiteral("model"), m_modelCombo->currentText().trimmed());
    body.insert(QStringLiteral("stream"), true);
    body.insert(QStringLiteral("messages"), buildMessagesArray());
    body.insert(QStringLiteral("options"), options);

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QByteArray MainWindow::buildOpenAIPayload() const
{
    QJsonObject body;
    body.insert(QStringLiteral("model"), m_modelCombo->currentText().trimmed());
    body.insert(QStringLiteral("stream"), true);
    body.insert(QStringLiteral("temperature"), m_tempSpin->value());
    body.insert(QStringLiteral("top_p"), m_topPSpin->value());
    body.insert(QStringLiteral("messages"), buildMessagesArray());

    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

void MainWindow::startChatRequest()
{
    const ProviderConfig *p = currentProvider();
    if (!p)
        return;

    QNetworkRequest request;
    QByteArray payload;

    if (p->kind == ProviderKind::Ollama) {
        request.setUrl(QUrl(joinUrl(ollamaBaseUrl(), QStringLiteral("/api/chat"))));
        payload = buildOllamaPayload();
    } else {
        request.setUrl(QUrl(joinUrl(p->baseUrl, QStringLiteral("/chat/completions"))));
        if (!p->apiKey.isEmpty())
            request.setRawHeader("Authorization", (QStringLiteral("Bearer ") + p->apiKey).toUtf8());
        payload = buildOpenAIPayload();
    }

    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    m_streamReply = m_net->post(request, payload);
    m_streamBuffer.clear();
    m_assistantAccum.clear();

    m_sendBtn->setEnabled(false);
    m_chat->moveCursor(QTextCursor::End);
    m_assistantBlockStart = m_chat->textCursor().position();
    m_assistantModel = m_modelCombo->currentText().trimmed();
    renderAssistantBlock();
    setStatus(QStringLiteral("Gerando resposta..."));

    connect(m_streamReply, &QNetworkReply::readyRead, this, &MainWindow::onStreamReadyRead);
    connect(m_streamReply, &QNetworkReply::finished, this, &MainWindow::onStreamFinished);
    connect(m_streamReply, &QNetworkReply::errorOccurred, this, &MainWindow::onStreamError);
}

QString MainWindow::extractStreamContent(const QByteArray &line) const
{
    const ProviderConfig *p = currentProvider();

    if (p && p->kind == ProviderKind::OpenAI) {
        if (!line.startsWith("data:"))
            return QString();
        const QByteArray payload = line.mid(5).trimmed();
        if (payload == "[DONE]")
            return QString();
        const QJsonObject obj = QJsonDocument::fromJson(payload).object();
        const QJsonArray choices = obj.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty())
            return QString();
        return choices.first().toObject().value(QStringLiteral("delta"))
            .toObject().value(QStringLiteral("content")).toString();
    }

    // Ollama (NDJSON).
    const QJsonObject obj = QJsonDocument::fromJson(line).object();
    return obj.value(QStringLiteral("message")).toObject()
        .value(QStringLiteral("content")).toString();
}

void MainWindow::onStreamReadyRead()
{
    if (!m_streamReply)
        return;

    m_streamBuffer += m_streamReply->readAll();

    int idx = -1;
    while ((idx = m_streamBuffer.indexOf('\n')) >= 0) {
        QByteArray line = m_streamBuffer.left(idx);
        m_streamBuffer.remove(0, idx + 1);
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        const QString content = extractStreamContent(line);
        if (!content.isEmpty())
            appendStreamingText(content);
    }
}

void MainWindow::onStreamFinished()
{
    if (!m_streamReply)
        return;

    // Consome qualquer resto sem quebra de linha final.
    if (!m_streamBuffer.isEmpty()) {
        const QString content = extractStreamContent(m_streamBuffer.trimmed());
        if (!content.isEmpty())
            appendStreamingText(content);
        m_streamBuffer.clear();
    }

    m_renderTimer->stop();
    renderAssistantBlock();

    if (!m_assistantAccum.isEmpty())
        m_history.append(qMakePair(QStringLiteral("assistant"), m_assistantAccum));

    m_streamReply->deleteLater();
    m_streamReply = nullptr;
    m_sendBtn->setEnabled(true);
    setStatus(QStringLiteral("Pronto."));
}

void MainWindow::onStreamError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if (!m_streamReply)
        return;

    appendSystemBlock(QStringLiteral("⚠ Erro de conexão: ") + m_streamReply->errorString());

    m_renderTimer->stop();
    renderAssistantBlock();

    if (!m_assistantAccum.isEmpty())
        m_history.append(qMakePair(QStringLiteral("assistant"), m_assistantAccum));

    m_streamReply->deleteLater();
    m_streamReply = nullptr;
    m_sendBtn->setEnabled(true);
    setStatus(QStringLiteral("Erro na conexão."), true);
}

// =====================================================================
// Sistema local
// =====================================================================

QString MainWindow::readFileContext(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    const QByteArray bytes = file.read(kMaxFileContextBytes);
    file.close();

    if (bytes.contains('\0'))
        return QString(); // binário

    return QString::fromUtf8(bytes);
}

void MainWindow::selectFile()
{
    const QString path = QFileDialog::getOpenFileName(this,
        QStringLiteral("Selecionar Arquivo"), m_lastPath);
    if (path.isEmpty())
        return;

    m_lastPath = path;

    const QString content = readFileContext(path);
    QString block = QStringLiteral("Arquivo: ") + path;
    if (!content.isEmpty())
        block += QStringLiteral("\n--- conteúdo ---\n") + content;

    m_contextBlocks.append(block);
    appendSystemBlock(QStringLiteral("📎 Contexto anexado: ") + path);
    setStatus(QStringLiteral("Arquivo anexado ao contexto."));
}

void MainWindow::selectFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this,
        QStringLiteral("Selecionar Pasta"), m_lastPath);
    if (path.isEmpty())
        return;

    m_lastPath = path;

    QDir dir(path);
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                                              QDir::Name);
    QString block = QStringLiteral("Pasta: ") + path + QStringLiteral("\nConteúdo:\n");
    for (const QString &entry : entries)
        block += QStringLiteral("- ") + entry + QLatin1Char('\n');

    m_contextBlocks.append(block);
    appendSystemBlock(QStringLiteral("📁 Contexto anexado: ") + path);
    setStatus(QStringLiteral("Pasta anexada ao contexto."));
}

void MainWindow::openInExplorer()
{
    QString target = m_lastPath;
    if (target.isEmpty()) {
        const QString chosen = QFileDialog::getExistingDirectory(this,
            QStringLiteral("Escolha uma pasta para abrir"));
        if (chosen.isEmpty())
            return;
        target = chosen;
        m_lastPath = chosen;
    }

    const QFileInfo info(target);
    const QString dir = info.isDir() ? target : info.absolutePath();

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dir)))
        QMessageBox::warning(this, QStringLiteral("Erro"),
            QStringLiteral("Não foi possível abrir: ") + dir);
}

// =====================================================================
// Terminal / QProcess
// =====================================================================

void MainWindow::runCommand()
{
    const QString command = m_termInput->text().trimmed();
    if (command.isEmpty())
        return;

    if (m_process->state() != QProcess::NotRunning) {
        QMessageBox::information(this, QStringLiteral("Aguarde"),
            QStringLiteral("Um comando ainda está em execução."));
        return;
    }

    m_termAccum.clear();
    m_termOutput->clear();
    m_sendOutputBtn->setEnabled(false);
    m_termInput->clear();

    m_termOutput->appendPlainText(QStringLiteral("$ ") + command + QLatin1Char('\n'));

#ifdef Q_OS_WIN
    m_process->start(QStringLiteral("cmd.exe"), QStringList() << QStringLiteral("/c") << command);
#else
    m_process->start(QStringLiteral("/bin/sh"), QStringList() << QStringLiteral("-c") << command);
#endif
}

void MainWindow::onCommandReadyRead()
{
    const QByteArray out = m_process->readAllStandardOutput();
    const QByteArray err = m_process->readAllStandardError();

    if (!out.isEmpty()) {
        m_termAccum += QString::fromLocal8Bit(out);
        m_termOutput->appendPlainText(QString::fromLocal8Bit(out));
    }
    if (!err.isEmpty()) {
        m_termAccum += QString::fromLocal8Bit(err);
        m_termOutput->appendPlainText(QString::fromLocal8Bit(err));
    }
}

void MainWindow::onCommandFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status);

    m_lastCommandOutput = m_termAccum;
    m_sendOutputBtn->setEnabled(!m_lastCommandOutput.trimmed().isEmpty());

    m_termOutput->appendPlainText(QStringLiteral("\n[Processo encerrado — código %1]\n").arg(exitCode));
}

void MainWindow::sendTerminalOutputToPrompt()
{
    if (m_lastCommandOutput.trimmed().isEmpty())
        return;

    const QString text = QStringLiteral(
        "Analise a seguinte saída de comando executado no terminal:\n```\n")
        + m_lastCommandOutput + QStringLiteral("\n```");

    appendSystemBlock(QStringLiteral("⌨ Saída do terminal enviada para a LLM."));
    sendUserMessage(text);
}

// =====================================================================
// Utilidades
// =====================================================================

void MainWindow::clearChat()
{
    if (!m_history.isEmpty() || !m_contextBlocks.isEmpty()) {
        const auto ret = QMessageBox::question(this, QStringLiteral("Limpar chat"),
            QStringLiteral("Deseja apagar todo o histórico da conversa e o contexto anexado?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return;
    }

    m_chat->clear();
    m_history.clear();
    m_contextBlocks.clear();
    m_assistantAccum.clear();
    m_assistantBlockStart = -1;
    saveCurrentConversation(); // persiste o estado limpo desta conversa
    setStatus(QStringLiteral("Chat limpo."));
}

void MainWindow::exportConversation()
{
    if (m_history.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Exportar"),
            QStringLiteral("Não há conversa para exportar."));
        return;
    }

    const QString path = QFileDialog::getSaveFileName(this,
        QStringLiteral("Exportar Conversa"),
        QDir::homePath() + QStringLiteral("/conversa"),
        QStringLiteral("Markdown (*.md);;Texto puro (*.txt)"));
    if (path.isEmpty())
        return;

    const bool markdown = path.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive);

    QString content;
    if (markdown)
        content += QStringLiteral("# Conversa LLM Studio\n\n");

    const QString sysPrompt = m_systemPrompt->toPlainText().trimmed();
    if (!sysPrompt.isEmpty()) {
        content += markdown
            ? QStringLiteral("**System Prompt:**\n\n") + sysPrompt + QStringLiteral("\n\n---\n\n")
            : QStringLiteral("System Prompt:\n") + sysPrompt + QStringLiteral("\n\n---\n\n");
    }

    for (const auto &entry : m_history) {
        const QString &role = entry.first;
        const QString &text = entry.second;

        if (markdown) {
            const QString heading = (role == QStringLiteral("user"))
                ? QStringLiteral("## 👤 Usuário")
                : QStringLiteral("## 🤖 Assistente — ") + m_modelCombo->currentText().trimmed();
            content += heading + QStringLiteral("\n\n") + text + QStringLiteral("\n\n");
        } else {
            const QString label = (role == QStringLiteral("user"))
                ? QStringLiteral("Usuário:")
                : QStringLiteral("Assistente — ") + m_modelCombo->currentText().trimmed() + QStringLiteral(":");
            content += label + QStringLiteral("\n");
            content += text + QStringLiteral("\n\n");
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Exportar"),
            QStringLiteral("Não foi possível gravar o arquivo:\n") + path);
        return;
    }
    file.write(content.toUtf8());
    file.close();

    setStatus(QStringLiteral("Conversa exportada para ") + path);
}

// =====================================================================
// Renderização do chat
// =====================================================================

void MainWindow::appendUserBlock(const QString &text)
{
    m_chat->moveCursor(QTextCursor::End);
    m_chat->insertHtml(
        QStringLiteral("<p style='color:#a6e3a1; font-weight:bold; margin-top:10px;'>👤 Você</p>")
        + markdownToHtml(text));
    m_chat->moveCursor(QTextCursor::End);
    m_chat->verticalScrollBar()->setValue(m_chat->verticalScrollBar()->maximum());
}

void MainWindow::appendSystemBlock(const QString &text)
{
    m_chat->moveCursor(QTextCursor::End);
    m_chat->insertHtml(
        QStringLiteral("<p style='color:#f9e2af; font-style:italic; margin-top:6px;'>")
        + text.toHtmlEscaped() + QStringLiteral("</p>"));
    m_chat->moveCursor(QTextCursor::End);
    m_chat->verticalScrollBar()->setValue(m_chat->verticalScrollBar()->maximum());
}

void MainWindow::appendAssistantBlock(const QString &text)
{
    m_chat->moveCursor(QTextCursor::End);
    m_assistantBlockStart = m_chat->textCursor().position();
    m_assistantAccum = text;
    m_assistantModel = m_modelCombo->currentText().trimmed();
    renderAssistantBlock();
}

void MainWindow::renderAssistantBlock()
{
    if (m_assistantBlockStart < 0)
        return;

    QString header = QStringLiteral("🤖 Assistente");
    if (!m_assistantModel.isEmpty())
        header += QStringLiteral(" — ") + m_assistantModel.toHtmlEscaped();

    QTextDocument *doc = m_chat->document();
    const int end = qMax(m_assistantBlockStart, doc->characterCount() - 1);

    QTextCursor cursor(doc);
    cursor.setPosition(m_assistantBlockStart);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertHtml(
        QStringLiteral("<p style='color:#89b4fa; font-weight:bold; margin-top:10px;'>")
        + header + QStringLiteral("</p>")
        + markdownToHtml(m_assistantAccum));

    m_chat->moveCursor(QTextCursor::End);
    m_chat->verticalScrollBar()->setValue(m_chat->verticalScrollBar()->maximum());
}

void MainWindow::appendStreamingText(const QString &delta)
{
    if (delta.isEmpty())
        return;

    m_assistantAccum += delta;
    if (!m_renderTimer->isActive())
        m_renderTimer->start(50); // renderiza a cada ~50ms para suavizar o streaming
}

// =====================================================================
// Eventos
// =====================================================================

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_input && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        const bool isEnter = (keyEvent->key() == Qt::Key_Return ||
                              keyEvent->key() == Qt::Key_Enter);
        if (isEnter && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            sendMessage();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
