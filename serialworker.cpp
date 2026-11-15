#include "serialworker.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>

#include <algorithm>

static HANDLE H(void* p) { return reinterpret_cast<HANDLE>(p); }

// ---------------------------------------------------------------- daftar port
QList<PortInfo> listSerialPorts()
{
    QList<PortInfo> out;
    HDEVINFO set = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (set == INVALID_HANDLE_VALUE) return out;

    SP_DEVINFO_DATA dev;
    dev.cbSize = sizeof(dev);
    for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
        char name[64] = {0};
        HKEY key = SetupDiOpenDevRegKey(set, &dev, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (key == INVALID_HANDLE_VALUE) continue;
        DWORD sz = sizeof(name) - 1, type = 0;
        LONG r = RegQueryValueExA(key, "PortName", nullptr, &type, reinterpret_cast<LPBYTE>(name), &sz);
        RegCloseKey(key);
        if (r != ERROR_SUCCESS || qstrncmp(name, "COM", 3) != 0) continue;

        char friendly[256] = {0};
        if (!SetupDiGetDeviceRegistryPropertyA(set, &dev, SPDRP_FRIENDLYNAME, nullptr,
                                               reinterpret_cast<PBYTE>(friendly), sizeof(friendly) - 1, nullptr))
            friendly[0] = 0;
        PortInfo p;
        p.name = QString::fromLatin1(name);
        p.desc = friendly[0] ? QString::fromLocal8Bit(friendly) : p.name;
        out.append(p);
    }
    SetupDiDestroyDeviceInfoList(set);

    std::sort(out.begin(), out.end(), [](const PortInfo& a, const PortInfo& b) {
        return a.name.mid(3).toInt() < b.name.mid(3).toInt();
    });
    return out;
}

// ---------------------------------------------------------------- worker
SerialWorker::SerialWorker(QObject* parent) : QThread(parent), m_h(INVALID_HANDLE_VALUE)
{
    m_retry.start();
    start();
}

SerialWorker::~SerialWorker()
{
    {
        QMutexLocker l(&m_mtx);
        m_quit = true;
    }
    wait();
}

void SerialWorker::connectPort(const SerialCfg& cfg, bool autoReconnect)
{
    QMutexLocker l(&m_mtx);
    m_cfg = cfg;
    m_auto = autoReconnect;
    m_reqConn = true;
    m_reqDisc = false;
}

void SerialWorker::disconnectPort()
{
    QMutexLocker l(&m_mtx);
    m_reqDisc = true;
    m_reqConn = false;
}

void SerialWorker::setAutoReconnect(bool v)
{
    QMutexLocker l(&m_mtx);
    m_auto = v;
}

void SerialWorker::setLines(int dtr, int rts)
{
    QMutexLocker l(&m_mtx);
    if (dtr >= 0) m_cfg.dtr = dtr;
    if (rts >= 0) m_cfg.rts = rts;
    if (m_h == INVALID_HANDLE_VALUE) return;
    if (dtr >= 0) EscapeCommFunction(H(m_h), dtr ? SETDTR : CLRDTR);
    if (rts >= 0 && m_cfg.flow != 1) EscapeCommFunction(H(m_h), rts ? SETRTS : CLRRTS);
}

bool SerialWorker::send(const QByteArray& data, QString* err)
{
    QMutexLocker l(&m_mtx);
    if (m_h == INVALID_HANDLE_VALUE) {
        if (err) *err = QStringLiteral("Belum tersambung");
        return false;
    }
    OVERLAPPED ov = {};
    ov.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    DWORD written = 0;
    bool ok = true;
    if (!WriteFile(H(m_h), data.constData(), DWORD(data.size()), &written, &ov)) {
        if (GetLastError() != ERROR_IO_PENDING || !GetOverlappedResult(H(m_h), &ov, &written, TRUE)) ok = false;
    }
    CloseHandle(ov.hEvent);
    if (!ok || written != DWORD(data.size())) {
        if (err) *err = QStringLiteral("Gagal menulis (kode %1)").arg(GetLastError());
        return false;
    }
    return true;
}

