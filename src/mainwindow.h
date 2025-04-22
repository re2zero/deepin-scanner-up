#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <DWidget>
#include <QPushButton>
#include <QListWidget>
#include <QLabel> // 用于图像预览
#include <QVBoxLayout>
#include <QWidget>
#include <QPixmap>
#include <QImage>   // <--- 添加 QImage
#include <QString>  // <--- 添加 QString
#include <QComboBox> // <--- 添加 QComboBox 用于分辨率选择

// #include "scannermanager.h" // 假设你创建了这个类

// --- 前向声明 Qt 类 (或者包含 <QtWidgets>) ---
class QListWidget;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QWidget;
// -------------------------------------------

class ScannerDevice; // <--- 前向声明 ScannerDevice
class WebcamDevice;  // <--- Forward declare WebcamDevice

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanButtonClicked();
    void onSaveButtonClicked();
    void updateDeviceList(); // 更新扫描设备列表
    void displayScannedImage(const QImage &image); // <--- 修改参数类型
    void onScanError(const QString &errorMessage); // <--- 添加错误处理槽
    // void updateScanProgress(int percentage); // <--- (可选) 进度槽
    void onDeviceSelectionChanged();
    void updateScannerPreview(const QImage &line);
    
    // 新增槽函数
    void onResolutionsChanged(const QList<QSize> &resolutions);
    void onResolutionSelected(int index);

private:
    Ui::MainWindow *ui; // 如果使用 .ui 文件

    // --- 或者手动创建 UI 元素 ---
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    QListWidget* deviceListWidget; // 显示设备列表
    QPushButton* scanButton;
    QPushButton* saveButton;
    QLabel* imagePreviewLabel; // 显示预览图像
    QLabel *livePreviewLabel;  // 用于实时预览
    QComboBox* resolutionComboBox; // 分辨率选择下拉框
    QLabel* resolutionLabel; // 分辨率标签
    // -----------------------------

    // ScannerManager scannerManager; // 扫描仪管理实例
    QPixmap currentScannedImage; // 保存当前扫描的图像

    ScannerDevice *scannerDevice; // <--- 添加 ScannerDevice 成员指针
    WebcamDevice  *webcamDevice;  // <--- Add WebcamDevice member pointer
    
    // 分辨率选择相关
    bool m_isSelecting = false;    // 标记是否正在进行分辨率选择
    bool m_isPreviewStarted = false; // 标记预览是否已启动

    // --- Helper to identify device type from list item text ---
    enum class DeviceType { Scanner, Webcam, Unknown };
    struct DeviceSelection {
        DeviceType type = DeviceType::Unknown;
        QString name; // Actual device name/description without prefix
    };
    DeviceSelection getSelectedDevice(); // New helper function

    // --- Constants for prefixes ---
    const QString SCANNER_PREFIX = tr("扫描仪: ");
    const QString WEBCAM_PREFIX = tr("摄像头: ");
    // --------------------------

    void updateLivePreview(const QImage &frame);
    
    // 更新分辨率下拉框UI状态
    void updateResolutionUI(bool visible);
    
    // 预览控制
    void startPreview();
    void stopPreview();
};
#endif // MAINWINDOW_H 