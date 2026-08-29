#include "Qalam.h"
#include "QalamTheme.h"
#include "SessionSlot.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFontDatabase>
#include <QIcon>

#include <QFileDialog>
#include "Constants.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(Constants::OrgName);
    QCoreApplication::setApplicationName(Constants::AppName);
    QCoreApplication::setApplicationVersion(Constants::AppVersion);
    app.setWindowIcon(QIcon(":/icons/resources/QalamLogo.png"));
    app.setLayoutDirection(Qt::RightToLeft);


    const int tajawalFontId = QFontDatabase::addApplicationFont(":/fonts/resources/fonts/Tajawal/Tajawal-Regular.ttf");
    const int kawkabMonoFontId = QFontDatabase::addApplicationFont(":/fonts/resources/fonts/KawkabMono-Regular.ttf");
    const int notoKufiFontId = QFontDatabase::addApplicationFont(":/fonts/resources/fonts/NotoKufiArabic-Regular.ttf");

    const QStringList tajawalFamilies = QFontDatabase::applicationFontFamilies(tajawalFontId);
    const QStringList kawkabFamilies = QFontDatabase::applicationFontFamilies(kawkabMonoFontId);
    const QStringList notoKufiFamilies = QFontDatabase::applicationFontFamilies(notoKufiFontId);

    if(tajawalFamilies.isEmpty() or kawkabFamilies.isEmpty() or notoKufiFamilies.isEmpty()) {
        qWarning() << "لم يستطع تحميل الخط";
    } else {
        QFont font{};
        QStringList fontFamilies{};
        fontFamilies << notoKufiFamilies.first() << tajawalFamilies.first() << kawkabFamilies.first();
        font.setFamilies(fontFamilies);
        font.setPixelSize(14); // General UI font size
        font.setWeight(QFont::Weight::Normal);
        app.setFont(font);
    }

    // Apply Qalam Theme
    QalamTheme::instance().apply(&app);

    // لتشغيل ملف باء بإستخدام محرر قلم عند إختيار المحرر ك برنامج للتشغيل
    QString filePath{};
    if (app.arguments().count() > 2) {
        int ret = QMessageBox::warning(nullptr, "قلم",
                                       "لا يمكن تمرير أكثر من معامل واحد",
                                       QMessageBox::Close);
        return ret;
    }

    if (app.arguments().count() == 2) {
        filePath = app.arguments().at(1);
    }

    app.setQuitOnLastWindowClosed(true);

    std::unique_ptr<SessionSlot> sessionSlot = SessionSlot::acquire();
    if (not sessionSlot) {
        QMessageBox::critical(
            nullptr, QStringLiteral("قلم"),
            QStringLiteral("تعذر حجز ملف جلسة مستقل لهذه النافذة."));
        return 1;
    }

    Qalam *editor = new Qalam(
        filePath, nullptr, sessionSlot->settingsFilePath());
    editor->setWindowTitle(QStringLiteral("قلم"));
    editor->show();
#ifdef Q_OS_WIN
    editor->raise();
    editor->activateWindow();
#endif

    return app.exec();
}
