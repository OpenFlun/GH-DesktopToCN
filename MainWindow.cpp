#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
// 不再需要手动指定入口点,CMake + Qt 会自动处理
#endif

#include "MainWindow.h"
#include "Utils/utils.hpp"
#include "WinReg/WinReg.hpp"
#include <QMessageBox>
#include <QDesktopServices>
#include <QVersionNumber>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QStyle>
#include <QScrollBar>
#include <regex>
#include <fstream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <TlHelp32.h>

static bool IsGitHubDesktopRunning();

// 统一的正则替换引擎
static std::string applyReplaceRules(const std::string &content,
                                     const std::vector<std::vector<std::string>> &rules)
{
    std::string result = content;
    for (const auto &r : rules)
    {
        if (r.empty())
            continue;
        const std::string &regexStr = r[0];
        if (regexStr.empty() || regexStr == "\"\"")
            continue;

        std::string replacement = (r.size() > 1) ? r[1] : "";
        std::regex pattern(regexStr);

        // 存在捕获组正则时,先提取捕获组并替换占位符
        if (r.size() >= 3 && !r[2].empty())
        {
            std::regex capPattern(r[2]);
            std::smatch match;
            if (std::regex_search(result, match, capPattern))
            {
                for (size_t i = 1; i < match.size(); ++i)
                {
                    std::string placeholder = "#\\{" + std::to_string(i) + "\\}";
                    replacement = std::regex_replace(replacement, std::regex(placeholder), match[i].str());
                }
            }
            else
            {
                continue; // 捕获失败,跳过当前规则
            }
        }

        result = std::regex_replace(result, pattern, replacement);
    }
    return result;
}

// ========== 构造函数 ==========
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUI();
    applyGlobalStyleSheet();
    m_currentVersion = FILEVERSION;
    setWindowTitle(QString("GitHubDesktop 汉化工具 %1").arg(m_currentVersion.c_str()));

    std::string versionsList = PREBUILT_VERSIONS;
    if (!versionsList.empty() && versionsList != "\"\"")
    {
        if (versionsList.front() == '"')
            versionsList.erase(0, 1);
        if (versionsList.back() == '"')
            versionsList.pop_back();

        std::stringstream ss(versionsList);
        std::string token;
        while (std::getline(ss, token, ';'))
        {
            if (!token.empty())
                m_prebuiltVersions.insert(token);
        }
    }

    detectInstallPath();
    autoDetectLocalJson();

    resize(780, 520);
}

MainWindow::~MainWindow()
{
    stopWorkerThread();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopWorkerThread();
    event->accept();
    QApplication::quit();
}

void MainWindow::stopWorkerThread()
{
    if (!m_workerThread)
        return;
    if (m_workerThread->isRunning())
    {
        m_workerThread->requestInterruption();
        m_workerThread->quit();
        if (!m_workerThread->wait(2000))
        {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }
    delete m_workerThread;
    m_workerThread = nullptr;
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *row1 = new QHBoxLayout();
    row1->addWidget(new QLabel("资源目录："));
    m_editBasePath = new QLineEdit();
    row1->addWidget(m_editBasePath);
    m_btnBrowseBase = new QPushButton("浏览...");
    m_btnBrowseBase->setToolTip("选择 GitHub Desktop 资源目录");
    row1->addWidget(m_btnBrowseBase);
    mainLayout->addLayout(row1);

    QGroupBox *groupJson = new QGroupBox("本地化文件");
    QVBoxLayout *jsonLayout = new QVBoxLayout(groupJson);
    QHBoxLayout *jsonFileRow = new QHBoxLayout();
    jsonFileRow->addWidget(new QLabel("自定义JSON路径："));
    m_editJsonPath = new QLineEdit();
    jsonFileRow->addWidget(m_editJsonPath);
    m_btnBrowseJson = new QPushButton("浏览...");
    m_btnBrowseJson->setToolTip("选择本地化 JSON 文件");
    jsonFileRow->addWidget(m_btnBrowseJson);
    jsonLayout->addLayout(jsonFileRow);
    QLabel *note = new QLabel("***提示:上述所有路径可编辑;汉化方式:快速汉化/自定义汉化(本地JSON)/自定义汉化(内置JSON)***");
    note->setStyleSheet("color: #efcd0e;");
    note->setAlignment(Qt::AlignCenter);
    jsonLayout->addWidget(note);
    mainLayout->addWidget(groupJson);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(15);
    m_btnStart = new QPushButton("汉化");
    m_btnStart->setObjectName("btnStart");
    m_btnReset = new QPushButton("恢复英文");
    m_btnReset->setObjectName("btnReset");
    m_btnUpdate = new QPushButton("检查更新");
    m_btnUpdate->setObjectName("btnUpdate");

    btnRow->addStretch();
    btnRow->addWidget(m_btnStart);
    btnRow->addWidget(m_btnReset);
    btnRow->addWidget(m_btnUpdate);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 0);
    m_progressBar->setMaximumHeight(20);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_logView = new QPlainTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(2000);
    mainLayout->addWidget(m_logView);

    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
    m_statusLabel = new QLabel("就绪");
    m_statusBar->addPermanentWidget(m_statusLabel);

    connect(m_btnBrowseBase, &QPushButton::clicked, this, &MainWindow::onBrowseBasePath);
    connect(m_btnBrowseJson, &QPushButton::clicked, this, &MainWindow::onBrowseJsonPath);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::onStartConversion);
    connect(m_btnReset, &QPushButton::clicked, this, &MainWindow::onResetToEnglish);
    connect(m_btnUpdate, &QPushButton::clicked, this, &MainWindow::onCheckUpdate);

    setAllButtonsCursorNormal();
}

