#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QScrollBar>
#include <QDateTime>

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

void MainWindow::on_cloneRepoButton_clicked()
{
    // Очищаем лог перед новой операцией
    ui->logTextEdit->clear();
    appendLog(tr("=== Запуск клонирования репозитория ssd_log ==="));
    appendLog("");

    // Выбор папки, куда клонировать
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите папку для клонирования ssd_log"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (dir.isEmpty()) {
        appendLog(tr("Операция отменена пользователем."));
        return;
    }

    QDir targetDir(dir);
    QString targetPath = targetDir.filePath("ssd");

    appendLog(tr("Выбрана папка: %1").arg(QDir::toNativeSeparators(dir)));
    appendLog(tr("Репозиторий будет клонирован в: %1").arg(QDir::toNativeSeparators(targetPath)));
    appendLog("");

    // Проверяем, существует ли уже папка ssd
    if (targetDir.exists("ssd")) {
        int ret = QMessageBox::question(
            this,
            tr("Папка уже существует"),
            tr("Папка <b>ssd</b> уже существует по пути:\n%1\n\n"
               "Удалить её и клонировать репозиторий заново?")
               .arg(QDir::toNativeSeparators(targetPath)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (ret == QMessageBox::Yes) {
            appendLog(tr("Удаление существующей папки ssd..."));

            QDir ssdDir(targetDir.filePath("ssd"));
            if (!ssdDir.removeRecursively()) {
                appendLog(tr("<font color=\"#ff5555\"><b>Ошибка: Не удалось удалить папку ssd!</b></font>"));
                appendLog(tr("Возможно, папка используется другим процессом или нет прав на удаление."));
                QMessageBox::critical(this, tr("Ошибка удаления"),
                    tr("Не удалось удалить существующую папку ssd.\n"
                       "Закройте все программы, использующие эту папку, и попробуйте снова."));
                return;
            }
            appendLog(tr("Существующая папка ssd успешно удалена."));
        } else {
            appendLog(tr("Клонирование отменено пользователем."));
            return;
        }
    }

    appendLog(tr("Запуск команды:"));
    appendLog(tr("git clone --depth=1 https://github.com/xinitronix/ssd_log ssd"));
    appendLog(tr("Ожидайте..."));
    appendLog("");

    QProcess *gitProcess = new QProcess(this);
    gitProcess->setWorkingDirectory(dir);

    // Вывод стандартного потока (прогресс git)
    connect(gitProcess, &QProcess::readyReadStandardOutput, this, [=]() {
        QString output = gitProcess->readAllStandardOutput();
        appendLog(output.trimmed(), false);
    });

    // Вывод ошибок — выделяем красным
    connect(gitProcess, &QProcess::readyReadStandardError, this, [=]() {
        QString error = gitProcess->readAllStandardError();
        appendLog(tr("<font color=\"#ff5555\">%1</font>").arg(error.trimmed()), false);
    });

    // Завершение процесса
    connect(gitProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus) mutable {

        appendLog(""); // отступ

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            appendLog(tr("<font color=\"#50fa7b\"><b>Репозиторий успешно клонирован!</b></font>"));
            appendLog(tr("Путь: <b>%1</b>").arg(QDir::toNativeSeparators(targetPath)));
            appendLog(tr("Готово к работе с ssd_log!"));
        } else {
            appendLog(tr("<font color=\"#ff5555\"><b>Ошибка клонирования (код выхода: %1)</b></font>").arg(exitCode));
            appendLog(tr("Проверьте подключение к интернету и наличие Git."));
        }

        ui->cloneRepoButton->setEnabled(true);
        gitProcess->deleteLater();
    });

    // Запуск git clone
    QStringList args = {"clone", "--depth=1", "https://github.com/xinitronix/ssd_log", "ssd"};
    gitProcess->start("git", args);

    if (!gitProcess->waitForStarted(5000)) {
        appendLog(tr("<font color=\"#ff5555\"><b>Критическая ошибка: Git не найден!</b></font>"));
        appendLog(tr("Убедитесь, что Git установлен и добавлен в PATH."));
        QMessageBox::critical(this, tr("Git не найден"),
            tr("Команда 'git' не найдена в системе.\n"
               "Установите Git: https://git-scm.com/downloads"));
        gitProcess->deleteLater();
        return;
    }

    // Блокируем кнопку на время клонирования
    ui->cloneRepoButton->setEnabled(false);
    appendLog(tr("Клонирование начато..."));
}


void MainWindow::on_pushRepoButton_clicked()
{

}



