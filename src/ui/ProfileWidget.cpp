#include "ProfileWidget.h"

#include "common/ProfileStore.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace comm {

ProfileWidget::ProfileWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(new QLabel(tr("Profile:"), this));

    m_nameCombo = new QComboBox(this);
    m_nameCombo->setEditable(true);
    m_nameCombo->setInsertPolicy(QComboBox::NoInsert);
    m_nameCombo->setMinimumWidth(160);
    layout->addWidget(m_nameCombo, 1);

    auto* refreshButton = new QPushButton(tr("Refresh"), this);
    m_loadButton = new QPushButton(tr("Load"), this);
    m_saveButton = new QPushButton(tr("Save"), this);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    layout->addWidget(refreshButton);
    layout->addWidget(m_loadButton);
    layout->addWidget(m_saveButton);
    layout->addWidget(m_deleteButton);

    connect(refreshButton, &QPushButton::clicked, this, &ProfileWidget::refresh);
    connect(m_loadButton, &QPushButton::clicked, this, &ProfileWidget::onLoadClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &ProfileWidget::onSaveClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &ProfileWidget::onDeleteClicked);
    connect(m_nameCombo, &QComboBox::editTextChanged, this, &ProfileWidget::onNameChanged);
    connect(m_nameCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onNameChanged(); });

    refresh();
    updateButtons();
}

void ProfileWidget::refresh()
{
    m_refreshing = true;
    const QString current = m_nameCombo->currentText();
    m_nameCombo->clear();
    for (const QString& name : ProfileStore::names())
        m_nameCombo->addItem(name);
    m_nameCombo->setEditText(current);
    m_refreshing = false;
    updateButtons();
}

void ProfileWidget::markDirty()
{
    m_dirty = true;
    updateButtons();
}

void ProfileWidget::onNameChanged()
{
    if (m_refreshing)
        return;
    markDirty();
}

void ProfileWidget::onLoadClicked()
{
    const QString name = m_nameCombo->currentText();
    if (name.isEmpty())
        return;

    emit loadRequested(name);
    m_dirty = false;
    updateButtons();
}

void ProfileWidget::onSaveClicked()
{
    const QString name = m_nameCombo->currentText().trimmed();
    if (name.isEmpty())
        return;

    emit saveRequested(name);
    m_dirty = false;
    updateButtons();
}

void ProfileWidget::onDeleteClicked()
{
    const QString name = m_nameCombo->currentText().trimmed();
    if (name.isEmpty() || m_nameCombo->currentIndex() < 0)
        return;

    ProfileStore::remove(name);

    m_refreshing = true;
    m_nameCombo->clear();
    for (const QString& item : ProfileStore::names())
        m_nameCombo->addItem(item);
    m_nameCombo->setEditText(QString());
    m_refreshing = false;

    updateButtons();
}

void ProfileWidget::updateButtons()
{
    const bool hasSelection = (m_nameCombo->currentIndex() >= 0);
    const bool hasText = !m_nameCombo->currentText().trimmed().isEmpty();
    m_loadButton->setEnabled(m_dirty && hasSelection);
    m_saveButton->setEnabled(m_dirty && hasText);
    m_deleteButton->setEnabled(hasSelection);
}

} // namespace comm
