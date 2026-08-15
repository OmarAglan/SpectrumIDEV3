#include "QalamActivityBar.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QStyle>
#include <QTimer>

QalamActivityBar::QalamActivityBar(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    applyStyles();
}

void QalamActivityBar::setupUi()
{
    using namespace Constants;
    
    setFixedWidth(Layout::ActivityBarWidth);
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Top section - main view buttons
    QWidget *topSection = new QWidget();
    m_topLayout = new QVBoxLayout(topSection);
    m_topLayout->setContentsMargins(0, 4, 0, 0);
    m_topLayout->setSpacing(0);
    
    // Create main buttons
    auto explorerBtn = createButton(":/icons/resources/explorer.svg",
                                    ":/icons/resources/explorer-active.svg",
                                    ExplorerLabel,
                                    ViewType::Explorer);
    auto searchBtn = createButton(":/icons/resources/search.svg",
                                  ":/icons/resources/search-active.svg",
                                  SearchLabel,
                                  ViewType::Search);
    auto scmBtn = createButton(":/icons/resources/source-control.svg",
                               ":/icons/resources/source-control-active.svg",
                               SourceControlLabel,
                               ViewType::SourceControl);
    auto runBtn = createButton(":/icons/resources/run.svg",
                               ":/icons/resources/run-active.svg",
                               RunLabel,
                               ViewType::Run,
                               true);
    auto extensionsBtn = createButton(":/icons/resources/extensions.svg",
                                      ":/icons/resources/extensions-active.svg",
                                      ExtensionsLabel,
                                      ViewType::Extensions);
    
    m_topLayout->addWidget(explorerBtn);
    m_topLayout->addWidget(searchBtn);
    m_topLayout->addWidget(scmBtn);
    m_topLayout->addWidget(runBtn);
    m_topLayout->addWidget(extensionsBtn);
    
    // Bottom section - settings (pushed to bottom)
    QWidget *bottomSection = new QWidget();
    m_bottomLayout = new QVBoxLayout(bottomSection);
    m_bottomLayout->setContentsMargins(0, 0, 0, 4);
    m_bottomLayout->setSpacing(0);
    
    auto settingsBtn = createButton(":/icons/resources/settings.svg",
                                    ":/icons/resources/settings-active.svg",
                                    SettingsLabel,
                                    ViewType::Settings,
                                    true);
    m_bottomLayout->addWidget(settingsBtn);
    
    // Assemble layout
    m_mainLayout->addWidget(topSection);
    m_mainLayout->addStretch(1);  // Push settings to bottom
    m_mainLayout->addWidget(bottomSection);
    
    // LayoutManager selects the initial view after all components are wired.
    m_currentView = ViewType::None;
    updateButtonStates();
}

QPushButton* QalamActivityBar::createButton(const QString &inactiveIconPath,
                                        const QString &activeIconPath,
                                        const QString &tooltip,
                                        ViewType view,
                                        bool isAction)
{
    using namespace Constants;
    
    QPushButton *btn = new QPushButton(this);
    btn->setIcon(QIcon(inactiveIconPath));
    btn->setIconSize(QSize(Layout::ActivityBarIconSize, Layout::ActivityBarIconSize));
    btn->setFixedSize(Layout::ActivityBarButtonSize, Layout::ActivityBarButtonSize);
    btn->setToolTip(tooltip);
    btn->setCheckable(!isAction);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setProperty("viewType", QVariant::fromValue(view));
    btn->setProperty("inactiveIconPath", inactiveIconPath);
    btn->setProperty("activeIconPath", activeIconPath);
    btn->setProperty("isAction", isAction);
    
    connect(btn, &QPushButton::clicked, this, &QalamActivityBar::onButtonClicked);
    
    m_buttons[view] = btn;
    return btn;
}

void QalamActivityBar::onButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    
    ViewType clickedView = btn->property("viewType").value<ViewType>();
    const bool isAction = btn->property("isAction").toBool();
    
    if (isAction) {
        const QString inactiveIcon = btn->property("inactiveIconPath").toString();
        const QString activeIcon = btn->property("activeIconPath").toString();

        btn->setIcon(QIcon(activeIcon));
        QTimer::singleShot(140, this, [btn, inactiveIcon]() {
            btn->setIcon(QIcon(inactiveIcon));
        });

        if (clickedView == ViewType::Settings) {
            emit viewToggled(clickedView, true);
        } else if (clickedView == ViewType::Run) {
            emit runRequested();
        }
        return;
    }
    
    if (clickedView == m_currentView) {
        // Toggle sidebar visibility (clicking active button hides sidebar)
        emit viewToggled(clickedView, false);
        m_currentView = ViewType::None;
        updateButtonStates();
    } else {
        // Switch to new view
        setCurrentView(clickedView);
        emit viewToggled(clickedView, true);
    }
}

void QalamActivityBar::setCurrentView(ViewType view)
{
    if (m_currentView == view) return;
    
    m_currentView = view;
    updateButtonStates();
    
    if (view != ViewType::None) {
        emit viewChanged(view);
    }
}

void QalamActivityBar::updateButtonStates()
{
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        QPushButton *btn = it.value();
        const ViewType view = it.key();
        const bool isAction = btn->property("isAction").toBool();
        if (isAction) {
            continue;
        }

        const bool checked = (view == m_currentView);
        btn->setChecked(checked);

        const QString inactiveIcon = btn->property("inactiveIconPath").toString();
        const QString activeIcon = btn->property("activeIconPath").toString();
        btn->setIcon(QIcon(checked ? activeIcon : inactiveIcon));
    }
}

void QalamActivityBar::applyStyles()
{
    setStyleSheet(QalamTheme::activityBarStyleSheet());
}
