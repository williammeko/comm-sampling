#include "SectionWidget.h"

#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace comm {

SectionWidget::SectionWidget(const QString& title, QWidget* content, QWidget* parent)
    : QWidget(parent)
    , m_content(content)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);

    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);

    auto* titleLabel = new QLabel(title, this);
    QFont font = titleLabel->font();
    font.setBold(true);
    titleLabel->setFont(font);
    titleRow->addWidget(titleLabel);
    titleRow->addStretch(1);

    auto* restoreButton = new QPushButton(tr("Restore"), this);
    auto* maximizeButton = new QPushButton(tr("Maximize"), this);
    titleRow->addWidget(restoreButton);
    titleRow->addWidget(maximizeButton);

    m_layout->addLayout(titleRow);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_layout->addWidget(separator);

    m_layout->addWidget(m_content);

    connect(maximizeButton, &QPushButton::clicked, this, &SectionWidget::maximizeRequested);
    connect(restoreButton, &QPushButton::clicked, this, &SectionWidget::restoreRequested);
}

void SectionWidget::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
        return;

    m_collapsed = collapsed;
    m_content->setVisible(!collapsed);
}

} // namespace comm
