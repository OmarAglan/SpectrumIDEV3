#include "Qalam.h"
#include "QalamTheme.h"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QFontDatabase>
#include <QIcon>

#include <QFileDialog>
#include <QLockFile>
#include <QDir>
#include "Constants.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(Constants::OrgName);
    QCoreApplication::setApplicationName(Constants::AppName);
    QCoreApplication::setApplicationVersion(Constants::AppVersion);
    app.setWindowIcon(QIcon(":/icons/resources/QalamLogo.png"));
    app.setLayoutDirection(Qt::RightToLeft);


    QString lockPath = QDir::tempPath() + "/qalam_ide.lock";
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);

    if (!lockFile.tryLock(100)) {
        QMessageBox::warning(nullptr, "قلم",
                             "البرنامج يعمل بالفعل!\nلا يمكن تشغيل أكثر من نسخة في نفس الوقت.");
        return 0;
    }

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

    Qalam *editor = new Qalam(filePath);
    editor->setWindowTitle(QStringLiteral("قلم"));
    editor->show();
#ifdef Q_OS_WIN
    editor->raise();
    editor->activateWindow();
#endif

    return app.exec();
}
