#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QtGlobal>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "windows:fontengine=freetype");   // spasi font variabel benar
    QApplication app(argc, argv);
    app.setApplicationName("SerialMonitor");
    app.setOrganizationName("SerialMonitor");
    app.setStyle("Fusion");   // dasar netral agar QSS tema tampil konsisten
    app.setWindowIcon(QIcon(":/favicon.ico"));

    // Font situs (ditanam di exe): Plus Jakarta Sans untuk UI, JetBrains Mono untuk angka/terminal
    QFontDatabase::addApplicationFont(":/fonts/PlusJakartaSans.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono.ttf");
    QFont base("Plus Jakarta Sans", 10);
    base.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(base);

    MainWindow w;
    if (app.arguments().contains("--demo")) w.startDemo();
    w.show();
    return app.exec();
}
