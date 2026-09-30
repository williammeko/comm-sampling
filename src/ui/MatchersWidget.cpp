#include "MatchersWidget.h"

#include "common/HexUtils.h"
#include "matchers/DecimalPickerMatcher.h"
#include "matchers/FixedKeywordFrameMatcher.h"
#include "ui/MatcherItemWidget.h"

#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
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

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setMinimumHeight(240);
    scrollArea->setMaximumHeight(900);

    auto* listContainer = new QWidget;
    m_itemsLayout = new QVBoxLayout(listContainer);
    m_itemsLayout->setContentsMargins(0, 0, 0, 0);
    m_itemsLayout->setSpacing(4);
    scrollArea->setWidget(listContainer);

    layout->addWidget(scrollArea, 1);

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
    addMatcherItem(createMatcherOfSelectedType());
    reassignPickerNames();
    emit matchersChanged();
    emit anythingChanged();
}

void MatchersWidget::addMatcherItem(Matcher* matcher)
{
    auto* item = new MatcherItemWidget(matcher, m_items.size(), this);

    connect(item, &MatcherItemWidget::structureChanged,
            this, &MatchersWidget::onItemStructureChanged);
    connect(item, &MatcherItemWidget::keywordUsed, this, &MatchersWidget::onKeywordUsed);
    connect(item, &MatcherItemWidget::pickerNameChanged, this, [this]() {
        emit matchersChanged();
        emit anythingChanged();
    });
    connect(item, &MatcherItemWidget::configChanged, this, [this]() {
        emit anythingChanged();
    });
    connect(item, &MatcherItemWidget::removeRequested, this,
            [this, item]() { removeMatcher(item); });
    connect(item, &MatcherItemWidget::moveUpRequested, this,
            [this, item]() { moveUp(item); });
    connect(item, &MatcherItemWidget::moveDownRequested, this,
            [this, item]() { moveDown(item); });

    m_items.append(item);
    m_itemsLayout->addWidget(item);
    item->refreshKeywordItems(m_keywordHistory);
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
    emit anythingChanged();
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
    emit anythingChanged();
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
    emit anythingChanged();
}

void MatchersWidget::onItemStructureChanged()
{
    reassignPickerNames();
    emit matchersChanged();
    emit anythingChanged();
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
            if (!picker.customName)
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
    emit anythingChanged();
}

void MatchersWidget::refreshKeywordItemsOnAll()
{
    for (MatcherItemWidget* item : m_items)
        item->refreshKeywordItems(m_keywordHistory);
}

QJsonArray MatchersWidget::matchersToJson() const
{
    QJsonArray array;
    for (MatcherItemWidget* item : m_items) {
        QJsonObject obj;
        if (auto* kw = dynamic_cast<FixedKeywordFrameMatcher*>(item->matcher())) {
            obj.insert(QStringLiteral("type"), QStringLiteral("keyword"));
            obj.insert(QStringLiteral("keyword"), HexUtils::toHexString(kw->keyword()));
            obj.insert(QStringLiteral("length"), kw->frameLength());
        } else if (auto* dp = dynamic_cast<DecimalPickerMatcher*>(item->matcher())) {
            obj.insert(QStringLiteral("type"), QStringLiteral("picker"));
            QJsonArray pickers;
            for (const Picker& picker : dp->pickers()) {
                QJsonObject po;
                po.insert(QStringLiteral("unit"), static_cast<int>(picker.unit));
                po.insert(QStringLiteral("start"), picker.start);
                po.insert(QStringLiteral("length"), picker.length);
                po.insert(QStringLiteral("numberType"), static_cast<int>(picker.numberType));
                po.insert(QStringLiteral("byteOrder"), static_cast<int>(picker.byteOrder));
                po.insert(QStringLiteral("name"), picker.name);
                po.insert(QStringLiteral("customName"), picker.customName);

                QJsonArray adjustments;
                for (const BitAdjustment& adj : picker.adjustments) {
                    QJsonObject ao;
                    ao.insert(QStringLiteral("srcByte"), adj.srcByte);
                    ao.insert(QStringLiteral("srcStartBit"), adj.srcStartBit);
                    ao.insert(QStringLiteral("srcEndBit"), adj.srcEndBit);
                    ao.insert(QStringLiteral("dstByte"), adj.dstByte);
                    ao.insert(QStringLiteral("dstStartBit"), adj.dstStartBit);
                    ao.insert(QStringLiteral("dstEndBit"), adj.dstEndBit);
                    adjustments.append(ao);
                }
                po.insert(QStringLiteral("adjustments"), adjustments);
                pickers.append(po);
            }
            obj.insert(QStringLiteral("pickers"), pickers);
        }
        array.append(obj);
    }
    return array;
}

void MatchersWidget::applyMatchersJson(const QJsonArray& array)
{
    for (MatcherItemWidget* item : m_items) {
        m_itemsLayout->removeWidget(item);
        item->deleteLater();
    }
    m_items.clear();

    for (const auto& value : array) {
        const QJsonObject obj = value.toObject();
        const QString type = obj.value(QStringLiteral("type")).toString();

        if (type == QLatin1String("keyword")) {
            auto* kw = new FixedKeywordFrameMatcher;
            QByteArray keyword;
            HexUtils::parseHexBytes(obj.value(QStringLiteral("keyword")).toString(), keyword);
            kw->setKeyword(keyword);
            kw->setFrameLength(obj.value(QStringLiteral("length")).toInt(8));
            addMatcherItem(kw);
        } else if (type == QLatin1String("picker")) {
            auto* dp = new DecimalPickerMatcher;
            const QJsonArray pickers = obj.value(QStringLiteral("pickers")).toArray();
            for (const auto& pv : pickers) {
                const QJsonObject po = pv.toObject();
                Picker picker;
                picker.unit = static_cast<Picker::Unit>(
                    po.value(QStringLiteral("unit")).toInt(static_cast<int>(Picker::Unit::Bytes)));
                picker.start = po.value(QStringLiteral("start")).toInt(0);
                picker.length = po.value(QStringLiteral("length")).toInt(2);
                picker.numberType = static_cast<NumberType>(
                    po.value(QStringLiteral("numberType")).toInt(static_cast<int>(NumberType::UInt)));
                picker.byteOrder = static_cast<ByteOrder>(
                    po.value(QStringLiteral("byteOrder")).toInt(static_cast<int>(ByteOrder::BigEndian)));
                picker.name = po.value(QStringLiteral("name")).toString();
                picker.customName = po.value(QStringLiteral("customName")).toBool(false);

                const QJsonArray adjustments = po.value(QStringLiteral("adjustments")).toArray();
                for (const auto& av : adjustments) {
                    const QJsonObject ao = av.toObject();
                    BitAdjustment adj;
                    adj.srcByte = ao.value(QStringLiteral("srcByte")).toInt(0);
                    adj.srcStartBit = ao.value(QStringLiteral("srcStartBit")).toInt(0);
                    adj.srcEndBit = ao.value(QStringLiteral("srcEndBit")).toInt(7);
                    adj.dstByte = ao.value(QStringLiteral("dstByte")).toInt(0);
                    adj.dstStartBit = ao.value(QStringLiteral("dstStartBit")).toInt(0);
                    adj.dstEndBit = ao.value(QStringLiteral("dstEndBit")).toInt(7);
                    picker.adjustments.append(adj);
                }
                dp->addPicker(picker);
            }
            addMatcherItem(dp);
        }
    }

    reassignPickerNames();
    emit matchersChanged();
    emit anythingChanged();
}

} // namespace comm
