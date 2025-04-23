#include "mainwindow.h"
// #include "ui_mainwindow.h" // 如果使用 .ui 文件
#include <QSharedPointer>
#include <QMessageBox>
#include <QFileDialog>   // 用于保存文件
#include <QDebug>   // For optional progress output
#include <QFileInfo>   // Needed for onSaveButtonClicked
#include <QTimer>   // Added for QTimer
#include <QApplication>
#include <QThread>   // Added for QThread
#include <QProcess> 
#include <QScreen>  // Added for QProcess
#include <QGraphicsDropShadowEffect>  // Added for shadow effect

#include <DTitlebar>

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    //, ui(new Ui::MainWindow) // 如果使用 .ui 文件
{
    // ui->setupUi(this); // 如果使用 .ui 文件

    // --- Initialize Devices ---
    QSharedPointer<ScannerDevice> scannerDevice(new ScannerDevice(this));
    QSharedPointer<WebcamDevice> webcamDevice(new WebcamDevice(this));
    
    // Store devices in map
    m_devices["scanner"] = scannerDevice;
    m_devices["webcam"] = webcamDevice;
    
    // Initialize SANE (Scanner)
    if (!scannerDevice->initializeSane()) {
        QMessageBox::critical(this, tr("扫描仪错误"), tr("无法初始化 SANE 后端。\n请确保已安装 SANE 库 (如 sane-backends) 并且可能需要配置权限（例如将用户添加到 'scanner' 或 'saned' 组）。\n扫描仪功能将不可用。"));
    }

    // --- 创建主界面 ---
    setWindowTitle(tr("扫描与捕获管理器"));
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    resize(screenRect.width() / 2, screenRect.height() / 2);
    move((screenRect.width() - width()) / 2, (screenRect.height() - height()) / 2);

    // 创建中心部件和堆叠布局
    QWidget *centralWidget = new QWidget(this);
    m_stackLayout = new QStackedLayout(centralWidget);
    
    // 初始化两个界面
    m_scannersWidget = new ScannersWidget();
    m_scanWidget = new ScanWidget();
    
    // 添加到堆叠布局
    m_stackLayout->addWidget(m_scannersWidget);
    m_stackLayout->addWidget(m_scanWidget);
    
    // 默认显示设备列表界面
    m_stackLayout->setCurrentWidget(m_scannersWidget);
    
    setCentralWidget(centralWidget);

    // 连接信号槽
    connect(m_scannersWidget, &ScannersWidget::deviceSelected,
            this, &MainWindow::showScanView);
    connect(m_scannersWidget, &ScannersWidget::updateDeviceListRequested,
            this, [this](){ updateDeviceList(); });
                       

    // 设置标题栏logo
    auto titleBar = titlebar();
    // titleBar->setIcon(QIcon(":/resources/logo.svg"));
    titleBar->setIcon(QIcon::fromTheme("deepin-scanner"));

    m_backBtn = new DIconButton();
    m_backBtn->setIcon(QIcon::fromTheme("go-previous"));
    m_backBtn->setVisible(false); // 初始隐藏
    titleBar->addWidget(m_backBtn, Qt::AlignLeft);
    
    // 延迟初始化设备列表，确保设备完全初始化
    QTimer::singleShot(300, this, [this](){
        qDebug() << "Initializing device list...";
        updateDeviceList();
    });
    
    // 连接返回按钮信号
    connect(m_backBtn, &DIconButton::clicked,
            this, &MainWindow::showDeviceListView);
}

MainWindow::~MainWindow()
{
    // scannerDevice and webcamDevice are children of MainWindow,
    // Qt's parent-child mechanism handles deletion.
    // Their destructors should call their respective closeDevice() methods.
}

void MainWindow::updateDeviceList()
{
    qDebug() << "Updating device list...";
    
    // 检查设备是否初始化
    if (!m_devices["scanner"] || !m_devices["webcam"]) {
        qDebug() << "Error: Devices not initialized";
        return;
    }
    
    // 使用类型安全的指针转换
    auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
    auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);
    
    if (scanner && webcam) {
        // 更新ScannersWidget中的设备列表
        m_scannersWidget->updateDeviceList(scanner, webcam);
    } else {
        qDebug() << "Error: Failed to cast device pointers";
    }
}

void MainWindow::showScanView(const QString &device, bool isScanner)
{
    m_currentDevice = device;
    m_isCurrentScanner = isScanner;
    
    // 设置当前设备指针
    m_currentDevicePtr = isScanner ? m_devices["scanner"] : m_devices["webcam"];
    qDebug() << "Current device: " << m_currentDevice;
    // 配置扫描界面，直接传递共享指针
    m_scanWidget->setupDeviceMode(m_currentDevicePtr, m_currentDevice);
    
    // 设置预览内容
    if (isScanner) {
        // 显示扫描仪默认图片
        QImage defaultScanImage (":/images/default_scan.png");
        m_scanWidget->setPreviewImage(defaultScanImage);
    } else {
        // 启动摄像头预览
        m_scanWidget->startCameraPreview();
    }
    
    // 切换到扫描界面
    m_stackLayout->setCurrentWidget(m_scanWidget);

    m_backBtn->setVisible(true); // 隐藏
}

void MainWindow::showDeviceListView()
{
    m_backBtn->setVisible(false); // 隐藏
    // 切换到设备列表界面
    m_stackLayout->setCurrentWidget(m_scannersWidget);
}
