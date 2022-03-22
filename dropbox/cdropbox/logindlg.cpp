#include "logindlg.h"
#include "ui_logindlg.h"

using namespace std;
extern bool Login(string username, string passwd);
extern bool Register(string username, string passwd);

LoginDlg::LoginDlg(QWidget *parent) : QDialog(parent),
                                      ui(new Ui::LoginDlg)
{
    ui->setupUi(this);
}

LoginDlg::~LoginDlg()
{
    delete ui;
}

void LoginDlg::on_btnLogin_clicked()
{
    // 判断用户名和密码是否正确，
    // 如果错误则弹出警告对话框
    // 在属性编辑器中将echoMode属性选择为Password
    string username = ui->lingEditUsername->text().toStdString();
    string password = ui->lineEditPasswd->text().toStdString();

    if (Login(username, password))
    {
        accept();
    }
    else
    {
        QMessageBox::warning(this, tr("Waring"),
                             tr("user name or password error!"),
                             QMessageBox::Yes);
    }
}

void LoginDlg::on_btnRegister_clicked()
{
    string username = ui->lingEditUsername->text().toStdString();
    string password = ui->lineEditPasswd->text().toStdString();

    if (Register(username, password))
    {
        QMessageBox::warning(this, tr("Waring"),
                             tr("register success!"),
                             QMessageBox::Yes);
    }
    else
    {
        QMessageBox::warning(this, tr("Waring"),
                             tr("error user name!"),
                             QMessageBox::Yes);
    }
}
