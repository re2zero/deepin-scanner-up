#include "webcamdevice.h"
#include <QVideoProbe>
#include <QBuffer>
#include <QEventLoop>

QImage WebcamDevice::getPreviewImage()
{
    if (!m_camera || m_camera->status() != QCamera::ActiveStatus) {
        qWarning() << "Cannot get preview - camera not active";
        return QImage();
    }

    // 使用QVideoProbe捕获视频帧
    QVideoProbe probe;
    if (!probe.setSource(m_camera)) {
        qWarning() << "Failed to set up video probe for preview";
        return QImage();
    }

    QImage preview;
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, &QEventLoop::quit); // 超时1秒

    // 连接视频帧信号
    QObject::connect(&probe, &QVideoProbe::videoFrameProbed, [&](const QVideoFrame &frame) {
        QVideoFrame cloneFrame(frame);
        if (cloneFrame.map(QAbstractVideoBuffer::ReadOnly)) {
            QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat());
            if (imageFormat != QImage::Format_Invalid) {
                preview = QImage(cloneFrame.bits(),
                               cloneFrame.width(),
                               cloneFrame.height(),
                               cloneFrame.bytesPerLine(),
                               imageFormat).copy();
            }
            cloneFrame.unmap();
            loop.quit();
        }
    });

    loop.exec();
    return preview;
}
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QDateTime>

WebcamDevice::WebcamDevice(QObject *parent)
    : QObject(parent), m_camera(nullptr), m_imageCapture(nullptr), m_deviceSelected(false), m_captureRetries(0)
{
    // 设置捕获重试定时器
    m_captureTimer.setSingleShot(true);
    connect(&m_captureTimer, &QTimer::timeout, this, &WebcamDevice::tryCapture);
}

WebcamDevice::~WebcamDevice()
{
    closeDevice();
}

QStringList WebcamDevice::getAvailableDevices()
{
    QStringList devices;
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        devices << cameraInfo.description();
        qDebug() << "Found camera:" << cameraInfo.description()
                 << "Position:" << (cameraInfo.position() == QCamera::FrontFace ? "Front" : "Back")
                 << "Orientation:" << cameraInfo.orientation();
    }

    if (devices.isEmpty()) {
        qDebug() << "No cameras found!";
    }

    return devices;
}

QCameraInfo WebcamDevice::findDevice(const QString &description)
{
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        if (cameraInfo.description() == description) {
            return cameraInfo;
        }
    }
    return QCameraInfo();
}

bool WebcamDevice::openDevice(const QString &deviceDescription)
{
    closeDevice();
    m_captureRetries = 0;

    m_selectedDevice = findDevice(deviceDescription);
    if (m_selectedDevice.isNull()) {
        emit captureError(tr("找不到指定的摄像头设备: %1").arg(deviceDescription));
        m_deviceSelected = false;
        return false;
    }

    m_deviceSelected = true;
    qDebug() << "Selected camera:" << m_selectedDevice.description();
    return true;
}

