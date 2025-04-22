#include "mainwindow.h"
// #include "ui_mainwindow.h" // 如果使用 .ui 文件
#include <QMessageBox>
#include <QFileDialog>   // 用于保存文件
#include "scannerdevice.h"   // <--- 添加 ScannerDevice 头文件
#include "webcamdevice.h"   // <--- Include WebcamDevice header
#include <QDebug>   // For optional progress output
#include <QFileInfo>   // Needed for onSaveButtonClicked
#include <QTimer>   // Added for QTimer
#include <QApplication>
#include <QThread>   // Added for QThread
#include <QProcess> 
#include <QScreen>  // Added for QProcess
#include <QGraphicsDropShadowEffect>  // Added for shadow effect

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    //, ui(new Ui::MainWindow) // 如果使用 .ui 文件
{
    // ui->setupUi(this); // 如果使用 .ui 文件

    // --- Initialize Devices ---
    scannerDevice = new ScannerDevice(this);
    webcamDevice = new WebcamDevice(this);   // <--- Create WebcamDevice instance

    // Initialize SANE (Scanner)
    if (!scannerDevice->initializeSane()) {
        QMessageBox::critical(this, tr("扫描仪错误"), tr("无法初始化 SANE 后端。\n请确保已安装 SANE 库 (如 sane-backends) 并且可能需要配置权限（例如将用户添加到 'scanner' 或 'saned' 组）。\n扫描仪功能将不可用。"));
    }
    // Note: Webcam initialization (finding devices) happens in updateDeviceList
    // --------------------------

    // --- 手动创建 UI ---
    setWindowTitle(tr("扫描与捕获管理器"));   // Updated title
    // take the desktop screen size and set half of it as the window size
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    resize(screenRect.width() / 2, screenRect.height() / 2);
    move((screenRect.width() - width()) / 2, (screenRect.height() - height()) / 2);

    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 设备列表卡片
    QFrame *deviceCard = new QFrame(this);
    deviceCard->setFrameShape(QFrame::StyledPanel);
    deviceCard->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceCard);
    deviceLayout->addWidget(new QLabel(tr("可用设备:")));
    deviceListWidget = new QListWidget(this);
    deviceLayout->addWidget(deviceListWidget);
    mainLayout->addWidget(deviceCard);

    // 分辨率选择卡片
    QFrame *resolutionCard = new QFrame(this);
    resolutionCard->setFrameShape(QFrame::StyledPanel);
    resolutionCard->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QHBoxLayout *resolutionLayout = new QHBoxLayout(resolutionCard);
    resolutionLabel = new QLabel(tr("分辨率:"));
    resolutionComboBox = new QComboBox();
    resolutionLayout->addWidget(resolutionLabel);
    resolutionLayout->addWidget(resolutionComboBox);
    resolutionLayout->addStretch(1);
    updateResolutionUI(false);
    mainLayout->addWidget(resolutionCard);

    // 视频预览卡片
    QFrame *videoCard = new QFrame(this);
    videoCard->setFrameShape(QFrame::StyledPanel);
    videoCard->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QVBoxLayout *videoLayout = new QVBoxLayout(videoCard);
    videoLayout->addWidget(webcamDevice->getVideoWidget());
    videoCard->setMinimumSize(320, 240);
    mainLayout->addWidget(videoCard);

    // 操作按钮卡片
    QFrame *buttonCard = new QFrame(this);
    buttonCard->setFrameShape(QFrame::StyledPanel);
    buttonCard->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonCard);
    scanButton = new QPushButton(tr("扫描 / 捕获"), this);
    buttonLayout->addWidget(scanButton);
    saveButton = new QPushButton(tr("保存图像"), this);
    saveButton->setEnabled(false);
    buttonLayout->addWidget(saveButton);
    mainLayout->addWidget(buttonCard);

    // 图像预览卡片
    QFrame *previewCard = new QFrame(this);
    previewCard->setFrameShape(QFrame::StyledPanel);
    previewCard->setStyleSheet("QFrame { background: white; border-radius: 8px; }");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewCard);
    previewLayout->addWidget(new QLabel(tr("预览:")));
    imagePreviewLabel = new QLabel(this);
    imagePreviewLabel->setAlignment(Qt::AlignCenter);
    imagePreviewLabel->setMinimumSize(300, 400);
    imagePreviewLabel->setStyleSheet("QLabel { background-color : lightgray; border: 1px solid gray; border-radius: 4px; }");
    imagePreviewLabel->setText(tr("等待扫描或捕获..."));
    previewLayout->addWidget(imagePreviewLabel);
    mainLayout->addWidget(previewCard);

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(2, 2);
    centralWidget->setGraphicsEffect(shadow);

    setCentralWidget(centralWidget);
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

    // 连接分辨率相关信号
    connect(webcamDevice, &WebcamDevice::resolutionsChanged, this, &MainWindow::onResolutionsChanged);
    connect(resolutionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onResolutionSelected);
    // ----------------------------------

    // 在 deviceListWidget 的选择改变时启动预览
    connect(deviceListWidget, &QListWidget::currentItemChanged,
            this, &MainWindow::onDeviceSelectionChanged);

    // 在构造函数中添加连接
    connect(scannerDevice, &ScannerDevice::previewLineAvailable,
            this, &MainWindow::updateScannerPreview);

    updateDeviceList();   // Find and list devices at startup
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
    QListWidgetItem *selectedItem = deviceListWidget->currentItem();
    if (!selectedItem) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择一个设备！"));
        return selection;   // Returns Unknown type
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
        deviceListWidget->setCurrentRow(0);   // Select the first one
        scanButton->setEnabled(true);
    } else {
        QMessageBox::information(this, tr("信息"),
                                 tr("未找到可用的扫描仪或摄像头设备。\n请检查设备连接和驱动程序/权限。"));
        scanButton->setEnabled(false);   // Disable button if no devices
    }
}

