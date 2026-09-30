#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace comm {

// A titled, collapsible panel with a title bar (name + Restore + Maximize).
// When collapsed, only the title bar remains visible. Maximize/restore are
// coordinated by the parent (MainWindow) through the emitted signals.
class SectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SectionWidget(const QString& title, QWidget* content, QWidget* parent = nullptr);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

signals:
    void maximizeRequested();
    void restoreRequested();

private:
    QWidget* m_content = nullptr;
    QVBoxLayout* m_layout = nullptr;
    bool m_collapsed = false;
};

} // namespace comm