void WebcamDevice::setupCamera()
{
    if (!m_deviceSelected) {
        emit captureError(tr("未选择摄像头设备"));
        return;
    }

    // 创建相机实例
    m_camera = new QCamera(m_selectedDevice, this);
    if (!m_camera) {
        emit captureError(tr("无法创建摄像头实例"));
        return;
    }

    // 创建图像捕获器
    m_imageCapture = new QCameraImageCapture(m_camera, this);
    if (!m_imageCapture) {
        emit captureError(tr("无法创建图像捕获实例"));
        delete m_camera;
        m_camera = nullptr;
        return;
    }

    // 设置捕获设置
    QImageEncoderSettings imageSettings;
    imageSettings.setCodec("image/jpeg");
    imageSettings.setResolution(1280, 720);
    imageSettings.setQuality(QMultimedia::NormalQuality);
    m_imageCapture->setEncodingSettings(imageSettings);

    // 连接所有可能的捕获信号
    connect(m_imageCapture, &QCameraImageCapture::imageCaptured,
            this, &WebcamDevice::handleImageCaptured);

    // 图像已保存信号
    connect(m_imageCapture, &QCameraImageCapture::imageSaved,
            this, [this](int id, const QString &fileName) {
                qDebug() << "Image saved to:" << fileName;
                // 从保存的文件加载图像
                QImage image(fileName);
                if (!image.isNull()) {
                    emit captureCompleted(image);
                    // 可选：删除临时文件
                    QFile::remove(fileName);
                } else {
                    emit captureError(tr("无法加载已保存的图像"));
                }
            });

    // 错误处理
    connect(m_imageCapture, static_cast<void (QCameraImageCapture::*)(int, QCameraImageCapture::Error, const QString &)>(&QCameraImageCapture::error),
            this, &WebcamDevice::handleCaptureError);

    connect(m_camera, static_cast<void (QCamera::*)(QCamera::Error)>(&QCamera::error),
            this, &WebcamDevice::handleCameraError);

    // 状态变化处理
    connect(m_camera, &QCamera::statusChanged,
            this, &WebcamDevice::handleCameraStatusChanged);

    connect(m_camera, &QCamera::stateChanged,
            this, &WebcamDevice::handleCameraStateChanged);

    // 设置捕获模式
    m_camera->setCaptureMode(QCamera::CaptureStillImage);

    // 确保捕获到文件
    m_imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToFile);

    qDebug() << "Camera setup completed";
    qDebug() << "Supported capture destinations:" << m_imageCapture->isCaptureDestinationSupported(QCameraImageCapture::CaptureToFile);
    qDebug() << "Capture destination:" << m_imageCapture->captureDestination();
}

void WebcamDevice::captureImage()
{
    qDebug() << "captureImage called";
    cleanupCamera();
    setupCamera();

    if (!m_camera || !m_imageCapture) {
        qDebug() << "Failed to initialize camera or image capture!";
        emit captureError(tr("摄像头未正确初始化"));
        return;
    }

    m_captureRetries = 0;
    emit captureStarted();

    // 打印相机信息
    qDebug() << "Starting camera:" << m_selectedDevice.description();
    qDebug() << "Camera location:" << m_selectedDevice.position();
    qDebug() << "Camera orientation:" << m_selectedDevice.orientation();

    // 设置捕获模式
    m_camera->setCaptureMode(QCamera::CaptureStillImage);

    // 启动相机
    qDebug() << "Starting camera...";
    m_camera->start();

    // 检查相机是否成功启动
    QTimer::singleShot(100, this, [this]() {
        qDebug() << "Camera state after start:" << m_camera->state();
        qDebug() << "Camera status after start:" << m_camera->status();
    });
}

void WebcamDevice::tryCapture()
{
    if (!m_camera || !m_imageCapture) {
        emit captureError(tr("摄像头或捕获器无效"));
        return;
    }

    if (m_captureRetries >= MAX_CAPTURE_RETRIES) {
        emit captureError(tr("达到最大重试次数，捕获失败"));
        closeDevice();
        return;
    }

    // 检查相机状态
    if (m_camera->status() != QCamera::ActiveStatus) {
        qDebug() << "Camera not active, current status:" << m_camera->status();
        m_captureRetries++;
        m_captureTimer.start(1000);
        return;
    }

    // 检查是否准备好捕获
    if (!m_imageCapture->isReadyForCapture()) {
        qDebug() << "Camera not ready for capture";
        m_captureRetries++;
        m_captureTimer.start(1000);
        return;
    }

    qDebug() << "Attempting to capture image... (Attempt" << m_captureRetries + 1
             << "of" << MAX_CAPTURE_RETRIES << ")";

    // 锁定相机设置
    m_camera->searchAndLock();

    // 生成临时文件路径
    QString tempPath = QDir::temp().absoluteFilePath(QString("webcam_capture_%1.jpg").arg(QDateTime::currentMSecsSinceEpoch()));
    qDebug() << "Capturing to:" << tempPath;

    // 捕获到临时文件
    int id = m_imageCapture->capture(tempPath);
    qDebug() << "Capture requested with id:" << id;

    // 解锁相机设置
    m_camera->unlock();
}

