#include "LineChartWidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <QtGlobal>

#include <cmath>

namespace comm {

namespace {

constexpr double kMinLabelDistance = 48.0;

double niceStepMs(double raw)
{
    if (raw <= 0.0)
        return 1.0;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double norm = raw / mag;
    double step = 10.0;
    if (norm <= 1.0)
        step = 1.0;
    else if (norm <= 2.0)
        step = 2.0;
    else if (norm <= 5.0)
        step = 5.0;
    return step * mag;
}

QString formatValue(double v)
{
    if (v == std::floor(v) && std::abs(v) < 1e15)
        return QString::number(static_cast<qint64>(v));
    return QString::number(v, 'g', 6);
}

} // namespace

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
        if (series.size() == m_series.size()) {
            // Same count: update names/colors in place, keep the plotted points.
            for (int i = 0; i < series.size(); ++i) {
                m_series[i].name = series.at(i).name;
                m_series[i].color = series.at(i).color;
            }
            update();
            return;
        }

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
        if (m_paused)
            return;

        if (seriesIndex < 0 || seriesIndex >= m_series.size())
            return;

        const double t = nowSeconds();
        m_series[seriesIndex].points.append({t, value});
        prune(nowSeconds());
    }

    void setSeriesVisible(int seriesIndex, bool visible)
    {
        if (seriesIndex >= 0 && seriesIndex < m_series.size()) {
            m_series[seriesIndex].visible = visible;
            update();
        }
    }

    void setShowValues(bool show)
    {
        m_showValues = show;
        update();
    }

    void setDuration(int ms) { m_durationMs = qMax(10, ms); }
    int duration() const { return m_durationMs; }

    void setPaused(bool paused)
    {
        if (paused == m_paused)
            return;

        if (paused) {
            const double now = (m_clock.elapsed() - m_pausedOffsetMs) / 1000.0;
            prune(now);
            m_viewTMin = now - m_durationMs / 1000.0;
            m_viewTMax = now;
            m_hasView = true;
            m_pauseStartMs = m_clock.elapsed();
            m_paused = true;
        } else {
            m_pausedOffsetMs += m_clock.elapsed() - m_pauseStartMs;
            m_paused = false;
            m_hasView = false;
            m_panning = false;
        }
        update();
    }

    bool isPaused() const { return m_paused; }

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

        const QRect area = plotArea();
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

        double tMin;
        double tMax;
        if (m_paused && m_hasView) {
            tMin = m_viewTMin;
            tMax = m_viewTMax;
        } else {
            tMin = now - m_durationMs / 1000.0;
            tMax = now;
        }

        // Auto-adapt Y to the data visible in the current time window.
        double vMin;
        double vMax;
        computeValueRange(tMin, tMax, vMin, vMax);

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

        // X axis time labels (relative ms, zero-padded), as many as fit.
        {
            const double spanMs = (tMax - tMin) * 1000.0;
            const double rawStep = spanMs * 60.0 / area.width();
            const double stepMs = niceStepMs(rawStep);
            const int digits = QString::number(qCeil(spanMs)).size();

            painter.setPen(Qt::black);
            for (double ms = 0.0; ms <= spanMs + stepMs * 0.5; ms += stepMs) {
                const double t = tMin + ms / 1000.0;
                const int x = qRound(tx(t));
                painter.drawLine(x, area.bottom(), x, area.bottom() + 4);
                painter.drawText(QRect(x - 40, area.bottom() + 4, 80, 16),
                                 Qt::AlignHCenter | Qt::AlignTop,
                                 QStringLiteral("%1").arg(qRound(ms), digits, 10, QLatin1Char('0')));
            }
        }

        // Draw each visible series in its own color, plus value labels.
        for (const SeriesData& s : m_series) {
            if (!s.visible || s.points.isEmpty())
                continue;

            if (s.points.size() >= 2) {
                QPolygonF polyline;
                polyline.reserve(s.points.size());
                for (const Point& pt : s.points)
                    polyline.append(QPointF(tx(pt.t), ty(pt.v)));

                painter.setPen(QPen(s.color, 2));
                painter.drawPolyline(polyline);
            }

            // Value labels near the points, spaced out to avoid crowding.
            if (m_showValues) {
                painter.setPen(s.color);
                QPointF lastLabelPos;
                bool hasLast = false;
                for (int i = 0; i < s.points.size(); ++i) {
                    const QPointF pos(tx(s.points.at(i).t), ty(s.points.at(i).v));
                    const bool isLast = (i == s.points.size() - 1);

                    bool label = isLast;
                    if (!label) {
                        if (!hasLast) {
                            label = true;
                        } else {
                            const double dx = pos.x() - lastLabelPos.x();
                            const double dy = pos.y() - lastLabelPos.y();
                            label = (dx * dx + dy * dy) >= kMinLabelDistance * kMinLabelDistance;
                        }
                    }

                    if (label) {
                        painter.drawText(pos + QPointF(4, -4), formatValue(s.points.at(i).v));
                        lastLabelPos = pos;
                        hasLast = true;
                    }
                }
            }
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

    void wheelEvent(QWheelEvent* event) override
    {
        if (!m_paused || !m_hasView) {
            QWidget::wheelEvent(event);
            return;
        }

        const QRect area = plotArea();
        if (area.width() <= 0 || area.height() <= 0) {
            event->ignore();
            return;
        }

        const QPointF pos = event->position();
        const double factor = (event->angleDelta().y() > 0) ? 0.8 : 1.25;
        const double tFrac = qBound(0.0, (pos.x() - area.left()) / double(area.width()), 1.0);
        const double tAtCursor = m_viewTMin + tFrac * (m_viewTMax - m_viewTMin);
        const double newTSpan = (m_viewTMax - m_viewTMin) * factor;
        m_viewTMin = tAtCursor - tFrac * newTSpan;
        m_viewTMax = m_viewTMin + newTSpan;

        update();
        event->accept();
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (m_paused && m_hasView && event->button() == Qt::LeftButton) {
            m_panning = true;
            m_panStart = event->position();
            m_panStartTMin = m_viewTMin;
            m_panStartTMax = m_viewTMax;
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (!m_panning) {
            QWidget::mouseMoveEvent(event);
            return;
        }

        const QRect area = plotArea();
        if (area.width() <= 0)
            return;

        const double deltaX = event->position().x() - m_panStart.x();
        const double dt = -deltaX / double(area.width()) * (m_panStartTMax - m_panStartTMin);
        m_viewTMin = m_panStartTMin + dt;
        m_viewTMax = m_panStartTMax + dt;
        update();
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (m_panning && event->button() == Qt::LeftButton) {
            m_panning = false;
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
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
        bool visible = true;
    };

    QRect plotArea() const { return rect().adjusted(56, 24, -12, -28); }

    qint64 effectiveElapsedMs() const
    {
        if (m_paused)
            return m_pauseStartMs - m_pausedOffsetMs;
        return m_clock.elapsed() - m_pausedOffsetMs;
    }

    double nowSeconds() const { return effectiveElapsedMs() / 1000.0; }

    void prune(double now)
    {
        const double cutoff = now - m_durationMs / 1000.0;
        for (SeriesData& s : m_series) {
            while (!s.points.isEmpty() && s.points.first().t < cutoff)
                s.points.removeFirst();
        }
    }

    void computeValueRange(double tMin, double tMax, double& vMin, double& vMax) const
    {
        vMin = 0.0;
        vMax = 0.0;
        bool first = true;
        for (const SeriesData& s : m_series) {
            for (const Point& pt : s.points) {
                if (pt.t < tMin || pt.t > tMax)
                    continue;
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
    }

    QList<SeriesData> m_series;
    QElapsedTimer m_clock;
    QTimer m_repaintTimer;
    int m_durationMs = 1000;
    bool m_showValues = true;

    bool m_paused = false;
    qint64 m_pauseStartMs = 0;
    qint64 m_pausedOffsetMs = 0;

    bool m_hasView = false;
    double m_viewTMin = 0.0;
    double m_viewTMax = 0.0;

    bool m_panning = false;
    QPointF m_panStart;
    double m_panStartTMin = 0.0;
    double m_panStartTMax = 0.0;
};

LineChartWidget::LineChartWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* topRow = new QHBoxLayout;
    m_showValuesCheck = new QCheckBox(tr("Show values"), this);
    m_showValuesCheck->setChecked(true);
    topRow->addWidget(m_showValuesCheck);
    topRow->addStretch(1);

    m_speedLayout = new QHBoxLayout;
    m_speedLayout->setSpacing(8);
    m_rawSpeedLabel = new QLabel(tr("Raw data: 0.0 kb/s"), this);
    m_speedLayout->addWidget(m_rawSpeedLabel);
    topRow->addLayout(m_speedLayout);

    topRow->addWidget(new QLabel(tr("Duration:")));
    m_durationSpin = new QSpinBox(this);
    m_durationSpin->setRange(10, 600000);
    m_durationSpin->setValue(1000);
    m_durationSpin->setSuffix(tr(" ms"));
    topRow->addWidget(m_durationSpin);
    m_pauseButton = new QPushButton(tr("Pause"), this);
    topRow->addWidget(m_pauseButton);
    layout->addLayout(topRow);

    m_checksLayout = new QHBoxLayout;
    m_checksLayout->setSpacing(8);
    layout->addLayout(m_checksLayout);

    m_canvas = new ChartCanvas(this);
    m_canvas->setMinimumHeight(160);
    layout->addWidget(m_canvas, 1);

    connect(m_durationSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int ms) {
        m_canvas->setDuration(ms);
    });
    connect(m_showValuesCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_canvas->setShowValues(checked);
        emit settingsChanged();
    });
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        m_paused = !m_paused;
        m_pauseButton->setText(m_paused ? tr("Resume") : tr("Pause"));
        m_canvas->setPaused(m_paused);
    });

    setEnabledState(false);
}

