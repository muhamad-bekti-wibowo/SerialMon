#include "plotwidget.h"

#include <QFile>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <limits>

static const QColor kBg(0x0c, 0x11, 0x17), kGrid(0x25, 0x30, 0x3c), kText(0x8f, 0xa0, 0xb2);  // --screen/--grid situs
static const QColor kPalette[] = {QColor("#3ddc84"), QColor("#5fb0ff"), QColor("#f5a524"), QColor("#ff7a2e"),
                                  QColor("#c586ff"), QColor("#5fd4e6"), QColor("#ff6fb5"), QColor("#b0b8c4")};
static const double kNaN = std::numeric_limits<double>::quiet_NaN();

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(320, 220);
    m_timer.setInterval(33);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (m_dirty) {
            m_dirty = false;
            update();
        }
    });
    m_timer.start();
}

void PlotWidget::addSample(const QVector<double>& v)
{
    const size_t len = m_ch.empty() ? 0 : m_ch[0].size();
    while (m_ch.size() < size_t(v.size())) {      // kanal baru: isi NaN untuk sampel sebelumnya
        m_ch.emplace_back(len, kNaN);
        m_visible.push_back(true);
    }
    for (size_t c = 0; c < m_ch.size(); ++c) m_ch[c].push_back(c < size_t(v.size()) ? v[int(c)] : kNaN);
    ++m_total;
    if (m_ch[0].size() > size_t(kMaxSamples)) {
        for (auto& d : m_ch) d.pop_front();
        ++m_dropped;
        if (m_paused && m_viewEnd > 0) --m_viewEnd;
    }
    m_dirty = true;
}

void PlotWidget::clearData()
{
    m_ch.clear();
    m_visible.clear();
    m_total = m_dropped = 0;
    m_viewEnd = 0;
    m_dirty = true;
}

void PlotWidget::setWindow(int points)
{
    m_window = std::max(10, points);
    m_dirty = true;
}

void PlotWidget::setPaused(bool p)
{
    m_paused = p;
    m_viewEnd = m_ch.empty() ? 0 : int(m_ch[0].size());
    m_dirty = true;
}

bool PlotWidget::saveCsv(const QString& fileName) const
{
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream t(&f);
    t << "n";
    for (size_t c = 0; c < m_ch.size(); ++c) t << ",ch" << (c + 1);
    t << "\n";
    const size_t n = m_ch.empty() ? 0 : m_ch[0].size();
    for (size_t i = 0; i < n; ++i) {
        t << (m_dropped + qint64(i) + 1);
        for (const auto& ch : m_ch) {
            t << ',';
            if (!std::isnan(ch[i])) t << QString::number(ch[i], 'g', 12);
        }
        t << "\n";
    }
    return true;
}

void PlotWidget::mousePressEvent(QMouseEvent* e)
{
    for (size_t i = 0; i < m_legendRects.size() && i < m_visible.size(); ++i)
        if (m_legendRects[i].contains(e->pos())) {
            m_visible[i] = !m_visible[i];
            m_dirty = true;
            return;
        }
}

void PlotWidget::mouseMoveEvent(QMouseEvent* e)
{
    m_hoverX = e->pos().x();
    m_dirty = true;
}

void PlotWidget::leaveEvent(QEvent*)
{
    m_hoverX = -1;
    m_dirty = true;
}

void PlotWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), kBg);
    p.setRenderHint(QPainter::Antialiasing, false);
    QFont small("JetBrains Mono");
    small.setPointSize(8);
    p.setFont(small);
    const QFontMetrics fm(small);

    const QRect pr(62, 30, width() - 62 - 12, height() - 30 - 22);
    m_plotRect = pr;
    if (pr.width() < 50 || pr.height() < 40) return;

    const int n = m_ch.empty() ? 0 : int(m_ch[0].size());
    const int end = m_paused ? std::min(m_viewEnd, n) : n;
    const int start = std::max(0, end - m_window);
    const int cnt = end - start;

    // indeks sampel di bawah kursor
    int hoverIdx = -1;
    if (cnt > 0 && m_hoverX >= pr.left() && m_hoverX <= pr.right())
        hoverIdx = start + int(std::lround(double(m_hoverX - pr.left()) * (cnt - 1) / std::max(1, pr.width())));

    // ---- legenda (klik untuk sembunyikan/tampilkan kanal)
    m_legendRects.assign(m_ch.size(), QRect());
    int lx = pr.left(), ly = 6;
    for (size_t c = 0; c < m_ch.size(); ++c) {
        const int idx = hoverIdx >= 0 ? hoverIdx : end - 1;
        QString val = (idx >= 0 && idx < n && !std::isnan(m_ch[c][idx])) ? QString::number(m_ch[c][idx], 'g', 7) : "-";
        const QString txt = QString("ch%1 %2").arg(c + 1).arg(val);
        const int w = fm.horizontalAdvance(txt) + 22;
        if (lx + w > width() - 6 && lx > pr.left()) { lx = pr.left(); ly += 16; }
        const QRect r(lx, ly, w, 16);
        m_legendRects[c] = r;
        const QColor col = kPalette[c % 8];
        p.fillRect(QRect(r.left() + 3, r.top() + 4, 9, 9), m_visible[c] ? col : kGrid);
        p.setPen(m_visible[c] ? kText : kGrid.lighter(150));
        p.drawText(r.adjusted(17, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft, txt);
        lx += w + 6;
    }

    p.setPen(kGrid);
    p.drawRect(pr);
    if (cnt < 1) {
        p.setPen(kText);
        p.drawText(pr, Qt::AlignCenter, "Menunggu data...  (angka diambil dari baris yang cocok dengan filter)");
        return;
    }

    // ---- rentang Y
    double lo = std::numeric_limits<double>::infinity(), hi = -lo;
    for (size_t c = 0; c < m_ch.size(); ++c) {
        if (!m_visible[c]) continue;
        for (int i = start; i < end; ++i) {
            const double v = m_ch[c][i];
            if (std::isnan(v)) continue;
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    }
    if (!std::isfinite(lo)) { lo = 0; hi = 1; }
    if (hi - lo < 1e-12) { lo -= 1; hi += 1; }
    const double pad = (hi - lo) * 0.05;
    lo -= pad;
    hi += pad;
    auto yOf = [&](double v) { return pr.bottom() - (v - lo) / (hi - lo) * pr.height(); };
    const double denom = std::max(1, cnt - 1);
    auto xOf = [&](int i) { return pr.left() + double(i - start) / denom * pr.width(); };

    // ---- grid + label sumbu
    p.setPen(kText);
    for (int k = 0; k <= 4; ++k) {
        const double v = lo + (hi - lo) * k / 4.0;
        const int y = int(yOf(v));
        p.setPen(kGrid);
        if (k > 0 && k < 4) p.drawLine(pr.left(), y, pr.right(), y);
        p.setPen(kText);
        p.drawText(QRect(0, y - 8, pr.left() - 5, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(v, 'g', 5));
    }
    for (int k = 0; k <= 4; ++k) {
        const int i = start + int(std::lround((cnt - 1) * k / 4.0));
        const int x = int(xOf(i));
        p.setPen(kGrid);
        if (k > 0 && k < 4) p.drawLine(x, pr.top(), x, pr.bottom());
        p.setPen(kText);
        const QString lab = QString::number(m_dropped + i + 1);
        const int tw = fm.horizontalAdvance(lab);
        int tx = std::clamp(x - tw / 2, 0, width() - tw - 2);
        p.drawText(tx, pr.bottom() + 15, lab);
    }

    // ---- garis data
    p.setClipRect(pr.adjusted(1, 1, 0, 0));
    for (size_t c = 0; c < m_ch.size(); ++c) {
        if (!m_visible[c]) continue;
        p.setPen(QPen(kPalette[c % 8], 1.4));
        QPolygonF poly;
        auto flush = [&] {
            if (poly.size() > 1) p.drawPolyline(poly);
            else if (poly.size() == 1) p.drawPoint(poly[0]);
            poly.clear();
        };
        if (cnt > pr.width() * 2) {          // terlalu rapat: ringkas jadi min/max per kolom piksel
            const int cols = pr.width();
            for (int col = 0; col < cols; ++col) {
                const int a = start + int(qint64(col) * cnt / cols);
                const int b = std::max(a + 1, start + int(qint64(col + 1) * cnt / cols));
                double mn = std::numeric_limits<double>::infinity(), mx = -mn;
                for (int i = a; i < b && i < end; ++i) {
                    const double v = m_ch[c][i];
                    if (std::isnan(v)) continue;
                    mn = std::min(mn, v);
                    mx = std::max(mx, v);
                }
                if (!std::isfinite(mn)) { flush(); continue; }
                const double x = pr.left() + col;
                poly << QPointF(x, yOf(mn)) << QPointF(x, yOf(mx));
            }
            flush();
        } else {
            for (int i = start; i < end; ++i) {
                const double v = m_ch[c][i];
                if (std::isnan(v)) { flush(); continue; }
                poly << QPointF(xOf(i), yOf(v));
            }
            flush();
        }
    }
    p.setClipping(false);

    // ---- kursor hover
    if (hoverIdx >= 0) {
        p.setPen(QPen(kText, 1, Qt::DashLine));
        const int x = int(xOf(hoverIdx));
        p.drawLine(x, pr.top(), x, pr.bottom());
        p.setPen(kText);
        p.drawText(pr.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignTop,
                   QString("#%1").arg(m_dropped + hoverIdx + 1));
    }
    if (m_paused) {
        p.setPen(QColor("#f5a524"));
        p.drawText(pr.adjusted(6, 4, 0, 0), Qt::AlignLeft | Qt::AlignTop, "DIJEDA");
    }
}
