#include "QalamEditorWorkspace.h"

#include "QalamDocumentModel.h"
#include "QalamEditor.h"

#include <QApplication>
#include <QDataStream>
#include <QDrag>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMouseEvent>
#include <QSplitter>
#include <QTabBar>

namespace {
constexpr auto EditorViewMimeType = "application/x-qalam-editor-view";

class QalamEditorTabBar final : public QTabBar
{
public:
    explicit QalamEditorTabBar(QalamEditorTabWidget *owner)
        : QTabBar(owner), m_owner(owner)
    {
        setAcceptDrops(true);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        m_dragStart = event->position().toPoint();
        QTabBar::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (not (event->buttons() & Qt::LeftButton)
            or (event->position().toPoint() - m_dragStart).manhattanLength()
                < QApplication::startDragDistance()) {
            QTabBar::mouseMoveEvent(event);
            return;
        }

        const int index = tabAt(m_dragStart);
        QWidget *view = index >= 0 ? m_owner->widget(index) : nullptr;
        if (not view) return;

        QByteArray payload;
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream << quint64(reinterpret_cast<quintptr>(view));
        auto *mimeData = new QMimeData();
        mimeData->setData(EditorViewMimeType, payload);
        auto *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent *event) override
    {
        if (event->mimeData()->hasFormat(EditorViewMimeType)) {
            event->acceptProposedAction();
        }
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        if (event->mimeData()->hasFormat(EditorViewMimeType)) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        QByteArray payload = event->mimeData()->data(EditorViewMimeType);
        QDataStream stream(&payload, QIODevice::ReadOnly);
        quint64 address{};
        stream >> address;
        QWidget *view = reinterpret_cast<QWidget*>(quintptr(address));
        if (view) emit m_owner->viewDropped(view);
        event->acceptProposedAction();
    }

private:
    QalamEditorTabWidget *m_owner{};
    QPoint m_dragStart;
};
}

QalamEditorTabWidget::QalamEditorTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    setTabBar(new QalamEditorTabBar(this));
}

QalamEditorWorkspace::QalamEditorWorkspace(QWidget *parent)
    : QWidget(parent),
      m_splitter(new QSplitter(Qt::Horizontal, this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);
    layout->addWidget(m_splitter);

    m_activeGroup = createGroup();
    m_splitter->addWidget(m_activeGroup);
}

QalamEditorTabWidget *QalamEditorWorkspace::createGroup()
{
    auto *group = new QalamEditorTabWidget(m_splitter);
    configureGroup(group);
    m_groups.push_back(group);
    return group;
}

void QalamEditorWorkspace::configureGroup(QalamEditorTabWidget *group)
{
    group->setObjectName(m_groups.isEmpty() ? "MainTabs" : "SecondaryTabs");
    group->setDocumentMode(m_documentMode);
    group->setTabsClosable(m_tabsClosable);
    group->setMovable(m_movable);
    group->setLayoutDirection(Qt::RightToLeft);
    group->tabBar()->setLayoutDirection(Qt::RightToLeft);
    group->tabBar()->setUsesScrollButtons(true);
    group->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    group->installEventFilter(this);

    connect(group, &QTabWidget::currentChanged, this,
            [this, group](int localIndex) {
        if (localIndex < 0) return;
        activateGroup(group);
        emit currentChanged(flatIndex(group, localIndex));
    });
    connect(group, &QTabWidget::tabCloseRequested, this,
            [this, group](int localIndex) {
        activateGroup(group);
        emit tabCloseRequested(flatIndex(group, localIndex));
    });
    connect(group->tabBar(), &QTabBar::customContextMenuRequested, this,
            [this, group](const QPoint &position) {
        activateGroup(group);
        emit tabBarContextMenuRequested(position);
    });
    connect(group->tabBar(), &QTabBar::tabMoved,
            this, [this]() { emit workspaceLayoutChanged(); });
    connect(group, &QalamEditorTabWidget::viewDropped, this,
            [this, group](QWidget *view) { moveViewToGroup(view, group); });
}