void LineChartWidget::setSeries(const QList<SeriesInfo>& series)
{
    m_canvas->setSeries(series);
    rebuildSeriesChecks(series);
}

void LineChartWidget::rebuildSeriesChecks(const QList<SeriesInfo>& series)
{
    // Rename only: keep the existing checkboxes and their checked state.
    if (series.size() == m_seriesChecks.size()) {
        for (int i = 0; i < series.size(); ++i) {
            m_seriesChecks.at(i)->setText(series.at(i).name);
            m_seriesChecks.at(i)->setStyleSheet(
                QStringLiteral("color: %1;").arg(series.at(i).color.name()));
        }
        return;
    }

    for (QCheckBox* check : m_seriesChecks) {
        m_checksLayout->removeWidget(check);
        delete check;
    }
    m_seriesChecks.clear();

    for (int i = 0; i < series.size(); ++i) {
        auto* check = new QCheckBox(series.at(i).name, this);
        check->setChecked(true);
        check->setStyleSheet(QStringLiteral("color: %1;").arg(series.at(i).color.name()));
        connect(check, &QCheckBox::toggled, this, [this, i](bool checked) {
            m_canvas->setSeriesVisible(i, checked);
            emit settingsChanged();
        });
        m_seriesChecks.append(check);
        m_checksLayout->addWidget(check);
    }
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

void LineChartWidget::setSpeedMatchers(const QList<bool>& isPicker)
{
    for (QLabel* label : m_speedLabels) {
        m_speedLayout->removeWidget(label);
        delete label;
    }
    m_speedLabels.clear();
    m_speedIsPicker = isPicker;

    for (int i = 0; i < isPicker.size(); ++i) {
        auto* label = new QLabel(this);
        m_speedLayout->addWidget(label);
        m_speedLabels.append(label);
        setMatcherSpeed(i, 0.0);
    }
}

void LineChartWidget::setRawSpeed(double kbps)
{
    if (m_rawSpeedLabel)
        m_rawSpeedLabel->setText(tr("Raw data: %1 kb/s").arg(kbps, 0, 'f', 1));
}

void LineChartWidget::setMatcherSpeed(int index, double rate)
{
    if (index < 0 || index >= m_speedLabels.size())
        return;

    const QString unit = m_speedIsPicker.at(index) ? tr("values/s") : tr("lines/s");
    m_speedLabels.at(index)->setText(
        tr("Matcher %1: %2 %3").arg(index + 1).arg(rate, 0, 'f', 1).arg(unit));
}

bool LineChartWidget::showValues() const
{
    return m_showValuesCheck->isChecked();
}

void LineChartWidget::setShowValues(bool checked)
{
    m_showValuesCheck->setChecked(checked);
}

QList<bool> LineChartWidget::seriesVisibility() const
{
    QList<bool> states;
    for (QCheckBox* check : m_seriesChecks)
        states.append(check->isChecked());
    return states;
}

void LineChartWidget::setSeriesVisibility(const QList<bool>& states)
{
    const int count = qMin(states.size(), m_seriesChecks.size());
    for (int i = 0; i < count; ++i)
        m_seriesChecks.at(i)->setChecked(states.at(i));
}

} // namespace comm