void MainWindow::onScanButtonClicked()
{
    DeviceSelection selection = getSelectedDevice();
    if (selection.type == DeviceType::Unknown) {
        return;   // Error message shown in getSelectedDevice
    }

    // --- Prepare UI for operation ---
    imagePreviewLabel->setText(tr("操作进行中..."));   // Generic message
    imagePreviewLabel->setPixmap(QPixmap());   // Clear previous image
    saveButton->setEnabled(false);
    scanButton->setEnabled(false);
    deviceListWidget->setEnabled(false);

    // --- Perform action based on device type ---
    bool success = false;
    if (selection.type == DeviceType::Scanner) {
        // Open scanner device
        if (scannerDevice->openDevice(selection.name)) {
            // Start scan (BLOCKING - needs thread for real app)
            qDebug() << "Starting SANE scan for:" << selection.name;
            // TODO: Use QTemporaryFile
            scannerDevice->startScan("temp_scan.png");
            // Result handled by signals scanCompleted/scanError
            // Re-enabling UI is done in those signal handlers
            success = true;   // Assume success for now, signals will report errors
        } else {
            // Error message emitted by scannerDevice's signal
            qDebug() << "Failed to open scanner:" << selection.name;
        }
    } else if (selection.type == DeviceType::Webcam) {
        // The WebcamDevice::captureImage() function already manages
        // stopping preview and checking device state internally
        webcamDevice->captureImage();
        success = true;
    }

    // --- If the initial open/select call failed immediately ---
    if (!success) {
        // Re-enable UI immediately if the operation couldn't even start
        imagePreviewLabel->setText(tr("操作失败"));
        scanButton->setEnabled(true);
        deviceListWidget->setEnabled(true);
        saveButton->setEnabled(false);
        // Error message should have been shown via signal/slot mechanism already
    }
}

