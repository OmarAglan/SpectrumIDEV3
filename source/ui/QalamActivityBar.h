#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMap>

/**
 * @brief Activity Bar - Vertical icon bar for switching views (like VSCode)
 * 
 * Uses a mirrored VS Code placement on the RIGHT side of the workbench for
 * Arabic/RTL users.
 * Contains icon buttons for: Explorer, Search, Source Control, Run, Extensions, Settings.
 * Settings button is pushed to the bottom.
 */
class QalamActivityBar : public QWidget
{
    Q_OBJECT

public:
    enum class ViewType {
        Explorer,
        Search,
        SourceControl,
        Run,
        Extensions,
        Settings,
        None
    };

    explicit QalamActivityBar(QWidget *parent = nullptr);
    ~QalamActivityBar() = default;

    ViewType currentView() const { return m_currentView; }
    void setCurrentView(ViewType view);

signals:
    void viewChanged(QalamActivityBar::ViewType view);
    void viewToggled(QalamActivityBar::ViewType view, bool visible);
    void runRequested();

private slots:
    void onButtonClicked();

private:
    void setupUi();
    QPushButton* createButton(const QString &inactiveIconPath,
                              const QString &activeIconPath,
                              const QString &tooltip,
                              ViewType view,
                              bool isAction = false);
    void updateButtonStates();
    void applyStyles();

    ViewType m_currentView = ViewType::None;
    QMap<ViewType, QPushButton*> m_buttons;
    QVBoxLayout *m_mainLayout = nullptr;
    QVBoxLayout *m_topLayout = nullptr;
    QVBoxLayout *m_bottomLayout = nullptr;
};

Q_DECLARE_METATYPE(QalamActivityBar::ViewType)
