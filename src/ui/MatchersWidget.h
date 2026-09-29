#pragma once

#include <QList>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include "common/Types.h"
#include "matchers/Matcher.h"

class QComboBox;
class QVBoxLayout;

namespace comm {

class MatcherItemWidget;

class MatchersWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MatchersWidget(QWidget* parent = nullptr);

    QList<Matcher*> matchers() const;
    QList<SeriesInfo> pickerSeries() const;
    int pickerCount() const;

    void showFrame(int matcherIndex, const QByteArray& frame);
    void updatePickerValue(int matcherIndex, int pickerIndex, const QVariant& value, bool valid);

    void setKeywordHistory(const QStringList& history);
    QStringList keywordHistory() const { return m_keywordHistory; }

signals:
    void matchersChanged();
    void keywordHistoryChanged();

private slots:
    void addMatcher();
    void removeMatcher(MatcherItemWidget* item);
    void moveUp(MatcherItemWidget* item);
    void moveDown(MatcherItemWidget* item);
    void onItemStructureChanged();
    void onKeywordUsed(const QString& text);

private:
    Matcher* createMatcherOfSelectedType();
    void rebuildLayout();
    void reindexItems();
    void reassignPickerNames();
    void refreshKeywordItemsOnAll();

    QComboBox* m_typeCombo = nullptr;
    QVBoxLayout* m_itemsLayout = nullptr;
    QList<MatcherItemWidget*> m_items;
    QStringList m_keywordHistory;
};

} // namespace comm
