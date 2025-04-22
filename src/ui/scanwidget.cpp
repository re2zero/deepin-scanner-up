#include "scanwidget.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <DPushButton>
#include <DLabel>
#include <QDebug>

ScanWidget::ScanWidget(QWidget *parent) : QWidget(parent),
    m_isScanner(false),
    m_previewTimer(this)
{
    setupUI();
    
    // 设置预览更新定时器
    m_previewTimer.setInterval(100); // 10 FPS
    connect(&m_previewTimer, &QTimer::timeout, this, &ScanWidget::updatePreview);
}

void ScanWidget::setupUI()
{
    splitter = new QSplitter(Qt::Horizontal, this);
    
    // Preview area (3/4 width)
    previewArea = new QWidget();
    previewArea->setMinimumSize(480, 360);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewArea);
    
    previewLabel = new DLabel();
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(previewLabel);
    
    splitter->addWidget(previewArea);
    
    // Settings area (1/4 width)
    settingsArea = new QWidget();
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsArea);
    
    // Device settings group
    QGroupBox *settingsGroup = new QGroupBox(tr("扫描设置"));
    QVBoxLayout *groupLayout = new QVBoxLayout(settingsGroup);
    
    // Device mode options
    m_modeLabel = new DLabel();
    m_modeCombo = new QComboBox();
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onScanModeChanged);
    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(m_modeLabel);
    modeLayout->addWidget(m_modeCombo);
    groupLayout->addLayout(modeLayout);

    // Resolution options
    DLabel *resLabel = new DLabel(tr("分辨率"));
    m_resolutionCombo = new QComboBox();
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onResolutionChanged);
    QHBoxLayout *resolutionLayout = new QHBoxLayout();
    resolutionLayout->addWidget(resLabel);
    resolutionLayout->addWidget(m_resolutionCombo);
    groupLayout->addLayout(resolutionLayout);

    // Color mode options
    DLabel *colorLabel = new DLabel(tr("色彩模式"));
    m_colorCombo = new QComboBox();
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onColorModeChanged);
    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(colorLabel);
    colorLayout->addWidget(m_colorCombo);
    groupLayout->addLayout(colorLayout);

    // Format options
    DLabel *formatLabel = new DLabel(tr("图片格式"));
    m_formatCombo = new QComboBox();
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onFormatChanged);
    QHBoxLayout *formatLayout = new QHBoxLayout();
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(m_formatCombo);
    groupLayout->addLayout(formatLayout);
    
    settingsLayout->addWidget(settingsGroup);
    settingsLayout->addStretch();
    
    // Action buttons
    DPushButton *scanButton = new DPushButton(tr("扫描"));
    DPushButton *saveButton = new DPushButton(tr("保存"));
    settingsLayout->addWidget(scanButton);
    settingsLayout->addWidget(saveButton);
    
    connect(scanButton, &DPushButton::clicked, this, &ScanWidget::scanRequested);
    connect(saveButton, &DPushButton::clicked, this, &ScanWidget::saveRequested);

    splitter->addWidget(settingsArea);
    splitter->setSizes({3, 1});
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
}

void ScanWidget::setupDeviceMode(QSharedPointer<DeviceBase> device, QString name)
{
    if (!device) return;
    
    // 释放之前的设备
    if (m_device) {
        m_device->closeDevice();
        m_device.clear();
    }
    
    // 设置新设备
    auto deviceType = device->getDeviceType();
    m_isScanner = device->getDeviceType() == DeviceBase::Scanner;

    if (!device->openDevice(name)) {
        qWarning() << "无法打开设备:" << name;
        return;
    }

    m_device = device;
    
    if (m_isScanner) {
        m_modeLabel->setText(tr("扫描模式"));
        m_modeCombo->addItems({"平板", "ADF", "双面"});
    } else {
        m_modeLabel->setText(tr("视频格式"));
        m_modeCombo->addItems({"MJPG", "YUYV", "H264"});
    }
    m_colorCombo->addItems({"彩色", "灰度", "黑白"});
    m_formatCombo->addItems({"PNG", "JPG", "TIFF", "OFD"});
    
    connectDeviceSignals();
    updateDeviceSettings();
}

