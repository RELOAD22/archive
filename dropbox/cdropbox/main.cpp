#include "boxmainwindow.h"
#include "logindlg.h"
#include <QApplication>
#include <QTimer>
extern bool logout_flag;
extern int tcpclient_init();
extern int tcpclient_release();

int main(int argc, char *argv[])
{
    tcpclient_init();
    QApplication a(argc, argv);

    LoginDlg dlg;

relogin:
    if (dlg.exec() == QDialog::Accepted)
    {
        BoxMainWindow w;
        w.show();
        a.exec();
        if (logout_flag == true)
        {
            logout_flag = false;
            goto relogin;
        }
    }
    tcpclient_release();

    return 0;
}
