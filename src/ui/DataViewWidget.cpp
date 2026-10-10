#include "DataViewWidget.h"

#include "common/HexUtils.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSpinBox>
#include <QVBoxLayout>

namespace comm {

DataViewWidget::DataViewWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* sendHeader = new QHBoxLayout;
    sendHeader->addWidget(m_sendCheck = new QCheckBox(tr("Sending preview:"), this));
    m_sendCheck->setChecked(true);
    sendHeader->addStretch(1);
    sendHeader->addWidget(new QLabel(tr("Keep:"), this));
    m_sendKeepSpin = new QSpinBox(this);
    m_sendKeepSpin->setRange(1, 10000);
    m_sendKeepSpin->setValue(100);
    sendHeader->addWidget(m_sendKeepSpin);
    layout->addLayout(sendHeader);

    m_sendPreview = new QPlainTextEdit(this);
    m_sendPreview->setReadOnly(true);
    m_sendPreview->setFixedHeight(100);
    layout->addWidget(m_sendPreview);

    connect(m_sendCheck, &QCheckBox::toggled, m_sendPreview, &QPlainTextEdit::setVisible);

    m_framesLayout = new QVBoxLayout;
    m_framesLayout->setContentsMargins(0, 0, 0, 0);
    m_framesLayout->setSpacing(4);
    layout->addLayout(m_framesLayout);
}

void DataViewWidget::setMatchers(const QStringList& titles)
{
    while (m_framesLayout->count() > 0) {
        QLayoutItem* item = m_framesLayout->takeAt(0);
        if (QWidget* widget = item->widget())
            delete widget;
        delete item;
    }
    m_frameChecks.clear();
    m_frameKeepSpins.clear();
    m_frameViews.clear();
    m_frameLines.clear();

    for (const QString& title : titles) {
        auto* container = new QWidget(this);
        auto* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(2);

        auto* headerRow = new QHBoxLayout;
        auto* check = new QCheckBox(title, container);
        check->setChecked(true);
        headerRow->addWidget(check);
        headerRow->addStretch(1);
        headerRow->addWidget(new QLabel(tr("Keep:"), container));
        auto* keepSpin = new QSpinBox(container);
        keepSpin->setRange(1, 10000);
        keepSpin->setValue(100);
        headerRow->addWidget(keepSpin);
        containerLayout->addLayout(headerRow);

        auto* view = new QPlainTextEdit(container);
        view->setReadOnly(true);
        view->setFixedHeight(100);
        containerLayout->addWidget(view);

        connect(check, &QCheckBox::toggled, view, &QPlainTextEdit::setVisible);

        m_framesLayout->addWidget(container);

        m_frameChecks.append(check);
        m_frameKeepSpins.append(keepSpin);
        m_frameViews.append(view);
        m_frameLines.append(QStringList());
    }
}

void DataViewWidget::appendSent(const QByteArray& data)
{
    m_sendLines.append(HexUtils::toHexString(data));
    const int keep = m_sendKeepSpin->value();
    while (m_sendLines.size() > keep)
        m_sendLines.removeFirst();
    m_sendPreview->setPlainText(m_sendLines.join(QLatin1String("\n")));
    m_sendPreview->verticalScrollBar()->setValue(m_sendPreview->verticalScrollBar()->maximum());
}

void DataViewWidget::showFrame(int matcherIndex, const QByteArray& frame)
{
    if (matcherIndex < 0 || matcherIndex >= m_frameViews.size())
        return;

    QStringList& lines = m_frameLines[matcherIndex];
    lines.append(HexUtils::toHexString(frame));
    const int keep = m_frameKeepSpins.at(matcherIndex)->value();
    while (lines.size() > keep)
        lines.removeFirst();

    m_frameViews.at(matcherIndex)->setPlainText(lines.join(QLatin1String("\n")));
    m_frameViews.at(matcherIndex)->verticalScrollBar()->setValue(
        m_frameViews.at(matcherIndex)->verticalScrollBar()->maximum());
}

void DataViewWidget::clear()
{
    m_sendLines.clear();
    m_sendPreview->clear();
    for (QPlainTextEdit* view : m_frameViews)
        view->clear();
    for (QStringList& lines : m_frameLines)
        lines.clear();
}

} // namespace comm
