#include "mainwindow.h"
// #include "ui_mainwindow.h" // 如果使用 .ui 文件
#include <QMessageBox>
#include <QFileDialog> // 用于保存文件
#include "scannerdevice.h" // <--- 添加 ScannerDevice 头文件
#include "webcamdevice.h"  // <--- Include WebcamDevice header
#include "ui/scanwindow.h" // 添加扫描窗口头文件
#include <QDebug> // For optional progress output
#include <QFileInfo> // Needed for onSaveButtonClicked

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    //, ui(new Ui::MainWindow) // 如果使用 .ui 文件
{
    // ui->setupUi(this); // 如果使用 .ui 文件

    // --- Initialize Devices ---
    scannerDevice = new ScannerDevice(this);
    webcamDevice = new WebcamDevice(this); // <--- Create WebcamDevice instance

    // Initialize SANE (Scanner)
    if (!scannerDevice->initializeSane()) {
        QMessageBox::critical(this, tr("扫描仪错误"), tr("无法初始化 SANE 后端。\n请确保已安装 SANE 库 (如 sane-backends) 并且可能需要配置权限（例如将用户添加到 'scanner' 或 'saned' 组）。\n扫描仪功能将不可用。"));
    }
    // Note: Webcam initialization (finding devices) happens in updateDeviceList
    // --------------------------

    // --- 手动创建 UI ---
    setWindowTitle(tr("扫描与捕获管理器"));
    resize(800, 600);

    centralWidget = new QWidget(this);
    mainLayout = new QHBoxLayout(centralWidget);

    // 左侧面板 (设备和设置)
    QWidget *leftPanel = new QWidget(centralWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // 设备列表
    deviceListWidget = new QListWidget(leftPanel);
    deviceListWidget->setStyleSheet("QListWidget { border: none; }");
    deviceListWidget->setIconSize(QSize(32, 32));
    
    // 添加测试设备
    QListWidgetItem *item1 = new QListWidgetItem(QIcon(":/images/scanner.svg"), "测试扫描仪1 (USB)");
    deviceListWidget->addItem(item1);
    
    QListWidgetItem *item2 = new QListWidgetItem(QIcon(":/images/scanner.svg"), "测试扫描仪2 (网络)");
    deviceListWidget->addItem(item2);
    
    QListWidgetItem *item3 = new QListWidgetItem(QIcon(":/images/webcam.svg"), "虚拟扫描设备");
    deviceListWidget->addItem(item3);
    
    leftLayout->addWidget(new QLabel(tr("可用设备:")));
    leftLayout->addWidget(deviceListWidget);

    // 扫描设置区域
    QGroupBox *settingsGroup = new QGroupBox(tr("扫描设置"), leftPanel);
    QFormLayout *settingsLayout = new QFormLayout(settingsGroup);
    
    resolutionCombo = new QComboBox(settingsGroup);
    resolutionCombo->addItems({"75 DPI", "150 DPI", "300 DPI", "600 DPI"});
    settingsLayout->addRow(tr("分辨率:"), resolutionCombo);
    
    colorModeCombo = new QComboBox(settingsGroup);
    colorModeCombo->addItems({tr("黑白"), tr("灰度"), tr("彩色")});
    settingsLayout->addRow(tr("颜色模式:"), colorModeCombo);
    
    leftLayout->addWidget(settingsGroup);
    
    mainLayout->addWidget(leftPanel, 1); // 1/3宽度

    // 右侧预览面板
    QWidget *rightPanel = new QWidget(centralWidget);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    imagePreviewLabel = new QLabel(rightPanel);
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setMinimumSize(400, 500);
    imagePreviewLabel->setStyleSheet("QLabel { background-color : lightgray; border: 1px solid gray; }");
    imagePreviewLabel->setText(tr("等待扫描或捕获..."));
    
    rightLayout->addWidget(new QLabel(tr("预览:")));
    rightLayout->addWidget(imagePreviewLabel);
    
    // 底部按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    scanButton = new QPushButton(tr("开始扫描"), rightPanel);
    saveButton = new QPushButton(tr("保存图像"), rightPanel);
    saveButton->setEnabled(false);
    
    buttonLayout->addWidget(scanButton);
    buttonLayout->addWidget(saveButton);
    rightLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(rightPanel, 2); // 2/3宽度

    setCentralWidget(centralWidget);

    // 连接设备选择信号
    connect(deviceListWidget, &QListWidget::itemClicked, this, &MainWindow::updatePreview);
    // ---------------------

    // --- 连接信号和槽 ---
    connect(scanButton, &QPushButton::clicked, this, &MainWindow::onScanButtonClicked);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);

    // Connect ScannerDevice signals
    connect(scannerDevice, &ScannerDevice::scanCompleted, this, &MainWindow::displayScannedImage);
    connect(scannerDevice, &ScannerDevice::scanError, this, &MainWindow::onScanError);
    // connect(scannerDevice, &ScannerDevice::scanProgress, this, &MainWindow::updateScanProgress);

    // Connect WebcamDevice signals <--- NEW ---
    connect(webcamDevice, &WebcamDevice::captureCompleted, this, &MainWindow::displayScannedImage);
    connect(webcamDevice, &WebcamDevice::captureError, this, &MainWindow::onScanError);
    // ----------------------------------

    updateDeviceList(); // Find and list devices at startup
}

