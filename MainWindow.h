#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QThread>
#include <QProgressBar>
#include <QLabel>
#include <QStatusBar>
#include <QCloseEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <filesystem>
#include <string>
#include <set>
#include <sstream>
#include <QMessageBox>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onBrowseBasePath();
    void onBrowseJsonPath();
    void onStartConversion();
    void onResetToEnglish();
    void onCheckUpdate();

private:
    void setupUI();
    void setAllButtonsCursorForbidden();
    void setAllButtonsCursorNormal();
    void appendLog(const QString &text);
    void applyGlobalStyleSheet();
    void detectInstallPath();
    void autoDetectLocalJson();
    void runTaskInThread(std::function<void()> task);
    void setBusyState(bool busy, const QString &statusMsg = QString());
    void stopWorkerThread();

    void doCustomConversion();
    void doDirectReplacement();
    void doRollback();

    bool loadInternalLocalization();
    bool syncAndValidateBasePath();
    bool readLocalizationFromFile(const fs::path &path);
    bool replaceFile(const fs::path &filePath, const std::string &jsonKey);
    bool hasPrebuiltVersion() const;
    bool loadPrebuiltJs(const std::string &version,
                        std::string &mainJsContent,
                        std::string &rendererJsContent);

    bool showSelectItemsDialog();
    bool processSelectItems(const std::string &jsType, const std::vector<bool> &enabled);

    bool ensureBaseFilesExist();
    void backupJsFiles();
    void patchRendererCss();

    QMessageBox::StandardButton showHandMessageBox(QMessageBox::Icon icon,
                                                   const QString &title,
                                                   const QString &text,
                                                   QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                                   QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

    fs::path m_basePath;
    fs::path m_jsonPath;
    nlohmann::json m_localizationJson;
    std::string m_currentVersion;
    std::string m_targetGhVersion;
    std::set<std::string> m_prebuiltVersions;

    QThread *m_workerThread = nullptr;

    QLineEdit *m_editBasePath;
    QPushButton *m_btnBrowseBase;
    QLineEdit *m_editJsonPath;
    QPushButton *m_btnBrowseJson;
    QPushButton *m_btnStart;
    QPushButton *m_btnReset;
    QPushButton *m_btnUpdate;
    QPlainTextEdit *m_logView;

    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QStatusBar *m_statusBar;

    std::vector<bool> m_selectEnables;
};