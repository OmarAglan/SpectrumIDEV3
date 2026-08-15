#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>

/**
 * @brief Search View - Find in files functionality
 * 
 * Shows:
 * - Search input field
 * - Replace input field (optional, toggled)
 * - Search results tree
 */
class QalamSearchView : public QWidget
{
    Q_OBJECT

public:
    explicit QalamSearchView(QWidget *parent = nullptr);
    ~QalamSearchView() = default;

    void setSearchPath(const QString &path);
    void focusSearchInput();
    void scheduleSearch();
    void clearResults();

signals:
    void searchRequested(const QString &query, bool caseSensitive, bool wholeWord, bool regex);
    void searchCancelled();
    void replaceRequested(const QString &query, const QString &replacement,
                          bool caseSensitive, bool wholeWord, bool regex);
    void resultClicked(const QString &filePath, int line, int column);

public slots:
    void addResult(const QString &filePath, int line, int column, const QString &lineText, const QString &matchText);
    void setSearching(bool searching);
    void setWorkspaceIndexing();
    void setSearchProgress(int scannedFiles, int totalFiles);
    void setReplacementProgress(int scannedFiles, int totalFiles);
    void setSearchError(const QString &message);
    void setResultCount(int fileCount, int matchCount,
                        bool truncated = false, int skippedFiles = 0);

private slots:
    void onSearchTextChanged();
    void onSearchTriggered();
    void onResultItemClicked(QTreeWidgetItem *item, int column);

private:
    void setupUi();
    void applyStyles();

    QString m_searchPath;
    
    QVBoxLayout *m_mainLayout = nullptr;
    
    // Search inputs
    QWidget *m_inputContainer = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QLineEdit *m_replaceInput = nullptr;
    QPushButton *m_toggleReplaceBtn = nullptr;
    QPushButton *m_replaceAllBtn = nullptr;
    
    // Search options
    QWidget *m_optionsContainer = nullptr;
    QPushButton *m_caseSensitiveBtn = nullptr;
    QPushButton *m_wholeWordBtn = nullptr;
    QPushButton *m_regexBtn = nullptr;
    
    // Results
    QLabel *m_resultSummary = nullptr;
    QTreeWidget *m_resultsTree = nullptr;
    
    // State
    bool m_replaceVisible = false;
    bool m_caseSensitive = false;
    bool m_wholeWord = false;
    bool m_useRegex = false;
    bool m_searching = false;
    
    QTimer *m_searchDebounce = nullptr;
};