void MainWindow::applyGlobalStyleSheet()
{
    const QString style = R"(
        QMainWindow { background-color: #2b2b2b; }
        QLabel { color: #cccccc; }
        QLineEdit {
            background-color: #3c3c3c;
            color: #cccccc;
            border: 1px solid #555;
            padding: 2px 4px;
            border-radius: 3px;
        }
        QPlainTextEdit {
            background-color: #2b2b2b;
            color: #cccccc;
            border: 1px solid #555;
        }
        QGroupBox {
            color: #cccccc;
            border: 1px solid #555;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 14px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
        QProgressBar {
            background-color: #3c3c3c;
            border: 1px solid #555;
            border-radius: 3px;
            text-align: center;
            color: white;
        }
        QProgressBar::chunk {
            background-color: #2ecc71;
            border-radius: 3px;
        }
        QPushButton {
            background-color: #3a3f44;
            color: #ffffff;
            border: 1px solid #555;
            padding: 6px 12px;
            border-radius: 4px;
            min-width: 70px;
        }
        QPushButton:hover { background-color: #4a4f54; }
        QPushButton:pressed { background-color: #2a2f34; }
        QPushButton:disabled {
            background-color: #555555;
            color: #888888;
            border: 1px solid #666;
        }
        QPushButton#btnStart {
            background-color: #2ecc71;
            color: white;
            border: none;
            padding: 6px 20px;
        }
        QPushButton#btnStart:hover { background-color: #27ae60; }
        QPushButton#btnStart:pressed { background-color: #1e8449; }
        QPushButton#btnStart:disabled { background-color: #5a6e5a; color: #aaaaaa; }
        QPushButton#btnReset {
            background-color: #e74c3c;
            color: white;
            border: none;
            padding: 6px 20px;
        }
        QPushButton#btnReset:hover { background-color: #c0392b; }
        QPushButton#btnReset:pressed { background-color: #a93226; }
        QPushButton#btnReset:disabled { background-color: #7a4a4a; color: #aaaaaa; }
        QPushButton#btnUpdate {
            background-color: #3498db;
            color: white;
            border: none;
            padding: 6px 20px;
        }
        QPushButton#btnUpdate:hover { background-color: #2980b9; }
        QPushButton#btnUpdate:pressed { background-color: #1f5a8b; }
        QPushButton#btnUpdate:disabled { background-color: #4a6a7a; color: #aaaaaa; }
        QStatusBar {
            background-color: #1e1e1e;
            color: #cccccc;
        }
        QStatusBar QLabel { color: #cccccc; }
        QCheckBox {
            color: #cccccc;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
    )";
    this->setStyleSheet(style);
}

QMessageBox::StandardButton MainWindow::showHandMessageBox(QMessageBox::Icon icon,
                                                           const QString &title,
                                                           const QString &text,
                                                           QMessageBox::StandardButtons buttons,
                                                           QMessageBox::StandardButton defaultButton)
{
    QString iconSymbol;
    switch (icon)
    {
    case QMessageBox::Information:
        iconSymbol = "ℹ️";
        break;
    case QMessageBox::Warning:
        iconSymbol = "⚠️";
        break;
    case QMessageBox::Critical:
        iconSymbol = "❌";
        break;
    case QMessageBox::Question:
        iconSymbol = "❓";
        break;
    default:
        iconSymbol = "";
    }

    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setMinimumWidth(400);
    dlg.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // 为对话框单独设置简洁样式,避免使用主窗口样式导致布局异常
    dlg.setStyleSheet(R"(
        QDialog { background-color: #2b2b2b; }
        QLabel { color: #cccccc; padding: 0px; font-size: 13px; }
        QPushButton {
            background-color: #3a3f44;
            color: white;
            border: 1px solid #555;
            padding: 5px 14px;
            border-radius: 3px;
            min-width: 65px;
        }
        QPushButton:hover { background-color: #4a4f54; }
        QPushButton:pressed { background-color: #2a2f34; }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);

    QHBoxLayout *topRow = new QHBoxLayout();
    if (!iconSymbol.isEmpty())
    {
        QLabel *iconLabel = new QLabel(iconSymbol);
        iconLabel->setStyleSheet("font-size: 28px; padding-right: 12px; border: none; background: transparent;");
        topRow->addWidget(iconLabel);
    }
    QLabel *textLabel = new QLabel(text);
    textLabel->setTextFormat(Qt::PlainText);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("padding: 6px;");
    topRow->addWidget(textLabel);
    mainLayout->addLayout(topRow);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dlg);
    QDialogButtonBox::StandardButtons stdButtons;
    if (buttons & QMessageBox::Ok)
        stdButtons |= QDialogButtonBox::Ok;
    if (buttons & QMessageBox::Cancel)
        stdButtons |= QDialogButtonBox::Cancel;
    if (buttons & QMessageBox::Yes)
        stdButtons |= QDialogButtonBox::Yes;
    if (buttons & QMessageBox::No)
        stdButtons |= QDialogButtonBox::No;
    if (buttons & QMessageBox::Close)
        stdButtons |= QDialogButtonBox::Close;
    if (buttons & QMessageBox::Abort)
        stdButtons |= QDialogButtonBox::Abort;
    if (buttons & QMessageBox::Retry)
        stdButtons |= QDialogButtonBox::Retry;
    if (buttons & QMessageBox::Ignore)
        stdButtons |= QDialogButtonBox::Ignore;
    buttonBox->setStandardButtons(stdButtons);

    switch (defaultButton)
    {
    case QMessageBox::Ok:
        if (buttonBox->button(QDialogButtonBox::Ok))
            buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
        break;
    case QMessageBox::Yes:
        if (buttonBox->button(QDialogButtonBox::Yes))
            buttonBox->button(QDialogButtonBox::Yes)->setDefault(true);
        break;
    case QMessageBox::No:
        if (buttonBox->button(QDialogButtonBox::No))
            buttonBox->button(QDialogButtonBox::No)->setDefault(true);
        break;
    case QMessageBox::Cancel:
        if (buttonBox->button(QDialogButtonBox::Cancel))
            buttonBox->button(QDialogButtonBox::Cancel)->setDefault(true);
        break;
    default:
        break;
    }

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    for (QAbstractButton *btn : buttonBox->buttons())
    {
        btn->setCursor(Qt::PointingHandCursor);
    }

    int ret = dlg.exec();

    if (ret == QDialog::Accepted)
    {
        if (buttons & QMessageBox::Ok)
            return QMessageBox::Ok;
        if (buttons & QMessageBox::Yes)
            return QMessageBox::Yes;
        return QMessageBox::Ok;
    }
    else
    {
        if (buttons & QMessageBox::Cancel)
            return QMessageBox::Cancel;
        if (buttons & QMessageBox::No)
            return QMessageBox::No;
        return QMessageBox::Cancel;
    }
}

void MainWindow::setBusyState(bool busy, const QString &statusMsg)
{
    if (busy)
    {
        m_btnStart->setEnabled(false);
        m_btnStart->setText("汉化中…");
        m_btnReset->setEnabled(false);
        m_btnUpdate->setEnabled(false);
        m_btnBrowseBase->setEnabled(false);
        m_btnBrowseJson->setEnabled(false);
        m_editBasePath->setEnabled(false);
        m_editJsonPath->setEnabled(false);

        setAllButtonsCursorForbidden();
        m_progressBar->setVisible(true);
    }
    else
    {
        m_btnStart->setEnabled(true);
        m_btnStart->setText("汉化");
        m_btnReset->setEnabled(true);
        m_btnUpdate->setEnabled(true);
        m_btnBrowseBase->setEnabled(true);
        m_btnBrowseJson->setEnabled(true);
        m_editBasePath->setEnabled(true);
        m_editJsonPath->setEnabled(true);

        setAllButtonsCursorNormal();
        m_progressBar->setVisible(false);
    }

    QString msg = statusMsg.isEmpty() ? (busy ? "处理中…" : "就绪") : statusMsg;
    m_statusLabel->setText(msg);
}

void MainWindow::onBrowseBasePath()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择 GitHub Desktop 的 app 资源目录");
    if (dir.isEmpty())
        return;
    fs::path p = dir.toStdString();
    if (fs::exists(p / "index.html"))
    {
        m_basePath = p;
        m_editBasePath->setText(dir);
        appendLog(QString("资源目录已设置为：%1").arg(dir));
    }
    else
    {
        showHandMessageBox(QMessageBox::Warning, "无效目录",
                           "所选目录下未找到 index.html,请检查是否选择了正确的 resources/app 目录;");
    }
}

void MainWindow::onBrowseJsonPath()
{
    QString file = QFileDialog::getOpenFileName(this, "选择本地化 JSON 文件", QString(), "JSON 文件 (*.json)");
    if (!file.isEmpty())
    {
        m_jsonPath = file.toStdString();
        m_editJsonPath->setText(file);
        appendLog(QString("已选择本地化文件：%1").arg(file));
    }
}

void MainWindow::autoDetectLocalJson()
{
    if (!m_editJsonPath->text().isEmpty())
        return;
    QString defaultPath = QDir(QApplication::applicationDirPath()).filePath("localization.json");
    if (QFile::exists(defaultPath))
    {
        m_jsonPath = defaultPath.toStdString();
        m_editJsonPath->setText(defaultPath);
        appendLog(QString("检测到汉化映射文件：%1").arg(defaultPath));
    }
}

bool MainWindow::syncAndValidateBasePath()
{
    QString pathStr = m_editBasePath->text().trimmed();
    if (pathStr.isEmpty())
    {
        showHandMessageBox(QMessageBox::Critical, "错误",
                           "资源目录不能为空,请选择或输入 GitHub Desktop 资源目录;");
        return false;
    }
    fs::path p = pathStr.toStdString();
    if (!fs::exists(p) || !fs::exists(p / "index.html"))
    {
        showHandMessageBox(QMessageBox::Critical, "错误",
                           "所选目录无效,请确保目录中包含 index.html;");
        return false;
    }
    m_basePath = p;
    m_editBasePath->setText(QString::fromStdString(m_basePath.string()));
    return true;
}

void MainWindow::onStartConversion()
{
    if (!syncAndValidateBasePath())
        return;
    if (IsGitHubDesktopRunning())
    {
        showHandMessageBox(QMessageBox::Warning, "提示",
                           "检测到 GitHub Desktop 正在运行,请先关闭后再汉化;");
        appendLog("错误：GitHub Desktop 正在运行,汉化中止;");
        return;
    }

    QStringList options;
    QList<std::function<void()>> actions;

    bool canFast = !m_targetGhVersion.empty() && hasPrebuiltVersion();
    if (canFast)
    {
        options << QString("⚡ 快速汉化（1秒完成,版本 %1）").arg(m_targetGhVersion.c_str());
        actions << [this]()
        {
            appendLog(QString("\n******开始快速汉化,目标版本：%1").arg(m_targetGhVersion.c_str()));
            setBusyState(true, "正在快速汉化…");
            runTaskInThread([this]()
                            { doDirectReplacement(); });
        };
    }
    options << "📁 自定义汉化（使用本地 JSON 文件）";
    actions << [this]()
    {
        QString displayedPath = m_editJsonPath->text().trimmed();
        if (displayedPath.isEmpty())
        {
            showHandMessageBox(QMessageBox::Warning, "提示",
                               "请先在“JSON 路径”输入框中填写或浏览选择一个本地 JSON 文件;");
            return;
        }
        QFileInfo fileInfo(displayedPath);
        if (!fileInfo.exists())
        {
            showHandMessageBox(QMessageBox::Warning, "文件不存在",
                               QString("指定的文件不存在,请重新选择;\n%1").arg(displayedPath));
            return;
        }
        if (!readLocalizationFromFile(displayedPath.toStdString()))
        {
            showHandMessageBox(QMessageBox::Critical, "错误",
                               "指定的 JSON 文件无法解析,请检查格式;");
            return;
        }
        if (!showSelectItemsDialog())
        {
            appendLog("用户取消了汉化流程;");
            return;
        }
        m_jsonPath = displayedPath.toStdString();
        appendLog(QString("\n******开始自定义汉化,JSON 来源：%1").arg(displayedPath));
        setBusyState(true, "正在自定义汉化…");
        runTaskInThread([this]()
                        { doCustomConversion(); });
    };
    options << "🧩 自定义汉化（使用内置 JSON,离线可用）";
    actions << [this]()
    {
        if (!loadInternalLocalization())
        {
            showHandMessageBox(QMessageBox::Critical, "错误",
                               "内置 localization.json 加载失败;");
            appendLog("错误：内置 JSON 加载失败;");
            return;
        }
        if (!showSelectItemsDialog())
        {
            appendLog("用户取消了汉化流程;");
            return;
        }
        m_jsonPath.clear();
        m_editJsonPath->clear();
        appendLog("\n******开始使用内置 JSON映射 汉化;");
        setBusyState(true, "正在自定义汉化…");
        runTaskInThread([this]()
                        { doCustomConversion(); });
    };

    QDialog dlg(this);
    dlg.setWindowTitle("选择汉化方式");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QLabel *label = new QLabel("请选择汉化方式：");
    layout->addWidget(label);
    for (int i = 0; i < options.size(); ++i)
    {
        QPushButton *btn = new QPushButton(options[i]);
        btn->setMinimumHeight(40);
        layout->addWidget(btn);
        QObject::connect(btn, &QPushButton::clicked, [&dlg, action = actions[i]]()
                         { action(); dlg.accept(); });
    }
    QPushButton *cancelBtn = new QPushButton("取消");
    layout->addWidget(cancelBtn);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    const QList<QPushButton *> dlgButtons = dlg.findChildren<QPushButton *>();
    for (QPushButton *btn : dlgButtons)
    {
        btn->setCursor(Qt::PointingHandCursor);
    }

    dlg.exec();
}

void MainWindow::onResetToEnglish()
{
    if (!syncAndValidateBasePath())
        return;
    auto reply = showHandMessageBox(QMessageBox::Question, "确认",
                                    "将从备份文件恢复原始英文界面,确定吗？",
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;
    appendLog("\n******开始恢复英文界面...");
    setBusyState(true, "正在恢复英文,请稍候…");
    runTaskInThread([this]()
                    { doRollback(); });
}

void MainWindow::onCheckUpdate()
{
    appendLog("\n******正在检查更新...");
    setBusyState(true, "正在检查更新…");

    auto manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl("https://api.github.com/repos/OpenFlun/GH-DesktopToCN/releases?per_page=1"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
            {
        reply->deleteLater();
        setBusyState(false);

        QString remoteVer = "获取失败";
        QString htmlUrl = "https://github.com/OpenFlun/GH-DesktopToCN/releases";

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            auto j = nlohmann::json::parse(data.toStdString(), nullptr, false);
            if (!j.is_discarded() && j.is_array() && !j.empty()) {
                auto release = j[0];
                if (release.contains("tag_name")) {
                    remoteVer = QString::fromStdString(release["tag_name"].get<std::string>());
                    htmlUrl   = QString::fromStdString(release.value("html_url", htmlUrl.toStdString()));
                }
            }
        } else {
            appendLog(QString("检查更新网络错误：%1").arg(reply->errorString()));
        }

        appendLog(QString("检查更新完成,远程版本：%1,当前版本：%2")
                      .arg(remoteVer, m_currentVersion));

        if (remoteVer == "获取失败") {
            showHandMessageBox(QMessageBox::Warning, "检查更新",
                               "无法连接到服务器,请检查网络;");
            return;
        }

        QVersionNumber remote = QVersionNumber::fromString(remoteVer);
        QVersionNumber current = QVersionNumber::fromString(m_currentVersion);

        if (remote > current) {
            QMessageBox::StandardButton btn = showHandMessageBox(
                QMessageBox::Information, "检查更新",
                QString("发现新版本！\n\n远程版本：%1\n当前版本：%2\n\n是否前往下载页面？")
                    .arg(remoteVer, m_currentVersion),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::Yes);
            if (btn == QMessageBox::Yes) {
                QDesktopServices::openUrl(QUrl(htmlUrl));
            }
        } else {
            showHandMessageBox(QMessageBox::Information, "检查更新",
                               QString("当前已是最新版本;\n\n远程版本：%1\n当前版本：%2")
                                   .arg(remoteVer, m_currentVersion));
        } });
}

void MainWindow::detectInstallPath()
{
    m_targetGhVersion.clear();
    winreg::RegKey key;
    winreg::RegResult res = key.TryOpen(HKEY_CURRENT_USER,
                                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\GitHubDesktop");
    if (res)
    {
        try
        {
            std::wstring ver = key.GetStringValue(L"DisplayVersion");
            std::wstring path = key.GetStringValue(L"InstallLocation");
            m_targetGhVersion = QString::fromStdWString(ver).toStdString();
            fs::path appDir = fs::path(path) / (L"app-" + ver) / L"resources\\app";
            if (fs::exists(appDir))
            {
                m_basePath = appDir;
                m_editBasePath->setText(QString::fromStdString(m_basePath.string()));
                appendLog(QString("自动检测到 GitHub Desktop 安装路径：%1,版本：%2")
                              .arg(QString::fromStdString(m_basePath.string()))
                              .arg(m_targetGhVersion.c_str()));
                return;
            }
        }
        catch (...)
        {
        }
    }
    appendLog("未能自动检测到 GitHub Desktop 安装路径,请手动指定;");
}

void MainWindow::runTaskInThread(std::function<void()> task)
{
    if (m_workerThread)
    {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
    }
    m_workerThread = QThread::create(task);
    connect(m_workerThread, &QThread::finished, this, [this]()
            { setBusyState(false, "就绪"); });
    m_workerThread->start();
}

static bool IsGitHubDesktopRunning()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, L"GitHubDesktop.exe") == 0)
            {
                found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return found;
}

// ========== 辅助检查 / 备份 / 修补 ==========
bool MainWindow::ensureBaseFilesExist()
{
    if (!fs::exists(m_basePath / "main.js") || !fs::exists(m_basePath / "renderer.js"))
    {
        appendLog("错误：目标目录缺少 main.js 或 renderer.js;");
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Critical, "错误",
                                                       "目标目录缺少 main.js 或 renderer.js;"); });
        return false;
    }
    return true;
}

void MainWindow::backupJsFiles()
{
    auto backupIfNeeded = [&](const std::string &name)
    {
        fs::path src = m_basePath / name;
        fs::path bak = m_basePath / (name + ".bak");
        if (!fs::exists(bak))
        {
            fs::copy_file(src, bak);
            appendLog(QString("已备份 %1 -> %1.bak").arg(QString::fromStdString(name)));
        }
        else
        {
            appendLog(QString("%1.bak 已存在,跳过备份;").arg(QString::fromStdString(name)));
        }
    };
    backupIfNeeded("main.js");
    backupIfNeeded("renderer.js");
}

void MainWindow::patchRendererCss()
{
    fs::path cssPath = m_basePath / "renderer.css";
    if (!fs::exists(cssPath))
    {
        appendLog("renderer.css 不存在,跳过 CSS 处理;");
        return;
    }
    appendLog("正在检查 renderer.css ...");
    std::string css = utils::ReadFile(cssPath.string());
    if (css.find("white-space: pre-wrap !important;") == std::string::npos)
    {
        css = "* {\n    white-space: pre-wrap !important;\n}\n" + css;
        utils::WriteFile(cssPath.string(), css);
        appendLog("已修改 renderer.css（添加 white-space 样式）;");
    }
    else
    {
        appendLog("renderer.css 已包含 white-space 样式,无需修改;");
    }
}

// ========== 核心汉化流程 ==========
void MainWindow::doCustomConversion()
{
    if (m_localizationJson.empty())
    {
        appendLog("错误：翻译数据为空,汉化中止;");
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Critical, "错误",
                                                       "翻译数据为空,汉化中止;"); });
        return;
    }

    if (!ensureBaseFilesExist())
        return;

    backupJsFiles();
    patchRendererCss();

    // 基础替换
    bool ok1 = replaceFile(m_basePath / "main.js", "main");
    bool ok2 = replaceFile(m_basePath / "renderer.js", "renderer");

    if (!ok1 || !ok2)
    {
        if (!ok1)
            appendLog("错误：main.js 基础替换失败;");
        if (!ok2)
            appendLog("错误：renderer.js 基础替换失败;");
    }

    // 用户选择的增强项
    if (!m_selectEnables.empty())
    {
        processSelectItems("main.js", m_selectEnables);
        processSelectItems("renderer.js", m_selectEnables);
        appendLog("已应用用户选择的优化项;");
    }

    if (ok1 && ok2)
    {
        appendLog("main.js 和 renderer.js 替换成功;");
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Information, "完成",
                                                       "汉化完成,重新启动 GitHub Desktop 即可生效;"); });
    }
    else
    {
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Warning, "部分失败",
                                                       "部分替换出现问题,请查看日志;"); });
    }
}

