#pragma once

#include <QObject>
#include <QMap>

/**
 * @brief Manages LSP feature implementations and coordination
 * 
 * This class coordinates different LSP features like completion,
 * hover, diagnostics, etc. It will be expanded in future tasks.
 */
class LspFeatureManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit LspFeatureManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~LspFeatureManager();

    /**
     * @brief Enable or disable a feature
     * @param feature Feature name
     * @param enabled Whether to enable the feature
     */
    void setFeatureEnabled(const QString& feature, bool enabled);

    /**
     * @brief Check if a feature is enabled
     * @param feature Feature name
     * @return true if feature is enabled
     */
    bool isFeatureEnabled(const QString& feature) const;

    /**
     * @brief Handle configuration changes
     */
    void onConfigurationChanged();

    /**
     * @brief Shutdown the feature manager
     */
    void shutdown();

private:
    QMap<QString, bool> m_enabledFeatures;
};
