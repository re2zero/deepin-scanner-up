#include "scanwindow.h"
#include "scannerdevice.h"
#include "webcamdevice.h"
#include <DTitlebar>
#include <DWidgetUtil>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QSettings>

ScanWindow::ScanWindow(QWidget *parent)
    : DMainWindow(parent),
      m_scannerDevice(new ScannerDevice(this)),
      m_webcamDevice(new WebcamDevice(this))
{
    initUI();
    initConnections();
    
    // 添加返回按钮到标题栏
    DTitlebar *tb = titlebar();
    QPushButton *backButton = new QPushButton("返回", tb);
    connect(backButton, &QPushButton::clicked, this, &ScanWindow::backRequested);
    tb->addWidget(backButton, Qt::AlignRight);
    
    // 设置窗口大小和居中
    resize(800, 600);
    Dtk::Widget::moveToCenter(this);
}

void ScanWindow::initUI()
{
    // 主布局
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    // 左侧设备面板
    m_devicePanel = new QWidget(centralWidget);
    m_deviceLayout = new QVBoxLayout(m_devicePanel);
    
    // 设备图标
    // 设备图标和标题
    QWidget *headerWidget = new QWidget(m_devicePanel);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    
    m_deviceIcon = new QLabel(headerWidget);
    m_deviceIcon->setPixmap(QPixmap(":/images/scanner.svg").scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    headerLayout->addWidget(m_deviceIcon);
    
    m_deviceDesc = new QLabel(tr("扫描设备"), headerWidget);
    QFont descFont = m_deviceDesc->font();
    descFont.setBold(true);
    m_deviceDesc->setFont(descFont);
    headerLayout->addWidget(m_deviceDesc);
    headerLayout->addStretch();
    
    m_deviceLayout->addWidget(headerWidget);
    
    // 设备列表容器
    QWidget *deviceContainer = new QWidget(m_devicePanel);
    QHBoxLayout *deviceContainerLayout = new QHBoxLayout(deviceContainer);
    deviceContainerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 设备列表
    m_deviceList = new QListWidget(deviceContainer);
    m_deviceList->setStyleSheet("QListWidget { border: none; }");
    m_deviceList->setIconSize(QSize(32, 32));
    
    // 创建自定义设备项
    auto createDeviceItem = [this](const QString &iconPath, const QString &name, const QString &desc) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setIcon(QIcon(iconPath));
        item->setData(Qt::UserRole, desc);
        
        QWidget *itemWidget = new QWidget();
        QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(5, 5, 5, 5);
        
        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(iconPath).pixmap(32, 32));
        itemLayout->addWidget(iconLabel);
        
        QVBoxLayout *textLayout = new QVBoxLayout();
        QLabel *nameLabel = new QLabel(name);
        nameLabel->setStyleSheet("font-weight: bold;");
        textLayout->addWidget(nameLabel);
        
        QLabel *descLabel = new QLabel(desc);
        descLabel->setStyleSheet("color: gray; font-size: 11px;");
        textLayout->addWidget(descLabel);
        itemLayout->addLayout(textLayout);
        
        itemLayout->addStretch();
        item->setSizeHint(itemWidget->sizeHint());
        m_deviceList->setItemWidget(item, itemWidget);
        return item;
    };
    
    // 添加测试设备
    createDeviceItem(":/images/scanner.svg", "测试扫描仪1", "USB连接 - 支持彩色扫描");
    createDeviceItem(":/images/scanner.svg", "测试扫描仪2", "网络连接 - 支持双面扫描");
    createDeviceItem(":/images/webcam.svg", "虚拟扫描设备", "摄像头模拟 - 支持拍照扫描");
    
    deviceContainerLayout->addWidget(m_deviceList, 3);
    
    // 扫描按钮
    m_scanButton = new QPushButton(tr("开始扫描"), deviceContainer);
    m_scanButton->setFixedWidth(100);
    deviceContainerLayout->addWidget(m_scanButton, 1);
    
    m_deviceLayout->addWidget(deviceContainer);
    
    mainLayout->addWidget(m_devicePanel, 1); // 1/3宽度
    
    // 右侧设置面板
    m_rightPanel = new QWidget(centralWidget);
    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPanel);
    
    // 扫描设置表单
    m_settingsLayout = new QFormLayout();
    
    // 添加分辨率设置
    m_resolutionCombo = new QComboBox();
    m_resolutionCombo->addItems({"300 dpi", "600 dpi", "1200 dpi"});
    m_settingsLayout->addRow("分辨率:", m_resolutionCombo);

    // 添加色彩模式设置
    m_colorModeCombo = new QComboBox();
    m_colorModeCombo->addItems({"彩色", "灰度", "黑白"});
    m_settingsLayout->addRow("色彩模式:", m_colorModeCombo);

    // 添加文件格式设置
    m_formatCombo = new QComboBox();
    m_formatCombo->addItems({"PDF", "PNG", "JPEG"});
    m_settingsLayout->addRow("文件格式:", m_formatCombo);

    // 加载偏好设置
    loadPreferences();
    
    rightLayout->addLayout(m_settingsLayout);
    
    // 扫描按钮
    m_scanButton = new QPushButton("开始扫描", m_rightPanel);
    m_scanButton->setFixedHeight(40);
    rightLayout->addWidget(m_scanButton);
    
    mainLayout->addWidget(m_rightPanel, 1); // 1/3宽度
    
    setCentralWidget(centralWidget);
}

