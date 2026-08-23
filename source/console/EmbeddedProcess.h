#pragma once

#include <QProcess>

#if defined(Q_OS_WIN)
#include <qt_windows.h>
#endif

namespace QalamProcess {
inline void configureForEmbeddedConsole(QProcess *process)
{
    if (not process) return;
#if defined(Q_OS_WIN)
    process->setCreateProcessArgumentsModifier(
        [](QProcess::CreateProcessArguments *arguments) {
            arguments->flags |= CREATE_NO_WINDOW;
        });
#endif
}
}
