#pragma once

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
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
class MatchersWidget;
class ProfileWidget;
class SectionWidget;
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
    void onMatchersChanged();
    void onChannelError(const QString& message);
    void onAnyChange();
    void onProfileLoad(const QString& name);
    void onProfileSave(const QString& name);
    void autosaveRecent();

private:
    void rebuildChartSeries();
    void loadRecentData();
    void saveRecentData();
    QJsonObject buildState() const;
    void applyState(const QJsonObject& state);
    void maximizeSection(SectionWidget* section);
    void restoreSections();

    CommunicationSettingsWidget* m_commWidget = nullptr;
    SendingWidget* m_sendingWidget = nullptr;
    MatchersWidget* m_matchersWidget = nullptr;
    ProfileWidget* m_profileWidget = nullptr;
    LineChartWidget* m_lineChartWidget = nullptr;

    ChannelManager* m_channelManager = nullptr;
    ByteCache* m_byteCache = nullptr;
    MatcherPipeline* m_pipeline = nullptr;
    DataSender* m_dataSender = nullptr;

    QTimer* m_drainTimer = nullptr;
    QTimer* m_autosaveTimer = nullptr;

    QVBoxLayout* m_centralLayout = nullptr;
    QGroupBox* m_commGroup = nullptr;
    SectionWidget* m_dataInteractionSection = nullptr;
    SectionWidget* m_matcherSettingSection = nullptr;
    SectionWidget* m_lineChartSection = nullptr;

    QList<SeriesInfo> m_series;
    QHash<int, int> m_pickerToSeries; // key = matcherIndex*10000 + pickerIndex

    RecentDataStore::State m_state;
    bool m_loadingState = false;
};

} // namespace comm
