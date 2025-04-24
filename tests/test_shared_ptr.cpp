// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include <QSharedPointer>
#include "src/device/devicebase.h"
#include "src/device/scannerdevice.h"
#include "src/mainwindow.h"
#include "src/ui/scanwidget.h"

class TestSharedPtr : public QObject
{
    Q_OBJECT

private slots:
    void testSharedPointerCreation();
    void testReferenceCounting();
    void testCrossComponentUsage();
    void testMemoryLeak();
};

void TestSharedPtr::testSharedPointerCreation()
{
    // 测试共享指针创建
    QSharedPointer<ScannerDevice> scanner(new ScannerDevice());
    QVERIFY(!scanner.isNull());

    // 测试动态转换
    QSharedPointer<DeviceBase> base = scanner.dynamicCast<DeviceBase>();
    QVERIFY(!base.isNull());
}

void TestSharedPtr::testReferenceCounting()
{
    QSharedPointer<ScannerDevice> scanner(new ScannerDevice());
    QCOMPARE(scanner.use_count(), 1);

    {
        QSharedPointer<ScannerDevice> scanner2 = scanner;
        QCOMPARE(scanner.use_count(), 2);
    }

    QCOMPARE(scanner.use_count(), 1);
}

void TestSharedPtr::testCrossComponentUsage()
{
    // 模拟MainWindow创建设备
    MainWindow mainWindow;
    QSharedPointer<DeviceBase> device = mainWindow.getDevice("scanner");
    QVERIFY(!device.isNull());

    // 模拟传递到ScanWidget
    ScanWidget scanWidget;
    scanWidget.setupDeviceMode(device, true);
    QVERIFY(!scanWidget.getDevice().isNull());
}

void TestSharedPtr::testMemoryLeak()
{
    // 使用QSignalSpy检测析构信号
    QSharedPointer<ScannerDevice> scanner(new ScannerDevice());
    QSignalSpy spy(scanner.data(), &ScannerDevice::destroyed);

    scanner.clear();
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestSharedPtr)
#include "test_shared_ptr.moc"