void ScanWindow::initConnections()
{
    // 连接扫描按钮信号
    connect(m_scanButton, &QPushButton::clicked, this, [this]() {
        if (!validateSettings()) {
            return;
        }

        // 保存当前偏好
        savePreferences();

        // 获取并转换参数
        int dpi = convertResolution(m_resolutionCombo->currentText());
        QString colorMode = convertColorMode(m_colorModeCombo->currentText());
        QString format = convertFileFormat(m_formatCombo->currentText());

        // 调用扫描设备
        m_scannerDevice->setScanParams(dpi, colorMode);
        m_scannerDevice->startScan("/tmp/scan_temp." + format.toLower());
    });
    
    // 连接设备信号
    connect(m_scannerDevice, &ScannerDevice::scanProgress,
            this, &ScanWindow::handleScanProgress);
    connect(m_scannerDevice, &ScannerDevice::scanCompleted,
            this, &ScanWindow::handleScanCompleted);
    connect(m_scannerDevice, &ScannerDevice::scanError,
            this, &ScanWindow::handleScanError);
            
    connect(m_webcamDevice, &WebcamDevice::captureCompleted,
            this, &ScanWindow::handleCaptureCompleted);
}

void ScanWindow::handleScanProgress(int percentage)
{
    // 更新进度条显示
    qDebug() << "扫描进度:" << percentage << "%";
}

void ScanWindow::handleScanCompleted(const QImage &scannedImage)
{
    updatePreview(scannedImage);
    qDebug() << "扫描完成";
}

void ScanWindow::handleScanError(const QString &errorMessage)
{
    qWarning() << "扫描错误:" << errorMessage;
}

void ScanWindow::handleCaptureCompleted(const QImage &capturedImage)
{
    updatePreview(capturedImage);
}

void ScanWindow::updatePreview(const QImage &image)
{
    // 已移除预览功能
}

// 参数转换函数
int ScanWindow::convertResolution(const QString &resolutionText)
{
    return resolutionText.split(" ").first().toInt();
}

QString ScanWindow::convertColorMode(const QString &colorModeText)
{
    static QMap<QString, QString> modeMap = {
        {"彩色", "Color"},
        {"灰度", "Grayscale"},
        {"黑白", "LineArt"}
    };
    return modeMap.value(colorModeText, "Color");
}

QString ScanWindow::convertFileFormat(const QString &formatText)
{
    return formatText.toLower();
}

// 参数验证函数
bool ScanWindow::validateSettings()
{
    int dpi = convertResolution(m_resolutionCombo->currentText());
    if (dpi < 100 || dpi > 2400) {
        qWarning() << "不支持的分辨率:" << dpi;
        return false;
    }
    return true;
}

// 偏好保存/加载
void ScanWindow::savePreferences()
{
    QSettings settings;
    settings.setValue("scan/resolution", m_resolutionCombo->currentIndex());
    settings.setValue("scan/colorMode", m_colorModeCombo->currentIndex());
    settings.setValue("scan/fileFormat", m_formatCombo->currentIndex());
}

void ScanWindow::loadPreferences()
{
    QSettings settings;
    m_resolutionCombo->setCurrentIndex(settings.value("scan/resolution", 0).toInt());
    m_colorModeCombo->setCurrentIndex(settings.value("scan/colorMode", 0).toInt());
    m_formatCombo->setCurrentIndex(settings.value("scan/fileFormat", 0).toInt());
}