// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>  // если используешь указатели на кнопки

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_cloneRepoButton_clicked();
    void on_pushRepoButton_clicked();
    void on_extraButton1_clicked();
    void on_extraButton2_clicked();

private:
    // === Добавь эти объявления! ===
    void appendLog(const QString &text, bool timestamp = true);

    void cloneRepository(const QString &repoUrl,
                         const QString &folderName,
                         const QString &displayName,
                         QPushButton *button);

    bool testSshAccess();

    void gitAddCommitPushSsh(const QString &repoPath,
                             const QString &remoteSshUrl);
    // ==============================

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H