void MainWindow::on_extraButton1_clicked()
{
    // Очищаем лог перед новой операцией
    ui->logTextEdit->clear();
    appendLog(tr("=== Запуск клонирования репозитория uname ==="));
    appendLog("");

    // Выбор папки, куда клонировать
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Выберите папку для клонирования uname"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (dir.isEmpty()) {
        appendLog(tr("Операция отменена пользователем."));
        return;
    }

    QDir targetDir(dir);
    QString targetPath = targetDir.filePath("uname");

    appendLog(tr("Выбрана папка: %1").arg(QDir::toNativeSeparators(dir)));
    appendLog(tr("Репозиторий будет клонирован в: %1").arg(QDir::toNativeSeparators(targetPath)));
    appendLog("");

    // Проверяем, существует ли уже папка uname
    if (targetDir.exists("uname")) {
        int ret = QMessageBox::question(
            this,
            tr("Папка уже существует"),
            tr("Папка <b>uname</b> уже существует по пути:\n%1\n\n"
               "Удалить её и клонировать репозиторий заново?")
               .arg(QDir::toNativeSeparators(targetPath)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (ret == QMessageBox::Yes) {
            appendLog(tr("Удаление существующей папки uname..."));

            QDir unameDir(targetDir.filePath("uname"));
            if (!unameDir.removeRecursively()) {
                appendLog(tr("<font color=\"#ff5555\"><b>Ошибка: Не удалось удалить папку uname!</b></font>"));
                appendLog(tr("Возможно, папка используется другим процессом или нет прав на удаление."));
                QMessageBox::critical(this, tr("Ошибка удаления"),
                    tr("Не удалось удалить существующую папку uname.\n"
                       "Закройте все программы, использующие эту папку, и попробуйте снова."));
                return;
            }
            appendLog(tr("Существующая папка uname успешно удалена."));
        } else {
            appendLog(tr("Клонирование отменено пользователем."));
            return;
        }
    }

    appendLog(tr("Запуск команды:"));
    appendLog(tr("git clone --depth=1 https://github.com/definitly486/uname uname"));
    appendLog(tr("Ожидайте..."));
    appendLog("");

    QProcess *gitProcess = new QProcess(this);
    gitProcess->setWorkingDirectory(dir);

    // Вывод стандартного потока (прогресс git)
    connect(gitProcess, &QProcess::readyReadStandardOutput, this, [=]() {
        QString output = gitProcess->readAllStandardOutput();
        appendLog(output.trimmed(), false);
    });

    // Вывод ошибок — выделяем красным
    connect(gitProcess, &QProcess::readyReadStandardError, this, [=]() {
        QString error = gitProcess->readAllStandardError();
        appendLog(tr("<font color=\"#ff5555\">%1</font>").arg(error.trimmed()), false);
    });

    // Завершение процесса
    connect(gitProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus) mutable {

        appendLog(""); // отступ

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            appendLog(tr("<font color=\"#50fa7b\"><b>Репозиторий успешно клонирован!</b></font>"));
            appendLog(tr("Путь: <b>%1</b>").arg(QDir::toNativeSeparators(targetPath)));
            appendLog(tr("Готово к работе с uname!"));
        } else {
            appendLog(tr("<font color=\"#ff5555\"><b>Ошибка клонирования (код выхода: %1)</b></font>").arg(exitCode));
            appendLog(tr("Проверьте подключение к интернету и наличие Git."));
        }

        ui->extraButton1->setEnabled(true); // восстанавливаем доступность кнопки
        gitProcess->deleteLater();          // освобождаем память
    });

    // Запуск git clone
    QStringList args = {"clone", "--depth=1", "https://github.com/definitly486/uname", "uname"};
    gitProcess->start("git", args);

    if (!gitProcess->waitForStarted(5000)) {
        appendLog(tr("<font color=\"#ff5555\"><b>Критическая ошибка: Git не найден!</b></font>"));
        appendLog(tr("Убедитесь, что Git установлен и добавлен в PATH."));
        QMessageBox::critical(this, tr("Git не найден"),
            tr("Команда 'git' не найдена в системе.\n"
               "Установите Git: https://git-scm.com/downloads"));
        gitProcess->deleteLater();
        return;
    }

    // Блокируем кнопку на время клонирования
    ui->extraButton1->setEnabled(false);
    appendLog(tr("Клонирование начато..."));
}


void MainWindow::on_extraButton2_clicked()
{

}


// Вставь это где-нибудь в mainwindow.cpp (лучше в конец файла, после всех слотов)
void MainWindow::appendLog(const QString &text, bool withTimestamp)
{
    QString line;

    if (withTimestamp) {
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        line = QString("<span style=\"color:#888888\">[%1]</span> %2")
               .arg(time)
               .arg(text.toHtmlEscaped());  // важно! защита от HTML-инъекций
    } else {
        line = text.toHtmlEscaped();
    }

    // Вставляем HTML-текст
    ui->logTextEdit->append(line);

    // Автоматическая прокрутка вниз
    QScrollBar *sb = ui->logTextEdit->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}
