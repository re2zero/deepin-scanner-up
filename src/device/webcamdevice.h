#ifndef WEBCAMDEVICE_H
#define WEBCAMDEVICE_H

#include <QObject>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <linux/videodev2.h>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "devicebase.h"

// 自定义预览窗口类
class PreviewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    void updateFrame(const QImage &frame);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QImage m_currentFrame;
};

class WebcamDevice : public DeviceBase
{
    Q_OBJECT

public:
    explicit WebcamDevice(QObject *parent = nullptr);
    ~WebcamDevice() override;

    // DeviceBase interface implementation
    bool initialize() override;
    QStringList getAvailableDevices() override;
    bool openDevice(const QString &devicePath) override;
    void closeDevice() override;
    void startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    DeviceType getDeviceType() const override { return DeviceType::Webcam; }
    QString currentDeviceName() const override { return m_currentDeviceName; }

    // Extended webcam-specific interface
    void startPreview();
    void stopPreview();

    QWidget *getVideoWidget();
    QSize getMaxResolution();
    bool setResolution(int width, int height);
    QList<QSize> getSupportedResolutions();
    QSize getCurrentResolution() const { return QSize(m_width, m_height); }
    QImage getLatestFrame();

    // 添加分辨率到支持列表
    void addSupportedResolution(const QSize &resolution)
    {
        if (!m_supportedResolutions.contains(resolution)) {
            m_supportedResolutions.append(resolution);
            // 按分辨率大小排序
            std::sort(m_supportedResolutions.begin(), m_supportedResolutions.end(),
                      [](const QSize &a, const QSize &b) {
                          return a.width() * a.height() < b.width() * b.height();
                      });
            emit resolutionsChanged(m_supportedResolutions);
        }
    }

    // 设置摄像头控制参数
    bool setCameraControl(uint32_t controlId, int value);
    void listCameraControls();
    bool adjustCommonCameraSettings();

    // 常用控制参数设置的快捷方法（可选）
    bool setCameraBrightness(int value);
    bool setCameraContrast(int value);
    bool setCameraExposure(int value);
    bool setCameraAutoExposure(bool enable);

signals:
    void captureStarted();
    // void scanCompleted(const QImage &image);
    // void captureError(const QString &error);
    // 新增信号
    void resolutionsChanged(const QList<QSize> &resolutions);

private slots:
    void updatePreview();

private:
    int m_fd;   // 设备文件描述符
    void *m_buffers[4];   // 视频缓冲区
    size_t m_bufferSizes[4];   // 缓冲区大小
    int m_currentBuffer;   // 当前使用的缓冲区
    bool m_isInitialized;
    bool m_deviceSelected;
    PreviewWidget *m_previewWidget;
    QTimer m_previewTimer;
    int m_width;
    int m_height;
    int m_pixelFormat;
    // 新增成员变量
    QList<QSize> m_supportedResolutions;
    // 用于存储最新的预览帧
    QImage m_latestFrame;
    QMutex m_frameMutex;   // 保护m_latestFrame的互斥锁

    bool initMmap();
    void uninitMmap();
    QImage frameToQImage(const void *data, int width, int height, int format);
    void captureImage();
    bool startCapturing();
    void stopCapturing();
    bool selectBestPixelFormat();
    void diagnoseCameraIssues();
    // 新增私有方法
    void enumerateSupportedResolutions();
};

#endif   // WEBCAMDEVICE_H
