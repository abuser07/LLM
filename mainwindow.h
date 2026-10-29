#pragma once

#include <QMainWindow>
#include <QProcess>
#include <QVector>
#include <QPair>
#include <QByteArray>
#include <QStringList>
#include <QHash>
#include <QJsonArray>
#include <QNetworkReply>

class QNetworkAccessManager;
class QComboBox;
class QTextBrowser;
class QPlainTextEdit;
class QDoubleSpinBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QTimer;

// Tipos de provedor suportados.
enum class ProviderKind {
    Ollama, // servidor local (sem chave)
    OpenAI   // API OpenAI-compatível (DeepSeek, OpenRouter, OpenAI, etc.)
};

// Configuração de um provedor de IA.
struct ProviderConfig {
    QString name;
    ProviderKind kind = ProviderKind::Ollama;
    QString baseUrl; // ex.: http://localhost:11434  ou  https://api.deepseek.com/v1
    QString apiKey;  // vazio para Ollama
};

// Uma conversa (histórico de mensagens + contexto de arquivos), por modelo.
struct Conversation {
    QVector<QPair<QString, QString>> messages; // <role, content>
    QStringList contextBlocks;
};

/*
 * LLM Studio — interface gráfica nativa (Qt6) para interagir com LLMs.
 * Suporta Ollama (local) e APIs OpenAI-compatíveis (DeepSeek e outras),
 * com importação de provedores a partir da configuração do OpenCode.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // Provedores & modelos
    void onProviderChanged();
    void manageProviders();
    void importFromOpenCode();
    void refreshModels();
    void onModelsLoaded(QNetworkReply *reply);
    void pullModel();
    void onPullReadyRead();
    void onPullFinished();

    // Chat
    void sendMessage();
    void onStreamReadyRead();
    void onStreamFinished();
    void onStreamError(QNetworkReply::NetworkError code);

    // Sistema local
    void selectFile();
    void selectFolder();
    void openInExplorer();

    // Terminal / QProcess
    void runCommand();
    void onCommandReadyRead();
    void onCommandFinished(int exitCode, QProcess::ExitStatus status);
    void sendTerminalOutputToPrompt();

    // Utilidades
    void clearChat();
    void exportConversation();

private:
    void buildUi();
    void applyTheme();
    void setStatus(const QString &text, bool isError = false);

    // Provedores
    void loadProviders();
    void saveProviders();
    void repopulateProviderCombo();
    void saveCurrentSelection();
    void updateServerInfo();
    ProviderConfig *currentProvider();
    const ProviderConfig *currentProvider() const;
    QString ollamaBaseUrl() const;   // base normalizada (sem "/api" e sem barra final)

    // Conversa por modelo
    QString currentConversationKey() const;
    void onModelChanged();
    void saveCurrentConversation();
    void loadConversation(const QString &key);
    void applyConversationToView();

    // Requisições de chat
    void sendUserMessage(const QString &text);
    void startChatRequest();
    QJsonArray buildMessagesArray() const;
    QByteArray buildOllamaPayload() const;
    QByteArray buildOpenAIPayload() const;
    QString extractStreamContent(const QByteArray &line) const;

    // Renderização
    void appendUserBlock(const QString &text);
    void appendSystemBlock(const QString &text);
    void appendAssistantBlock(const QString &text);
    void appendStreamingText(const QString &delta);
    void renderAssistantBlock();

    QString readFileContext(const QString &path) const;

    // Estado
    QNetworkAccessManager *m_net = nullptr;
    QNetworkReply *m_streamReply = nullptr;
    QByteArray m_streamBuffer;
    QString m_assistantAccum;
    int m_assistantBlockStart = -1;
    QString m_assistantModel;

    QNetworkReply *m_pullReply = nullptr;
    QByteArray m_pullBuffer;

    QNetworkReply *m_modelsReply = nullptr;

    QProcess *m_process = nullptr;
    QString m_termAccum;
    QString m_lastCommandOutput;

    QVector<ProviderConfig> m_providers;

    QVector<QPair<QString, QString>> m_history;   // conversa atual
    QStringList m_contextBlocks;                   // contexto atual
    QHash<QString, Conversation> m_conversations;
    QString m_currentConvKey;
    QString m_restoreModel;

    QString m_lastPath;
    QTimer *m_renderTimer = nullptr;

    // Widgets
    QComboBox *m_providerCombo = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QTextBrowser *m_chat = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QPushButton *m_sendBtn = nullptr;
    QPushButton *m_clearBtn = nullptr;
    QPushButton *m_exportBtn = nullptr;
    QDoubleSpinBox *m_tempSpin = nullptr;
    QDoubleSpinBox *m_topPSpin = nullptr;
    QPlainTextEdit *m_systemPrompt = nullptr;
    QPushButton *m_folderBtn = nullptr;
    QPushButton *m_fileBtn = nullptr;
    QPushButton *m_explorerBtn = nullptr;
    QLineEdit *m_termInput = nullptr;
    QPlainTextEdit *m_termOutput = nullptr;
    QPushButton *m_runBtn = nullptr;
    QPushButton *m_sendOutputBtn = nullptr;
    QPushButton *m_pullBtn = nullptr;
    QLabel *m_serverInfo = nullptr;
    QLabel *m_statusLabel = nullptr;
};
