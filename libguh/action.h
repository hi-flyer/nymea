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

#ifndef ACTION_H
#define ACTION_H

#include <QUuid>
#include <QVariantList>

class Action
{
public:
    explicit Action(const QUuid &deviceId, const QUuid &actionTypeId);

    bool isValid() const;

    QUuid actionTypeId() const;
    QUuid deviceId() const;

    QVariantMap params() const;
    void setParams(const QVariantMap &params);

private:
    QUuid m_actionTypeId;
    QUuid m_deviceId;
    QVariantMap m_params;
};

#endif // ACTION_H