// Handles both scanCompleted and captureCompleted
void MainWindow::displayScannedImage(const QImage &image)
{
    scanButton->setEnabled(true);
    deviceListWidget->setEnabled(true);

    if (!image.isNull()) {
        currentScannedImage = QPixmap::fromImage(image);
        imagePreviewLabel->setPixmap(currentScannedImage.scaled(
                imagePreviewLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
        saveButton->setEnabled(true);
    } else {
        imagePreviewLabel->setText(tr("操作完成但未能获取图像数据"));
        saveButton->setEnabled(false);
        QMessageBox::warning(this, tr("警告"), tr("操作过程已完成，但未能成功加载图像。"));
    }

    // Close the scanner device after use
    if (sender() == scannerDevice) {
        scannerDevice->closeDevice();
    } else if (sender() == webcamDevice) {
        // 重新启动预览 - 延迟启动以确保设备稳定
        DeviceSelection selection = getSelectedDevice();
        if (selection.type == DeviceType::Webcam) {
            QTimer::singleShot(300, [this]() {
                webcamDevice->startPreview();
            });
        }
    }
}

// Handles both scanError and captureError
void MainWindow::onScanError(const QString &errorMessage)
{
    // 创建一个日志按钮的自定义消息框
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(tr("操作错误"));
    msgBox.setInformativeText(errorMessage);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
    msgBox.setDefaultButton(QMessageBox::Ok);

    int ret = msgBox.exec();

    if (ret == QMessageBox::Help) {
        // 显示一个对话框，提供详细的调试建议
        QMessageBox::information(this, tr("扫描仪排障指南"),
                                 tr("请尝试以下步骤解决扫描仪问题:\n\n"
                                    "1. 确认扫描仪已连接并开机\n"
                                    "2. 在终端运行以下命令检查扫描仪:\n"
                                    "   scanimage -L\n"
                                    "3. 添加用户到scanner组:\n"
                                    "   sudo usermod -a -G scanner $USER\n"
                                    "4. 重启SANE服务:\n"
                                    "   sudo service saned restart\n"
                                    "5. 检查SANE配置:\n"
                                    "   ls -la /etc/sane.d/\n"
                                    "6. 安装更多SANE后端:\n"
                                    "   sudo apt-get install libsane-extras\n"
                                    "7. 重启计算机\n\n"
                                    "同时，您可以使用虚拟测试扫描仪继续开发应用程序功能。"));
    }

    imagePreviewLabel->setText(tr("操作失败"));
    saveButton->setEnabled(false);
    scanButton->setEnabled(true);
    deviceListWidget->setEnabled(true);

    // 关闭设备
    if (sender() == scannerDevice) {
        scannerDevice->closeDevice();
    } else if (sender() == webcamDevice) {
        webcamDevice->closeDevice();
    }
}

void MainWindow::onSaveButtonClicked()
{
    if (currentScannedImage.isNull()) {
        QMessageBox::warning(this, tr("警告"), tr("没有可保存的图像。"));
        return;
    }

    QString defaultFileName = "capture.png";   // More generic default name
    QString filters = tr("PNG 图像 (*.png);;JPEG 图像 (*.jpg *.jpeg);;BMP 图像 (*.bmp);;所有文件 (*)");
    QString selectedFilter;

    QFileDialog saveDialog(this, tr("保存图像"), defaultFileName, filters);
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("png");

    if (saveDialog.exec() == QDialog::Accepted) {
        QStringList files = saveDialog.selectedFiles();
        if (files.isEmpty()) {
            return;
        }
        QString fileName = files.first();
        selectedFilter = saveDialog.selectedNameFilter();

        QFileInfo fileInfo(fileName);
        QString suffix = fileInfo.suffix().toLower();
        const char *format = nullptr;

        // Determine format (simplified logic relying more on QFileDialog)
        if (selectedFilter.contains("*.png") || suffix == "png")
            format = "PNG";
        else if (selectedFilter.contains("*.jpg") || selectedFilter.contains("*.jpeg") || suffix == "jpg" || suffix == "jpeg")
            format = "JPG";
        else if (selectedFilter.contains("*.bmp") || suffix == "bmp")
            format = "BMP";
        else
            format = "PNG";   // Default fallback

        // QFileDialog usually handles adding the suffix if default suffix is set
        // and user selects a filter without typing an extension. Check if necessary.
        if (QFileInfo(fileName).suffix().isEmpty()) {
            if (format == "PNG" && !fileName.endsWith(".png", Qt::CaseInsensitive))
                fileName += ".png";
            else if (format == "JPG" && !(fileName.endsWith(".jpg", Qt::CaseInsensitive) || fileName.endsWith(".jpeg", Qt::CaseInsensitive)))
                fileName += ".jpg";
            else if (format == "BMP" && !fileName.endsWith(".bmp", Qt::CaseInsensitive))
                fileName += ".bmp";
        }

        if (!currentScannedImage.save(fileName, format)) {
            QMessageBox::critical(this, tr("错误"), tr("无法保存图像到 %1 (尝试格式: %2).\n请检查文件名和写入权限。").arg(fileName).arg(format));
        } else {
            QMessageBox::information(this, tr("成功"), tr("图像已保存到 %1.").arg(fileName));
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

// 在 deviceListWidget 的选择改变时启动预览
void MainWindow::onDeviceSelectionChanged()
{
    // 避免重复处理
    static bool isChanging = false;
    if (isChanging) return;

    isChanging = true;

    DeviceSelection selection = getSelectedDevice();
    if (selection.type == DeviceType::Webcam) {
        webcamDevice->stopPreview();   // 确保先停止任何预览
        if (webcamDevice->openDevice(selection.name)) {
            // 更新分辨率UI
            updateResolutionUI(true);

            QTimer::singleShot(300, [this]() {
                webcamDevice->startPreview();
                // 使用函数而不是引用捕获静态变量
                QTimer::singleShot(100, []() {
                    isChanging = false;
                });
            });
        } else {
            isChanging = false;
        }
    } else {
        webcamDevice->stopPreview();
        updateResolutionUI(false);   // 隐藏分辨率UI
        isChanging = false;
    }
}

// 添加新的槽函数
void MainWindow::updateScannerPreview(const QImage &line)
{
    static QImage accumulator;
    static int currentLine = 0;

    // 如果是新的扫描，重置累积器
    if (currentLine == 0 || accumulator.isNull()) {
        accumulator = QImage(line.width(), 2000, line.format());   // 预设高度
        accumulator.fill(Qt::white);
        currentLine = 0;
    }

    // 复制新的扫描线到累积器
    if (currentLine < accumulator.height()) {
        for (int x = 0; x < line.width(); ++x) {
            accumulator.setPixel(x, currentLine, line.pixel(x, 0));
        }
        currentLine++;

        // 更新预览
        if (currentLine % 10 == 0) {   // 每10行更新一次以提高性能
            imagePreviewLabel->setPixmap(QPixmap::fromImage(accumulator).scaled(imagePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            QApplication::processEvents();   // 允许UI更新
        }
    }
}

// 添加新的处理函数
void MainWindow::onResolutionsChanged(const QList<QSize> &resolutions)
{
    // 标记正在更新分辨率列表，避免触发循环
    static bool isUpdating = false;
    if (isUpdating) return;

    isUpdating = true;

    // 清空原有内容
    resolutionComboBox->blockSignals(true);
    resolutionComboBox->clear();

    if (resolutions.isEmpty()) {
        updateResolutionUI(false);
        resolutionComboBox->blockSignals(false);
        isUpdating = false;
        return;
    }

    // 添加所有支持的分辨率
    for (const QSize &res : resolutions) {
        QString item = QString("%1x%2").arg(res.width()).arg(res.height());
        resolutionComboBox->addItem(item, QVariant::fromValue(res));
    }

    // 尝试选择当前分辨率
    QSize currentRes = webcamDevice->getCurrentResolution();
    for (int i = 0; i < resolutionComboBox->count(); ++i) {
        QSize itemRes = resolutionComboBox->itemData(i).toSize();
        if (itemRes == currentRes) {
            resolutionComboBox->setCurrentIndex(i);
            break;
        }
    }

    // 显示分辨率UI
    updateResolutionUI(true);
    resolutionComboBox->blockSignals(false);
    isUpdating = false;
}

void MainWindow::onResolutionSelected(int index)
{
    // 防止重复选择操作
    if (m_isSelecting) {
        qDebug() << "已有分辨率选择操作正在进行中";
        return;
    }

    m_isSelecting = true;

    // 检查设备状态
    DeviceSelection selection = getSelectedDevice();
    if (selection.type != DeviceType::Webcam || !webcamDevice) {
        qDebug() << "没有选择摄像头设备或设备未初始化";
        m_isSelecting = false;
        return;
    }

    // 获取选择的分辨率
    QVariant var = resolutionComboBox->itemData(index);
    if (!var.isValid()) {
        qDebug() << "无效的分辨率数据";
        m_isSelecting = false;
        return;
    }

    QSize resolution = var.toSize();

    if (!resolution.isValid()) {
        qDebug() << "无效的分辨率";
        m_isSelecting = false;
        return;
    }

    // 显示一个等待光标
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qDebug() << "正在设置分辨率:" << resolution.width() << "x" << resolution.height();

    // 记录当前预览状态
    bool wasPreviewActive = m_isPreviewStarted;

    // 停止预览但不关闭设备，让WebcamDevice自己控制打开和关闭
    if (wasPreviewActive) {
        stopPreview();
    }

    // 尝试设置新的分辨率，让WebcamDevice处理内部细节
    bool success = webcamDevice->setResolution(resolution.width(), resolution.height());

    // 如果设置分辨率失败，可能需要尝试更积极的方法
    if (!success) {
        qDebug() << "使用标准方法设置分辨率失败，尝试更积极的方法";

        // 先完全关闭设备
        webcamDevice->closeDevice();

        // 提取实际设备路径
        QString devicePath = selection.name;
        int idx = devicePath.indexOf('(');
        if (idx > 0) {
            devicePath = devicePath.mid(idx + 1).chopped(1);
        }

        // 使用fuser尝试释放设备
        QProcess fuserProcess;
        fuserProcess.start("fuser", QStringList() << "-k" << devicePath);
        fuserProcess.waitForFinished(2000);
        qDebug() << "尝试强制关闭设备使用者: " << devicePath;
        qDebug() << "fuser命令输出:" << fuserProcess.readAllStandardOutput();

        // 等待系统释放资源
        QThread::msleep(1000);

        // 询问是否尝试重置摄像头驱动
        bool forceReset = false;

        if (QMessageBox::question(this, tr("设备仍然被占用"),
                                  tr("摄像头设备可能仍然被占用，无法更改分辨率。\n\n"
                                     "是否尝试通过重置摄像头驱动解决此问题？\n"
                                     "注意：这需要root权限，并且会暂时使所有摄像头停止工作。"),
                                  QMessageBox::Yes | QMessageBox::No)
            == QMessageBox::Yes) {

            forceReset = true;

            // 显示等待对话框
            QMessageBox pleaseWaitMsg(QMessageBox::Information,
                                      tr("请稍候"),
                                      tr("正在重置摄像头设备，请稍候..."),
                                      QMessageBox::NoButton,
                                      this);
            pleaseWaitMsg.setStandardButtons(0);   // 移除所有按钮

            // 在另一个线程中显示等待对话框
            QTimer::singleShot(100, [&pleaseWaitMsg]() {
                pleaseWaitMsg.show();
                QApplication::processEvents();
            });

            // 尝试卸载并重新加载uvcvideo模块
            QProcess resetProcess;
            resetProcess.start("pkexec", QStringList() << "sh"
                                                       << "-c"
                                                       << "rmmod uvcvideo && sleep 2 && modprobe uvcvideo");

            // 等待命令完成，最多30秒
            if (!resetProcess.waitForFinished(30000)) {
                resetProcess.kill();
                QMessageBox::warning(this, tr("超时"),
                                     tr("重置摄像头设备的操作超时。"));
            }

            // 关闭等待对话框
            pleaseWaitMsg.close();

            // 如果命令成功执行
            if (resetProcess.exitCode() == 0) {
                QMessageBox::information(this, tr("成功"),
                                         tr("摄像头设备已重置。"));
            } else {
                QString error = resetProcess.readAllStandardError();
                QMessageBox::warning(this, tr("重置失败"),
                                     tr("重置设备失败：%1").arg(error.isEmpty() ? tr("未知错误") : error));

                // 如果重置失败，不继续尝试更改分辨率
                QApplication::restoreOverrideCursor();
                m_isSelecting = false;
                return;
            }

            // 重置后等待系统重新识别摄像头设备
            QThread::msleep(5000);
        }

        // 更新设备列表
        if (forceReset) {
            updateDeviceList();

            // 重新获取当前设备
            bool found = false;
            for (int i = 0; i < deviceListWidget->count(); i++) {
                QString itemText = deviceListWidget->item(i)->text();
                if (itemText.startsWith(WEBCAM_PREFIX) && (itemText.contains(selection.name) || selection.name.contains(itemText))) {
                    deviceListWidget->setCurrentRow(i);
                    selection = getSelectedDevice();
                    found = true;
                    break;
                }
            }

            if (!found) {
                QMessageBox::warning(this, tr("错误"),
                                     tr("在重置后无法找到之前的摄像头设备，请手动重新选择。"));
                QApplication::restoreOverrideCursor();
                m_isSelecting = false;
                return;
            }
        }

        // 重新打开设备
        qDebug() << "尝试重新打开设备:" << selection.name;

        // 多次尝试打开设备
        bool deviceOpened = false;
        for (int tryCount = 0; tryCount < 3; tryCount++) {
            qDebug() << "第" << tryCount + 1 << "次尝试打开设备";

            if (webcamDevice->openDevice(selection.name)) {
                deviceOpened = true;
                break;
            }

            // 等待后再次尝试
            QThread::msleep(1000);
        }

        if (!deviceOpened) {
            QApplication::restoreOverrideCursor();
            QMessageBox::warning(this, tr("错误"), tr("设置分辨率失败：无法重新打开设备"));
            m_isSelecting = false;
            return;
        }

        // 现在尝试设置分辨率
        success = webcamDevice->setResolution(resolution.width(), resolution.height());
    }

    // 显示设置结果
    if (success) {
        qDebug() << "分辨率设置成功:" << resolution.width() << "x" << resolution.height();
        QMessageBox::information(this, tr("设置成功"),
                                 tr("分辨率已成功设置为 %1×%2").arg(resolution.width()).arg(resolution.height()));

        // 确保支持的分辨率列表包含当前成功设置的分辨率
        webcamDevice->addSupportedResolution(resolution);
    } else {
        qDebug() << "分辨率设置失败";
        QMessageBox::warning(this, tr("设置失败"),
                             tr("无法设置分辨率为 %1×%2").arg(resolution.width()).arg(resolution.height()));
    }

    // 恢复预览
    if (wasPreviewActive && webcamDevice->openDevice(selection.name)) {
        QTimer::singleShot(500, [this]() {
            startPreview();
        });
    }

    QApplication::restoreOverrideCursor();
    m_isSelecting = false;
}

void MainWindow::updateResolutionUI(bool visible)
{
    resolutionLabel->setVisible(visible);
    resolutionComboBox->setVisible(visible);
}

void MainWindow::startPreview()
{
    if (m_isPreviewStarted) {
        qDebug() << "预览已经启动";
        return;
    }

    auto deviceSelection = getSelectedDevice();

    if (deviceSelection.type == DeviceType::Webcam && webcamDevice) {
        webcamDevice->startPreview();
        m_isPreviewStarted = true;
        qDebug() << "摄像头预览已启动";
    } else if (deviceSelection.type == DeviceType::Scanner && scannerDevice) {
        // 扫描仪预览启动逻辑（如果需要）
        qDebug() << "扫描仪不支持实时预览";
    }
}

void MainWindow::stopPreview()
{
    if (!m_isPreviewStarted) {
        qDebug() << "预览尚未启动";
        return;
    }

    auto deviceSelection = getSelectedDevice();

    if (deviceSelection.type == DeviceType::Webcam && webcamDevice) {
        webcamDevice->stopPreview();
        m_isPreviewStarted = false;
        qDebug() << "摄像头预览已停止";
    } else if (deviceSelection.type == DeviceType::Scanner && scannerDevice) {
        // 扫描仪预览停止逻辑（如果需要）
    }
}
// ---------------------------------------