void MainWindow::doDirectReplacement()
{
    std::string mainContent, rendererContent;
    if (!loadPrebuiltJs(m_targetGhVersion, mainContent, rendererContent))
    {
        appendLog("快速汉化失败：无法加载预汉化文件;");
        QMetaObject::invokeMethod(this, [this]()
                                  {
            showHandMessageBox(QMessageBox::Warning, "快速汉化失败",
                               "无法加载内置预汉化文件,请尝试自定义汉化;");
            setBusyState(false, "就绪"); });
        return;
    }

    if (!ensureBaseFilesExist())
        return;

    backupJsFiles();
    patchRendererCss();

    utils::WriteFile((m_basePath / "main.js").string(), mainContent);
    utils::WriteFile((m_basePath / "renderer.js").string(), rendererContent);
    appendLog("已写入预汉化的 main.js 和 renderer.js;");

    appendLog("快速汉化完成;");
    QMetaObject::invokeMethod(this, [this]()
                              { showHandMessageBox(QMessageBox::Information, "完成",
                                                   "汉化完成！重启 GitHub Desktop 即可生效;"); });
}

void MainWindow::doRollback()
{
    if (IsGitHubDesktopRunning())
    {
        appendLog("错误：恢复失败,GitHub Desktop 正在运行;");
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Warning, "提示",
                                                       "检测到 GitHub Desktop 正在运行,请先关闭后再恢复;"); });
        return;
    }

    fs::path mainJs = m_basePath / "main.js";
    fs::path mainBak = m_basePath / "main.js.bak";
    fs::path rendererJs = m_basePath / "renderer.js";
    fs::path rendererBak = m_basePath / "renderer.js.bak";

    bool ok = true;
    if (fs::exists(mainBak))
    {
        if (fs::exists(mainJs))
            fs::remove(mainJs);
        fs::copy_file(mainBak, mainJs);
        appendLog("从备份恢复 main.js;");
    }
    else
    {
        ok = false;
        appendLog("错误：main.js.bak 备份文件缺失;");
    }
    if (fs::exists(rendererBak))
    {
        if (fs::exists(rendererJs))
            fs::remove(rendererJs);
        fs::copy_file(rendererBak, rendererJs);
        appendLog("从备份恢复 renderer.js;");
    }
    else
    {
        ok = false;
        appendLog("错误：renderer.js.bak 备份文件缺失;");
    }

    if (ok)
    {
        appendLog("英文界面恢复完成;");
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Information, "完成",
                                                       "已成功恢复英文界面;"); });
    }
    else
    {
        QMetaObject::invokeMethod(this, [this]()
                                  { showHandMessageBox(QMessageBox::Warning, "部分失败",
                                                       "部分文件恢复失败,请检查备份文件是否完整;"); });
    }
}

