#include "CommandRegistry.h"

CommandRegistry::CommandRegistry(QObject *parent)
    : QObject(parent)
{
}

void CommandRegistry::clear()
{
    if (m_commands.isEmpty()) return;
    m_commands.clear();
    emit commandsChanged();
}

void CommandRegistry::registerCommand(const Command &command)
{
    if (command.id.trimmed().isEmpty()) return;

    for (Command &existing : m_commands) {
        if (existing.id == command.id) {
            existing = command;
            emit commandsChanged();
            return;
        }
    }

    m_commands.push_back(command);
    emit commandsChanged();
}

bool CommandRegistry::contains(const QString &id) const
{
    for (const Command &command : m_commands) {
        if (command.id == id) return true;
    }
    return false;
}

CommandRegistry::Command CommandRegistry::command(const QString &id) const
{
    for (const Command &command : m_commands) {
        if (command.id == id) return command;
    }
    return {};
}

QVector<CommandRegistry::Command> CommandRegistry::commands() const
{
    return m_commands;
}

QVector<CommandRegistry::Command> CommandRegistry::defaultCommands()
{
    return {
        {"file.new", "ملف: ملف جديد", "إنشاء ملف باء جديد", "Ctrl+N"},
        {"file.open", "ملف: فتح ملف", "اختيار ملف من الجهاز", "Ctrl+O"},
        {"folder.open", "ملف: فتح مجلد", "فتح مجلد عمل أو مشروع", ""},
        {"file.save", "ملف: حفظ", "حفظ الملف الحالي", "Ctrl+S"},
        {"file.saveAs", "ملف: حفظ باسم", "حفظ نسخة باسم جديد", "Ctrl+Shift+S"},
        {"quick.open", "انتقال: فتح سريع للملفات", "البحث داخل ملفات المشروع الحالي", "Ctrl+P"},
        {"go.line", "انتقال: الذهاب إلى سطر", "القفز إلى رقم سطر", "Ctrl+G"},
        {"view.search", "عرض: البحث في الملفات", "فتح بحث المشروع", "Ctrl+Shift+F"},
        {"view.sidebar", "عرض: إظهار/إخفاء الشريط الجانبي", "تبديل الشريط الجانبي", "Ctrl+B"},
        {"view.panel", "عرض: إظهار/إخفاء اللوحة السفلية", "تبديل الطرفية/اللوحة السفلية", "Ctrl+J"},
        {"view.problems", "عرض: المشاكل", "فتح لوحة المشاكل", "Ctrl+Shift+M"},
        {"view.debug", "عرض: لوحة التصحيح", "فتح لوحة التصحيح", "Ctrl+Shift+D"},
        {"code.definition", "الشفرة: الانتقال إلى التعريف", "البحث عن تعريف الرمز الحالي", "F12"},
        {"code.references", "الشفرة: البحث عن المراجع", "البحث عن استخدامات الرمز الحالي", "Shift+F12"},
        {"code.rename", "الشفرة: إعادة تسمية الرمز", "إعادة تسمية آمنة لكل مراجع الرمز", "F2"},
        {"code.quickFix", "الشفرة: إصلاح سريع", "تطبيق تعديل آمن اقترحه مترجم باء", "Ctrl+."},
        {"code.format", "الشفرة: تنسيق مستند باء", "تنسيق المستند وفق النمط الرسمي الذي يملكه مترجم باء", "Shift+Alt+F"},
        {"project.build", "تشغيل: بناء مشروع تكوين", "بناء المشروع الذي يحتوي الملف الحالي", "Ctrl+Shift+B"},
        {"project.test", "تشغيل: اختبار مشروع تكوين", "تشغيل أهداف الاختبار عبر تكوين", ""},
        {"run.baa", "تشغيل: تشغيل ملف أو مشروع باء", "حفظ وتشغيل الملف الحالي أو مشروع تكوين", "F5"},
        {"project.stop", "تشغيل: إلغاء العملية الحالية", "إيقاف بناء أو تشغيل تكوين الجاري", "Shift+F5"},
        {"project.clean", "تشغيل: تنظيف مشروع تكوين", "حذف مخرجات المشروع عبر تكوين", ""},
        {"settings.open", "إعدادات: فتح الإعدادات", "تخصيص الخط والثيم ومسار المترجم", ""},
        {"help.about", "مساعدة: عن قلم", "معلومات الإصدار والمشروع", ""},
    };
}
