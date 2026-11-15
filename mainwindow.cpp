#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#include <QDockWidget>
#include <QFrame>
#include <QIcon>
#include <QStyle>
#include <QStyleHints>
#include <cmath>

#include "plotwidget.h"
#include "serialworker.h"
#include "theme.h"

// Warna layar terminal: selalu gelap (seperti layar osiloskop di situs), sama dengan --screen situs
static const QColor kFg(0xd9, 0xe1, 0xea), kDim(0x6f, 0x7f, 0x90), kErr(0xff, 0x7b, 0x6b),
    kWarn(0xf5, 0xa5, 0x24), kEcho(0x5f, 0xb0, 0xff), kHl(0x6b, 0x3a, 0x12);

static void repolish(QWidget* w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

static QTextCharFormat fmt(const QColor& c, bool italic = false)
{
    QTextCharFormat f;
    f.setForeground(c);
    f.setFontItalic(italic);
    return f;
}

MainWindow::MainWindow()
{
    worker = new SerialWorker(this);
    buildUi();
    loadSettings();
    applyFilterAll();
    {
        const QString th = QSettings().value("theme", "auto").toString();
        const bool sysDark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
        applyTheme(th == "dark" || (th == "auto" && sysDark));
    }
    connect(themeBtn, &QPushButton::clicked, this, [this] {
        applyTheme(!darkTheme);
        QSettings().setValue("theme", darkTheme ? "dark" : "light");
    });

    connect(worker, &SerialWorker::dataReceived, this, &MainWindow::onData, Qt::QueuedConnection);
    connect(worker, &SerialWorker::statusChanged, this, &MainWindow::onStatus, Qt::QueuedConnection);

    auto* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this] { if (!connected && !wanted) refreshPorts(); });
    t->start(1500);
    refreshPorts();
    onStatus(false, false, QString());
}

void MainWindow::applyTheme(bool dark)
{
    darkTheme = dark;
    QGuiApplication::styleHints()->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    static_cast<QApplication*>(QCoreApplication::instance())->setStyleSheet(buildStyleSheet(makeTheme(dark)));
    themeBtn->setText(dark ? QStringLiteral("☀") : QStringLiteral("☾"));
    for (QWidget* w : {static_cast<QWidget*>(pill), static_cast<QWidget*>(dot), static_cast<QWidget*>(stateLabel)}) repolish(w);
}

void MainWindow::updateStateUi()
{
    const char* st = connected ? "ok" : wanted ? "wait" : "";
    pill->setProperty("state", st);
    stateLabel->setText(connected ? QString("Tersambung · %1 @ %2").arg(portCombo->currentData().toString(), baudCombo->currentText())
                        : wanted  ? QStringLiteral("Menyambung...")
                                  : QStringLiteral("Belum terhubung"));
    for (QWidget* w : {static_cast<QWidget*>(pill), static_cast<QWidget*>(dot), static_cast<QWidget*>(stateLabel)}) repolish(w);
}