void WebcamDevice::handleCameraStateChanged(QCamera::State state)
{
    qDebug() << "Camera state changed to:" << state;

    switch (state) {
    case QCamera::ActiveState:
        qDebug() << "Camera is active, waiting for it to stabilize...";
        // 给相机一些时间稳定
        QTimer::singleShot(2000, this, &WebcamDevice::tryCapture);
        break;

    case QCamera::UnloadedState:
        qDebug() << "Camera is unloaded";
        break;

    case QCamera::LoadedState:
        qDebug() << "Camera is loaded but not active";
        break;
    }
}

void WebcamDevice::handleCameraStatusChanged(QCamera::Status status)
{
    qDebug() << "Camera status changed to:" << status;

    switch (status) {
    case QCamera::UnavailableStatus:
        emit captureError(tr("摄像头不可用"));
        closeDevice();
        break;
    case QCamera::StartingStatus:
        qDebug() << "Camera is starting...";
        break;
    case QCamera::StoppingStatus:
        qDebug() << "Camera is stopping...";
        break;
    case QCamera::StandbyStatus:
        qDebug() << "Camera is on standby...";
        break;
    case QCamera::LoadingStatus:
        qDebug() << "Camera is loading...";
        break;
    case QCamera::LoadedStatus:
        qDebug() << "Camera is loaded...";
        break;
    case QCamera::ActiveStatus:
        qDebug() << "Camera is active and ready";
        QTimer::singleShot(500, this, &WebcamDevice::tryCapture);
        break;
    case QCamera::UnloadingStatus:
        qDebug() << "Camera is unloading...";
        break;
    }
}

void WebcamDevice::handleImageCaptured(int id, const QImage &preview)
{
    qDebug() << "Image captured (preview) with id:" << id;

    if (!preview.isNull()) {
        qDebug() << "Preview image size:" << preview.size();
        // 可以选择使用预览图像
        // emit captureCompleted(preview);
    }
    // 注意：我们等待 imageSaved 信号来获取完整质量的图像
}

void WebcamDevice::handleCaptureError(int id, QCameraImageCapture::Error error, const QString &errorString)
{
    qDebug() << "Capture error occurred:";
    qDebug() << "  ID:" << id;
    qDebug() << "  Error:" << error;
    qDebug() << "  Error string:" << errorString;

    if (m_captureRetries < MAX_CAPTURE_RETRIES) {
        m_captureRetries++;
        qDebug() << "Retrying capture... (Attempt" << m_captureRetries << "of" << MAX_CAPTURE_RETRIES << ")";
        m_captureTimer.start(1000);
    } else {
        qDebug() << "Max retries reached, giving up";
        emit captureError(tr("图像捕获失败: %1").arg(errorString));
        closeDevice();
    }
}

void WebcamDevice::handleCameraError(QCamera::Error error)
{
    QString errorMessage;
    if (m_camera) {
        errorMessage = m_camera->errorString();
    } else {
        errorMessage = tr("未知错误");
    }

    qDebug() << "Camera error:" << error << "-" << errorMessage;
    emit captureError(tr("摄像头错误: %1").arg(errorMessage));
    closeDevice();
}

void WebcamDevice::cleanupCamera()
{
    if (m_camera) {
        m_camera->stop();
    }

    delete m_imageCapture;
    m_imageCapture = nullptr;

    delete m_camera;
    m_camera = nullptr;
}

void WebcamDevice::closeDevice()
{
    m_captureTimer.stop();
    cleanupCamera();
    m_deviceSelected = false;
    m_captureRetries = 0;
}
