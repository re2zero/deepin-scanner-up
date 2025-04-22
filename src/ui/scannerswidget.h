#ifndef SCANNERSWIDGET_H
#define SCANNERSWIDGET_H

#include <DWidget>
#include <QVBoxLayout>
#include <DListWidget>
#include <DPushButton>
#include "device/scannerdevice.h"
#include "device/webcamdevice.h"
#include <QSharedPointer>

#include <DWidget>
#include <DPushButton>
#include <DLabel>
#include <DListWidget>

DWIDGET_USE_NAMESPACE

class ScannersWidget : public DWidget
{
    Q_OBJECT
public:
    explicit ScannersWidget(QWidget *parent = nullptr);
    
    void updateDeviceList(QSharedPointer<ScannerDevice> scanner, QSharedPointer<WebcamDevice> webcam);
    
signals:
    void deviceSelected(const QString &deviceName, bool isScanner);
    void updateDeviceListRequested();
    
private:
    QVBoxLayout *mainLayout;
    QListWidget *deviceList;
    QPushButton *refreshButton;
    
    void setupUI();
    void addDeviceItem(const QString &name, const QString &model, 
                      const QString &status, bool isScanner);
};

#endif // SCANNERSWIDGET_H