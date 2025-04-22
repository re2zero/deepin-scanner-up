#ifndef SCANWINDOW_H
#define SCANWINDOW_H

#include <DMainWindow>
#include <QGraphicsView>
#include <QPushButton>
#include <QFormLayout>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include "device/scannerdevice.h"
#include "device/webcamdevice.h"

DWIDGET_USE_NAMESPACE

class ScanWindow : public DMainWindow
{
    Q_OBJECT

public:
    explicit ScanWindow(QWidget *parent = nullptr);

signals:
    void backRequested(); // 返回主界面信号

private:
    void initUI();
    void initConnections();
    void updatePreview(const QImage &image);

private slots:
    void handleScanProgress(int percentage);
    void handleScanCompleted(const QImage &scannedImage);
    void handleScanError(const QString &errorMessage);
    void handleCaptureCompleted(const QImage &capturedImage);

private:
    QWidget *m_devicePanel;
    QVBoxLayout *m_deviceLayout;
    QListWidget *m_deviceList;
    QPushButton *m_scanButton;
    QLabel *m_deviceIcon;
    QLabel *m_deviceDesc;
    
    QWidget *m_rightPanel;
    QFormLayout *m_settingsLayout;
    
    ScannerDevice *m_scannerDevice;
    WebcamDevice *m_webcamDevice;

    // 参数设置控件
    QComboBox *m_resolutionCombo;
    QComboBox *m_colorModeCombo;
    QComboBox *m_formatCombo;

    // 参数转换函数
    int convertResolution(const QString &resolutionText);
    QString convertColorMode(const QString &colorModeText);
    QString convertFileFormat(const QString &formatText);

    // 参数验证函数
    bool validateSettings();

    // 偏好保存/加载
    void savePreferences();
    void loadPreferences();
};

#endif // SCANWINDOW_H