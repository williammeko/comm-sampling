#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QVariant>

#include "common/RecentDataStore.h"
#include "common/Types.h"
#include "ui/LineChartWidget.h"

class QTimer;
class QVBoxLayout;
class QGroupBox;

namespace comm {

class CommunicationSettingsWidget;
class SendingWidget;
class RawDataWidget;
class MatchersWidget;
class ChannelManager;
class ByteCache;
class MatcherPipeline;
class DataSender;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectRequested();
    void onDisconnectRequested();
    void onChannelStateChanged(bool open);
    void onBytesReceived(const QByteArray& data);
    void onDrainCache();
    void onFrameMatched(int matcherIndex, const QByteArray& frame);
    void onPickerValueChanged(int matcherIndex, int pickerIndex, const QVariant& value, bool valid);
    void onSendRequested(const QByteArray& data, bool loop, int intervalMs);
    void onMatchersChanged();
    void onChannelError(const QString& message);
    void onChartMaximize(bool maximize);

private:
    void rebuildChartSeries();
    void loadRecentData();
    void saveRecentData();

    CommunicationSettingsWidget* m_commWidget = nullptr;
    SendingWidget* m_sendingWidget = nullptr;
    RawDataWidget* m_rawDataWidget = nullptr;
    MatchersWidget* m_matchersWidget = nullptr;
    LineChartWidget* m_lineChartWidget = nullptr;

    ChannelManager* m_channelManager = nullptr;
    ByteCache* m_byteCache = nullptr;
    MatcherPipeline* m_pipeline = nullptr;
    DataSender* m_dataSender = nullptr;

    QTimer* m_drainTimer = nullptr;

    QVBoxLayout* m_centralLayout = nullptr;
    QGroupBox* m_commGroup = nullptr;
    QGroupBox* m_sendingGroup = nullptr;
    QGroupBox* m_matchersGroup = nullptr;

    QList<SeriesInfo> m_series;
    QHash<int, int> m_pickerToSeries; // key = matcherIndex*10000 + pickerIndex

    RecentDataStore::State m_state;
};

} // namespace comm
