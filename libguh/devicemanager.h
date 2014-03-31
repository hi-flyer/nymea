/****************************************************************************
 *                                                                          *
 *  This file is part of guh.                                               *
 *                                                                          *
 *  Guh is free software: you can redistribute it and/or modify             *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, version 2 of the License.                 *
 *                                                                          *
 *  Guh is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with guh.  If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                          *
 ***************************************************************************/

#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "deviceclass.h"
#include "event.h"
#include "action.h"

#include <QObject>
#include <QTimer>

class Device;
class DevicePlugin;
class Radio433;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    enum HardwareResource {
        HardwareResourceNone = 0x00,
        HardwareResourceRadio433 = 0x01,
        HardwareResourceRadio868 = 0x02,
        HardwareResourceTimer = 0x04
    };
    Q_DECLARE_FLAGS(HardwareResources, HardwareResource)

    enum DeviceError {
        DeviceErrorNoError,
        DeviceErrorDeviceNotFound,
        DeviceErrorDeviceClassNotFound,
        DeviceErrorActionTypeNotFound,
        DeviceErrorMissingParameter,
        DeviceErrorPluginNotFound,
        DeviceErrorSetupFailed
    };

    explicit DeviceManager(QObject *parent = 0);

    QList<DevicePlugin*> plugins() const;
    DevicePlugin* plugin(const QUuid &id) const;
    QList<DeviceClass> supportedDevices() const;

    QList<Device*> configuredDevices() const;
    DeviceError addConfiguredDevice(const QUuid &deviceClassId, const QVariantMap &params);

    Device* findConfiguredDevice(const QUuid &id) const;
    QList<Device*> findConfiguredDevices(const QUuid &deviceClassId) const;
    DeviceClass findDeviceClassforEvent(const QUuid &eventTypeId) const;
    DeviceClass findDeviceClass(const QUuid &deviceClassId) const;

signals:
    void loaded();
    void emitEvent(const Event &event);

public slots:
    DeviceError executeAction(const Action &action);

private slots:
    void loadPlugins();
    void loadConfiguredDevices();
    void storeConfiguredDevices();

    void radio433SignalReceived(QList<int> rawData);
    void timerEvent();

private:
    bool setupDevice(Device *device);

    QHash<QUuid, DeviceClass> m_supportedDevices;
    QList<Device*> m_configuredDevices;

    QHash<QUuid, DevicePlugin*> m_devicePlugins;

    // Hardware Resources
    Radio433* m_radio433;
    QTimer m_pluginTimer;

    friend class DevicePlugin;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(DeviceManager::HardwareResources)

#endif // DEVICEMANAGER_H
