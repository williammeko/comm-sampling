#include "MainWindow.h"

#include "common/ByteCache.h"
#include "common/ProfileStore.h"
#include "common/RecentDataStore.h"
#include "communication/ChannelManager.h"
#include "matchers/DecimalPickerMatcher.h"
#include "matchers/MatcherPipeline.h"
#include "sender/DataSender.h"
#include "sender/SendStrategy.h"
#include "ui/CommunicationSettingsWidget.h"
#include "ui/MatchersWidget.h"
#include "ui/ProfileWidget.h"
#include "ui/SectionWidget.h"
#include "ui/SendingWidget.h"

#include <QCoreApplication>
#include <QGroupBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace comm {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Comm Sampling"));

    m_channelManager = new ChannelManager(this);
    m_byteCache = new ByteCache(10000);
    m_pipeline = new MatcherPipeline(this);
    m_dataSender = new DataSender(this);

    auto* central = new QWidget(this);
    m_centralLayout = new QVBoxLayout(central);

    m_commWidget = new CommunicationSettingsWidget(central);
    m_sendingWidget = new SendingWidget(m_dataSender, central);
    m_matchersWidget = new MatchersWidget(central);
    m_profileWidget = new ProfileWidget(central);
    m_lineChartWidget = new LineChartWidget(central);

    m_commGroup = new QGroupBox(central);
    auto* commGroupLayout = new QVBoxLayout(m_commGroup);
    commGroupLayout->addWidget(m_commWidget);

    m_dataInteractionSection = new SectionWidget(tr("data interaction"), m_sendingWidget, central);
    m_matcherSettingSection = new SectionWidget(tr("matcher setting"), m_matchersWidget, central);
    m_lineChartSection = new SectionWidget(tr("line chart"), m_lineChartWidget, central);

    m_centralLayout->addWidget(m_profileWidget);
    m_centralLayout->addWidget(m_commGroup);
    m_centralLayout->addWidget(m_dataInteractionSection);
    m_centralLayout->addWidget(m_matcherSettingSection);
    m_centralLayout->addWidget(m_lineChartSection, 1);
    setCentralWidget(central);

    // Sending goes through the active channel.
    m_dataSender->setWriteFunc([this](const QByteArray& data) {
        return m_channelManager->write(data);
    });

    // Drain the byte cache into the matcher pipeline on a fast timer.
    m_drainTimer = new QTimer(this);
    m_drainTimer->setInterval(5);
    connect(m_drainTimer, &QTimer::timeout, this, &MainWindow::onDrainCache);
    m_drainTimer->start();

    connect(m_commWidget, &CommunicationSettingsWidget::connectRequested,
            this, &MainWindow::onConnectRequested);
    connect(m_commWidget, &CommunicationSettingsWidget::disconnectRequested,
            this, &MainWindow::onDisconnectRequested);
    connect(m_channelManager, &ChannelManager::bytesReceived,
            this, &MainWindow::onBytesReceived);
    connect(m_channelManager, &ChannelManager::stateChanged,
            this, &MainWindow::onChannelStateChanged);
    connect(m_channelManager, &ChannelManager::errorOccurred,
            this, &MainWindow::onChannelError);
    connect(m_pipeline, &MatcherPipeline::frameMatched,
            this, &MainWindow::onFrameMatched);
    connect(m_pipeline, &MatcherPipeline::pickerValueChanged,
            this, &MainWindow::onPickerValueChanged);
    connect(m_matchersWidget, &MatchersWidget::matchersChanged,
            this, &MainWindow::onMatchersChanged);

    connect(m_dataInteractionSection, &SectionWidget::maximizeRequested,
            this, [this]() { maximizeSection(m_dataInteractionSection); });
    connect(m_matcherSettingSection, &SectionWidget::maximizeRequested,
            this, [this]() { maximizeSection(m_matcherSettingSection); });
    connect(m_lineChartSection, &SectionWidget::maximizeRequested,
            this, [this]() { maximizeSection(m_lineChartSection); });

    connect(m_dataInteractionSection, &SectionWidget::restoreRequested,
            this, &MainWindow::restoreSections);
    connect(m_matcherSettingSection, &SectionWidget::restoreRequested,
            this, &MainWindow::restoreSections);
    connect(m_lineChartSection, &SectionWidget::restoreRequested,
            this, &MainWindow::restoreSections);

    m_sendingWidget->setConnected(false);
    rebuildChartSeries();

    loadRecentData();

    // Profile + autosave wiring. Connected after the initial load so that
    // applying the recent state does not mark the profile as dirty.
    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setSingleShot(true);
    m_autosaveTimer->setInterval(800);
    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::autosaveRecent);

    connect(m_profileWidget, &ProfileWidget::loadRequested,
            this, &MainWindow::onProfileLoad);
    connect(m_profileWidget, &ProfileWidget::saveRequested,
            this, &MainWindow::onProfileSave);

    connect(m_commWidget, &CommunicationSettingsWidget::settingsChanged,
            this, &MainWindow::onAnyChange);
    connect(m_matchersWidget, &MatchersWidget::anythingChanged,
            this, &MainWindow::onAnyChange);
    connect(m_dataSender, &DataSender::historyChanged,
            this, &MainWindow::onAnyChange);
    connect(m_sendingWidget, &SendingWidget::settingsChanged,
            this, &MainWindow::onAnyChange);

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
            this, &MainWindow::saveRecentData);
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnectRequested()
{
    m_byteCache->setCapacity(m_commWidget->byteCacheSize());
    m_byteCache->clear();
    m_sendingWidget->clearReceivedData();
    m_sendingWidget->clearSendPreview();

    switch (m_commWidget->mode()) {
    case ChannelMode::Serial:
        m_channelManager->startSerial(m_commWidget->serialConfig());
        break;
    case ChannelMode::TcpClient:
        m_channelManager->startTcpClient(m_commWidget->tcpClientConfig());
        break;
    case ChannelMode::TcpServer:
        m_channelManager->startTcpServer(m_commWidget->tcpServerConfig());
        break;
    }

    saveRecentData();
}