void MainWindow::startDemo()
{
    // Data tiruan tanpa hardware: jalankan dengan  SerialMonitor.exe --demo
    plotBtn->setChecked(true);
    if (filterEdit->text().isEmpty()) { filterEdit->setText("adc="); applyFilterAll(); }
    auto* t = new QTimer(this);
    auto* n = new int(0);
    connect(t, &QTimer::timeout, this, [this, n] {
        const double x = (*n)++ * 0.07;
        QString l;
        if (*n % 5 == 0) l = QString("info: heartbeat %1\r\n").arg(*n);
        else l = QString("t=%1 adc=%2,%3,%4.5,%5\r\n").arg(*n).arg(int(2048 + 1800 * std::sin(x)))
                     .arg(int(2048 + 1500 * std::cos(x * 0.6))).arg(int(100 + 50 * std::sin(x * 3))).arg(*n % 7);
        onData(l.toUtf8());
    });
    t->start(20);
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Serial Monitor ") + QStringLiteral(APP_VERSION));
    resize(1280, 760);
    auto* root = new QWidget;
    setCentralWidget(root);
    auto* rv = new QVBoxLayout(root);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);

    // ---- navbar bergaya situs: logo favicon, wordmark mono, pill status, tombol tema
    auto* nav = new QFrame;
    nav->setObjectName("nav");
    nav->setFixedHeight(56);
    auto* nl = new QHBoxLayout(nav);
    nl->setContentsMargins(20, 0, 20, 0);
    nl->setSpacing(10);
    auto* icon = new QLabel;
    icon->setPixmap(QIcon(":/favicon.ico").pixmap(28, 28));
    auto* logo = new QLabel("SERIAL<span style='color:#f2600c'>MON</span>");
    logo->setObjectName("logo");
    logo->setTextFormat(Qt::RichText);
    auto* hint = new QLabel("Pilih port, atur baud, lalu Sambung");
    hint->setObjectName("hint");
    pill = new QFrame;
    pill->setObjectName("pill");
    pill->setFixedHeight(28);
    auto* pl = new QHBoxLayout(pill);
    pl->setContentsMargins(14, 0, 14, 0);
    pl->setSpacing(8);
    dot = new QLabel;
    dot->setObjectName("dot");
    stateLabel = new QLabel("Belum terhubung");
    stateLabel->setObjectName("state");
    pl->addWidget(dot);
    pl->addWidget(stateLabel);
    themeBtn = new QPushButton;
    themeBtn->setObjectName("iconbtn");
    themeBtn->setToolTip("Ganti tema terang / gelap");
    nl->addWidget(icon);
    nl->addWidget(logo);
    nl->addSpacing(8);
    nl->addWidget(hint);
    nl->addStretch();
    nl->addWidget(pill);
    nl->addWidget(themeBtn);
    rv->addWidget(nav);

    auto* body = new QWidget;
    body->setObjectName("body");
    auto* v = new QVBoxLayout(body);
    v->setContentsMargins(16, 12, 16, 12);
    v->setSpacing(12);
    rv->addWidget(body, 1);

    auto labeled = [](QHBoxLayout* l, const QString& text, QWidget* w) {
        l->addWidget(new QLabel(text));
        l->addWidget(w);
    };
    auto makeCard = [](const QString& title, std::initializer_list<QLayout*> rows) {
        auto* f = new QFrame;
        f->setObjectName("card");
        auto* l = new QVBoxLayout(f);
        l->setContentsMargins(16, 10, 16, 12);
        l->setSpacing(8);
        if (!title.isEmpty()) {
            auto* h = new QLabel(title);
            h->setObjectName("h2");
            l->addWidget(h);
        }
        for (QLayout* r : rows) l->addLayout(r);
        return f;
    };

    // ---- baris 1: koneksi
    auto* r1 = new QHBoxLayout;
    portCombo = new QComboBox;
    portCombo->setMinimumWidth(280);
    portCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    baudCombo = new QComboBox;
    baudCombo->setEditable(true);
    baudCombo->addItems({"9600", "19200", "38400", "57600", "74880", "115200", "230400", "460800", "500000",
                         "921600", "1000000", "2000000", "3000000"});
    baudCombo->setCurrentText("115200");
    baudCombo->setValidator(new QIntValidator(50, 12000000, this));
    baudCombo->setMinimumWidth(90);
    dataCombo = new QComboBox;
    dataCombo->addItems({"8", "7", "6", "5"});
    parityCombo = new QComboBox;
    parityCombo->addItem("None", "N");
    parityCombo->addItem("Even", "E");
    parityCombo->addItem("Odd", "O");
    parityCombo->addItem("Mark", "M");
    parityCombo->addItem("Space", "S");
    stopCombo = new QComboBox;
    stopCombo->addItems({"1", "1.5", "2"});
    flowCombo = new QComboBox;
    flowCombo->addItem("None", 0);
    flowCombo->addItem("RTS/CTS", 1);
    flowCombo->addItem("XON/XOFF", 2);
    connBtn = new QPushButton("Sambung");
    connBtn->setMinimumWidth(110);
    connBtn->setObjectName("primary");

    labeled(r1, "Port", portCombo);
    labeled(r1, "Baud", baudCombo);
    labeled(r1, "Data", dataCombo);
    labeled(r1, "Parity", parityCombo);
    labeled(r1, "Stop", stopCombo);
    labeled(r1, "Flow", flowCombo);
    r1->addWidget(connBtn);
    r1->addStretch();

    // ---- baris 2: DTR/RTS + status
    auto* r2 = new QHBoxLayout;
    dtrChk = new QCheckBox("DTR");
    rtsChk = new QCheckBox("RTS");
    dtrChk->setChecked(true);
    rtsChk->setChecked(true);
    dtrChk->setToolTip("Pada ESP32/Arduino, DTR/RTS mengatur reset dan mode boot");
    autoChk = new QCheckBox("Auto-reconnect");
    autoChk->setChecked(true);
    autoChk->setToolTip("Sambung ulang otomatis saat board di-reset atau kabel dicabut");
    msgLabel = new QLabel;
    msgLabel->setObjectName("msg");
    msgLabel->setMinimumWidth(50);
    msgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    r2->addWidget(dtrChk);
    r2->addWidget(rtsChk);
    r2->addWidget(autoChk);
    r2->addSpacing(12);
    r2->addWidget(msgLabel, 1);
    v->addWidget(makeCard("KONEKSI", {r1, r2}));

    // ---- terminal
    term = new QPlainTextEdit;
    term->setReadOnly(true);
    term->setMaximumBlockCount(10000);
    term->setUndoRedoEnabled(false);
    term->setObjectName("term");
    QFont f("JetBrains Mono");
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(10);
    term->setFont(f);
    v->addWidget(term, 1);

    // ---- kirim
    auto* r3 = new QHBoxLayout;
    sendEdit = new QLineEdit;
    sendEdit->setPlaceholderText("Ketik lalu Enter untuk kirim   (Atas/Bawah = riwayat)");
    sendEdit->installEventFilter(this);
    eolCombo = new QComboBox;
    eolCombo->addItem("LF (\\n)", "lf");
    eolCombo->addItem("CR+LF", "crlf");
    eolCombo->addItem("CR (\\r)", "cr");
    eolCombo->addItem("Tanpa akhiran", "none");
    sendHexChk = new QCheckBox("Kirim HEX");
    auto* sendBtn = new QPushButton("Kirim");
    sendBtn->setObjectName("primary");
    r3->addWidget(sendEdit, 1);
    r3->addWidget(eolCombo);
    r3->addWidget(sendHexChk);
    r3->addWidget(sendBtn);
    v->addWidget(makeCard("KIRIM", {r3}));

    // ---- opsi tampilan
    auto* r4 = new QHBoxLayout;
    tsChk = new QCheckBox("Timestamp");
    tsChk->setChecked(true);
    scrollChk = new QCheckBox("Auto-scroll");
    scrollChk->setChecked(true);
    hexChk = new QCheckBox("Tampil HEX");
    echoChk = new QCheckBox("Echo kirim");
    echoChk->setChecked(true);
    pauseBtn = new QPushButton("Jeda");
    pausedLabel = new QLabel;
    pausedLabel->setObjectName("warn");
    auto* clearBtn = new QPushButton("Bersihkan");
    auto* saveBtn = new QPushButton("Simpan log");
    auto* copyBtn = new QPushButton("Salin semua");
    rxLabel = new QLabel("RX 0 B");
    txLabel = new QLabel("TX 0 B");
    rxLabel->setObjectName("mono");
    txLabel->setObjectName("mono");
    for (QWidget* w : std::initializer_list<QWidget*>{tsChk, scrollChk, hexChk, echoChk, pauseBtn, pausedLabel,
                                                       clearBtn, saveBtn, copyBtn})
        r4->addWidget(w);
    r4->addStretch();
    r4->addWidget(rxLabel);
    r4->addWidget(txLabel);

    // ---- filter + plotter
    auto* r5 = new QHBoxLayout;
    filterEdit = new QLineEdit;
    filterEdit->setPlaceholderText("Filter: tampilkan hanya baris yang mengandung teks ini, mis. adc=");
    filterEdit->setClearButtonEnabled(true);
    regexChk = new QCheckBox("Regex");
    hideChk = new QCheckBox("Sembunyikan yang lain");
    hideChk->setChecked(true);
    plotBtn = new QPushButton("Plotter");
    plotBtn->setCheckable(true);
    plotBtn->setToolTip("Ambil angka dari baris yang cocok filter dan gambar sebagai grafik");
    r5->addWidget(new QLabel("Filter"));
    r5->addWidget(filterEdit, 1);
    r5->addWidget(regexChk);
    r5->addWidget(hideChk);
    r5->addWidget(plotBtn);
    v->addWidget(makeCard("TAMPILAN & FILTER", {r4, r5}));

    plotDock = new QDockWidget("PLOTTER", this);
    plotDock->setObjectName("plotDock");
    auto* pw = new QWidget;
    pw->setObjectName("dockbody");
    auto* pv = new QVBoxLayout(pw);
    pv->setContentsMargins(12, 12, 12, 12);
    pv->setSpacing(10);
    auto* pr1 = new QHBoxLayout;
    auto* winCombo = new QComboBox;
    for (int n : {200, 500, 1000, 2000, 5000, 20000, 100000}) winCombo->addItem(QString("%1 titik").arg(n), n);
    winCombo->setCurrentIndex(2);
    afterChk = new QCheckBox("Angka setelah teks filter");
    afterChk->setChecked(true);
    afterChk->setToolTip("Contoh: filter \"adc=\" dan baris \"t=5 adc=123,456\" menghasilkan 2 kanal (123 dan 456), angka 5 diabaikan");
    auto* plotPause = new QPushButton("Jeda");
    plotPause->setCheckable(true);
    auto* plotClear = new QPushButton("Bersihkan");
    auto* plotCsv = new QPushButton("Simpan CSV");
    pr1->addWidget(winCombo);
    pr1->addWidget(afterChk);
    pr1->addStretch();
    pr1->addWidget(plotPause);
    pr1->addWidget(plotClear);
    pr1->addWidget(plotCsv);
    plot = new PlotWidget;
    plotInfo = new QLabel("Sampel 0 · Kanal 0");
    plotInfo->setObjectName("mono");
    pv->addLayout(pr1);
    pv->addWidget(plot, 1);
    pv->addWidget(plotInfo);
    plotDock->setWidget(pw);
    plotDock->setMinimumWidth(460);
    addDockWidget(Qt::RightDockWidgetArea, plotDock);
    plotDock->hide();

    filterTimer = new QTimer(this);
    filterTimer->setSingleShot(true);
    filterTimer->setInterval(250);

    connect(filterTimer, &QTimer::timeout, this, &MainWindow::applyFilterAll);
    connect(filterEdit, &QLineEdit::textChanged, this, [this] { filterTimer->start(); });
    connect(regexChk, &QCheckBox::toggled, this, [this] { filterTimer->start(); });
    connect(hideChk, &QCheckBox::toggled, this, [this] { filterTimer->start(); });
    connect(plotBtn, &QPushButton::toggled, plotDock, &QWidget::setVisible);
    connect(plotDock, &QDockWidget::visibilityChanged, plotBtn, &QPushButton::setChecked);
    connect(winCombo, &QComboBox::currentIndexChanged, this, [this, winCombo](int) { plot->setWindow(winCombo->currentData().toInt()); });
    connect(plotPause, &QPushButton::toggled, this, [this, plotPause](bool b) { plot->setPaused(b); plotPause->setText(b ? "Lanjut" : "Jeda"); });
    connect(plotClear, &QPushButton::clicked, this, [this] { plot->clearData(); lastTotal = 0; plotInfo->setText("Sampel 0 · Kanal 0"); });
    connect(plotCsv, &QPushButton::clicked, this, [this] {
        const QString fn = QFileDialog::getSaveFileName(this, "Simpan data plotter", QDateTime::currentDateTime().toString("'plot_'yyyyMMdd_HHmmss'.csv'"), "CSV (*.csv)");
        if (!fn.isEmpty() && !plot->saveCsv(fn)) msgLabel->setText("Gagal menyimpan CSV");
    });
    auto* infoTimer = new QTimer(this);
    connect(infoTimer, &QTimer::timeout, this, [this] {
        const qint64 t = plot->totalSamples();
        plotInfo->setText(QString("Sampel %1 · Kanal %2 · %3 sampel/s").arg(t).arg(plot->channelCount()).arg(t - lastTotal));
        lastTotal = t;
    });
    infoTimer->start(1000);

    // ---- sinyal
    connect(connBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::doSend);
    connect(sendEdit, &QLineEdit::returnPressed, this, &MainWindow::doSend);
    connect(autoChk, &QCheckBox::toggled, this, [this](bool b) { worker->setAutoReconnect(b); });
    connect(dtrChk, &QCheckBox::toggled, this, [this](bool b) { worker->setLines(b, -1); });
    connect(rtsChk, &QCheckBox::toggled, this, [this](bool b) { worker->setLines(-1, b); });
    connect(hexChk, &QCheckBox::toggled, this, [this] {
        decoder = QStringDecoder(QStringDecoder::Utf8);
        hexCount = 0;
        lastCR = false;
    });
    connect(scrollChk, &QCheckBox::toggled, this, [this](bool b) { if (b) scrollIfNeeded(); });
    connect(term->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int val) {
        if (programmaticScroll) return;
        scrollChk->setChecked(term->verticalScrollBar()->maximum() - val < 24);  // scroll manual ke atas = matikan
    });
    connect(pauseBtn, &QPushButton::clicked, this, [this] {
        paused = !paused;
        pauseBtn->setText(paused ? "Lanjut" : "Jeda");
        pausedLabel->setText(paused ? "dijeda (data tetap ditampung)" : QString());
        if (!paused) {
            for (const QString& s : std::as_const(pending)) feedText(s);
            pending.clear();
        }
    });
    connect(clearBtn, &QPushButton::clicked, this, [this] {
        term->clear();
        lineOpen = anyLine = false;
        curLine.clear();
        pending.clear();
        rxBytes = txBytes = 0;
        rxLabel->setText("RX 0 B");
        txLabel->setText("TX 0 B");
    });
    connect(copyBtn, &QPushButton::clicked, this, [this] {
        QGuiApplication::clipboard()->setText(term->toPlainText());
        msgLabel->setText("Log disalin ke clipboard");
    });
    connect(saveBtn, &QPushButton::clicked, this, [this] {
        const QString def = QDateTime::currentDateTime().toString("'serial_'yyyyMMdd_HHmmss'.txt'");
        const QString fn = QFileDialog::getSaveFileName(this, "Simpan log", def, "Teks (*.txt);;Semua (*)");
        if (fn.isEmpty()) return;
        QFile out(fn);
        if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.write(term->toPlainText().toUtf8());
            msgLabel->setText("Tersimpan: " + fn);
        } else {
            msgLabel->setText("Gagal menyimpan: " + out.errorString());
        }
    });
}

