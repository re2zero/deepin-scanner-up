// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicebase.h"

DeviceBase::DeviceBase(QObject *parent)
    : QObject(parent),
      m_state(Disconnected)
{
}

void DeviceBase::setState(DeviceState state)
{
    if (m_state != state) {
        m_state = state;
        switch (state) {
        case Initialized:
            emit deviceInitialized(true);
            break;
        case Connected:
            emit deviceOpened(true);
            break;
        case Disconnected:
            emit deviceClosed();
            break;
        case Capturing:
            emit captureStarted();
            break;
        }
    }
}