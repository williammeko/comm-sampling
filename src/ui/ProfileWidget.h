#pragma once

#include <QWidget>

class QComboBox;
class QPushButton;

namespace comm {

// Profile selection component: an editable name dropdown plus Refresh/Load/Save.
class ProfileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProfileWidget(QWidget* parent = nullptr);

    void refresh();
    void markDirty();

signals:
    void loadRequested(const QString& name);
    void saveRequested(const QString& name);

private slots:
    void onNameChanged();
    void onLoadClicked();
    void onSaveClicked();
    void onDeleteClicked();

private:
    void updateButtons();

    QComboBox* m_nameCombo = nullptr;
    QPushButton* m_loadButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    bool m_dirty = true;
    bool m_refreshing = false;
};

} // namespace comm
