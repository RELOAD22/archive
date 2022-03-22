#ifndef BOXMAINWINDOW_H
#define BOXMAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BoxMainWindow;
}
QT_END_NAMESPACE

class BoxMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    BoxMainWindow(QWidget *parent = nullptr);
    ~BoxMainWindow();
    virtual void timerEvent(QTimerEvent *e);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_btnUpload_clicked();

    void on_btnOpenDir_clicked();

    void on_btnDownload_clicked();

    void on_btnDelete_clicked();

    void on_btnLogout_clicked();

    void on_btnDeleteAccount_clicked();

private:
    Ui::BoxMainWindow *ui;
};
#endif // BOXMAINWINDOW_H
