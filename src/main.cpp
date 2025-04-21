#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置应用程序信息
    a.setApplicationName("deepin-scanner");
    a.setApplicationDisplayName(QObject::tr("扫描管理器"));
    
    MainWindow w;
    w.show();

    return a.exec();
}