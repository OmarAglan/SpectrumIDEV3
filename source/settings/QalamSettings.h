#pragma once

#include "QalamFlatButton.h"
#include "ThemeManager.h"

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QStackedWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QFontDatabase>
#include <QFormLayout>

class QalamSettings : public QWidget {
    Q_OBJECT
public:
    explicit QalamSettings(QWidget* parent = nullptr);

    QVector<std::shared_ptr<SyntaxTheme>> getAvailableThemes() const { return ThemeManager::getAvailableThemes(); }

    QComboBox *getThemeCombo() const;

protected:
    void closeEvent(QCloseEvent* event) override;

signals:
    void fontSizeChanged(int size);
    void fontTypeChanged(QString font);
    void highlighterThemeChanged(int themeIdx);


private:
    void switchPage();
    void createCategory(const QString&, const QString&);
    void createAppearancePage(QVBoxLayout*);

    QVBoxLayout* optionsLayout{};
    QStackedWidget* stackedWidget{};
    QList<QalamFlatButton*> categories{};

    QSpinBox* fontSpin{};
    QComboBox* fontCombo{};
    QComboBox* themeCombo{};

};
