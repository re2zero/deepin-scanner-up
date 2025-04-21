#ifndef WEBCAMDEVICE_H
#define WEBCAMDEVICE_H

#include <QObject>
#include <QImage>
#include <QStringList>
#include <QCamera>        // Qt5 camera
#include <QCameraInfo>    // Qt5 camera info
#include <QCameraImageCapture>  // For image capture
#include <QTimer>
#include <QDebug>

class WebcamDevice : public QObject
{
    Q_OBJECT

public:
    explicit WebcamDevice(QObject *parent = nullptr);
    ~WebcamDevice();

    // Returns a list of available camera descriptions
    QStringList getAvailableDevices();

    // Opens the camera with the given description
    bool openDevice(const QString &deviceDescription);

    // Captures an image from the camera
    void captureImage();

    // Closes the current camera
    void closeDevice();
    
    // Get preview image from webcam
    QImage getPreviewImage();

signals:
    // Emitted when an image is successfully captured.
    void captureCompleted(const QImage &capturedImage);

    // Emitted if an error occurs during device selection, camera operation, or capture.
    void captureError(const QString &errorMessage);

    void captureStarted(); // 新增：表示开始捕获

private slots:
    // Slot connected to QCameraImageCapture::imageCaptured signal.
    void handleImageCaptured(int id, const QImage &preview);

    // Slot connected to QCameraImageCapture::errorOccurred signal.
    void handleCaptureError(int id, QCameraImageCapture::Error error, const QString &errorString);

    // Slot connected to QCamera::errorOccurred signal.
    void handleCameraError(QCamera::Error error);

    void handleCameraStateChanged(QCamera::State state);
    void handleCameraStatusChanged(QCamera::Status status);

private:
    // Finds the QCameraInfo corresponding to the description.
    QCameraInfo findDevice(const QString &description);

    void setupCamera();
    void cleanupCamera();
    void tryCapture();

    // Selected device info (doesn't own the actual camera hardware resource yet)
    QCameraInfo m_selectedDevice;
    bool m_deviceSelected = false; // Flag indicating if a device description has been successfully matched

    // Qt Multimedia objects - use QScopedPointer for automatic cleanup
    // These are created only when captureImage is called
    QCamera *m_camera;
    QCameraImageCapture *m_imageCapture;

    QTimer m_captureTimer;
    int m_captureRetries;
    static const int MAX_CAPTURE_RETRIES = 3;
};

#endif // WEBCAMDEVICE_H 