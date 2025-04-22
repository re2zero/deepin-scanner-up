#include "mainwindow.h"


#include <DApplication>
#include <DLog>

#include <unistd.h>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication app(argc, argv);

    // 设置应用信息
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-scanner");
    app.setApplicationDisplayName("Deepin Scanner");
    app.setApplicationVersion(APP_VERSION);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // 设置终端和文件记录日志
    const QString logFormat = "%{time}{yyyyMMdd.HH:mm:ss.zzz}[%{type:1}][%{function:-35} %{line:-4} %{threadid} ] %{message}\n";
    DLogManager::setLogFormat(logFormat);
    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    // 设置单例运行程序
    if (!app.setSingleInstance("deepin-scanner")) {
        qWarning() << "set single instance failed!I (pid:" << getpid() << ") will exit.";
        return -1;
    }

    // 加载翻译
    app.loadTranslator();
    
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}