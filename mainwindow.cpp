// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>
#include <QInputDialog>
#include <QSettings>
#include <QTextStream>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ==================================================================
// Универсальное логирование
// ==================================================================
void MainWindow::appendLog(const QString &text, bool timestamp)
{
    QString line = timestamp
        ? QString("<span style=\"color:#888888\">[%1]</span> %2")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
              .arg(text.toHtmlEscaped())
        : text.toHtmlEscaped();

    ui->logTextEdit->append(line);
    if (auto *sb = ui->logTextEdit->verticalScrollBar())
        sb->setValue(sb->maximum());
}

// ==================================================================
// Универсальное клонирование репозитория
// ==================================================================
void MainWindow::cloneRepository(const QString &repoUrl,
                                 const QString &folderName,
                                 const QString &displayName,
                                 QPushButton *button)
{
    ui->logTextEdit->clear();
    appendLog(tr("=== Клонирование %1 ===").arg(displayName));

    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Выберите папку для клонирования %1").arg(displayName),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) {
        appendLog(tr("Операция отменена пользователем."));
        return;
    }

    QDir target(dir);
    QString targetPath = target.filePath(folderName);

    appendLog(tr("Цель: %1").arg(QDir::toNativeSeparators(targetPath)));

    // Удаление старой папки, если есть
    if (target.exists(folderName)) {
        int ret = QMessageBox::question(this, tr("Папка уже существует"),
            tr("Удалить существующую папку <b>%1</b> и клонировать заново?").arg(folderName),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            appendLog(tr("Удаление старой папки..."));
            if (!QDir(target.filePath(folderName)).removeRecursively()) {
                appendLog(tr("<font color=\"#ff5555\"><b>Ошибка: не удалось удалить папку!</b></font>"));
                QMessageBox::critical(this, tr("Ошибка"),
                    tr("Не удалось удалить папку. Закройте все программы, которые могут её использовать."));
                return;
            }
            appendLog(tr("Старая папка удалена."));
        } else {
            appendLog(tr("Клонирование отменено пользователем."));
            return;
        }
    }

    appendLog(tr("Запуск: git clone --depth=1 %1 %2").arg(repoUrl, folderName));

    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(dir);

    connect(proc, &QProcess::readyReadStandardOutput, this, [=]() {
        appendLog(proc->readAllStandardOutput().trimmed(), false);
    });
    connect(proc, &QProcess::readyReadStandardError, this, [=]() {
        appendLog("<font color=\"#ff5555\">" + proc->readAllStandardError().trimmed() + "</font>", false);
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int code, QProcess::ExitStatus status) {
        appendLog("");
        if (status == QProcess::NormalExit && code == 0) {
            appendLog(tr("<font color=\"#50fa7b\"><b>%1 успешно клонирован!</b></font>").arg(displayName));
            appendLog(tr("Путь: %1").arg(QDir::toNativeSeparators(targetPath)));
        } else {
            appendLog(tr("<font color=\"#ff5555\"><b>Ошибка клонирования (код %1)</b></font>").arg(code));
        }
        button->setEnabled(true);
        proc->deleteLater();
    });

    button->setEnabled(false);
    proc->start("git", {"clone", "--depth=1", repoUrl, folderName});

    if (!proc->waitForStarted(3000)) {
        appendLog(tr("<font color=\"#ff5555\"><b>Git не найден в системе!</b></font>"));
        QMessageBox::critical(this, tr("Git не найден"),
            tr("Установите Git: <a href=\"https://git-scm.com/downloads\">git-scm.com</a>"));
        button->setEnabled(true);
        proc->deleteLater();
    } else {
        appendLog(tr("Клонирование начато..."));
    }
}

// ==================================================================
// Проверка SSH-доступа к GitHub
// ==================================================================
bool MainWindow::testSshAccess()
{
    QProcess p(this);
    p.start("ssh", QStringList() << "-T" << "git@github.com");
    p.waitForFinished(5000);
    QString output = p.readAllStandardOutput() + p.readAllStandardError();
    return output.contains("successfully authenticated") || p.exitCode() == 1;
}

