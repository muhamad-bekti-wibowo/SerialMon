#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QIcon>
#include <QTimer>
#include <QtGlobal>

#include "mainwindow.h"

// Opsi baris perintah:
//   --demo            data tiruan (tanpa hardware)
//   --theme=light|dark paksa tema (tidak disimpan)
//   --shot=<file.png> simpan tangkapan jendela lalu keluar (dipakai untuk screenshot README)
int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "windows:fontengine=freetype");   // spasi font variabel benar
    QApplication app(argc, argv);
    app.setApplicationName("SerialMonitor");
    app.setOrganizationName("SerialMonitor");
    app.setApplicationVersion(APP_VERSION);
    app.setStyle("Fusion");   // dasar netral agar QSS tema tampil konsisten
    app.setWindowIcon(QIcon(":/favicon.ico"));

    // Font situs (ditanam di exe): Plus Jakarta Sans untuk UI, JetBrains Mono untuk angka/terminal
    QFontDatabase::addApplicationFont(":/fonts/PlusJakartaSans.ttf");
    QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono.ttf");
    QFont base("Plus Jakarta Sans", 10);
    base.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(base);

    MainWindow w;
    QString shot;
    for (const QString& a : app.arguments()) {
        if (a == "--theme=dark") w.applyTheme(true);
        else if (a == "--theme=light") w.applyTheme(false);
        else if (a.startsWith("--shot=")) shot = a.mid(7);
    }
    if (app.arguments().contains("--demo")) w.startDemo();
    w.show();
    if (!shot.isEmpty()) {
        QTimer::singleShot(4500, &app, [&] {
            w.grab().save(shot);
            app.quit();
        });
    }
    return app.exec();
}