void MainWindow::onDisconnectRequested()
{
    m_channelManager->stop();
    m_dataSender->stopLoop();
    m_sendingWidget->setLooping(false);
    saveRecentData();
}

void MainWindow::onChannelStateChanged(bool open)
{
    m_commWidget->setConnected(open);
    m_sendingWidget->setConnected(open);

    if (!open) {
        m_dataSender->stopLoop();
        m_sendingWidget->setLooping(false);
    }
}

void MainWindow::onBytesReceived(const QByteArray& data)
{
    m_byteCache->append(data);
    m_sendingWidget->appendReceivedBytes(data);
}

void MainWindow::onDrainCache()
{
    if (m_byteCache->size() == 0)
        return;
    m_pipeline->feed(m_byteCache->take());
}

void MainWindow::onFrameMatched(int matcherIndex, const QByteArray& frame)
{
    m_matchersWidget->showFrame(matcherIndex, frame);
}

void MainWindow::onPickerValueChanged(int matcherIndex, int pickerIndex,
                                      const QVariant& value, bool valid)
{
    m_matchersWidget->updatePickerValue(matcherIndex, pickerIndex, value, valid);

    const int key = matcherIndex * 10000 + pickerIndex;
    const auto it = m_pickerToSeries.constFind(key);
    if (it != m_pickerToSeries.constEnd() && valid)
        m_lineChartWidget->appendValue(it.value(), value.toDouble());
}

void MainWindow::onMatchersChanged()
{
    m_pipeline->setMatchers(m_matchersWidget->matchers());
    rebuildChartSeries();
}

