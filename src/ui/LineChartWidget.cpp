#include "LineChartWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <QtGlobal>

namespace comm {

class ChartCanvas : public QWidget
{
public:
    explicit ChartCanvas(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_clock.start();
        m_repaintTimer.setInterval(33); // ~30 fps
        connect(&m_repaintTimer, &QTimer::timeout, this, [this]() { update(); });
        m_repaintTimer.start();
    }

    void setSeries(const QList<SeriesInfo>& series)
    {
        m_series.clear();
        m_series.reserve(series.size());
        for (const SeriesInfo& info : series) {
            SeriesData data;
            data.name = info.name;
            data.color = info.color;
            m_series.append(data);
        }
        update();
    }

    void appendValue(int seriesIndex, double value)
    {
        if (seriesIndex < 0 || seriesIndex >= m_series.size())
            return;

        const double t = m_clock.elapsed() / 1000.0;
        m_series[seriesIndex].points.append({t, value});
        prune(nowSeconds());
    }

    void setDuration(int ms) { m_durationMs = qMax(10, ms); }
    int duration() const { return m_durationMs; }

    void clearChart()
    {
        for (SeriesData& s : m_series)
            s.points.clear();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(rect(), Qt::white);

        const double now = nowSeconds();
        prune(now);

        const QRect area = rect().adjusted(56, 24, -12, -28);
        if (area.width() <= 0 || area.height() <= 0)
            return;

        // Axes
        painter.setPen(Qt::black);
        painter.drawLine(area.bottomLeft(), area.topLeft());
        painter.drawLine(area.bottomLeft(), area.bottomRight());

        int totalPoints = 0;
        for (const SeriesData& s : m_series)
            totalPoints += s.points.size();

        if (totalPoints < 1) {
            painter.setPen(Qt::gray);
            painter.drawText(area, Qt::AlignCenter, tr("No data"));
            return;
        }

        const double tMin = now - m_durationMs / 1000.0;
        const double tMax = now;

        // Auto-scale Y across all series.
        double vMin = 0.0;
        double vMax = 0.0;
        bool first = true;
        for (const SeriesData& s : m_series) {
            for (const Point& pt : s.points) {
                if (first) {
                    vMin = vMax = pt.v;
                    first = false;
                } else {
                    vMin = qMin(vMin, pt.v);
                    vMax = qMax(vMax, pt.v);
                }
            }
        }

        if (vMin == vMax) {
            vMin -= 1.0;
            vMax += 1.0;
        } else {
            const double pad = (vMax - vMin) * 0.1;
            vMin -= pad;
            vMax += pad;
        }

        const auto tx = [&](double t) {
            return area.left() + (t - tMin) / (tMax - tMin) * area.width();
        };
        const auto ty = [&](double v) {
            return area.bottom() - (v - vMin) / (vMax - vMin) * area.height();
        };

        // Horizontal grid lines + Y labels.
        for (int i = 0; i <= 4; ++i) {
            const double v = vMin + (vMax - vMin) * i / 4.0;
            const int y = qRound(ty(v));

            painter.setPen(QColor(225, 225, 225));
            painter.drawLine(area.left(), y, area.right(), y);

            painter.setPen(Qt::black);
            painter.drawText(QRect(0, y - 8, area.left() - 6, 16),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(v, 'g', 4));
        }

        // Draw each series polyline in its own color.
        for (const SeriesData& s : m_series) {
            if (s.points.size() < 2)
                continue;

            QPolygonF polyline;
            polyline.reserve(s.points.size());
            for (const Point& pt : s.points)
                polyline.append(QPointF(tx(pt.t), ty(pt.v)));

            painter.setPen(QPen(s.color, 2));
            painter.drawPolyline(polyline);
        }

        // Legend: names in their series colors, top-right.
        int legendX = area.right();
        const int legendY = rect().top() + 2;
        for (const SeriesData& s : m_series) {
            const int textWidth = painter.fontMetrics().horizontalAdvance(s.name) + 14;
            legendX -= textWidth;
            if (legendX < area.left())
                break;
            painter.setPen(s.color);
            painter.drawText(QRect(legendX, legendY, textWidth, 16),
                             Qt::AlignRight | Qt::AlignVCenter, s.name);
        }
    }

private:
    struct Point {
        double t;
        double v;
    };

    struct SeriesData {
        QString name;
        QColor color;
        QList<Point> points;
    };

    double nowSeconds() const { return m_clock.elapsed() / 1000.0; }

    void prune(double now)
    {
        const double cutoff = now - m_durationMs / 1000.0;
        for (SeriesData& s : m_series) {
            while (!s.points.isEmpty() && s.points.first().t < cutoff)
                s.points.removeFirst();
        }
    }

    QList<SeriesData> m_series;
    QElapsedTimer m_clock;
    QTimer m_repaintTimer;
    int m_durationMs = 1000;
};

LineChartWidget::LineChartWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel(tr("Line chart")));
    topRow->addStretch(1);
    topRow->addWidget(new QLabel(tr("Duration:")));
    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(10, 600000);
    m_durationSpin->setValue(1000);
    m_durationSpin->setSuffix(tr(" ms"));
    topRow->addWidget(m_durationSpin);
    m_maximizeButton = new QPushButton(tr("Maximize"), this);
    topRow->addWidget(m_maximizeButton);
    layout->addLayout(topRow);

    m_canvas = new ChartCanvas(this);
    m_canvas->setMinimumHeight(160);
    layout->addWidget(m_canvas, 1);

    connect(m_durationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int ms) {
        m_canvas->setDuration(ms);
    });
    connect(m_maximizeButton, &QPushButton::clicked, this, &LineChartWidget::onMaximizeClicked);

    setEnabledState(false);
}

void LineChartWidget::setSeries(const QList<SeriesInfo>& series)
{
    m_canvas->setSeries(series);
}

void LineChartWidget::appendValue(int seriesIndex, double value)
{
    if (m_enabled)
        m_canvas->appendValue(seriesIndex, value);
}

void LineChartWidget::setEnabledState(bool enabled)
{
    m_enabled = enabled;
    m_canvas->setVisible(enabled);
    if (!enabled)
        clearChart();
}

int LineChartWidget::durationMs() const
{
    return m_durationSpin->value();
}

void LineChartWidget::clearChart()
{
    m_canvas->clearChart();
}

void LineChartWidget::onMaximizeClicked()
{
    m_maximized = !m_maximized;
    m_maximizeButton->setText(m_maximized ? tr("Restore") : tr("Maximize"));
    emit maximizeRequested(m_maximized);
}

} // namespace comm
