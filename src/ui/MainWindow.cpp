#include "MainWindow.h"

#include "common/ByteCache.h"
#include "common/RecentDataStore.h"
#include "communication/ChannelManager.h"
#include "matchers/DecimalPickerMatcher.h"
#include "matchers/MatcherPipeline.h"
#include "sender/DataSender.h"
#include "ui/CommunicationSettingsWidget.h"
#include "ui/MatchersWidget.h"
#include "ui/RawDataWidget.h"
#include "ui/SendingWidget.h"

#include <QCoreApplication>
#include <QGroupBox>
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
    m_rawDataWidget = new RawDataWidget(central);
    m_matchersWidget = new MatchersWidget(central);
    m_lineChartWidget = new LineChartWidget(central);

    m_commGroup = new QGroupBox(central);
    auto* commGroupLayout = new QVBoxLayout(m_commGroup);
    commGroupLayout->addWidget(m_commWidget);

    m_sendingGroup = new QGroupBox(central);
    auto* sendingGroupLayout = new QVBoxLayout(m_sendingGroup);
    sendingGroupLayout->addWidget(m_sendingWidget);

    m_matchersGroup = new QGroupBox(central);
    auto* matchersGroupLayout = new QVBoxLayout(m_matchersGroup);
    matchersGroupLayout->addWidget(m_matchersWidget);

    m_centralLayout->addWidget(m_commGroup);
    m_centralLayout->addWidget(m_sendingGroup);
    m_centralLayout->addWidget(m_rawDataWidget);
    m_centralLayout->addWidget(m_matchersGroup);
    m_centralLayout->addWidget(m_lineChartWidget, 1);
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
    connect(m_sendingWidget, &SendingWidget::sendRequested,
            this, &MainWindow::onSendRequested);
    connect(m_sendingWidget, &SendingWidget::stopRequested, this, [this]() {
        m_dataSender->stopLoop();
        m_sendingWidget->setLooping(false);
    });
    connect(m_matchersWidget, &MatchersWidget::matchersChanged,
            this, &MainWindow::onMatchersChanged);
    connect(m_lineChartWidget, &LineChartWidget::maximizeRequested,
            this, &MainWindow::onChartMaximize);

    m_sendingWidget->setConnected(false);
    rebuildChartSeries();

    loadRecentData();

    connect(m_dataSender, &DataSender::historyChanged, this, &MainWindow::saveRecentData);
    connect(m_matchersWidget, &MatchersWidget::keywordHistoryChanged,
            this, &MainWindow::saveRecentData);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
            this, &MainWindow::saveRecentData);
}

MainWindow::~MainWindow() = default;

void MainWindow::onConnectRequested()
{
    m_byteCache->setCapacity(m_commWidget->byteCacheSize());
    m_byteCache->clear();
    m_rawDataWidget->clear();

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
    m_rawDataWidget->appendBytes(data);
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

void MainWindow::onSendRequested(const QByteArray& data, bool loop, int intervalMs)
{
    if (!m_channelManager->isOpen()) {
        QMessageBox::warning(this, tr("Not connected"),
                             tr("Please connect to a device first."));
        return;
    }

    if (loop) {
        m_dataSender->startLoop(data, intervalMs);
        m_sendingWidget->setLooping(true);
    } else {
        m_dataSender->sendOnce(data);
    }
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
    m_rawDataWidget->setMaxBytes(m_state.rawDataBytes);

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
    m_state.rawDataBytes = m_rawDataWidget->maxBytes();

    m_state.sendHistory = m_dataSender->history();
    m_state.keywordHistory = m_matchersWidget->keywordHistory();

    RecentDataStore::save(m_state);
}

void MainWindow::onChartMaximize(bool maximize)
{
    m_commGroup->setVisible(!maximize);
    m_sendingGroup->setVisible(!maximize);
    m_rawDataWidget->setVisible(!maximize);
    m_matchersGroup->setVisible(!maximize);

    if (maximize) {
        m_centralLayout->setContentsMargins(0, 0, 0, 0);
        statusBar()->hide();
    } else {
        m_centralLayout->unsetContentsMargins();
        statusBar()->show();
    }
}

void MainWindow::onChannelError(const QString& message)
{
    statusBar()->showMessage(message, 5000);
}

} // namespace comm
