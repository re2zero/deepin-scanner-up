#ifndef SCANWIDGET_H
#define SCANWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QSharedPointer>
#include <QTimer>
#include <QMutex>

#include <DLabel>
#include "device/devicebase.h"
#include "device/scannerdevice.h"
#include "device/webcamdevice.h"

DWIDGET_USE_NAMESPACE

class QComboBox;
class ScanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScanWidget(QWidget *parent = nullptr);
    
    void setupDeviceMode(QSharedPointer<DeviceBase> device, QString name);
    void setPreviewImage(const QImage &image);
    void startCameraPreview();
    void stopCameraPreview();

signals:
    void scanRequested();
    void scanFinished(const QImage &image);
    void saveRequested();
    void deviceSettingsChanged();

public slots:
    void startScanning();

private slots:
    void updatePreview();
    void onResolutionChanged(int index);
    void onColorModeChanged(int index);
    void onFormatChanged(int index);
    void onScanModeChanged(int index);
    void onScanFinished(const QImage &image);
    void handleDeviceError(const QString &error);

private:
    void setupUI();
    void connectDeviceSignals(bool bind);
    void updateDeviceSettings();

    QSharedPointer<DeviceBase> m_device;
    bool m_isScanner;
    QTimer m_previewTimer;
    QMutex m_previewMutex;
    
    // preview area
    DLabel *m_previewLabel;

    DLabel *m_modeLabel;
    QComboBox *m_modeCombo;
    QComboBox *m_resolutionCombo;
    QComboBox *m_colorCombo;
    QComboBox *m_formatCombo;
};

#endif // SCANWIDGET_H