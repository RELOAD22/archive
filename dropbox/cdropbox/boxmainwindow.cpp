#include "boxmainwindow.h"
#include "./ui_boxmainwindow.h"

using namespace std;
bool logout_flag = false;

extern vector<string> OpenDir();
extern void DownloadFile(string path);
extern void SendFile(string path);
extern void DeleteFile(string path);
extern void DeleteAccount();
extern void KeepAlive();

BoxMainWindow::BoxMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::BoxMainWindow)
{
    ui->setupUi(this);
    // tcpclient_init();
    this->startTimer(600 * 1000);
}

BoxMainWindow::~BoxMainWindow()
{
    delete ui;
}

void BoxMainWindow::on_btnUpload_clicked()
{
    //选择单个文件
    QString curPath = QDir::currentPath(); //获取系统当前目录
    //获取应用程序的路径
    QString dlgTitle = "选择一个文件";                                              //对话框标题
    QString filter = "文本文件(*.txt);;图片文件(*.jpg *.gif *.png);;所有文件(*.*)"; //文件过滤器
    QString aFileName = QFileDialog::getOpenFileName(this, dlgTitle, curPath, filter);
    if (!aFileName.isEmpty())
    {
        // ui->txtEditTest->appendPlainText(aFileName);
        printf("%s\n", (aFileName.toStdString()).c_str());
        SendFile(aFileName.toStdString());
    }
}

void BoxMainWindow::on_btnOpenDir_clicked()
{
    vector<string> file_items;
    QStringList qls;

    ui->listWidgetFiles->clear();
    file_items = OpenDir();
    for (auto item : file_items)
    {
        qls << item.c_str();
    }

    ui->listWidgetFiles->addItems(qls);
}

void BoxMainWindow::on_btnDownload_clicked()
{
    QString qpath = ui->lineEditPath->text();
    string path = qpath.toStdString();
    DownloadFile(path);
}

void BoxMainWindow::closeEvent(QCloseEvent *event)
{
    switch (QMessageBox::information(this, tr("CT Control View"),
                                     tr("Do you really want to log out CT Control View?"),
                                     tr("Yes"), tr("No"), 0, 1))
    {
    case 0:
        // tcpclient_release();
        event->accept();
        break;
    case 1:
    default:
        event->ignore();
        break;
    }
}

void BoxMainWindow::on_btnDelete_clicked()
{
    QString qpath = ui->lineEditPath->text();
    string path = qpath.toStdString();
    DeleteFile(path);
}

void BoxMainWindow::on_btnLogout_clicked()
{
    logout_flag = true;
    close();
}

void BoxMainWindow::on_btnDeleteAccount_clicked()
{
    DeleteAccount();
    logout_flag = true;
    close();
}

void BoxMainWindow::timerEvent(QTimerEvent *e)
{
    QMessageBox::warning(this, tr("Waring"),
                         tr("keepalive!"),
                         QMessageBox::Yes);
    KeepAlive();
}
