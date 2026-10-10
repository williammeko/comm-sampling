#include "RawDataWidget.h"

#include "common/HexUtils.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>

namespace comm {

RawDataWidget::RawDataWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* headerRow = new QHBoxLayout;
    headerRow->addWidget(new QLabel(tr("Raw data (recent"), this));
    m_maxBytesSpin = new QSpinBox(this);
    m_maxBytesSpin->setRange(1, 1000000);
    m_maxBytesSpin->setValue(2000);
    headerRow->addWidget(m_maxBytesSpin);
    headerRow->addWidget(new QLabel(tr("bytes):"), this));
    headerRow->addStretch(1);
    layout->addLayout(headerRow);

    m_view = new QPlainTextEdit(this);
    m_view->setReadOnly(true);
    m_view->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_view->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_view->setMinimumHeight(48);
    layout->addWidget(m_view, 1);

    connect(m_maxBytesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &RawDataWidget::settingsChanged);

    m_clock.start();
}

int RawDataWidget::maxBytes() const
{
    return m_maxBytesSpin->value();
}

void RawDataWidget::setMaxBytes(int bytes)
{
    m_maxBytesSpin->setValue(bytes);
}

void RawDataWidget::appendBytes(const QByteArray& bytes)
{
    if (bytes.isEmpty())
        return;

    const qint64 now = m_clock.elapsed();
    const bool newLine = (m_lastAppendMs < 0) || (now - m_lastAppendMs >= 5);

    if (newLine || m_lines.isEmpty())
        m_lines.append(bytes);
    else
        m_lines.last().append(bytes);

    m_lastAppendMs = now;
    m_totalBytes += bytes.size();

    const int max = maxBytes();
    while (m_totalBytes > max && !m_lines.isEmpty()) {
        m_totalBytes -= m_lines.first().size();
        m_lines.removeFirst();
    }

    refreshDisplay();
}

void RawDataWidget::clear()
{
    m_lines.clear();
    m_totalBytes = 0;
    m_lastAppendMs = -1;
    refreshDisplay();
}

void RawDataWidget::refreshDisplay()
{
    QStringList lines;
    lines.reserve(m_lines.size());
    for (const QByteArray& line : m_lines)
        lines.append(HexUtils::toHexString(line));

    m_view->setPlainText(lines.join(QLatin1String("\n")));
    m_view->moveCursor(QTextCursor::End);
    m_view->verticalScrollBar()->setValue(m_view->verticalScrollBar()->maximum());
}

} // namespace comm
