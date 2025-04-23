#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <DIconButton>
#include <QStackedLayout>
#include <QComboBox> // <--- 添加 QComboBox 用于分辨率选择

#include "device/scannerdevice.h"
#include "device/webcamdevice.h"
#include "ui/scannerswidget.h"
#include "ui/scanwidget.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QSharedPointer<DeviceBase> m_currentDevicePtr;
    QMap<QString, QSharedPointer<DeviceBase>> m_devices;
    
    // UI Components
    QStackedLayout *m_stackLayout;
    ScannersWidget *m_scannersWidget;
    ScanWidget *m_scanWidget;
    
    // Current device info
    QString m_currentDevice;
    bool m_isCurrentScanner;
    
private slots:
    void showScanView(const QString &device, bool isScanner);
    void showDeviceListView();
    void updateDeviceList();

private:
    DIconButton *m_backBtn = nullptr;
};

#endif // MAINWINDOW_H