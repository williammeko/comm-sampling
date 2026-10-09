#pragma once

#include <QElapsedTimer>
#include <QList>
#include <QTimer>
#include <QWidget>

#include "common/Types.h"

class QSpinBox;
class QPushButton;
class QCheckBox;
class QHBoxLayout;

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

private:
    void rebuildSeriesChecks(const QList<SeriesInfo>& series);

    QSpinBox* m_durationSpin = nullptr;
    QCheckBox* m_showValuesCheck = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QHBoxLayout* m_checksLayout = nullptr;
    QList<QCheckBox*> m_seriesChecks;
    class ChartCanvas* m_canvas = nullptr;
    bool m_enabled = false;
    bool m_paused = false;
};

} // namespace comm
