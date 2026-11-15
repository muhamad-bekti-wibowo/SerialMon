#pragma once
// Plotter multi-kanal real-time (digambar manual dengan QPainter, tanpa QtCharts).
#include <QRect>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <deque>
#include <vector>

class PlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget* parent = nullptr);

    void addSample(const QVector<double>& v);   // satu sampel = satu baris, tiap angka = satu kanal
    void clearData();
    void setWindow(int points);
    void setPaused(bool p);
    bool saveCsv(const QString& fileName) const;
    qint64 totalSamples() const { return m_total; }
    int channelCount() const { return int(m_ch.size()); }

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    static constexpr int kMaxSamples = 200000;

    std::vector<std::deque<double>> m_ch;
    std::vector<bool> m_visible;
    qint64 m_total = 0;     // jumlah sampel sejak Bersihkan
    qint64 m_dropped = 0;   // sampel terbuang karena batas memori
    int m_window = 1000;
    bool m_paused = false;
    int m_viewEnd = 0;
    int m_hoverX = -1;
    bool m_dirty = true;
    QRect m_plotRect;
    std::vector<QRect> m_legendRects;
    QTimer m_timer;
};