MainWindow::~MainWindow()
{
    // scannerDevice and webcamDevice are children of MainWindow,
    // Qt's parent-child mechanism handles deletion.
    // Their destructors should call their respective closeDevice() methods.
}

// --- Helper Function to get selected device type and name ---
MainWindow::DeviceSelection MainWindow::getSelectedDevice()
{
    DeviceSelection selection;
    QListWidgetItem* selectedItem = deviceListWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个设备！"));
        return selection; // Returns Unknown type
    }

    QString itemText = selectedItem->text();
    if (itemText.startsWith(SCANNER_PREFIX)) {
        selection.type = DeviceType::Scanner;
        selection.name = itemText.mid(SCANNER_PREFIX.length());
    } else if (itemText.startsWith(WEBCAM_PREFIX)) {
        selection.type = DeviceType::Webcam;
        selection.name = itemText.mid(WEBCAM_PREFIX.length());
    } else {
        QMessageBox::warning(this, tr("警告"), tr("无法识别所选设备类型。"));
    }
    return selection;
}
// ----------------------------------------------------------

void MainWindow::updateDeviceList()
{
    deviceListWidget->clear();
    bool devicesFound = false;

    // --- Get Scanner Devices ---
    QStringList scannerNames = scannerDevice->getAvailableDevices();
    for (const QString &name : scannerNames) {
        deviceListWidget->addItem(SCANNER_PREFIX + name);
        devicesFound = true;
    }

    // --- Get Webcam Devices ---
    QStringList webcamNames = webcamDevice->getAvailableDevices();
    for (const QString &name : webcamNames) {
        deviceListWidget->addItem(WEBCAM_PREFIX + name);
        devicesFound = true;
    }
    // -------------------------

    if (deviceListWidget->count() > 0) {
        deviceListWidget->setCurrentRow(0); // Select the first one
        scanButton->setEnabled(true);
    } else {
        QMessageBox::information(this, tr("信息"), 
            tr("未找到可用的扫描仪或摄像头设备。\n请检查设备连接和驱动程序/权限。"));
        scanButton->setEnabled(false); // Disable button if no devices
    }
}

void MainWindow::onScanButtonClicked()
{
    DeviceSelection selection = getSelectedDevice();
    if (selection.type == DeviceType::Unknown) {
        return; // Error message shown in getSelectedDevice
    }

    // 创建并显示扫描窗口
    ScanWindow *scanWindow = new ScanWindow(this);
    connect(scanWindow, &ScanWindow::backRequested, this, [this, scanWindow]() {
        scanWindow->close();
        this->show(); // 显示主窗口
    });
    
    this->hide(); // 隐藏主窗口
    scanWindow->show();
}

