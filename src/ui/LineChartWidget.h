#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QTimer>
#include <QWidget>

#include "common/Types.h"

class QSpinBox;
class QPushButton;

namespace comm {

// Real-time multi-series line chart. Each series (a decimal picker) is drawn
// with its own color and labeled by name. The Y range auto-adapts to all
// series within the duration window.
class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget* parent = nullptr);

    void setSeries(const QList<SeriesInfo>& series);
    void appendValue(int seriesIndex, double value);
    void setEnabledState(bool enabled);
    int durationMs() const;
    void clearChart();

signals:
    void maximizeRequested(bool maximize);

private slots:
    void onMaximizeClicked();

private:
    QSpinBox* m_durationSpin = nullptr;
    QPushButton* m_maximizeButton = nullptr;
    class ChartCanvas* m_canvas = nullptr;
    bool m_enabled = false;
    bool m_maximized = false;
};

} // namespace comm
