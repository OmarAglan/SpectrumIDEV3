#include "QalamSettings.h"
#include "../../qalam/Constants.h"
#include "ToolchainDiscovery.h"
#include "texteditor/autocomplete/QalamCompletionHistory.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>

QalamSettings::QalamSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    setStyleSheet(QStringLiteral("color: %1; background-color: %2;")
                      .arg(Constants::Colors::TextPrimary,
                           Constants::Colors::WindowBackground));

    // Layout setup
    QHBoxLayout* mainLayout = new QHBoxLayout();
    optionsLayout = new QVBoxLayout();

    stackedWidget = new QStackedWidget();

    createCategory("المحرر", "إعدادات مظهر المحرر");
    createCategory("الأدوات", "مسارات أدوات منظومة باء وحالة اكتشافها");


    optionsLayout->setAlignment(Qt::AlignTop);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);

    QWidget* optionsWidget = new QWidget();
    optionsWidget->setStyleSheet(QStringLiteral(
        ".QWidget { border-left: 1px solid %1; }")
        .arg(Constants::Colors::BorderSubtle));
    optionsWidget->setLayout(optionsLayout);
    optionsWidget->setMinimumWidth(200);
    optionsWidget->setMaximumWidth(300);

    mainLayout->addWidget(optionsWidget);
    mainLayout->addWidget(stackedWidget);

    setLayout(mainLayout);
}


void QalamSettings::closeEvent(QCloseEvent* event) {
    Q_UNUSED(event)
    QSettings settings(Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyFontSize, fontSpin->value());
    settings.setValue(Constants::SettingsKeyFontType, fontCombo->currentText());
    settings.setValue(Constants::SettingsKeyTheme, themeCombo->currentIndex());
    saveToolPaths();
    settings.sync();
    emit toolPathsChanged();

    // emit windowClosed();
    // event->accept();
}


void QalamSettings::switchPage() {
    QalamFlatButton* btn = qobject_cast<QalamFlatButton*>(sender());
    if (btn) {
        int index = btn->property("pageIndex").toInt();
        stackedWidget->setCurrentIndex(index);

        // Update button states
        for (QalamFlatButton* category : categories) {
            bool active = (category == btn);
            category->setStyleSheet(active ?
                                        "font-weight: bold; color: #10a8f4;" :
                                        "");
        }
    }
}

void QalamSettings::createCategory(const QString& name, const QString& description) {
    // Create and configure button
    QalamFlatButton* btn = new QalamFlatButton(this, name);
    btn->setProperty("pageIndex", categories.size());
    connect(btn, &QPushButton::clicked, this, &QalamSettings::switchPage);

    // Add button to left panel
    optionsLayout->addWidget(btn);

    // Create settings page
    QWidget* page = new QWidget;
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setAlignment(Qt::AlignTop);

    // Add description label
    QLabel* descLabel = new QLabel(description);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888; margin-bottom: 20px;");
    pageLayout->addWidget(descLabel);

    // Add category-specific content
    if (name == "المحرر") {
        createAppearancePage(pageLayout);
    } else if (name == "الأدوات") {
        createToolsPage(pageLayout);
    }

    // Add page to stacked widget
    stackedWidget->addWidget(page);
    categories.append(btn);
}

void QalamSettings::createToolsPage(QVBoxLayout *layout)
{
    auto *group = new QGroupBox(QStringLiteral("أدوات البناء"));
    group->setStyleSheet(
        "QGroupBox { border: 1px solid gray; border-radius: 6px; margin-top: 2.0ex;}"
        " QGroupBox::title { subcontrol-origin: margin; padding: 0 2px; left: 10px; }");
    auto *grid = new QGridLayout(group);
    grid->setColumnStretch(1, 1);

    QSettings settings(Constants::OrgName, Constants::AppName);
    auto addToolRow = [this, grid, &settings](
        int row,
        const QString &label,
        QalamToolKind kind,
        QLineEdit **editor,
        QLabel **status) {
        auto *pathEdit = new QLineEdit;
        pathEdit->setMinimumHeight(36);
        pathEdit->setClearButtonEnabled(true);
        pathEdit->setLayoutDirection(Qt::LeftToRight);
        pathEdit->setText(settings.value(ToolchainDiscovery::settingsKey(kind)).toString());
        pathEdit->setPlaceholderText(QStringLiteral("اكتشاف تلقائي من متغير البيئة ثم PATH"));

        auto *browse = new QPushButton(QStringLiteral("اختيار…"));
        browse->setMinimumHeight(36);
        connect(browse, &QPushButton::clicked, this,
                [this, kind, pathEdit]() { chooseToolPath(kind, pathEdit); });
        connect(pathEdit, &QLineEdit::textChanged, this,
                [this]() { refreshToolHealth(); });

        auto *statusLabel = new QLabel;
        statusLabel->setWordWrap(true);
        statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        grid->addWidget(new QLabel(label), row * 2, 0);
        grid->addWidget(pathEdit, row * 2, 1);
        grid->addWidget(browse, row * 2, 2);
        grid->addWidget(statusLabel, row * 2 + 1, 1, 1, 2);
        *editor = pathEdit;
        *status = statusLabel;
    };

    addToolRow(0, QStringLiteral("مصرّف باء:"), QalamToolKind::Baa,
               &baaPathEdit, &baaStatusLabel);
    addToolRow(1, QStringLiteral("نظام تكوين:"), QalamToolKind::Takween,
               &takweenPathEdit, &takweenStatusLabel);
    addToolRow(2, QStringLiteral("مجمّع نظم:"), QalamToolKind::Nazm,
               &nazmPathEdit, &nazmStatusLabel);

    auto *hint = new QLabel(QStringLiteral(
        "اترك الحقل فارغاً للاكتشاف التلقائي. الترتيب: متغير QALAM_*_PATH، "
        "ثم PATH، ثم حزمة محمولة قديمة. يحفظ قلم المسار المختار فقط ولا ينسخ الأدوات."));
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #aaaaaa;"));

    auto *refresh = new QPushButton(QStringLiteral("إعادة فحص الأدوات"));
    refresh->setMinimumHeight(36);
    connect(refresh, &QPushButton::clicked, this, &QalamSettings::refreshToolHealth);

    layout->addWidget(group);
    layout->addWidget(hint);
    layout->addWidget(refresh, 0, Qt::AlignRight);
    refreshToolHealth();
}

