#include "LspFeatureManager.h"
#include <QDebug>

LspFeatureManager::LspFeatureManager(QObject* parent)
    : QObject(parent)
{
    qDebug() << "LspFeatureManager: Initializing feature manager";
}

LspFeatureManager::~LspFeatureManager()
{
    qDebug() << "LspFeatureManager: Destructor called";
    shutdown();
}

void LspFeatureManager::setFeatureEnabled(const QString& feature, bool enabled)
{
    m_enabledFeatures[feature] = enabled;
    qDebug() << "LspFeatureManager: Feature" << feature << (enabled ? "enabled" : "disabled");
}

bool LspFeatureManager::isFeatureEnabled(const QString& feature) const
{
    return m_enabledFeatures.value(feature, false);
}

void LspFeatureManager::onConfigurationChanged()
{
    qDebug() << "LspFeatureManager: Configuration changed";
    // TODO: Implement configuration handling in future tasks
}

void LspFeatureManager::shutdown()
{
    qDebug() << "LspFeatureManager: Shutting down";
    m_enabledFeatures.clear();
}