void MainWindow::loadSettings()
{
    QSettings s;
    baudCombo->setCurrentText(s.value("baud", "115200").toString());
    dataCombo->setCurrentText(s.value("data", "8").toString());
    parityCombo->setCurrentIndex(qMax(0, parityCombo->findData(s.value("parity", "N"))));
    stopCombo->setCurrentText(s.value("stop", "1").toString());
    flowCombo->setCurrentIndex(qMax(0, flowCombo->findData(s.value("flow", 0))));
    eolCombo->setCurrentIndex(qMax(0, eolCombo->findData(s.value("eol", "lf"))));
    dtrChk->setChecked(s.value("dtr", true).toBool());
    rtsChk->setChecked(s.value("rts", true).toBool());
    autoChk->setChecked(s.value("auto", true).toBool());
    tsChk->setChecked(s.value("ts", true).toBool());
    hexChk->setChecked(s.value("hex", false).toBool());
    echoChk->setChecked(s.value("echo", true).toBool());
    sendHexChk->setChecked(s.value("sendhex", false).toBool());
    filterEdit->setText(s.value("filter").toString());
    regexChk->setChecked(s.value("regex", false).toBool());
    hideChk->setChecked(s.value("hide", true).toBool());
    afterChk->setChecked(s.value("after", true).toBool());
    history = s.value("history").toStringList();
    histPos = history.size();
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings s;
    s.setValue("port", portCombo->currentData());
    s.setValue("baud", baudCombo->currentText());
    s.setValue("data", dataCombo->currentText());
    s.setValue("parity", parityCombo->currentData());
    s.setValue("stop", stopCombo->currentText());
    s.setValue("flow", flowCombo->currentData());
    s.setValue("eol", eolCombo->currentData());
    s.setValue("dtr", dtrChk->isChecked());
    s.setValue("rts", rtsChk->isChecked());
    s.setValue("auto", autoChk->isChecked());
    s.setValue("ts", tsChk->isChecked());
    s.setValue("hex", hexChk->isChecked());
    s.setValue("echo", echoChk->isChecked());
    s.setValue("sendhex", sendHexChk->isChecked());
    s.setValue("filter", filterEdit->text());
    s.setValue("regex", regexChk->isChecked());
    s.setValue("hide", hideChk->isChecked());
    s.setValue("after", afterChk->isChecked());
    s.setValue("history", history);
    s.setValue("geometry", saveGeometry());
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    saveSettings();
    QMainWindow::closeEvent(e);
}

