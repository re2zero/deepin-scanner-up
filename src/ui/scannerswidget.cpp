#include "scannerswidget.h"
#include <QSharedPointer>
#include <QHBoxLayout>
#include <DLabel>
#include <DFrame>
#include <QListWidgetItem>
#include <QGraphicsDropShadowEffect>
#include <DIcon>

ScannersWidget::ScannersWidget(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void ScannersWidget::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    
    // Title bar with refresh button
    QHBoxLayout *titleLayout = new QHBoxLayout();
    DLabel *titleLabel = new DLabel(tr("扫描仪设备"));
    refreshButton = new DPushButton(tr("刷新"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);
    mainLayout->addLayout(titleLayout);

    // Device list
    deviceList = new QListWidget();
    // deviceList->setStyleSheet(
    //     "QListWidget { border: 1px solid #ddd; border-radius: 10px; spacing: 10px; }"
    //     "QListWidget::item { border-bottom: 1px solid #eee; }"
    //     "QListWidget::item:hover { background: #f5f5f5; }"
    // );
    deviceList->setSpacing(10);
    mainLayout->addWidget(deviceList);

    connect(refreshButton, &QPushButton::clicked, this, [this](){
        emit updateDeviceListRequested();
    });
}

void ScannersWidget::updateDeviceList(QSharedPointer<ScannerDevice> scanner, QSharedPointer<WebcamDevice> webcam)
{
    deviceList->clear();
    
    // Add scanner devices
    QStringList scannerDevices = scanner->getAvailableDevices();
    for (const QString &name : scannerDevices) {
        addDeviceItem(name, tr("扫描仪"), tr("空闲"), true);
    }
    
    // Add webcam devices
    QStringList webcamDevices = webcam->getAvailableDevices();
    for (const QString &name : webcamDevices) {
        addDeviceItem(name, tr("摄像头"), tr("空闲"), false);
    }
    
    // Update UI based on device availability
    if (deviceList->count() == 0) {
        DLabel *emptyLabel = new DLabel(tr("未找到可用设备"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        deviceList->addItem(new QListWidgetItem());
        deviceList->setItemWidget(deviceList->item(0), emptyLabel);
    }
}

void ScannersWidget::addDeviceItem(const QString &name, const QString &model, 
                                 const QString &status, bool isScanner)
{
    QListWidgetItem *item = new QListWidgetItem(deviceList);
    item->setData(Qt::UserRole, name);
    item->setData(Qt::UserRole + 1, isScanner);
    
    DFrame *itemWidget = new DFrame();
    // // 设置阴影效果和背景色
    // itemWidget->setStyleSheet(R"(
    //     DFrame {
    //         background-color: #f5f5f5;  /* 淡灰色背景 */
    //         border-radius: 8px;
    //         border: 1px solid #e0e0e0;
    //         padding: 5px;
    //     }
    //     DFrame:hover {
    //         background-color: #eeeeee;  /* 鼠标悬停时稍深的灰色 */
    //     }
    // )");
    itemWidget->setGraphicsEffect(new QGraphicsDropShadowEffect());
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(itemWidget);
    shadow->setBlurRadius(10);  // 阴影模糊半径
    shadow->setOffset(2, 2);    // 阴影偏移
    shadow->setColor(QColor(0, 0, 0, 50));  // 半透明黑色阴影
    itemWidget->setGraphicsEffect(shadow);
    itemWidget->setBackgroundRole(QPalette::Window);

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    
    // Device icon
    DLabel *iconLabel = new DLabel();
    // TODO: Replace with actual device icon
    iconLabel->setFixedSize(150, 150);
    QIcon icon;
    if (isScanner) {
        icon = QIcon::fromTheme("scanner");
    } else {
        icon = QIcon::fromTheme("camera");
    }
    iconLabel->setPixmap(icon.pixmap(150, 150));
    layout->addWidget(iconLabel);
    
    // Device info
    QVBoxLayout *infoLayout = new QVBoxLayout();
    DLabel *nameLabel = new DLabel(name);
    DLabel *modelLabel = new DLabel(tr("型号: %1").arg(model));
    DLabel *statusLabel = new DLabel(tr("状态: %1").arg(status));
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(modelLabel);
    infoLayout->addWidget(statusLabel);
    infoLayout->setMargin(10);
    layout->addLayout(infoLayout);
    layout->addStretch();
    
    // Scan button
    DPushButton *scanButton = new DPushButton(tr("扫描"));
    scanButton->setProperty("deviceName", name);
    scanButton->setProperty("isScanner", isScanner);
    scanButton->setFixedWidth(150);
    scanButton->setFixedHeight(60);
    layout->addWidget(scanButton);
    layout->setMargin(10);
    
    connect(scanButton, &DPushButton::clicked, this, [this, name, isScanner](){
        emit deviceSelected(name, isScanner);
    });

    if (status == "空闲") {
        scanButton->setEnabled(true);
    } else {
        scanButton->setEnabled(false);
    }
    
    // itemWidget->setLayout(layout);
    item->setSizeHint(itemWidget->sizeHint());
    deviceList->setItemWidget(item, itemWidget);
}