int QalamEditorWorkspace::count() const
{
    int total{};
    for (QTabWidget *group : m_groups) total += group->count();
    return total;
}

QPair<QTabWidget*, int> QalamEditorWorkspace::locate(int flat) const
{
    if (flat < 0) return {};
    for (QTabWidget *group : m_groups) {
        if (flat < group->count()) return {group, flat};
        flat -= group->count();
    }
    return {};
}

int QalamEditorWorkspace::flatIndex(QTabWidget *target, int localIndex) const
{
    int offset{};
    for (QTabWidget *group : m_groups) {
        if (group == target) return offset + localIndex;
        offset += group->count();
    }
    return -1;
}

QWidget *QalamEditorWorkspace::widget(int index) const
{
    const auto [group, localIndex] = locate(index);
    return group ? group->widget(localIndex) : nullptr;
}

int QalamEditorWorkspace::indexOf(QWidget *view) const
{
    for (QTabWidget *group : m_groups) {
        const int localIndex = group->indexOf(view);
        if (localIndex >= 0) return flatIndex(group, localIndex);
    }
    return -1;
}

int QalamEditorWorkspace::currentIndex() const
{
    return m_activeGroup
        ? flatIndex(m_activeGroup, m_activeGroup->currentIndex()) : -1;
}

QWidget *QalamEditorWorkspace::currentWidget() const
{
    return m_activeGroup ? m_activeGroup->currentWidget() : nullptr;
}

QalamEditor *QalamEditorWorkspace::currentEditor() const
{
    return qobject_cast<QalamEditor*>(currentWidget());
}

QString QalamEditorWorkspace::tabText(int index) const
{
    const auto [group, localIndex] = locate(index);
    return group ? group->tabText(localIndex) : QString();
}

QString QalamEditorWorkspace::tabToolTip(int index) const
{
    const auto [group, localIndex] = locate(index);
    return group ? group->tabToolTip(localIndex) : QString();
}

void QalamEditorWorkspace::setTabText(int index, const QString &text)
{
    const auto [group, localIndex] = locate(index);
    if (group) group->setTabText(localIndex, text);
}

void QalamEditorWorkspace::setTabToolTip(int index, const QString &toolTip)
{
    const auto [group, localIndex] = locate(index);
    if (group) group->setTabToolTip(localIndex, toolTip);
}

int QalamEditorWorkspace::addTab(QWidget *view, const QString &label)
{
    return addTabToGroup(view, label, activeGroupIndex());
}

int QalamEditorWorkspace::addTabToGroup(QWidget *view, const QString &label,
                                         int groupIndex)
{
    if (groupIndex < 0 or groupIndex >= m_groups.size()) groupIndex = 0;
    QalamEditorTabWidget *target = m_groups.at(groupIndex);
    const int localIndex = target->addTab(view, label);
    view->installEventFilter(this);
    activateGroup(target);
    target->setCurrentIndex(localIndex);
    emit workspaceLayoutChanged();
    return flatIndex(target, localIndex);
}

void QalamEditorWorkspace::removeTab(int index)
{
    const auto [group, localIndex] = locate(index);
    if (not group) return;
    group->removeTab(localIndex);
    removeEmptySecondaryGroup();
    emit workspaceLayoutChanged();
}

void QalamEditorWorkspace::setCurrentIndex(int index)
{
    const auto [group, localIndex] = locate(index);
    if (not group) return;
    activateGroup(static_cast<QalamEditorTabWidget*>(group));
    group->setCurrentIndex(localIndex);
}

void QalamEditorWorkspace::setCurrentWidget(QWidget *view)
{
    const int index = indexOf(view);
    if (index >= 0) setCurrentIndex(index);
}

void QalamEditorWorkspace::setDocumentMode(bool enabled)
{
    m_documentMode = enabled;
    for (QTabWidget *group : m_groups) group->setDocumentMode(enabled);
}