void MainWindow::rebuildChartSeries()
{
    m_series = m_matchersWidget->pickerSeries();

    m_pickerToSeries.clear();
    int seriesIndex = 0;
    const QList<Matcher*> matchers = m_matchersWidget->matchers();
    for (int mi = 0; mi < matchers.size(); ++mi) {
        auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(matchers.at(mi));
        if (!pickerMatcher)
            continue;

        const QList<Picker>& pickers = pickerMatcher->pickers();
        for (int pi = 0; pi < pickers.size(); ++pi) {
            m_pickerToSeries.insert(mi * 10000 + pi, seriesIndex);
            ++seriesIndex;
        }
    }

    m_lineChartWidget->setSeries(m_series);
    m_lineChartWidget->setEnabledState(!m_series.isEmpty());
}

void MainWindow::loadRecentData()
{
    if (!RecentDataStore::load(m_state))
        return;

    m_commWidget->setMode(m_state.mode);

    SerialConfig serial;
    serial.portName = m_state.serialPortName;
    serial.baudRate = m_state.baudRate;
    serial.dataBits = static_cast<DataBits>(m_state.dataBits);
    serial.parity = static_cast<Parity>(m_state.parity);
    serial.stopBits = static_cast<StopBits>(m_state.stopBits);
    m_commWidget->setSerialConfig(serial);

    TcpClientConfig client;
    client.host = m_state.clientHost;
    client.port = static_cast<quint16>(m_state.clientPort);
    m_commWidget->setTcpClientConfig(client);

    TcpServerConfig server;
    server.listenAddress = QHostAddress(m_state.serverInterface);
    server.port = static_cast<quint16>(m_state.serverPort);
    m_commWidget->setTcpServerConfig(server);

    m_commWidget->setByteCacheSize(m_state.byteCacheSize);
    m_sendingWidget->setRawDataMaxBytes(m_state.rawDataBytes);
    m_sendingWidget->setIntervalMs(m_state.loopIntervalMs);
    m_sendingWidget->setStrategyType(static_cast<SendStrategyType>(m_state.sendStrategy));
    m_sendingWidget->setCrcAlgorithm(static_cast<CrcAlgorithm>(m_state.sendCrcAlgorithm));
    m_sendingWidget->setFieldSpec(m_state.fieldSpec);
    m_sendingWidget->setFieldSpecHistory(m_state.fieldSpecHistory);

    m_dataSender->setHistory(m_state.sendHistory);
    m_matchersWidget->setKeywordHistory(m_state.keywordHistory);
}

void MainWindow::saveRecentData()
{
    const SerialConfig serial = m_commWidget->serialConfig();
    m_state.serialPortName = serial.portName;
    m_state.baudRate = serial.baudRate;
    m_state.dataBits = static_cast<int>(serial.dataBits);
    m_state.parity = static_cast<int>(serial.parity);
    m_state.stopBits = static_cast<int>(serial.stopBits);

    const TcpClientConfig client = m_commWidget->tcpClientConfig();
    const TcpServerConfig server = m_commWidget->tcpServerConfig();

    m_state.mode = m_commWidget->mode();
    m_state.clientHost = client.host;
    m_state.clientPort = client.port;
    m_state.serverInterface = server.listenAddress.toString();
    m_state.serverPort = server.port;
    m_state.byteCacheSize = m_commWidget->byteCacheSize();
    m_state.rawDataBytes = m_sendingWidget->rawDataMaxBytes();
    m_state.loopIntervalMs = m_sendingWidget->intervalMs();
    m_state.sendStrategy = static_cast<int>(m_sendingWidget->strategyType());
    m_state.sendCrcAlgorithm = static_cast<int>(m_sendingWidget->crcAlgorithm());
    m_state.fieldSpec = m_sendingWidget->fieldSpec();
    m_state.fieldSpecHistory = m_sendingWidget->fieldSpecHistory();

    m_state.sendHistory = m_dataSender->history();
    m_state.keywordHistory = m_matchersWidget->keywordHistory();

    RecentDataStore::save(m_state);
}