void ScanWidget::connectDeviceSignals()
{
    if (!m_device) return;
    
    if (m_isScanner) {
        auto scanner = qSharedPointerDynamicCast<ScannerDevice>(m_device);
        if (scanner) {
            // 使用data()获取原始指针并添加生命周期管理
            connect(scanner.data(), &ScannerDevice::previewLineAvailable,
                   this, &ScanWidget::setPreviewImage);
            connect(scanner.data(), &ScannerDevice::scanError,
                   this, &ScanWidget::handleDeviceError);
        }
    } else {
        auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
        if (webcam) {
            // 使用data()获取原始指针并添加生命周期管理
            connect(webcam.data(), &WebcamDevice::captureCompleted,
                   this, &ScanWidget::setPreviewImage);
            connect(webcam.data(), &WebcamDevice::captureError,
                   this, &ScanWidget::handleDeviceError);
        }
    }
}

void ScanWidget::updateDeviceSettings()
{
    // 更新分辨率选项
    m_resolutionCombo->clear();
    if (m_isScanner) {
        auto scanner = qSharedPointerDynamicCast<ScannerDevice>(m_device);
        if (scanner) {
            m_resolutionCombo->addItems({"300", "600", "1200"});
        }
    } else {
        auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
        if (webcam) {
            for (const auto &res : webcam->getSupportedResolutions()) {
                m_resolutionCombo->addItem(QString("%1x%2").arg(res.width()).arg(res.height()));
            }
        }
    }
}

void ScanWidget::startCameraPreview()
{
    if (!m_device) return;
    
    if (m_isScanner) {
        auto scanner = qSharedPointerDynamicCast<ScannerDevice>(m_device);
        if (scanner) {
            scanner->startScan("temp_scan.png");
        }
    } else {
        auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
        if (webcam) {
            previewLabel->setText(tr("预览初始化中..."));
            webcam->stopPreview();   // 确保先停止任何预览
            QTimer::singleShot(100, [webcam]() {
                webcam->startPreview();
            });
            m_previewTimer.start();
        } else {
            previewLabel->setText(tr("设备无法预览"));
        }
    }
}

void ScanWidget::stopCameraPreview()
{
    if (!m_device) return;

    m_previewTimer.stop();
    
    if (m_isScanner) {
        auto scanner = qSharedPointerDynamicCast<ScannerDevice>(m_device);
        if (scanner) {
            scanner->cancelScan();
        }
    } else {
        auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
        if (webcam) {
            webcam->stopPreview();
        }
    }
}

void ScanWidget::updatePreview()
{
    if (!m_device || m_isScanner) return;
    
    auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
    if (webcam) {
        // 获取最新帧并更新预览
        QImage frame = webcam->getLatestFrame();
        if (!frame.isNull()) {
            setPreviewImage(frame);
        }
    }
}

void ScanWidget::setPreviewImage(const QImage &image)
{
    if (image.isNull()) {
        previewLabel->setText(tr("无预览图像"));
        return;
    }
    
    QMutexLocker locker(&m_previewMutex);
    QPixmap pixmap = QPixmap::fromImage(image);
    QPixmap scaled = pixmap.scaled(previewLabel->size(),
                                  Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);
    previewLabel->setPixmap(scaled);
    previewLabel->setAlignment(Qt::AlignCenter);
}

void ScanWidget::handleDeviceError(const QString &error)
{
    qWarning() << "Device error:" << error;
    previewLabel->setText(error);
}

// 以下为参数变更处理函数
void ScanWidget::onResolutionChanged(int index)
{
    if (!m_device) return;
    
    if (m_isScanner) {
        auto scanner = qSharedPointerDynamicCast<ScannerDevice>(m_device);
        if (scanner) {
            scanner->setResolution(m_resolutionCombo->itemText(index).toInt());
        }
    } else {
        auto webcam = qSharedPointerDynamicCast<WebcamDevice>(m_device);
        if (webcam) {
            QString res = m_resolutionCombo->itemText(index);
            QStringList parts = res.split('x');
            if (parts.size() == 2) {
                webcam->setResolution(parts[0].toInt(), parts[1].toInt());
            }
        }
    }
    
    emit deviceSettingsChanged();
}

void ScanWidget::onColorModeChanged(int index)
{
    // 实现颜色模式变更逻辑
    emit deviceSettingsChanged();
}

void ScanWidget::onFormatChanged(int index)
{
    // 实现格式变更逻辑
    emit deviceSettingsChanged();
}

void ScanWidget::onScanModeChanged(int index)
{
    if (m_isScanner) {
        // "平板", "ADF", "双面"
    } else {
        // "MJPG", "YUYV", "H264"
    }
    // 实现扫描模式变更逻辑
    emit deviceSettingsChanged();
}