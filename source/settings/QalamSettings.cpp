#include "QalamSettings.h"
#include "../../qalam/Constants.h"

QalamSettings::QalamSettings(QWidget* parent) : QWidget(parent) {
    setWindowTitle("الإعدادات");
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setMinimumSize(800, 600);
    setStyleSheet("color: #dddddd; background-color: #1e202e;");

    // Layout setup
    QHBoxLayout* mainLayout = new QHBoxLayout();
    optionsLayout = new QVBoxLayout();

    stackedWidget = new QStackedWidget();

    createCategory("المحرر", "إعدادات مظهر المحرر");
    // createCategory("متقدم", "الإعداد المتقدمة");


    optionsLayout->setAlignment(Qt::AlignTop);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(0);

    QWidget* optionsWidget = new QWidget();
    optionsWidget->setStyleSheet(".QWidget { border-left-width: 3px; border-left-style: ridge; border-left-color: #1e202f; }");
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
    settings.sync();

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
    } else if (name == "متقدم") {
        // createFontsPage(pageLayout);
    }

    // Add page to stacked widget
    stackedWidget->addWidget(page);
    categories.append(btn);
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




    layout->addWidget(fontGroup);
    layout->addWidget(themeGroup);
}

QComboBox *QalamSettings::getThemeCombo() const {
    return themeCombo;
}
