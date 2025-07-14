#pragma once

//#include "SPFolders.h"
#include "SPEditor.h"
//#include "SPTerminal.h"
#include "SPMenu.h"
#include "SPSettings.h"

#include <QMainWindow>


class Spectrum : public QMainWindow
{
    Q_OBJECT

public:
    Spectrum(const QString& filePath = "", QWidget* parent = nullptr);
    ~Spectrum();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile(QString);
    void saveFile();
    void saveFileAs();
    void openSettings();
    void exitApp();

    void runAlif();
    void aboutSpectrum();

    void updateWindowTitle();
    void onModificationChanged(bool modified);

    // LSP Client slots
    void onLspServerReady();
    void onLspError(const QString& error);

private:
    void initializeLspClient();
    int needSave();

private:
    SPEditor* editor{};
    SPMenuBar* menuBar{};
    SPSettings* settings{};

    QString currentFilePath{};

};
