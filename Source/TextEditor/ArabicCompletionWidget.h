#pragma once

#include <QWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QJsonObject>
#include <QJsonArray>
#include <QFont>
#include <QTimer>

// Forward declarations
class SpectrumLspClient;
class QPlainTextEdit;

/**
 * @brief Enhanced Arabic completion item structure
 */
struct ArabicCompletionItem {
    QString label;                      // "اطبع"
    QString arabicName;                 // "اطبع"
    QString englishName;                // "print"
    int kind;                          // LSP completion item kind
    
    // Arabic-specific metadata
    QString arabicDescription;          // "يطبع النص المحدد إلى وحدة التحكم"
    QString arabicDetailedDesc;         // Extended Arabic explanation
    QString usageExample;               // "اطبع(\"مرحبا بالعالم\")"
    QString arabicExample;              // Full code example with Arabic comments
    
    // Function-specific data
    QJsonArray parameters;              // Function parameters
    QString returnType;                 // Return type
    QString arabicReturnDesc;           // Arabic return description
    
    // Metadata
    int priority = 50;                  // 1-100 relevance score
    QStringList contexts;               // ["global", "function", "class"]
    QStringList tags;                   // ["io", "basic", "beginner"]
    QString category;                   // "control_flow", "io", "math", etc.
    
    // UI data
    QString insertText;                 // Text to insert
    QString filterText;                 // Text used for filtering
    QString sortText;                   // Text used for sorting
    
    // Constructors
    ArabicCompletionItem() = default;
    ArabicCompletionItem(const QJsonObject& json);
    
    // Utility methods
    QString getDisplayText() const;
    QString getDetailText() const;
    QString getTypeText() const;
    bool isApplicableInContext(const QString& context) const;
};

/**
 * @brief Modern multi-panel Arabic completion widget
 */
class ArabicCompletionWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ArabicCompletionWidget(QWidget* parent = nullptr);
    ~ArabicCompletionWidget();
    
    // Main interface
    void showCompletions(const QList<ArabicCompletionItem>& items, const QString& filter = "");
    void updateFilter(const QString& filter);
    void hide();
    
    // Selection management
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();
    ArabicCompletionItem getSelectedItem() const;
    bool hasSelection() const;
    
    // Configuration
    void setMaxVisibleItems(int count);
    void setMinimumWidth(int width);
    void enableRichDescriptions(bool enabled);
    
signals:
    void itemSelected(const ArabicCompletionItem& item);
    void itemActivated(const ArabicCompletionItem& item);
    void exampleInsertRequested(const ArabicCompletionItem& item);
    void cancelled();
    
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    
private slots:
    void onItemSelectionChanged();
    void onItemDoubleClicked(QListWidgetItem* item);
    void onInsertExampleClicked();
    
private:
    // UI Components
    QSplitter* m_mainSplitter;
    
    // Left panel - completion list
    QWidget* m_listPanel;
    QListWidget* m_listWidget;
    QLabel* m_statusLabel;
    
    // Right panel - details
    QWidget* m_detailsPanel;
    QLabel* m_titleLabel;
    QLabel* m_typeLabel;
    QTextEdit* m_descriptionText;
    QTextEdit* m_exampleText;
    QPushButton* m_insertExampleButton;
    
    // Data
    QList<ArabicCompletionItem> m_allItems;
    QList<ArabicCompletionItem> m_filteredItems;
    QString m_currentFilter;
    int m_maxVisibleItems;
    bool m_richDescriptionsEnabled;
    
    // Fonts and styling
    QFont m_arabicFont;
    QFont m_codeFont;
    QFont m_typeFont;
    
    // Setup methods
    void setupUI();
    void setupFonts();
    void setupStyling();
    void setupConnections();
    void setupRTLSupport();
    
    // List management
    void populateList();
    void filterItems(const QString& filter);
    void updateItemDisplay(QListWidgetItem* listItem, const ArabicCompletionItem& item);
    
    // Details panel
    void updateDetailsPanel(const ArabicCompletionItem& item);
    void clearDetailsPanel();
    
    // Utility methods
    QString formatArabicDescription(const QString& desc);
    QString formatCodeExample(const QString& example);
    QString getKindText(int kind);
    QColor getKindColor(int kind);
    
    // Fuzzy matching
    int calculateMatchScore(const QString& filter, const QString& text);
    QList<int> getHighlightPositions(const QString& filter, const QString& text);
};

/**
 * @brief Custom list widget item for Arabic completions
 */
class ArabicCompletionListItem : public QListWidgetItem {
public:
    explicit ArabicCompletionListItem(const ArabicCompletionItem& item, QListWidget* parent = nullptr);
    
    const ArabicCompletionItem& getCompletionItem() const { return m_item; }
    
private:
    ArabicCompletionItem m_item;
};

// EnhancedAutoComplete functionality is now integrated into the existing AutoComplete class
