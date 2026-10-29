#include <QApplication>
#include <QStyleFactory>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("LocalAI"));
    QCoreApplication::setApplicationName(QStringLiteral("LLM Studio"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    // Deixa a interface o mais nativa possível, respeitando o QSS aplicado.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    MainWindow window;
    window.show();

    return app.exec();
}