void MainWindow::maximizeSection(SectionWidget* section)
{
    const QList<SectionWidget*> sections = {
        m_dataInteractionSection, m_matcherSettingSection, m_lineChartSection
    };

    // Non-section widgets have no title bar to fold into, so hide them.
    m_profileWidget->setVisible(false);
    m_commGroup->setVisible(false);

    for (SectionWidget* s : sections) {
        const bool current = (s == section);
        s->setCollapsed(!current);
        m_centralLayout->setStretchFactor(s, current ? 1 : 0);
    }
}

void MainWindow::restoreSections()
{
    m_profileWidget->setVisible(true);
    m_commGroup->setVisible(true);

    m_dataInteractionSection->setCollapsed(false);
    m_matcherSettingSection->setCollapsed(false);
    m_lineChartSection->setCollapsed(false);

    m_centralLayout->setStretchFactor(m_dataInteractionSection, 0);
    m_centralLayout->setStretchFactor(m_matcherSettingSection, 0);
    m_centralLayout->setStretchFactor(m_lineChartSection, 1);
}

void MainWindow::onChannelError(const QString& message)
{
    statusBar()->showMessage(message, 5000);
}

void MainWindow::onAnyChange()
{
    if (m_loadingState)
        return;

    m_profileWidget->markDirty();
    m_autosaveTimer->start();
}

void MainWindow::autosaveRecent()
{
    saveRecentData();
}

void MainWindow::onProfileLoad(const QString& name)
{
    QJsonObject state;
    if (!ProfileStore::load(name, state)) {
        QMessageBox::warning(this, tr("Load profile"),
                             tr("Could not load profile \"%1\".").arg(name));
        return;
    }

    applyState(state);
}

void MainWindow::onProfileSave(const QString& name)
{
    if (!ProfileStore::save(name, buildState())) {
        QMessageBox::warning(this, tr("Save profile"),
                             tr("Could not save profile \"%1\".").arg(name));
        return;
    }

    m_profileWidget->refresh();
}

QJsonObject MainWindow::buildState() const
{
    QJsonObject comm;
    comm.insert(QStringLiteral("mode"), static_cast<int>(m_commWidget->mode()));

    const SerialConfig serial = m_commWidget->serialConfig();
    comm.insert(QStringLiteral("serialPort"), serial.portName);
    comm.insert(QStringLiteral("baud"), serial.baudRate);
    comm.insert(QStringLiteral("dataBits"), static_cast<int>(serial.dataBits));
    comm.insert(QStringLiteral("parity"), static_cast<int>(serial.parity));
    comm.insert(QStringLiteral("stopBits"), static_cast<int>(serial.stopBits));

    const TcpClientConfig client = m_commWidget->tcpClientConfig();
    const TcpServerConfig server = m_commWidget->tcpServerConfig();
    comm.insert(QStringLiteral("clientHost"), client.host);
    comm.insert(QStringLiteral("clientPort"), client.port);
    comm.insert(QStringLiteral("serverInterface"), server.listenAddress.toString());
    comm.insert(QStringLiteral("serverPort"), server.port);
    comm.insert(QStringLiteral("byteCacheSize"), m_commWidget->byteCacheSize());
    comm.insert(QStringLiteral("rawDataBytes"), m_sendingWidget->rawDataMaxBytes());
    comm.insert(QStringLiteral("loopIntervalMs"), m_sendingWidget->intervalMs());

    QJsonArray sendArr;
    for (const QString& text : m_dataSender->history())
        sendArr.append(text);

    QJsonArray keywordArr;
    for (const QString& text : m_matchersWidget->keywordHistory())
        keywordArr.append(text);

    QJsonArray fieldSpecArr;
    for (const QString& text : m_sendingWidget->fieldSpecHistory())
        fieldSpecArr.append(text);

    QJsonObject root;
    root.insert(QStringLiteral("comm"), comm);
    root.insert(QStringLiteral("matchers"), m_matchersWidget->matchersToJson());
    root.insert(QStringLiteral("sendStrategy"), static_cast<int>(m_sendingWidget->strategyType()));
    root.insert(QStringLiteral("sendCrcAlgorithm"), static_cast<int>(m_sendingWidget->crcAlgorithm()));
    root.insert(QStringLiteral("fieldSpec"), m_sendingWidget->fieldSpec());
    root.insert(QStringLiteral("fieldSpecHistory"), fieldSpecArr);
    root.insert(QStringLiteral("sendHistory"), sendArr);
    root.insert(QStringLiteral("keywordHistory"), keywordArr);
    return root;
}

