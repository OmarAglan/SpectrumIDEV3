#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabBar>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>

class QalamConsole;

/**
 * @brief QalamPanelArea - VSCode-like bottom panel with tabs
 * 
 * Contains three tabbed sections:
 * - المشاكل (Problems): Shows compilation errors/warnings
 * - المخرجات (Output): Shows build output
 * - الطرفية (Terminal): Interactive terminal (QalamConsole)
 * 
 * RTL layout with tabs on the right side.
 */
class QalamPanelArea : public QWidget
{
    Q_OBJECT

public:
    enum class Tab {
        Problems,
        Output,
        Terminal,
        Debug
    };
    Q_ENUM(Tab)

    explicit QalamPanelArea(QWidget* parent = nullptr);
    ~QalamPanelArea() override = default;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    /// Set the active tab
    void setCurrentTab(Tab tab);
    Tab currentTab() const;
    
    /// Get the terminal console widget
    QalamConsole* terminal() const { return m_terminal; }
    
    /// Add a problem entry (error, warning, info)
    void addProblem(const QString& message, const QString& file, int line, int column, 
                    const QString& severity = "error");
    void clearProblems();
    int problemCount() const;
    int errorCount() const { return m_errorCount; }
    int warningCount() const { return m_warningCount; }
    
    /// Add output text
    void appendOutput(const QString& text);
    void clearOutput();
    QString outputText() const;
    int outputBlockCount() const;
    
    /// Update the problems badge count
    void updateProblemsBadge();
    
    /// Show/hide the panel
    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

signals:
    void tabChanged(Tab tab);
    void problemClicked(const QString& file, int line, int column);
    void closeRequested();
    void maximizeRequested();
    void collapseToggled(bool collapsed);

private:
    void setupUI();
    void setupTabBar();
    void setupProblemsView();
    void setupOutputView();
    void setupTerminal();
    void setupDebugView();
    void applyStyles();
    QWidget* createHeaderBar();

    QVBoxLayout* m_mainLayout{nullptr};
    QTabBar* m_tabBar{nullptr};
    QStackedWidget* m_stackedWidget{nullptr};
    
    // Tab contents
    QWidget* m_problemsView{nullptr};
    QWidget* m_outputView{nullptr};
    QalamConsole* m_terminal{nullptr};
    QWidget* m_debugView{nullptr};
    
    // Problems list
    QVBoxLayout* m_problemsLayout{nullptr};
    QWidget* m_problemsContent{nullptr};
    
    // Output display
    QPlainTextEdit* m_outputText{nullptr};
    
    // Header buttons
    QPushButton* m_closeBtn{nullptr};
    QPushButton* m_maximizeBtn{nullptr};
    
    // Badge for problem count
    QLabel* m_problemsBadge{nullptr};
    int m_errorCount{0};
    int m_warningCount{0};
    
    bool m_collapsed{false};
};
