// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include <QSharedPointer>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>
#include <QApplication>
#include <QThread>
#include <QProcess>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

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
        QMessageBox::critical(this, tr("Scanner error"), tr("Failed to initialize SANE backend.\nPlease ensure SANE libraries (e.g. sane-backends) are installed and you may need to configure permissions (e.g. add user to 'scanner' or 'saned' group).\nScanner functionality will be unavailable."));
    }

    // --- 创建主界面 ---
    setWindowTitle(tr("Document Scanner"));
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
            this, [this]() { updateDeviceList(); });

    // 设置标题栏logo
    auto titleBar = titlebar();
    // titleBar->setIcon(QIcon(":/resources/logo.svg"));
    titleBar->setIcon(QIcon::fromTheme("deepin-scanner"));

    m_backBtn = new DIconButton();
    m_backBtn->setIcon(QIcon::fromTheme("go-previous"));
    m_backBtn->setVisible(false);   // 初始隐藏
    titleBar->addWidget(m_backBtn, Qt::AlignLeft);

    // 延迟初始化设备列表，确保设备完全初始化
    QTimer::singleShot(300, this, [this]() {
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

    m_scanWidget->startCameraPreview();

    // 切换到扫描界面
    m_stackLayout->setCurrentWidget(m_scanWidget);

    m_backBtn->setVisible(true);
}

void MainWindow::showDeviceListView()
{
    m_backBtn->setVisible(false);
    // 切换到设备列表界面
    m_stackLayout->setCurrentWidget(m_scannersWidget);
}
