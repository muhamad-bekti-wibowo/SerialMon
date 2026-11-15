#pragma once
#include <QMainWindow>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QDockWidget;
class QFrame;
class QTimer;
class PlotWidget;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class SerialWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    void startDemo();
    void applyTheme(bool dark);

protected:
    void closeEvent(QCloseEvent* e) override;
    bool eventFilter(QObject* o, QEvent* e) override;

private:
    void buildUi();
    void loadSettings();
    void saveSettings();
    void refreshPorts();
    void onConnectClicked();
    void onStatus(bool connected, bool wanted, const QString& msg);
    void onData(const QByteArray& data);
    void doSend();
    void feedText(const QString& s);
    void specialLine(const QString& text, const QColor& color);
    void beginLine(class QTextCursor& c);
    void endLine(class QTextCursor& c);
    void scrollIfNeeded();
    void updateStateUi();
    void applyFilterAll();
    bool matchFilter(const QString& s, int* ms, int* me) const;
    void plotLine(const QString& line, int from);

    QLabel* stateLabel;
    QFrame* pill;
    QPushButton* themeBtn;
    bool darkTheme = false;
    QLineEdit* filterEdit;
    QCheckBox *regexChk, *hideChk, *afterChk;
    QPushButton* plotBtn;
    QDockWidget* plotDock;
    PlotWidget* plot;
    QLabel* plotInfo;
    QTimer* filterTimer;
    QString filterText;
    bool filterIsRegex = false;
    QRegularExpression filterRe;
    qint64 lastTotal = 0;

    SerialWorker* worker;
    QComboBox *portCombo, *baudCombo, *dataCombo, *parityCombo, *stopCombo, *flowCombo, *eolCombo;
    QPushButton *connBtn, *pauseBtn;
    QCheckBox *dtrChk, *rtsChk, *autoChk, *tsChk, *scrollChk, *hexChk, *echoChk, *sendHexChk;
    QLabel *dot, *msgLabel, *rxLabel, *txLabel, *pausedLabel;
    QLineEdit* sendEdit;
    QPlainTextEdit* term;

    bool connected = false, wanted = false;
    QString lastPortSig, lastMsg;

    QStringDecoder decoder{QStringDecoder::Utf8};
    bool lastCR = false, lineOpen = false, anyLine = false, programmaticScroll = false, paused = false;
    int hexCount = 0, lineStartPos = 0;
    QString curLine;
    QStringList pending, history;
    int histPos = 0;
    qint64 rxBytes = 0, txBytes = 0;
};