void QalamEditorWorkspace::setTabsClosable(bool enabled)
{
    m_tabsClosable = enabled;
    for (QTabWidget *group : m_groups) group->setTabsClosable(enabled);
}

void QalamEditorWorkspace::setMovable(bool enabled)
{
    m_movable = enabled;
    for (QTabWidget *group : m_groups) group->setMovable(enabled);
}

QTabBar *QalamEditorWorkspace::tabBar() const
{
    return m_activeGroup ? m_activeGroup->tabBar() : nullptr;
}

int QalamEditorWorkspace::activeGroupIndex() const
{
    return m_groups.indexOf(m_activeGroup);
}

void QalamEditorWorkspace::setActiveGroupIndex(int groupIndex)
{
    if (groupIndex >= 0 and groupIndex < m_groups.size()) {
        activateGroup(m_groups.at(groupIndex));
    }
}

QTabWidget *QalamEditorWorkspace::group(int groupIndex) const
{
    return groupIndex >= 0 and groupIndex < m_groups.size()
        ? m_groups.at(groupIndex) : nullptr;
}

int QalamEditorWorkspace::groupIndexFor(QWidget *view) const
{
    for (int index = 0; index < m_groups.size(); ++index) {
        if (m_groups.at(index)->indexOf(view) >= 0) return index;
    }
    return -1;
}

int QalamEditorWorkspace::tabIndexInGroup(QWidget *view) const
{
    const int groupIndex = groupIndexFor(view);
    return groupIndex >= 0 ? m_groups.at(groupIndex)->indexOf(view) : -1;
}

QList<QalamEditor*> QalamEditorWorkspace::editors() const
{
    QList<QalamEditor*> result;
    for (QTabWidget *group : m_groups) {
        for (int index = 0; index < group->count(); ++index) {
            if (auto *editor = qobject_cast<QalamEditor*>(group->widget(index))) {
                result.push_back(editor);
            }
        }
    }
    return result;
}

void QalamEditorWorkspace::ensureTwoGroups(Qt::Orientation orientation)
{
    setSplitOrientation(orientation);
    ensureSecondaryGroup(false);
}

QalamEditorTabWidget *QalamEditorWorkspace::ensureSecondaryGroup(bool before)
{
    if (m_groups.size() == 2) {
        QalamEditorTabWidget *secondary = m_groups.at(1);
        m_splitter->insertWidget(before ? 0 : 1, secondary);
        return secondary;
    }

    QalamEditorTabWidget *second = createGroup();
    m_splitter->insertWidget(before ? 0 : 1, second);
    m_splitter->setSizes({1, 1});
    return second;
}

bool QalamEditorWorkspace::splitCurrent(Qt::Orientation orientation,
                                        bool before)
{
    QalamEditor *source = currentEditor();
    if (not source) return false;

    setSplitOrientation(orientation);
    ensureSecondaryGroup(before);

    return addSharedView(source, activeGroupIndex() == 0 ? 1 : 0,
                         tabText(indexOf(source)),
                         tabToolTip(indexOf(source)));
}

bool QalamEditorWorkspace::addSharedView(QalamEditor *source, int groupIndex,
                                          const QString &label,
                                          const QString &toolTip)
{
    if (not source or groupIndex < 0 or groupIndex > 1) return false;
    if (m_groups.size() == 1) ensureSecondaryGroup(false);
    QalamEditorTabWidget *target = m_groups.at(groupIndex);
    for (int index = 0; index < target->count(); ++index) {
        auto *existing = qobject_cast<QalamEditor*>(target->widget(index));
        if (existing and existing->documentModel() == source->documentModel()) {
            activateGroup(target);
            target->setCurrentIndex(index);
            return true;
        }
    }

    source->documentModel()->setParent(this);
    auto *view = new QalamEditor(source->documentModel(), target);
    const QString resolvedLabel = label.isEmpty()
        ? QFileInfo(source->currentFilePath()).fileName() : label;
    const int localIndex = target->addTab(view, resolvedLabel);
    target->setTabToolTip(localIndex, toolTip);
    view->installEventFilter(this);
    activateGroup(target);
    target->setCurrentIndex(localIndex);
    emit editorViewCreated(view);
    emit workspaceLayoutChanged();
    return true;
}

