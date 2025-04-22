#ifndef SCANNERDEVICE_H
#define SCANNERDEVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>

// Only include and use SANE on non-Windows platforms
#ifndef _WIN32 // Or use Q_OS_WIN if preferred
#include <sane/sane.h>
#include <png.h> // For the scan_it implementation details
#include <stdio.h> // For FILE* used in scan_it
#endif

// Forward declaration for Image struct if needed within scan_it
struct Image;

class ScannerDevice : public QObject
{
    Q_OBJECT

public:
    // Helper structure used by scanning logic
    // You need to provide the definition for this based on your original code.
    struct Image {
        unsigned char *data;
        int width;
        int height;
        int x, y;
        int line_buffer_size; // Example member, adjust as needed
    };

    explicit ScannerDevice(QObject *parent = nullptr);
    ~ScannerDevice();

    // Initializes the SANE backend. Returns true on success.
    bool initializeSane();

    // Returns a list of detected SANE device names.
    QStringList getAvailableDevices();

    // Opens the specified device. Returns true if successful.
    // Note: Blocking operation. Consider async or thread later.
    bool openDevice(const QString &deviceName);

    // Closes the currently open device.
    void closeDevice();

    // Starts the scanning process.
    // The image will be saved temporarily and then emitted via signal.
    // Note: This is currently blocking. Consider thread later.
    void startScan(const QString &tempOutputFilePath);

    // Attempts to cancel an ongoing scan (if supported by backend/device).
    void cancelScan();

    // Helper function (if needed publicly, otherwise move to private/protected)
    // Ensure the signature matches the cpp definition (should return bool)
    bool advance(Image *im); // Example signature, adjust as needed

signals:
    // Emitted when the scan successfully completes.
    void scanCompleted(const QImage &scannedImage);

    // Emitted if an error occurs during initialization, opening, or scanning.
    void scanError(const QString &errorMessage);

    // Emitted periodically during scanning to report progress (0-100).
    // Note: Requires modification in scan_it to emit this.
    void scanProgress(int percentage);

    // 新增信号
    void previewLineAvailable(const QImage &line); // 用于实时预览扫描线

private:
#ifndef _WIN32
    // The core scanning function (refactored from your original code).
    // Returns SANE_STATUS_GOOD on success.
    SANE_Status scan_it(FILE *ofp);

    SANE_Handle m_device = nullptr; // Handle for the currently open device
    bool m_saneInitialized = false; // Flag indicating if sane_init was called
#endif
    bool m_deviceOpen = false; // Flag indicating if a device is open
    bool m_usingTestDevice = false;
    
    void generateTestImage(const QString &outputPath);
};

#endif // SCANNERDEVICE_H 