void MainWindow::applyState(const QJsonObject& root)
{
    m_loadingState = true;

    const QJsonObject comm = root.value(QStringLiteral("comm")).toObject();

    m_commWidget->setMode(static_cast<ChannelMode>(
        comm.value(QStringLiteral("mode")).toInt(static_cast<int>(ChannelMode::Serial))));

    SerialConfig serial;
    serial.portName = comm.value(QStringLiteral("serialPort")).toString();
    serial.baudRate = comm.value(QStringLiteral("baud")).toInt(115200);
    serial.dataBits = static_cast<DataBits>(comm.value(QStringLiteral("dataBits")).toInt(8));
    serial.parity = static_cast<Parity>(comm.value(QStringLiteral("parity")).toInt(0));
    serial.stopBits = static_cast<StopBits>(comm.value(QStringLiteral("stopBits")).toInt(0));
    m_commWidget->setSerialConfig(serial);

    TcpClientConfig client;
    client.host = comm.value(QStringLiteral("clientHost")).toString();
    client.port = static_cast<quint16>(comm.value(QStringLiteral("clientPort")).toInt(8080));
    m_commWidget->setTcpClientConfig(client);

    TcpServerConfig server;
    server.listenAddress = QHostAddress(
        comm.value(QStringLiteral("serverInterface")).toString(QStringLiteral("0.0.0.0")));
    server.port = static_cast<quint16>(comm.value(QStringLiteral("serverPort")).toInt(8080));
    m_commWidget->setTcpServerConfig(server);

    m_commWidget->setByteCacheSize(comm.value(QStringLiteral("byteCacheSize")).toInt(10000));
    m_sendingWidget->setRawDataMaxBytes(comm.value(QStringLiteral("rawDataBytes")).toInt(2000));
    m_sendingWidget->setIntervalMs(comm.value(QStringLiteral("loopIntervalMs")).toInt(1000));

    QStringList sendHistory;
    const QJsonArray sendArr = root.value(QStringLiteral("sendHistory")).toArray();
    for (const auto& value : sendArr)
        sendHistory.append(value.toString());
    m_dataSender->setHistory(sendHistory);

    QStringList keywordHistory;
    const QJsonArray keywordArr = root.value(QStringLiteral("keywordHistory")).toArray();
    for (const auto& value : keywordArr)
        keywordHistory.append(value.toString());
    m_matchersWidget->setKeywordHistory(keywordHistory);

    m_sendingWidget->setStrategyType(static_cast<SendStrategyType>(
        root.value(QStringLiteral("sendStrategy")).toInt(0)));
    m_sendingWidget->setCrcAlgorithm(static_cast<CrcAlgorithm>(
        root.value(QStringLiteral("sendCrcAlgorithm")).toInt(0)));
    m_sendingWidget->setFieldSpec(root.value(QStringLiteral("fieldSpec")).toString());

    QStringList fieldSpecHistory;
    const QJsonArray fieldSpecArr = root.value(QStringLiteral("fieldSpecHistory")).toArray();
    for (const auto& value : fieldSpecArr)
        fieldSpecHistory.append(value.toString());
    m_sendingWidget->setFieldSpecHistory(fieldSpecHistory);

    m_matchersWidget->applyMatchersJson(root.value(QStringLiteral("matchers")).toArray());

    m_loadingState = false;
}

} // namespace comm