bool QalamEditorWorkspace::moveCurrentToOtherGroup(Qt::Orientation orientation,
                                                    bool before)
{
    QWidget *view = currentWidget();
    if (not view) return false;
    setSplitOrientation(orientation);
    ensureSecondaryGroup(before);
    QalamEditorTabWidget *target = m_activeGroup == m_groups.at(0)
        ? m_groups.at(1) : m_groups.at(0);
    moveViewToGroup(view, target);
    return true;
}

void QalamEditorWorkspace::moveViewToGroup(QWidget *view,
                                            QalamEditorTabWidget *target)
{
    if (not view or not target) return;
    const int sourceGroupIndex = groupIndexFor(view);
    if (sourceGroupIndex < 0 or m_groups.at(sourceGroupIndex) == target) return;
    QalamEditorTabWidget *source = m_groups.at(sourceGroupIndex);
    const int sourceIndex = source->indexOf(view);
    const QString label = source->tabText(sourceIndex);
    const QString toolTip = source->tabToolTip(sourceIndex);
    source->removeTab(sourceIndex);
    const int targetIndex = target->addTab(view, label);
    target->setTabToolTip(targetIndex, toolTip);
    activateGroup(target);
    target->setCurrentIndex(targetIndex);
    removeEmptySecondaryGroup();
    emit workspaceLayoutChanged();
}

void QalamEditorWorkspace::closeSecondaryGroup()
{
    if (m_groups.size() < 2) return;
    QalamEditorTabWidget *secondary = m_groups.at(1);
    QalamEditorTabWidget *primary = m_groups.at(0);
    while (secondary->count() > 0) {
        QWidget *view = secondary->widget(0);
        const QString label = secondary->tabText(0);
        const QString toolTip = secondary->tabToolTip(0);
        secondary->removeTab(0);
        const int index = primary->addTab(view, label);
        primary->setTabToolTip(index, toolTip);
    }
    m_groups.removeLast();
    secondary->deleteLater();
    activateGroup(primary);
    emit workspaceLayoutChanged();
}

void QalamEditorWorkspace::removeEmptySecondaryGroup()
{
    if (m_groups.size() == 2 and m_groups.at(1)->count() == 0) {
        QalamEditorTabWidget *secondary = m_groups.takeLast();
        secondary->deleteLater();
        activateGroup(m_groups.constFirst());
    }
}

Qt::Orientation QalamEditorWorkspace::splitOrientation() const
{
    return m_splitter->orientation();
}

void QalamEditorWorkspace::setSplitOrientation(Qt::Orientation orientation)
{
    if (m_splitter->orientation() == orientation) return;
    m_splitter->setOrientation(orientation);
    emit workspaceLayoutChanged();
}

QList<int> QalamEditorWorkspace::splitSizes() const
{
    return m_splitter->sizes();
}

void QalamEditorWorkspace::setSplitSizes(const QList<int> &sizes)
{
    if (sizes.size() == m_groups.size()) m_splitter->setSizes(sizes);
}

void QalamEditorWorkspace::activateGroup(QalamEditorTabWidget *group)
{
    if (not group or m_activeGroup == group) return;
    m_activeGroup = group;
    emit currentChanged(currentIndex());
}

bool QalamEditorWorkspace::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FocusIn
        or event->type() == QEvent::MouseButtonPress) {
        QWidget *view = qobject_cast<QWidget*>(watched);
        const int groupIndex = groupIndexFor(view);
        if (groupIndex >= 0) activateGroup(m_groups.at(groupIndex));
    }
    return QWidget::eventFilter(watched, event);
}
