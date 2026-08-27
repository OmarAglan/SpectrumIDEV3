#pragma once

#include <QList>
#include <QPoint>
#include <QTabWidget>
#include <QWidget>

class QalamDocumentModel;
class QalamEditor;
class QSplitter;

class QalamEditorTabWidget final : public QTabWidget
{
    Q_OBJECT

public:
    explicit QalamEditorTabWidget(QWidget *parent = nullptr);

signals:
    void viewDropped(QWidget *view);
};

class QalamEditorWorkspace final : public QWidget
{
    Q_OBJECT

public:
    explicit QalamEditorWorkspace(QWidget *parent = nullptr);

    int count() const;
    QWidget *widget(int index) const;
    int indexOf(QWidget *widget) const;
    int currentIndex() const;
    QWidget *currentWidget() const;
    QalamEditor *currentEditor() const;
    QString tabText(int index) const;
    QString tabToolTip(int index) const;
    void setTabText(int index, const QString &text);
    void setTabToolTip(int index, const QString &toolTip);
    int addTab(QWidget *widget, const QString &label);
    int addTabToGroup(QWidget *widget, const QString &label, int groupIndex);
    void removeTab(int index);
    void setCurrentIndex(int index);
    void setCurrentWidget(QWidget *widget);

    void setDocumentMode(bool enabled);
    void setTabsClosable(bool enabled);
    void setMovable(bool enabled);
    QTabBar *tabBar() const;

    int groupCount() const { return m_groups.size(); }
    int activeGroupIndex() const;
    void setActiveGroupIndex(int groupIndex);
    QTabWidget *group(int groupIndex) const;
    int groupIndexFor(QWidget *widget) const;
    int tabIndexInGroup(QWidget *widget) const;
    QList<QalamEditor*> editors() const;
    void ensureTwoGroups(Qt::Orientation orientation);

    bool splitCurrent(Qt::Orientation orientation, bool before = false);
    bool moveCurrentToOtherGroup(Qt::Orientation orientation,
                                 bool before = false);
    bool addSharedView(QalamEditor *source, int groupIndex,
                       const QString &label = QString(),
                       const QString &toolTip = QString());
    void closeSecondaryGroup();
    Qt::Orientation splitOrientation() const;
    void setSplitOrientation(Qt::Orientation orientation);
    QList<int> splitSizes() const;
    void setSplitSizes(const QList<int> &sizes);

signals:
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void tabBarContextMenuRequested(const QPoint &position);
    void editorViewCreated(QalamEditor *editor);
    void workspaceLayoutChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QalamEditorTabWidget *createGroup();
    void configureGroup(QalamEditorTabWidget *group);
    void activateGroup(QalamEditorTabWidget *group);
    int flatIndex(QTabWidget *group, int localIndex) const;
    QPair<QTabWidget*, int> locate(int flatIndex) const;
    void moveViewToGroup(QWidget *view, QalamEditorTabWidget *target);
    QalamEditorTabWidget *ensureSecondaryGroup(bool before);
    void removeEmptySecondaryGroup();

    QSplitter *m_splitter{};
    QList<QalamEditorTabWidget*> m_groups;
    QalamEditorTabWidget *m_activeGroup{};
    bool m_documentMode{true};
    bool m_tabsClosable{true};
    bool m_movable{true};
};