void MainWindow::refreshPorts()
{
    const QList<PortInfo> list = listSerialPorts();
    QString sig;
    for (const PortInfo& p : list) sig += p.name + '|' + p.desc + ';';
    if (sig == lastPortSig) return;
    lastPortSig = sig;

    const QString cur = portCombo->count() ? portCombo->currentData().toString()
                                           : QSettings().value("port").toString();
    portCombo->clear();
    if (list.isEmpty()) portCombo->addItem("(tidak ada port)", QString());
    for (const PortInfo& p : list) {
        QString text = p.desc.contains(p.name) ? p.desc : p.name + " - " + p.desc;
        portCombo->addItem(text, p.name);
    }
    int i = portCombo->findData(cur);
    if (i >= 0) portCombo->setCurrentIndex(i);
}

void MainWindow::onConnectClicked()
{
    if (connected || wanted) {
        worker->disconnectPort();
        return;
    }
    SerialCfg c;
    c.port = portCombo->currentData().toString();
    if (c.port.isEmpty()) {
        msgLabel->setText("Pilih port dulu");
        return;
    }
    bool ok = false;
    c.baud = baudCombo->currentText().toInt(&ok);
    if (!ok || c.baud <= 0) {
        msgLabel->setText("Baud rate tidak valid");
        return;
    }
    c.data = dataCombo->currentText().toInt();
    c.parity = parityCombo->currentData().toString().at(0).toLatin1();
    c.stop = stopCombo->currentText();
    c.flow = flowCombo->currentData().toInt();
    c.dtr = dtrChk->isChecked();
    c.rts = rtsChk->isChecked();
    worker->connectPort(c, autoChk->isChecked());
}

