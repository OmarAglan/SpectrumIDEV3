#include "QalamBreadcrumb.h"
#include "QalamTheme.h"
#include "Constants.h"
#include <QDir>
#include <QFileInfo>

QalamBreadcrumb::QalamBreadcrumb(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(Constants::Layout::BreadcrumbHeight);
    
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 0, 8, 0);
    m_layout->setSpacing(0);
    m_layout->setDirection(QBoxLayout::RightToLeft);  // RTL layout
    
    // Add stretch at the end (left side in RTL)
    m_layout->addStretch();
    
    applyStyles();
}

void QalamBreadcrumb::setFilePath(const QString& filePath)
{
    if (m_filePath == filePath) {
        return;
    }
    m_filePath = filePath;
    rebuildBreadcrumb();
}

void QalamBreadcrumb::setProjectRoot(const QString& rootPath)
{
    if (m_projectRoot == rootPath) {
        return;
    }
    m_projectRoot = rootPath;
    rebuildBreadcrumb();
}

void QalamBreadcrumb::setCurrentSymbol(const QString& symbol)
{
    if (m_currentSymbol == symbol) {
        return;
    }
    m_currentSymbol = symbol;
    rebuildBreadcrumb();
}

void QalamBreadcrumb::clear()
{
    m_filePath.clear();
    m_currentSymbol.clear();
    m_pathSegments.clear();
    rebuildBreadcrumb();
}

void QalamBreadcrumb::rebuildBreadcrumb()
{
    // Clear existing items (except the stretch)
    while (m_layout->count() > 1) {
        QLayoutItem* item = m_layout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    if (m_filePath.isEmpty()) {
        return;
    }
    
    // Build path segments
    m_pathSegments.clear();
    QString relativePath = m_filePath;
    
    // Make path relative to project root if available
    if (!m_projectRoot.isEmpty() && m_filePath.startsWith(m_projectRoot)) {
        relativePath = QDir(m_projectRoot).relativeFilePath(m_filePath);
    }
    
    QFileInfo fileInfo(relativePath);
    
    // Split path into segments
    QString path = fileInfo.path();
    if (path != ".") {
        m_pathSegments = path.split(QDir::separator(), Qt::SkipEmptyParts);
        // Also handle forward slashes
        if (m_pathSegments.size() == 1 && m_pathSegments[0].contains('/')) {
            m_pathSegments = m_pathSegments[0].split('/', Qt::SkipEmptyParts);
        }
    }
    m_pathSegments.append(fileInfo.fileName());
    
    // Add symbol if present
    bool hasSymbol = !m_currentSymbol.isEmpty();
    
    // Build cumulative paths for each segment
    QString cumulativePath;
    bool isFirst = true;  // First in RTL = rightmost
    
    for (int i = 0; i < m_pathSegments.size(); ++i) {
        const QString& segment = m_pathSegments[i];
        
        // Build cumulative path
        if (cumulativePath.isEmpty()) {
            cumulativePath = segment;
        } else {
            cumulativePath += QDir::separator() + segment;
        }
        
        bool isLastPath = (i == m_pathSegments.size() - 1) && !hasSymbol;
        
        // Add separator before segment (after in visual RTL order)
        if (!isFirst) {
            m_layout->insertWidget(m_layout->count() - 1, createSeparator());
        }
        
        // Add segment button
        QPushButton* btn = createSegmentButton(segment, cumulativePath, isLastPath);
        m_layout->insertWidget(m_layout->count() - 1, btn);
        
        isFirst = false;
    }
    
    // Add symbol segment if present
    if (hasSymbol) {
        m_layout->insertWidget(m_layout->count() - 1, createSeparator());
        
        QPushButton* symbolBtn = createSegmentButton(m_currentSymbol, QString(), true);
        connect(symbolBtn, &QPushButton::clicked, this, [this]() {
            emit symbolClicked(m_currentSymbol);
        });
        m_layout->insertWidget(m_layout->count() - 1, symbolBtn);
    }
}

QPushButton* QalamBreadcrumb::createSegmentButton(const QString& text, const QString& fullPath, bool isLast)
{
    QPushButton* btn = new QPushButton(text, this);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFlat(true);
    
    QString style = QString(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: %1;
            padding: 2px 4px;
            font-size: %2px;
            text-align: right;
        }
        QPushButton:hover {
            color: %3;
            background: %4;
        }
    )").arg(isLast ? Constants::Colors::BreadcrumbFocusForeground 
                   : Constants::Colors::BreadcrumbForeground)
      .arg(Constants::Fonts::BreadcrumbSize)
      .arg(Constants::Colors::TextPrimary)
      .arg(Constants::Colors::ListHoverBackground);
    
    btn->setStyleSheet(style);
    
    if (!fullPath.isEmpty()) {
        connect(btn, &QPushButton::clicked, this, [this, fullPath]() {
            emit segmentClicked(fullPath);
        });
    }
    
    return btn;
}

QLabel* QalamBreadcrumb::createSeparator()
{
    QLabel* sep = new QLabel(this);
    // Use chevron left for RTL (points in reading direction)
    sep->setText("\u276E");  // ❮ LEFT-POINTING ANGLE BRACKET
    sep->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            padding: 0 2px;
            font-size: %2px;
        }
    )").arg(Constants::Colors::TextMuted)
      .arg(Constants::Fonts::BreadcrumbSize - 2));
    
    return sep;
}

void QalamBreadcrumb::applyStyles()
{
    setStyleSheet(QString(R"(
        QalamBreadcrumb {
            background-color: %1;
            border-bottom: 1px solid %2;
        }
    )").arg(Constants::Colors::BreadcrumbBackground)
      .arg(Constants::Colors::BorderSubtle));
}
