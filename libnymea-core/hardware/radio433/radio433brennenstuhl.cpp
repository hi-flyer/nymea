/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
  \class nymeaserver::Radio433Brennenstuhl
  \brief The Radio433 class helps to interact with the 433 MHz receiver and transmitter.

  \ingroup hardware
  \inmodule libnymea

  This class handles all supported radio 433 MHz transmitter. Receiving data on the 433.92 MHz frequency
  is only supported, if there are \l{Gpio}{GPIO's} available and a suitable receiver is connected to GPIO 27. Examples for receiver
  can be found \l{https://www.futurlec.com/Radio-433MHZ.shtml}{here}. The antenna has a very large impact on the quality
  of the signal and how well it is recognized. In many forums and blogs it is described that a 17, 3 mm piece of wire is enough.
  Experiments have shown, it's not. A 50 Ohm coaxial cabel (thickness = 1mm), mounted on the antenna pin of the receiver
  with a minimum distance of 5 cm away from the circuit and unisolated 17.3 mm at the end has shown the best results.

  In order to send data to a 433 MHz device, there currently are two possibilitis. If there are \l{Gpio}{GPIO's}
  available, the data will be sent over the transmitter connected to GPIO 22. Also in this case the antenna is a verry
  important part.

  The second possibility to sent data to a 433 MHz device is the \l{http://www.brennenstuhl.de/de-DE/steckdosenleisten-schaltgeraete-und-adapter/brematic-hausautomation/brematic-home-automation-gateway-gwy-433-1.html}
  {Brennenstuhl 433 MHz LAN Gateway}. If there is a Gateway in the local network, this class will automatically detect
  it and will be used. If both transmitter are available (Gateway + GPIO), each signal will be transmitted over both sender.

  \note: Radio 433 on GPIO's is by default disabled. If you want to enable it, you need to compile the source with the qmake config \tt{CONFIG+=radio433gpio}

*/


#include "radio433brennenstuhl.h"
#include "loggingcategories.h"
#include "gpio.h"

#include <QFileInfo>

namespace nymeaserver {

/*! Construct the hardware resource Radio433 with the given \a parent. Each possible 433 MHz hardware will be initialized here. */
Radio433Brennenstuhl::Radio433Brennenstuhl(QObject *parent) :
    Radio433(parent)
{
    m_brennenstuhlTransmitter = new Radio433BrennenstuhlGateway(this);
    connect(m_brennenstuhlTransmitter, &Radio433BrennenstuhlGateway::availableChanged, this, &Radio433Brennenstuhl::brennenstuhlAvailableChanged);

    qCDebug(dcHardware()) << "-->" << name() << "created successfully.";
}

bool Radio433Brennenstuhl::available() const
{
    return m_available;
}

bool Radio433Brennenstuhl::enabled() const
{
    return m_enabled;
}

void Radio433Brennenstuhl::brennenstuhlAvailableChanged(bool available)
{
    if (available) {
        qCDebug(dcHardware()) << name() << "Brennenstuhl LAN Gateway available.";
        m_available = true;
    } else {
        qCWarning(dcHardware()) << name() << "Brennenstuhl LAN Gateway not available.";
        m_available = false;
    }
    emit availableChanged(m_available);
}

/*! Returns true, if the \a rawData with a certain \a delay (pulse length) could be sent \a repetitions times. */
bool Radio433Brennenstuhl::sendData(int delay, QList<int> rawData, int repetitions)
{
    if (!available()) {
        qCWarning(dcHardware()) << name() << "Brennenstuhl gateway not available";
        return false;
    }

    if (!enabled()) {
        qCWarning(dcHardware()) << name() << "Hardware reouce disabled.";
        return false;
    }

    return m_brennenstuhlTransmitter->sendData(delay, rawData, repetitions);
}

void Radio433Brennenstuhl::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        qCDebug(dcHardware()) << "Radio433 Brennenstuhl gateway already" << (enabled ? "enabled" : "disabled");
        return;
    }
    if (enabled) {
        m_brennenstuhlTransmitter->enable();
        qCDebug(dcHardware()) << "Radio433 Brennenstuhl gateway enabled";
    } else {
        m_brennenstuhlTransmitter->disable();
        qCDebug(dcHardware()) << "Radio433 Brennenstuhl gateway disabled";
    }
    m_enabled = enabled;
    emit enabledChanged(m_enabled);
}

}
