#include "BaaLogEvent.h"

namespace {
QString severityLabel(const QString &severity)
{
    if (severity == QStringLiteral("error")) return QStringLiteral("خطأ");
    if (severity == QStringLiteral("warning")) return QStringLiteral("تحذير");
    if (severity == QStringLiteral("info")) return QStringLiteral("معلومة");
    return QStringLiteral("تتبّع");
}
}

int BaaLogEvent::lspType() const
{
    if (severity == QStringLiteral("error")) return 1;
    if (severity == QStringLiteral("warning")) return 2;
    if (severity == QStringLiteral("info")) return 3;
    return 4;
}

QString BaaLogEvent::arabicSummary() const
{
    if (event == QStringLiteral("workspace.root.invalid"))
        return QStringLiteral("تعذر تفسير مجلد مساحة العمل.");
    if (event == QStringLiteral("workspace.root.removed"))
        return QStringLiteral("أزيل مجلد من مساحة العمل.");
    if (event == QStringLiteral("workspace.manifest.missing"))
        return QStringLiteral("أزيل سياق المشروع لغياب ملف مشروع.تكوين.");
    if (event == QStringLiteral("workspace.plan.unavailable"))
        return QStringLiteral("تعذر تشغيل تكوين لتحميل خطة المشروع.");
    if (event == QStringLiteral("workspace.plan.failed")) {
        const qint64 exitCode = data.value(QStringLiteral("exit_code")).toInteger(-1);
        return exitCode >= 0
            ? QStringLiteral("فشل تحميل خطة تكوين، رمز الخروج %1.").arg(exitCode)
            : QStringLiteral("فشل تحميل خطة تكوين.");
    }
    if (event == QStringLiteral("workspace.plan.invalid"))
        return QStringLiteral("أعاد تكوين خطة مشروع لا تطابق العقد المدعوم.");
    if (event == QStringLiteral("workspace.plan.empty"))
        return QStringLiteral("لا تحتوي خطة تكوين على وحدات ترجمة باء.");
    if (event == QStringLiteral("workspace.plan.loaded")) {
        const qint64 sourceCount =
            data.value(QStringLiteral("source_count")).toInteger(-1);
        return sourceCount >= 0
            ? QStringLiteral("حُمّلت خطة تكوين وفيها %1 من ملفات مصدر باء.")
                  .arg(sourceCount)
            : QStringLiteral("حُمّلت خطة تكوين.");
    }
    if (event == QStringLiteral("workspace.symbol.source-skipped"))
        return QStringLiteral("تجاوز فهرس مساحة العمل ملف مصدر باء واحدا.");
    if (event == QStringLiteral("document.open.rejected"))
        return QStringLiteral("رفض خادم اللغة فتح مستند باء.");
    if (event == QStringLiteral("document.change.invalid"))
        return QStringLiteral("وصل تغيير مستند باء دون نص كامل صالح.");
    if (event == QStringLiteral("document.change.rejected"))
        return QStringLiteral("رفض خادم اللغة تغيير مستند باء.");
    if (event == QStringLiteral("document.uri.unsupported"))
        return QStringLiteral("لا يدعم خادم اللغة هذا النوع من عناوين المستندات.");
    if (event == QStringLiteral("compiler.analysis.failed"))
        return QStringLiteral("فشل تحليل مصرف باء.");
    if (event == QStringLiteral("compiler.completion-data.failed"))
        return QStringLiteral("تعذر تحميل بيانات الإكمال من مصرف باء.");
    if (event == QStringLiteral("compiler.semantic.warning"))
        return QStringLiteral("أصدر مصرف باء تحذيرا أثناء طلب دلالي.");
    return message;
}

QString BaaLogEvent::formattedLine() const
{
    return QStringLiteral("[%1] %2")
        .arg(severityLabel(severity), arabicSummary());
}