// ==================================================================
// Универсальный add → commit → push по SSH
// ==================================================================
void MainWindow::gitAddCommitPushSsh(const QString &repoPath, const QString &remoteSshUrl)
{
    if (!testSshAccess()) {
        appendLog(tr("<font color=\"#ff5555\"><b>SSH-доступ к GitHub не настроен!</b></font>"));
        appendLog(tr("Выполните в терминале:"));
        appendLog(tr("  ssh-keygen -t ed25519 -C \"you@example.com\""));
        appendLog(tr("  eval \"$(ssh-agent -s)\""));
        appendLog(tr("  ssh-add ~/.ssh/id_ed25519"));
        appendLog(tr("  cat ~/.ssh/id_ed25519.pub → добавьте на https://github.com/settings/keys"));
        QMessageBox::warning(this, tr("SSH не настроен"),
            tr("SSH-ключи не найдены или не добавлены.\nСм. инструкцию в логе."));
        return;
    }

    // Запрос имени/email (с сохранением)
    QSettings settings;
    QString email = settings.value("git/email").toString();
    QString name  = settings.value("git/name").toString();

    bool ok;
    if (email.isEmpty()) {
        email = QInputDialog::getText(this, tr("Git Email"), tr("Ваш email:"), QLineEdit::Normal, "", &ok);
        if (!ok || email.isEmpty()) return;
        settings.setValue("git/email", email);
    }
    if (name.isEmpty()) {
        name = QInputDialog::getText(this, tr("Git Имя"), tr("Ваше имя:"), QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;
        settings.setValue("git/name", name);
    }

    // Настройка identity в репозитории
    QProcess cfg(this);
    cfg.setWorkingDirectory(repoPath);
    cfg.start("git", {"config", "user.email", email});
    cfg.waitForFinished(3000);
    cfg.start("git", {"config", "user.name", name});
    cfg.waitForFinished(3000);

    appendLog(tr("Git настроен: %1 <%2>").arg(name, email));

    // Асинхронная цепочка команд
    QProcess *proc = new QProcess(this);
    proc->setWorkingDirectory(repoPath);

    int step = 0;
    QStringList steps[3] = {
        {"add", "."},
        {"commit", "-m", "Update changes " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm")},
        {"push", remoteSshUrl}
    };

    auto nextStep = [=]() mutable {
        if (step >= 3) {
            appendLog(tr("<font color=\"#50fa7b\"><b>Все изменения успешно отправлены по SSH!</b></font>"));
            proc->deleteLater();
            return;
        }
        proc->start("git", steps[step]);
        step++;
    };

    connect(proc, &QProcess::readyReadStandardOutput, this, [=]() {
        appendLog(proc->readAllStandardOutput().trimmed(), false);
    });
    connect(proc, &QProcess::readyReadStandardError, this, [=]() {
        appendLog("<font color=\"#ff5555\">" + proc->readAllStandardError().trimmed() + "</font>", false);
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=, this](int code, QProcess::ExitStatus st) mutable {  // ← mutable обязателен!
        if (st != QProcess::NormalExit || code != 0) {
            appendLog(tr("<font color=\"#ff5555\"><b>Ошибка на этапе (код %1)</b></font>").arg(code));
            QString err = proc->readAllStandardError();
            if (err.contains("Permission denied", Qt::CaseInsensitive))
                appendLog(tr("→ SSH-ключ не добавлен или не запущен ssh-agent"));
            proc->deleteLater();
            return;
        }
        nextStep();
    });

    appendLog(tr("Запуск git add → commit → push..."));
    nextStep();
}

// ==================================================================
// Кнопки
// ==================================================================

void MainWindow::on_cloneRepoButton_clicked()
{
    cloneRepository("https://github.com/xinitronix/ssd_log",
                    "ssd", "ssd_log", ui->cloneRepoButton);
}

void MainWindow::on_extraButton1_clicked()
{
    cloneRepository("https://github.com/definitly486/uname",
                    "uname", "uname", ui->extraButton1);
}

void MainWindow::on_pushRepoButton_clicked()
{
    ui->logTextEdit->clear();
    appendLog("=== Push в ssd_log (SSH) ===");

    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите папку с репозиторием ssd_log"));
    if (dir.isEmpty()) return;

    QString repoPath = QDir(dir).filePath("ssd");
    if (!QDir(repoPath).exists()) {
        appendLog(tr("<font color=\"#ff5555\">Папка ssd не найдена!</font>"));
        return;
    }

    gitAddCommitPushSsh(repoPath, "git@github.com:xinitronix/ssd_log.git");
}

void MainWindow::on_extraButton2_clicked()
{
    ui->logTextEdit->clear();
    appendLog("=== Обновление uname (SSH) ===");

    QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите папку с репозиторием uname"));
    if (dir.isEmpty()) return;

    QString repoPath = QDir(dir).filePath("uname");
    if (!QDir(repoPath).exists()) {
        appendLog(tr("<font color=\"#ff5555\">Папка uname не найдена!</font>"));
        return;
    }

    // uname -a
    QProcess p(this);
    p.start("uname", {"-a"});
    p.waitForFinished(3000);
    QString kernelLine = p.readAllStandardOutput().trimmed();

    // Проверка на дубликат
    QFile f(repoPath + "/uname");
    bool alreadyExists = false;
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            if (f.readLine().trimmed() == kernelLine) {
                alreadyExists = true;
                break;
            }
        }
        f.close();
    }

    if (alreadyExists) {
        appendLog(tr("Эта запись уже существует. Пропускаем."));
        return;
    }

    // Добавляем новую строку
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        out << kernelLine << "  # " << QDateTime::currentDateTime().toString() << "\n";
        f.close();
        appendLog(tr("Новая запись добавлена в uname"));
    } else {
        appendLog(tr("<font color=\"#ff5555\">Не удалось записать в файл uname</font>"));
        return;
    }

    gitAddCommitPushSsh(repoPath, "git@github.com:definitly486/uname.git");
}