void QalamSettings::chooseToolPath(QalamToolKind kind, QLineEdit *editor)
{
    if (not editor) return;
    QString initial = editor->text().trimmed();
    if (not initial.isEmpty()) initial = QFileInfo(initial).absolutePath();
    const QString chosen = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("اختيار %1").arg(QalamToolResolution{kind}.toolLabel()),
        initial,
#if defined(Q_OS_WIN)
        QStringLiteral("ملف تنفيذي (*.exe);;كل الملفات (*)")
#else
        QStringLiteral("كل الملفات (*)")
#endif
    );
    if (not chosen.isEmpty()) editor->setText(QDir::toNativeSeparators(chosen));
}

void QalamSettings::saveToolPaths()
{
    if (not baaPathEdit or not takweenPathEdit or not nazmPathEdit) return;
    QSettings settings(Constants::OrgName, Constants::AppName);
    settings.setValue(Constants::SettingsKeyCompilerPath, baaPathEdit->text().trimmed());
    settings.setValue(Constants::SettingsKeyTakweenPath, takweenPathEdit->text().trimmed());
    settings.setValue(Constants::SettingsKeyNazmPath, nazmPathEdit->text().trimmed());
    settings.sync();
}

void QalamSettings::refreshToolHealth()
{
    if (not baaPathEdit or not takweenPathEdit or not nazmPathEdit) return;

    QSettings settings(Constants::OrgName, Constants::AppName);
    const QList<QPair<QString, QString>> previous = {
        {Constants::SettingsKeyCompilerPath,
         settings.value(Constants::SettingsKeyCompilerPath).toString()},
        {Constants::SettingsKeyTakweenPath,
         settings.value(Constants::SettingsKeyTakweenPath).toString()},
        {Constants::SettingsKeyNazmPath,
         settings.value(Constants::SettingsKeyNazmPath).toString()}
    };
    settings.setValue(Constants::SettingsKeyCompilerPath, baaPathEdit->text().trimmed());
    settings.setValue(Constants::SettingsKeyTakweenPath, takweenPathEdit->text().trimmed());
    settings.setValue(Constants::SettingsKeyNazmPath, nazmPathEdit->text().trimmed());
    settings.sync();

    const QList<QalamToolResolution> resolutions = ToolchainDiscovery::resolveAll();
    const QList<QLabel *> labels = {baaStatusLabel, takweenStatusLabel, nazmStatusLabel};
    for (qsizetype index = 0; index < resolutions.size(); ++index) {
        const QalamToolResolution &resolution = resolutions.at(index);
        QLabel *label = labels.at(index);
        if (resolution.isAvailable()) {
            label->setText(QStringLiteral("✓ جاهز من %1: %2")
                               .arg(resolution.sourceLabel(),
                                    QDir::toNativeSeparators(resolution.program)));
            label->setStyleSheet(QStringLiteral("color: #73c991;"));
        } else {
            const QString requested = resolution.requestedProgram.isEmpty()
                ? QStringLiteral("لم يُعثر على ملف تنفيذي")
                : QDir::toNativeSeparators(resolution.requestedProgram);
            label->setText(QStringLiteral("✗ غير جاهز: %1").arg(requested));
            label->setStyleSheet(QStringLiteral("color: #f14c4c;"));
        }
    }

    for (const auto &entry : previous) {
        settings.setValue(entry.first, entry.second);
    }
    settings.sync();
}

