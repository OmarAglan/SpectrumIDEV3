#pragma once

#include <QObject>
#include <QMap>
#include <QString>

/**
 * @brief Manages document synchronization with the LSP server
 * 
 * This class handles document lifecycle and synchronization.
 * It will be expanded in future tasks to include full document
 * synchronization capabilities.
 */
class DocumentManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit DocumentManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~DocumentManager();

    /**
     * @brief Handle configuration changes
     */
    void onConfigurationChanged();

    /**
     * @brief Shutdown the document manager
     */
    void shutdown();

private:
    QMap<QString, int> m_documentVersions; // URI -> version
};