void MainWindow::onStatus(bool conn, bool want, const QString& msg)
{
    connected = conn;
    wanted = want;
    const bool busy = conn || want;
    connBtn->setText(busy ? "Putuskan" : "Sambung");
    for (QWidget* w : std::initializer_list<QWidget*>{portCombo, baudCombo, dataCombo, parityCombo, stopCombo, flowCombo})
        w->setEnabled(!busy);
    connBtn->setProperty("active", busy);
    repolish(connBtn);
    updateStateUi();

    if (!msg.isEmpty()) {
        const bool bad = !conn && (msg.contains("Gagal") || msg.contains("putus") || msg.contains("dipakai") ||
                                   msg.contains("tidak ditemukan"));
        msgLabel->setProperty("err", bad);
        repolish(msgLabel);
        msgLabel->setText(msg);
        if (msg != lastMsg) specialLine(msg, kDim);
        lastMsg = msg;
    } else if (!conn) {
        msgLabel->setText("Belum tersambung");
    }
    if (!conn) {
        decoder = QStringDecoder(QStringDecoder::Utf8);
        lastCR = false;
        hexCount = 0;
        lastPortSig.clear();
        refreshPorts();
    }
}

// ------------------------------------------------------------------ terminal
void MainWindow::onData(const QByteArray& data)
{
    rxBytes += data.size();
    rxLabel->setText(QString("RX %1 B").arg(rxBytes));
    QString s;
    if (hexChk->isChecked()) {
        s.reserve(data.size() * 3 + data.size() / 16 + 2);
        for (uchar b : data) {
            s += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
            if (++hexCount % 16 == 0) s += '\n';
        }
    } else {
        s = decoder.decode(data);
        static const QRegularExpression ansi("\x1b\\[[0-9;?]*[A-Za-z]");
        s.remove(ansi);
        if (lastCR && s.startsWith('\n')) s.remove(0, 1);
        if (!s.isEmpty()) lastCR = s.endsWith('\r');
        s.replace("\r\n", "\n");
        s.replace('\r', '\n');
    }
    if (paused) pending << s;
    else feedText(s);
}

