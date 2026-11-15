#pragma once
// Backend serial murni Win32 (tanpa QtSerialPort): thread baca + auto-reconnect.
#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QMutex>
#include <QString>
#include <QThread>

struct PortInfo {
    QString name;   // "COM7"
    QString desc;   // "USB-SERIAL CH340 (COM7)"
};
QList<PortInfo> listSerialPorts();

struct SerialCfg {
    QString port;
    int baud = 115200;
    int data = 8;
    char parity = 'N';      // N E O M S
    QString stop = "1";     // 1, 1.5, 2
    int flow = 0;           // 0 none, 1 RTS/CTS, 2 XON/XOFF
    bool dtr = true;
    bool rts = true;
};

class SerialWorker : public QThread {
    Q_OBJECT
public:
    explicit SerialWorker(QObject* parent = nullptr);
    ~SerialWorker() override;

    void connectPort(const SerialCfg& cfg, bool autoReconnect);
    void disconnectPort();
    void setAutoReconnect(bool v);
    void setLines(int dtr, int rts);               // -1 = tidak diubah
    bool send(const QByteArray& data, QString* err);

signals:
    void dataReceived(const QByteArray& data);
    void statusChanged(bool connected, bool wanted, const QString& message);

protected:
    void run() override;

private:
    bool openLocked(QString& msg);
    void closeLocked();
    void lostLocked(const QString& why);

    QMutex m_mtx;
    void* m_h;                  // HANDLE
    SerialCfg m_cfg;
    bool m_want = false, m_auto = true, m_quit = false;
    bool m_reqConn = false, m_reqDisc = false;
    QElapsedTimer m_retry, m_errEmit;
    bool m_errEmitValid = false;
};
