#include "MatchersWidget.h"

#include "matchers/DecimalPickerMatcher.h"
#include "matchers/FixedKeywordFrameMatcher.h"
#include "ui/MatcherItemWidget.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

#include <cmath>

namespace comm {

namespace {

// Deterministic "random-looking" dark color with good contrast on white.
QColor pickerColor(int globalIndex)
{
    const double hue = std::fmod(globalIndex * 137.508, 360.0);
    return QColor::fromHsv(static_cast<int>(hue), 200, 140);
}

} // namespace

MatchersWidget::MatchersWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    auto* listContainer = new QWidget(this);
    m_itemsLayout = new QVBoxLayout(listContainer);
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(listContainer, 1);

    auto* addRow = new QHBoxLayout;
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Fixed-keyword-frame matcher"), 0);
    m_typeCombo->addItem(tr("Decimal-picker matcher"), 1);
    auto* addButton = new QPushButton(tr("Add matcher"), this);
    addRow->addWidget(m_typeCombo, 1);
    addRow->addWidget(addButton);
    layout->addLayout(addRow);

    connect(addButton, &QPushButton::clicked, this, &MatchersWidget::addMatcher);
}

Matcher* MatchersWidget::createMatcherOfSelectedType()
{
    if (m_typeCombo->currentData().toInt() == 1)
        return new DecimalPickerMatcher;
    return new FixedKeywordFrameMatcher;
}

void MatchersWidget::addMatcher()
{
    Matcher* matcher = createMatcherOfSelectedType();
    auto* item = new MatcherItemWidget(matcher, m_items.size(), this);

    connect(item, &MatcherItemWidget::structureChanged,
            this, &MatchersWidget::onItemStructureChanged);
    connect(item, &MatcherItemWidget::keywordUsed, this, &MatchersWidget::onKeywordUsed);
    connect(item, &MatcherItemWidget::removeRequested, this,
            [this, item]() { removeMatcher(item); });
    connect(item, &MatcherItemWidget::moveUpRequested, this,
            [this, item]() { moveUp(item); });
    connect(item, &MatcherItemWidget::moveDownRequested, this,
            [this, item]() { moveDown(item); });

    m_items.append(item);
    m_itemsLayout->addWidget(item);
    item->refreshKeywordItems(m_keywordHistory);
    reassignPickerNames();
    emit matchersChanged();
}

void MatchersWidget::removeMatcher(MatcherItemWidget* item)
{
    if (!m_items.contains(item))
        return;

    m_items.removeAll(item);
    m_itemsLayout->removeWidget(item);
    item->deleteLater();
    reindexItems();
    reassignPickerNames();
    emit matchersChanged();
}

void MatchersWidget::moveUp(MatcherItemWidget* item)
{
    const int idx = m_items.indexOf(item);
    if (idx <= 0)
        return;

    m_items.move(idx, idx - 1);
    rebuildLayout();
    reindexItems();
    reassignPickerNames();
    emit matchersChanged();
}

void MatchersWidget::moveDown(MatcherItemWidget* item)
{
    const int idx = m_items.indexOf(item);
    if (idx < 0 || idx >= m_items.size() - 1)
        return;

    m_items.move(idx, idx + 1);
    rebuildLayout();
    reindexItems();
    reassignPickerNames();
    emit matchersChanged();
}

void MatchersWidget::onItemStructureChanged()
{
    reassignPickerNames();
    emit matchersChanged();
}

void MatchersWidget::rebuildLayout()
{
    while (m_itemsLayout->count() > 0)
        delete m_itemsLayout->takeAt(0);

    for (MatcherItemWidget* item : m_items)
        m_itemsLayout->addWidget(item);
}

void MatchersWidget::reindexItems()
{
    for (int i = 0; i < m_items.size(); ++i)
        m_items.at(i)->setMatcherIndex(i);
}

void MatchersWidget::reassignPickerNames()
{
    int index = 0;
    for (MatcherItemWidget* item : m_items) {
        auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(item->matcher());
        if (!pickerMatcher)
            continue;

        for (Picker& picker : pickerMatcher->pickers()) {
            picker.name = QStringLiteral("val%1").arg(index);
            picker.color = pickerColor(index);
            ++index;
        }
    }

    for (MatcherItemWidget* item : m_items)
        item->refreshPickerPresentation();
}

QList<Matcher*> MatchersWidget::matchers() const
{
    QList<Matcher*> result;
    result.reserve(m_items.size());
    for (MatcherItemWidget* item : m_items)
        result.append(item->matcher());
    return result;
}

QList<SeriesInfo> MatchersWidget::pickerSeries() const
{
    QList<SeriesInfo> series;
    for (MatcherItemWidget* item : m_items) {
        auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(item->matcher());
        if (!pickerMatcher)
            continue;

        for (const Picker& picker : pickerMatcher->pickers())
            series.append(SeriesInfo{picker.name, picker.color});
    }
    return series;
}

int MatchersWidget::pickerCount() const
{
    int count = 0;
    for (MatcherItemWidget* item : m_items) {
        auto* pickerMatcher = dynamic_cast<DecimalPickerMatcher*>(item->matcher());
        if (pickerMatcher)
            count += pickerMatcher->pickers().size();
    }
    return count;
}

void MatchersWidget::showFrame(int matcherIndex, const QByteArray& frame)
{
    if (matcherIndex >= 0 && matcherIndex < m_items.size())
        m_items.at(matcherIndex)->appendFrame(frame);
}

void MatchersWidget::updatePickerValue(int matcherIndex, int pickerIndex,
                                       const QVariant& value, bool valid)
{
    if (matcherIndex >= 0 && matcherIndex < m_items.size())
        m_items.at(matcherIndex)->updatePickerValue(pickerIndex, value, valid);
}

void MatchersWidget::setKeywordHistory(const QStringList& history)
{
    m_keywordHistory = history;
    refreshKeywordItemsOnAll();
}

void MatchersWidget::onKeywordUsed(const QString& text)
{
    if (text.isEmpty())
        return;

    m_keywordHistory.removeAll(text);
    m_keywordHistory.prepend(text);
    while (m_keywordHistory.size() > 50)
        m_keywordHistory.removeLast();

    refreshKeywordItemsOnAll();
    emit keywordHistoryChanged();
}

void MatchersWidget::refreshKeywordItemsOnAll()
{
    for (MatcherItemWidget* item : m_items)
        item->refreshKeywordItems(m_keywordHistory);
}

} // namespace comm