bool SerialWorker::openLocked(QString& msg)
{
    const SerialCfg& c = m_cfg;
    const QByteArray path = ("\\\\.\\" + c.port).toLatin1();
    HANDLE h = CreateFileA(path.constData(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED) msg = QStringLiteral("%1 sedang dipakai aplikasi lain (akses ditolak)").arg(c.port);
        else if (e == ERROR_FILE_NOT_FOUND) msg = QStringLiteral("%1 tidak ditemukan").arg(c.port);
        else msg = QStringLiteral("Gagal membuka %1 (kode %2)").arg(c.port).arg(e);
        return false;
    }

    DCB dcb = {};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        msg = QStringLiteral("GetCommState gagal pada %1").arg(c.port);
        CloseHandle(h);
        return false;
    }
    dcb.BaudRate = DWORD(c.baud);
    dcb.ByteSize = BYTE(c.data);
    switch (c.parity) {
    case 'E': dcb.Parity = EVENPARITY; break;
    case 'O': dcb.Parity = ODDPARITY; break;
    case 'M': dcb.Parity = MARKPARITY; break;
    case 'S': dcb.Parity = SPACEPARITY; break;
    default:  dcb.Parity = NOPARITY; break;
    }
    dcb.fParity = c.parity != 'N';
    dcb.StopBits = c.stop == "2" ? TWOSTOPBITS : c.stop == "1.5" ? ONE5STOPBITS : ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fDtrControl = c.dtr ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE;
    dcb.fOutxCtsFlow = c.flow == 1;
    dcb.fRtsControl = c.flow == 1 ? RTS_CONTROL_HANDSHAKE : (c.rts ? RTS_CONTROL_ENABLE : RTS_CONTROL_DISABLE);
    dcb.fOutX = dcb.fInX = (c.flow == 2);
    if (!SetCommState(h, &dcb)) {
        msg = QStringLiteral("Pengaturan %1 baud/format ditolak oleh driver").arg(c.baud);
        CloseHandle(h);
        return false;
    }
    // Baca: kembali 5 ms setelah byte terakhir, atau 50 ms jika tidak ada data.
    COMMTIMEOUTS to = {};
    to.ReadIntervalTimeout = 5;
    to.ReadTotalTimeoutMultiplier = 0;
    to.ReadTotalTimeoutConstant = 50;
    to.WriteTotalTimeoutConstant = 2000;
    SetCommTimeouts(h, &to);
    SetupComm(h, 1 << 16, 1 << 16);
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);

    m_h = h;
    msg = QStringLiteral("Tersambung %1 @ %2").arg(c.port).arg(c.baud);
    return true;
}

void SerialWorker::closeLocked()
{
    if (m_h != INVALID_HANDLE_VALUE) {
        CloseHandle(H(m_h));
        m_h = INVALID_HANDLE_VALUE;
    }
}

void SerialWorker::lostLocked(const QString& why)
{
    closeLocked();
    if (!m_auto) m_want = false;
    m_retry.restart();
    m_errEmit.restart();
    m_errEmitValid = true;
    emit statusChanged(false, m_want,
                       QStringLiteral("Koneksi putus (%1)").arg(why) +
                           (m_want ? QStringLiteral(" - mencoba sambung ulang...") : QString()));
}

void SerialWorker::run()
{
    HANDLE evt = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    char buf[8192];

    for (;;) {
        HANDLE rh = INVALID_HANDLE_VALUE;
        {
            QMutexLocker l(&m_mtx);
            if (m_quit) break;

            if (m_reqDisc) {
                m_reqDisc = false;
                m_want = false;
                closeLocked();
                emit statusChanged(false, false, QStringLiteral("Terputus"));
            }
            bool tryOpen = false;
            if (m_reqConn) {
                m_reqConn = false;
                closeLocked();
                m_want = true;
                m_errEmitValid = false;
                tryOpen = true;
            } else if (m_h == INVALID_HANDLE_VALUE && m_want && m_retry.elapsed() >= 500) {
                tryOpen = true;
            }
            if (tryOpen) {
                QString msg;
                if (openLocked(msg)) {
                    emit statusChanged(true, true, msg);
                } else {
                    if (!m_auto) m_want = false;
                    m_retry.restart();
                    if (!m_errEmitValid || m_errEmit.elapsed() > 5000 || !m_want) {
                        m_errEmitValid = true;
                        m_errEmit.restart();
                        emit statusChanged(false, m_want, msg + (m_want ? QStringLiteral(" - mencoba lagi...") : QString()));
                    }
                }
            }
            rh = H(m_h);
        }

        if (rh == INVALID_HANDLE_VALUE) {
            msleep(30);
            continue;
        }

        OVERLAPPED ov = {};
        ov.hEvent = evt;
        ResetEvent(evt);
        DWORD n = 0;
        bool fail = false;
        if (!ReadFile(rh, buf, sizeof(buf), &n, &ov)) {
            if (GetLastError() != ERROR_IO_PENDING || !GetOverlappedResult(rh, &ov, &n, TRUE)) fail = true;
        }
        if (!fail && n == 0) {
            DWORD ms = 0;
            if (!GetCommModemStatus(rh, &ms)) fail = true;   // port dicabut
        }
        if (fail) {
            QString why = QStringLiteral("kode %1").arg(GetLastError());
            QMutexLocker l(&m_mtx);
            if (m_h == rh) lostLocked(why);
            continue;
        }
        if (n > 0) emit dataReceived(QByteArray(buf, int(n)));
    }

    CloseHandle(evt);
    QMutexLocker l(&m_mtx);
    closeLocked();
}
