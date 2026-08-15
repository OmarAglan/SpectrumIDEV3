#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStringList>

/**
 * @brief QalamBreadcrumb - VSCode-like breadcrumb navigation bar
 * 
 * Shows the current file path and symbol hierarchy:
 * Project > folder > file.baa > functionName()
 * 
 * Each segment is clickable for navigation.
 * RTL layout: reads right-to-left with arrows pointing left.
 */
class QalamBreadcrumb : public QWidget
{
    Q_OBJECT

public:
    explicit QalamBreadcrumb(QWidget* parent = nullptr);
    ~QalamBreadcrumb() override = default;

    /// Set the current file path (will be split into segments)
    void setFilePath(const QString& filePath);
    
    /// Set the project root path (used to make paths relative)
    void setProjectRoot(const QString& rootPath);
    
    /// Set the current symbol (function, class, etc.) from cursor position
    void setCurrentSymbol(const QString& symbol);
    
    /// Clear all breadcrumb segments
    void clear();

signals:
    /// Emitted when a path segment is clicked
    void segmentClicked(const QString& fullPath);
    
    /// Emitted when the symbol segment is clicked
    void symbolClicked(const QString& symbol);

private:
    void rebuildBreadcrumb();
    QPushButton* createSegmentButton(const QString& text, const QString& fullPath, bool isLast);
    QLabel* createSeparator();
    void applyStyles();

    QHBoxLayout* m_layout{nullptr};
    QString m_projectRoot;
    QString m_filePath;
    QString m_currentSymbol;
    QStringList m_pathSegments;
};