void MainWindow::beginLine(QTextCursor& c)
{
    if (anyLine) c.insertBlock(QTextBlockFormat(), fmt(kFg));
    anyLine = true;
    c.block().setUserState(-1);   // -1 = baris sistem/belum selesai: selalu tampil
    if (tsChk->isChecked())
        c.insertText(QTime::currentTime().toString("HH:mm:ss.zzz") + "  ", fmt(kDim));
    lineStartPos = c.positionInBlock();   // relatif terhadap blok (aman saat blok lama dibuang)
    lineOpen = true;
    curLine.clear();
}

void MainWindow::endLine(QTextCursor& c)
{
    static const QRegularExpression errRe("^\\s*E \\(\\d+\\)|\\b(error|fail(ed)?|panic|assert|guru meditation)\\b",
                                          QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression warnRe("^\\s*W \\(\\d+\\)|\\bwarn(ing)?\\b", QRegularExpression::CaseInsensitiveOption);
    QColor col;
    if (errRe.match(curLine).hasMatch()) col = kErr;
    else if (warnRe.match(curLine).hasMatch()) col = kWarn;
    QTextBlock blk = c.block();
    const int base = blk.position();
    if (col.isValid()) {
        QTextCursor sel(c);
        sel.setPosition(base + lineStartPos);
        sel.setPosition(c.position(), QTextCursor::KeepAnchor);
        sel.mergeCharFormat(fmt(col));
    }
    blk.setUserState(lineStartPos);

    int ms = 0, me = 0;
    const bool match = matchFilter(curLine, &ms, &me);
    if (!filterText.isEmpty() && match && me > ms) {
        QTextCursor sel(c);
        sel.setPosition(base + lineStartPos + ms);
        sel.setPosition(base + lineStartPos + me, QTextCursor::KeepAnchor);
        QTextCharFormat hl;
        hl.setBackground(kHl);
        sel.mergeCharFormat(hl);
    }
    if (!match && hideChk->isChecked() && !filterText.isEmpty()) {
        blk.setVisible(false);
        term->document()->markContentsDirty(base, blk.length());
    }
    if (match && plotDock->isVisible() && !hexChk->isChecked()) plotLine(curLine, afterChk->isChecked() ? me : 0);
    lineOpen = false;
}

bool MainWindow::matchFilter(const QString& s, int* ms, int* me) const
{
    *ms = *me = 0;
    if (filterText.isEmpty()) return true;
    if (filterIsRegex) {
        if (!filterRe.isValid()) return true;
        const auto m = filterRe.match(s);
        if (!m.hasMatch()) return false;
        *ms = m.capturedStart();
        *me = m.capturedEnd();
        return true;
    }
    const int i = s.indexOf(filterText, 0, Qt::CaseInsensitive);
    if (i < 0) return false;
    *ms = i;
    *me = i + filterText.size();
    return true;
}

void MainWindow::applyFilterAll()
{
    filterText = filterEdit->text();
    filterIsRegex = regexChk->isChecked();
    if (filterIsRegex) {
        filterRe = QRegularExpression(filterText, QRegularExpression::CaseInsensitiveOption);
        filterEdit->setProperty("bad", !filterRe.isValid());
    } else {
        filterEdit->setProperty("bad", false);
    }
    repolish(filterEdit);
    const bool active = !filterText.isEmpty();
    const bool hide = hideChk->isChecked();

    QTextDocument* d = term->document();
    QTextCursor cur(d);
    cur.beginEditBlock();
    QTextCharFormat none;
    none.setBackground(QBrush(Qt::NoBrush));
    QTextCharFormat hl;
    hl.setBackground(kHl);
    for (QTextBlock b = d->begin(); b.isValid(); b = b.next()) {
        const int st = b.userState();
        if (st < 0) { b.setVisible(true); continue; }
        const int base = b.position();
        QTextCursor sel(d);
        sel.setPosition(base + st);
        sel.setPosition(base + b.length() - 1, QTextCursor::KeepAnchor);
        if (sel.hasSelection()) sel.mergeCharFormat(none);
        int ms = 0, me = 0;
        const bool m = matchFilter(b.text().mid(st), &ms, &me);
        if (active && m && me > ms) {
            QTextCursor h(d);
            h.setPosition(base + st + ms);
            h.setPosition(base + st + me, QTextCursor::KeepAnchor);
            h.mergeCharFormat(hl);
        }
        b.setVisible(m || !hide || !active);
    }
    cur.endEditBlock();
    d->markContentsDirty(0, d->characterCount());
    term->viewport()->update();
    scrollIfNeeded();
}

void MainWindow::plotLine(const QString& line, int from)
{
    static const QRegularExpression numRe("[-+]?(?:\\d+\\.?\\d*|\\.\\d+)(?:[eE][-+]?\\d+)?");
    QVector<double> v;
    auto it = numRe.globalMatch(line, from);
    while (it.hasNext()) {
        bool ok = false;
        const double d = it.next().captured().toDouble(&ok);
        if (ok && std::isfinite(d)) v << d;
    }
    if (!v.isEmpty()) plot->addSample(v);
}

void MainWindow::feedText(const QString& s)
{
    if (s.isEmpty()) return;
    QTextCursor c(term->document());
    c.movePosition(QTextCursor::End);
    c.beginEditBlock();
    int start = 0;
    for (;;) {
        const int nl = s.indexOf('\n', start);
        const QString part = nl < 0 ? s.mid(start) : s.mid(start, nl - start);
        if (!part.isEmpty() || nl >= 0) {
            if (!lineOpen) beginLine(c);
            c.insertText(part, fmt(kFg));
            curLine += part;
            if (nl >= 0) endLine(c);
        }
        if (nl < 0) break;
        start = nl + 1;
    }
    c.endEditBlock();
    scrollIfNeeded();
}

void MainWindow::specialLine(const QString& text, const QColor& color)
{
    QTextCursor c(term->document());
    c.movePosition(QTextCursor::End);
    c.beginEditBlock();
    if (lineOpen) endLine(c);
    if (anyLine) c.insertBlock(QTextBlockFormat(), fmt(kFg));
    anyLine = true;
    c.block().setUserState(-1);
    c.insertText(QTime::currentTime().toString("HH:mm:ss.zzz") + "  ", fmt(kDim));
    c.insertText(text, fmt(color, color == kDim));
    c.endEditBlock();
    scrollIfNeeded();
}

void MainWindow::scrollIfNeeded()
{
    if (!scrollChk->isChecked()) return;
    programmaticScroll = true;
    term->verticalScrollBar()->setValue(term->verticalScrollBar()->maximum());
    programmaticScroll = false;
}

// ------------------------------------------------------------------ kirim
void MainWindow::doSend()
{
    const QString text = sendEdit->text();
    const QString eol = eolCombo->currentData().toString();
    const bool hex = sendHexChk->isChecked();
    QByteArray data;
    if (hex) {
        QString h = text;
        h.remove("0x", Qt::CaseInsensitive);
        h.remove(QRegularExpression("[^0-9a-fA-F]"));
        if (h.size() % 2) {
            msgLabel->setText("Jumlah digit HEX harus genap");
            return;
        }
        data = QByteArray::fromHex(h.toLatin1());
    } else {
        data = text.toUtf8();
        if (eol == "lf") data += "\n";
        else if (eol == "cr") data += "\r";
        else if (eol == "crlf") data += "\r\n";
    }
    if (data.isEmpty()) return;
    QString err;
    if (!worker->send(data, &err)) {
        specialLine("Kirim gagal: " + err, kErr);
        return;
    }
    txBytes += data.size();
    txLabel->setText(QString("TX %1 B").arg(txBytes));
    if (echoChk->isChecked()) specialLine("> " + (hex ? "[HEX] " + text : text), kEcho);
    if (!text.isEmpty() && (history.isEmpty() || history.last() != text)) {
        history << text;
        while (history.size() > 50) history.removeFirst();
    }
    histPos = history.size();
    sendEdit->clear();
}

bool MainWindow::eventFilter(QObject* o, QEvent* e)
{
    if (o == sendEdit && e->type() == QEvent::KeyPress) {
        auto* k = static_cast<QKeyEvent*>(e);
        if (k->key() == Qt::Key_Up && !history.isEmpty()) {
            histPos = qMax(0, histPos - 1);
            sendEdit->setText(history.at(histPos));
            return true;
        }
        if (k->key() == Qt::Key_Down) {
            histPos = qMin(int(history.size()), histPos + 1);
            sendEdit->setText(histPos < history.size() ? history.at(histPos) : QString());
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}