// Handles both scanCompleted and captureCompleted
void MainWindow::displayScannedImage(const QImage& image)
{
    scanButton->setEnabled(true); // Re-enable button
    deviceListWidget->setEnabled(true); // Re-enable list

    if (!image.isNull()) {
        currentScannedImage = QPixmap::fromImage(image);
        imagePreviewLabel->setPixmap(currentScannedImage.scaled(
            imagePreviewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
        saveButton->setEnabled(true);
    } else {
        // This case might happen if scan/capture technically succeeded but returned no data
        imagePreviewLabel->setText(tr("操作完成但未能获取图像数据"));
        saveButton->setEnabled(false);
        QMessageBox::warning(this, tr("警告"), tr("操作过程已完成，但未能成功加载图像。"));
    }

    // Close the scanner device after use. Webcam closes itself internally.
    // Check sender to be safe, although closing an already closed device is harmless.
    if (sender() == scannerDevice) {
       scannerDevice->closeDevice();
    }
    // WebcamDevice closes automatically after capture/error in its implementation.
}

// Handles both scanError and captureError
void MainWindow::onScanError(const QString &errorMessage)
{
    QMessageBox::critical(this, tr("操作错误"), errorMessage);
    imagePreviewLabel->setText(tr("操作失败")); // Update preview area status
    saveButton->setEnabled(false);
    scanButton->setEnabled(true); // Re-enable button for retry
    deviceListWidget->setEnabled(true); // Re-enable list

    // Attempt to close the relevant device
     if (sender() == scannerDevice) {
       scannerDevice->closeDevice();
     } else if (sender() == webcamDevice) {
       webcamDevice->closeDevice(); // Ensure cleanup if error happened before internal close
     }
}

void MainWindow::onSaveButtonClicked()
{
    if (currentScannedImage.isNull()) {
        QMessageBox::warning(this, tr("警告"), tr("没有可保存的图像。"));
        return;
    }

    QString defaultFileName = "capture.png"; // More generic default name
    QString filters = tr("PNG 图像 (*.png);;JPEG 图像 (*.jpg *.jpeg);;BMP 图像 (*.bmp);;所有文件 (*)");
    QString selectedFilter;

    QFileDialog saveDialog(this, tr("保存图像"), defaultFileName, filters);
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("png");

    if (saveDialog.exec() == QDialog::Accepted) {
        QStringList files = saveDialog.selectedFiles();
        if (files.isEmpty()) { return; }
        QString fileName = files.first();
        selectedFilter = saveDialog.selectedNameFilter();

        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix().toLower();
        const char* format = nullptr;

        // Determine format (simplified logic relying more on QFileDialog)
        if (selectedFilter.contains("*.png") || suffix == "png") format = "PNG";
        else if (selectedFilter.contains("*.jpg") || selectedFilter.contains("*.jpeg") || suffix == "jpg" || suffix == "jpeg") format = "JPG";
        else if (selectedFilter.contains("*.bmp") || suffix == "bmp") format = "BMP";
        else format = "PNG"; // Default fallback

        // QFileDialog usually handles adding the suffix if default suffix is set
        // and user selects a filter without typing an extension. Check if necessary.
        if (QFileInfo(fileName).suffix().isEmpty()) {
             if (format == "PNG" && !fileName.endsWith(".png", Qt::CaseInsensitive)) fileName += ".png";
             else if (format == "JPG" && !(fileName.endsWith(".jpg", Qt::CaseInsensitive) || fileName.endsWith(".jpeg", Qt::CaseInsensitive)) ) fileName += ".jpg";
             else if (format == "BMP" && !fileName.endsWith(".bmp", Qt::CaseInsensitive)) fileName += ".bmp";
        }

        if (!currentScannedImage.save(fileName, format)) {
             QMessageBox::critical(this, tr("错误"), tr("无法保存图像到 %1 (尝试格式: %2).\n请检查文件名和写入权限。").arg(fileName).arg(format));
        } else {
             QMessageBox::information(this, tr("成功"), tr("图像已保存到 %1.").arg(fileName));
        }
    }
}

void MainWindow::updatePreview(QListWidgetItem *item)
{
    if (!item) return;
    
    // 根据选择的设备类型更新预览
    DeviceSelection selection = getSelectedDevice();
    if (selection.type == DeviceType::Scanner) {
        // 扫描仪预览
        QImage preview = scannerDevice->getPreviewImage();
        if (!preview.isNull()) {
            imagePreviewLabel->setPixmap(QPixmap::fromImage(preview.scaled(
                imagePreviewLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation)));
        }
    } else if (selection.type == DeviceType::Webcam) {
        // 摄像头预览
        QImage preview = webcamDevice->getPreviewImage();
        if (!preview.isNull()) {
            imagePreviewLabel->setPixmap(QPixmap::fromImage(preview.scaled(
                imagePreviewLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation)));
        }
    }
}

/*
// Optional Progress Slot (Only for Scanner)
void MainWindow::updateScanProgress(int percentage)
{
    if (sender() == scannerDevice) { // Check if the signal came from the scanner
        qDebug() << "Scan progress:" << percentage << "%";
        // Update a progress bar or status bar here
    }
}
*/
// --------------------------------------- 