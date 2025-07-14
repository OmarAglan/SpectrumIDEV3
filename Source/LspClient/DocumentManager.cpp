#include "DocumentManager.h"
#include <QDebug>

DocumentManager::DocumentManager(QObject* parent)
    : QObject(parent)
{
    qDebug() << "DocumentManager: Initializing document manager";
}

DocumentManager::~DocumentManager()
{
    qDebug() << "DocumentManager: Destructor called";
    shutdown();
}

void DocumentManager::onConfigurationChanged()
{
    qDebug() << "DocumentManager: Configuration changed";
    // TODO: Implement configuration handling in future tasks
}

void DocumentManager::shutdown()
{
    qDebug() << "DocumentManager: Shutting down";
    m_documentVersions.clear();
}