void QalamSettings::createAppearancePage(QVBoxLayout* layout) {

    // ================== Font selection ==================
    QGroupBox* fontGroup = new QGroupBox("الخط");
    fontGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 6px; margin-top: 2.0ex;}"
                             " QGroupBox::title { subcontrol-origin: margin; padding: 0 2px; left: 10px; }");
    QVBoxLayout* fontLayout = new QVBoxLayout(fontGroup);
    QFormLayout* fontSizeLayout = new QFormLayout();
    QFormLayout* fontFamilyLayout = new QFormLayout();

    fontSpin = new QSpinBox;
    fontSpin->setRange(12, 36);
    fontSpin->setMinimumHeight(40);
    fontSpin->setMaximumWidth(80);

    QSettings settingsVal(Constants::OrgName, Constants::AppName);
    int savedSize = settingsVal.value(Constants::SettingsKeyFontSize).toInt();
    savedSize ? fontSpin->setValue(savedSize) : fontSpin->setValue(Constants::DefaultFontSize);

    fontSizeLayout->addRow("حجم الخط: ", fontSpin);
    connect(fontSpin, &QSpinBox::valueChanged, this, &QalamSettings::fontSizeChanged);


    fontCombo = new QComboBox();
    fontCombo->setEditable(true);
    fontCombo->setInsertPolicy(QComboBox::NoInsert);
    fontCombo->setMinimumHeight(40);
    fontCombo->setMaximumWidth(200);

    QStringList fontFamilies = QFontDatabase::families();
    fontFamilies.sort(Qt::CaseInsensitive);

    const QStringList preferredFonts = {
        Constants::DefaultFontType,
        "Noto Kufi Arabic",
        "Tajawal",
        "Kawkab Mono"
    };
    for (const QString &family : preferredFonts) {
        if (!family.isEmpty() && !fontFamilies.contains(family, Qt::CaseInsensitive)) {
            fontFamilies.prepend(family);
        }
    }

    fontCombo->addItems(fontFamilies);
    QString savedFont = settingsVal.value(Constants::SettingsKeyFontType).toString();
    !savedFont.isEmpty() ? fontCombo->setCurrentText(savedFont) : fontCombo->setCurrentText(Constants::DefaultFontType);

    fontFamilyLayout->addRow("نوع الخط: ", fontCombo);
    connect(fontCombo, &QComboBox::currentTextChanged, this, &QalamSettings::fontTypeChanged);

    fontLayout->addLayout(fontSizeLayout);
    fontLayout->addLayout(fontFamilyLayout);

    // ================== Themes ==================
    QGroupBox* themeGroup = new QGroupBox("المظهر");
    themeGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 6px; margin-top: 2.0ex;}"
                             " QGroupBox::title { subcontrol-origin: margin; padding: 0 2px; left: 10px; }");
    QVBoxLayout* themeLayout = new QVBoxLayout(themeGroup);
    QFormLayout* comboLayout = new QFormLayout();

    themeCombo = new QComboBox();
    themeCombo->setInsertPolicy(QComboBox::NoInsert);
    themeCombo->setMinimumHeight(40);
    themeCombo->setMaximumWidth(250);

    // Populate UI
    auto availableThemes = ThemeManager::getAvailableThemes();
    for (const auto& theme : availableThemes) {
        themeCombo->addItem(theme->name());
    }

    int savedTheme = settingsVal.value(Constants::SettingsKeyTheme).toInt();
    if (savedTheme < 0 || savedTheme >= themeCombo->count()) savedTheme = 0;
    themeCombo->setCurrentIndex(savedTheme);

    comboLayout->addRow("مظهر الشيفرة: ", themeCombo);
    connect(themeCombo, &QComboBox::currentIndexChanged, this, &QalamSettings::highlighterThemeChanged);

    themeLayout->addLayout(comboLayout);

    auto *completionGroup = new QGroupBox(QStringLiteral("ترتيب الإكمال"));
    auto *completionLayout = new QVBoxLayout(completionGroup);
    auto *completionHint = new QLabel(QStringLiteral(
        "يرتب قلم النتائج المتساوية دلالياً بحسب الاقتراحات التي اخترتها "
        "فعلياً داخل السياق نفسه. لا تُرسل هذه البيانات خارج الجهاز."));
    completionHint->setWordWrap(true);
    completionHint->setStyleSheet(QStringLiteral("color: #aaaaaa;"));
    auto *clearCompletionHistory = new QPushButton(
        QStringLiteral("مسح سجل ترتيب الاقتراحات"));
    clearCompletionHistory->setMinimumHeight(36);
    connect(clearCompletionHistory, &QPushButton::clicked, this,
            [clearCompletionHistory]() {
        QSettings settings(Constants::OrgName, Constants::AppName);
        QalamCompletionHistory::clear(settings);
        clearCompletionHistory->setText(QStringLiteral("تم مسح السجل"));
    });
    completionLayout->addWidget(completionHint);
    completionLayout->addWidget(clearCompletionHistory, 0, Qt::AlignRight);




    layout->addWidget(fontGroup);
    layout->addWidget(themeGroup);
    layout->addWidget(completionGroup);
}

QComboBox *QalamSettings::getThemeCombo() const {
    return themeCombo;
}