bool MainWindow::loadInternalLocalization()
{
    QFile file(":/prebuilt/localization.json");
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QByteArray data = file.readAll();
    file.close();
    try
    {
        m_localizationJson = nlohmann::json::parse(data.toStdString());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool MainWindow::readLocalizationFromFile(const fs::path &path)
{
    std::ifstream f(path);
    if (!f)
        return false;
    try
    {
        m_localizationJson = nlohmann::json::parse(f);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool MainWindow::replaceFile(const fs::path &filePath, const std::string &jsonKey)
{
    std::string content = utils::ReadFile(filePath.string());
    try
    {
        auto &items = m_localizationJson[jsonKey];
        std::vector<std::vector<std::string>> rules;
        for (auto &item : items.items())
        {
            if (!item.value().is_array() || item.value().size() < 2)
                continue;
            std::string regexStr = item.value()[0].get<std::string>();
            if (regexStr.empty() || regexStr == "\"\"")
                continue;
            std::vector<std::string> rule;
            rule.push_back(regexStr);
            rule.push_back(item.value()[1].get<std::string>());
            if (item.value().size() >= 3)
                rule.push_back(item.value()[2].get<std::string>());
            rules.push_back(std::move(rule));
        }
        content = applyReplaceRules(content, rules);
        utils::WriteFile(filePath.string(), content);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool MainWindow::hasPrebuiltVersion() const
{
    return m_prebuiltVersions.find(m_targetGhVersion) != m_prebuiltVersions.end();
}

bool MainWindow::loadPrebuiltJs(const std::string &version,
                                std::string &mainJsContent,
                                std::string &rendererJsContent)
{
    QString mainPath = QString(":/prebuilt/%1/main.js").arg(QString::fromStdString(version));
    QString rendererPath = QString(":/prebuilt/%1/renderer.js").arg(QString::fromStdString(version));
    QFile fMain(mainPath);
    QFile fRenderer(rendererPath);
    if (!fMain.open(QIODevice::ReadOnly))
        return false;
    mainJsContent = fMain.readAll().toStdString();
    fMain.close();
    if (!fRenderer.open(QIODevice::ReadOnly))
        return false;
    rendererJsContent = fRenderer.readAll().toStdString();
    fRenderer.close();
    return true;
}

void MainWindow::setAllButtonsCursorForbidden()
{
    const QList<QPushButton *> buttons = centralWidget()->findChildren<QPushButton *>();
    for (QPushButton *btn : buttons)
    {
        btn->setCursor(Qt::ForbiddenCursor);
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)btn->winId();
        HCURSOR hCur = LoadCursor(NULL, IDC_NO);
        if (hCur)
            SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR)hCur);
#endif
    }
}

void MainWindow::setAllButtonsCursorNormal()
{
    const QList<QPushButton *> buttons = centralWidget()->findChildren<QPushButton *>();
    for (QPushButton *btn : buttons)
    {
        btn->setCursor(Qt::PointingHandCursor);
#ifdef Q_OS_WIN
        HWND hwnd = (HWND)btn->winId();
        SetClassLongPtr(hwnd, GCLP_HCURSOR, NULL);
#endif
    }
}

void MainWindow::appendLog(const QString &text)
{
    QMetaObject::invokeMethod(this, [this, text]()
                              {
        m_logView->appendPlainText(text);
        QScrollBar *sb = m_logView->verticalScrollBar();
        if (sb)
            sb->setValue(sb->maximum()); }, Qt::QueuedConnection);
}

// ========== 选择项对话框与处理 ==========
bool MainWindow::showSelectItemsDialog()
{
    m_selectEnables.clear();

    if (!m_localizationJson.contains("select") || !m_localizationJson["select"].is_array())
        return true;

    auto &selectArr = m_localizationJson["select"];
    if (selectArr.empty())
        return true;

    QDialog dlg(this);
    dlg.setWindowTitle("选择要应用的汉化优化项");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel("以下为可选的汉化增强功能（如预览版特性等）,请勾选需要启用的项：");
    hint->setStyleSheet("color: #cccccc; margin-bottom: 8px;");
    layout->addWidget(hint);

    std::vector<QCheckBox *> checkBoxes;
    for (auto &item : selectArr.items())
    {
        if (!item.value().is_object())
            continue;
        std::string tooltip = item.value().value("tooltip", "未命名选项");
        bool initiallyEnabled = item.value().value("enable", false);

        QCheckBox *cb = new QCheckBox(QString::fromStdString(tooltip));
        cb->setChecked(initiallyEnabled);
        layout->addWidget(cb);
        checkBoxes.push_back(cb);
    }

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("确定");
    QPushButton *btnCancel = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    QObject::connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    for (QPushButton *btn : dlg.findChildren<QPushButton *>())
        btn->setCursor(Qt::PointingHandCursor);

    if (dlg.exec() == QDialog::Accepted)
    {
        for (auto cb : checkBoxes)
            m_selectEnables.push_back(cb->isChecked());
        return true;
    }
    else
    {
        m_selectEnables.clear();
        return false;
    }
}

bool MainWindow::processSelectItems(const std::string &jsType, const std::vector<bool> &enabled)
{
    if (!m_localizationJson.contains("select") || !m_localizationJson["select"].is_array())
        return true;

    std::string filePath = (jsType == "main.js") ? (m_basePath / "main.js").string()
                                                 : (m_basePath / "renderer.js").string();
    std::string content = utils::ReadFile(filePath);
    std::vector<std::vector<std::string>> rules;

    size_t idx = 0;
    for (auto &item : m_localizationJson["select"].items())
    {
        if (idx >= enabled.size() || !enabled[idx])
        {
            ++idx;
            continue;
        }
        ++idx;

        const auto &sel = item.value();
        if (!sel.is_object())
            continue;

        std::string targetFile = sel.value("replaceFile", "");
        if (targetFile != jsType)
            continue;

        if (!sel.contains("replace") || !sel["replace"].is_array())
            continue;

        std::vector<std::vector<std::string>> replaces =
            sel["replace"].get<std::vector<std::vector<std::string>>>();

        for (auto &r : replaces)
        {
            if (r.empty())
                continue;
            std::string regexStr = r[0];
            if (regexStr.empty() || regexStr == "\"\"")
                continue;
            // 收集规则,统一处理
            std::vector<std::string> rule;
            rule.push_back(regexStr);
            if (r.size() > 1)
                rule.push_back(r[1]);
            else
                rule.push_back("");
            if (r.size() >= 3 && !r[2].empty())
                rule.push_back(r[2]);
            rules.push_back(std::move(rule));
        }
    }

    if (rules.empty())
        return true;

    std::string newContent = applyReplaceRules(content, rules);
    if (newContent != content)
    {
        utils::WriteFile(filePath, newContent);
        appendLog(QString("已应用 %1 的 select 优化项").arg(QString::fromStdString(jsType)));
    }
    return true;
}

// ========== 应用程序入口 ==========
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}