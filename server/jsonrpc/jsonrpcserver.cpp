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

#include "jsonrpcserver.h"
#include "jsontypes.h"

#ifdef TESTING_ENABLED
#include "mocktcpserver.h"
#else
#include "tcpserver.h"
#endif
#include "jsonhandler.h"

#include "guhcore.h"
#include "devicemanager.h"
#include "deviceplugin.h"
#include "deviceclass.h"
#include "device.h"
#include "rule.h"
#include "ruleengine.h"

#include "devicehandler.h"
#include "actionhandler.h"
#include "ruleshandler.h"

#include <QJsonDocument>
#include <QStringList>

JsonRPCServer::JsonRPCServer(QObject *parent):
    JsonHandler(parent),
#ifdef TESTING_ENABLED
    m_tcpServer(new MockTcpServer(this))
#else
    m_tcpServer(new TcpServer(this))
#endif
{
    // First, define our own JSONRPC methods
    QVariantMap returns;
    QVariantMap params;

    params.clear(); returns.clear();
    setDescription("Introspect", "Introspect this API.");
    setParams("Introspect", params);
    returns.insert("methods", "object");
    returns.insert("types", "object");
    setReturns("Introspect", returns);

    params.clear(); returns.clear();
    setDescription("Version", "Version of this Guh/JSONRPC interface.");
    setParams("Version", params);
    returns.insert("version", "string");
    setReturns("Version", returns);

    params.clear(); returns.clear();
    setDescription("SetNotificationStatus", "Enable/Disable notifications for this connections.");
    setParams("SetNotificationStatus", params);
    returns.insert("status", "string");
//    returns.insert("enabled", "bool");
    setReturns("SetNotificationStatus", returns);

    // Now set up the logic
    connect(m_tcpServer, SIGNAL(clientConnected(const QUuid &)), this, SLOT(clientConnected(const QUuid &)));
    connect(m_tcpServer, SIGNAL(clientDisconnected(const QUuid &)), this, SLOT(clientDisconnected(const QUuid &)));
    connect(m_tcpServer, SIGNAL(dataAvailable(const QUuid &, QByteArray)), this, SLOT(processData(const QUuid &, QByteArray)));
    m_tcpServer->startServer();

    registerHandler(this);
    registerHandler(new DeviceHandler(this));
    registerHandler(new ActionHandler(this));
    registerHandler(new RulesHandler(this));
}

QString JsonRPCServer::name() const
{
    return QStringLiteral("JSONRPC");
}

QVariantMap JsonRPCServer::Introspect(const QVariantMap &params) const
{
    QVariantMap data;
    data.insert("types", JsonTypes::allTypes());
    QVariantMap methods;
    foreach (JsonHandler *handler, m_handlers) {
//                qDebug() << "got handler" << handler->name() << handler->introspect();
        methods.unite(handler->introspect());
    }
    data.insert("methods", methods);
    return data;
}

QVariantMap JsonRPCServer::Version(const QVariantMap &params) const
{
    QVariantMap data;
    data.insert("version", "0.0.0");
    return data;
}

QVariantMap JsonRPCServer::SetNotificationStatus(const QVariantMap &params)
{
    QUuid clientId = this->property("clientId").toUuid();
//    qDebug() << "got client socket" << clientId;
    m_clients[clientId] = params.value("enabled").toBool();
    QVariantMap returns;
    returns.insert("status", "success");
    returns.insert("enabled", m_clients[clientId]);
    return returns;
}

void JsonRPCServer::processData(const QUuid &clientId, const QByteArray &jsonData)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &error);

    if(error.error != QJsonParseError::NoError) {
        qDebug() << "failed to parse data" << jsonData << ":" << error.errorString();
    }
//    qDebug() << "-------------------------\n" << jsonDoc.toJson();

    QVariantMap message = jsonDoc.toVariant().toMap();

    bool success;
    int commandId = message.value("id").toInt(&success);
    if (!success) {
        qWarning() << "Error parsing command. Missing \"id\":" << jsonData;
        return;
    }

    QStringList commandList = message.value("method").toString().split('.');
    if (commandList.count() != 2) {
        qWarning() << "Error parsing method.\nGot:" << message.value("method").toString() << "\nExpected: \"Namespace.method\"";
        return;
    }

    QString targetNamespace = commandList.first();
    QString method = commandList.last();
    QVariantMap params = message.value("params").toMap();

    qDebug() << "got:" << targetNamespace << method << params;
    emit commandReceived(targetNamespace, method, params);

    JsonHandler *handler = m_handlers.value(targetNamespace);
    if (!handler) {
        sendErrorResponse(clientId, commandId, "No such namespace");
        return;
    }
    if (!handler->hasMethod(method)) {
        sendErrorResponse(clientId, commandId, "No such method");
        return;
    }
    if (!handler->validateParams(method, params)) {
        sendErrorResponse(clientId, commandId, "Invalid params");
        return;
    }

    // Hack: attach clientId to handler to be able to handle the JSONRPC methods. Do not use this outside of jsonrpcserver
    handler->setProperty("clientId", clientId);

    QVariantMap returns;
    QMetaObject::invokeMethod(handler, method.toLatin1().data(), Q_RETURN_ARG(QVariantMap, returns), Q_ARG(QVariantMap, params));
    Q_ASSERT((targetNamespace == "JSONRPC" && method == "Introspect") || handler->validateReturns(method, returns));
    sendResponse(clientId, commandId, returns);
}

void JsonRPCServer::registerHandler(JsonHandler *handler)
{
    m_handlers.insert(handler->name(), handler);
}

void JsonRPCServer::clientConnected(const QUuid &clientId)
{
    // Notifications disabled by default
    m_clients.insert(clientId, false);
}

void JsonRPCServer::clientDisconnected(const QUuid &clientId)
{
    m_clients.remove(clientId);
}

void JsonRPCServer::sendResponse(const QUuid &clientId, int commandId, const QVariantMap &params)
{
    QVariantMap rsp;
    rsp.insert("id", commandId);
    rsp.insert("status", "success");
    rsp.insert("params", params);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(rsp);
    m_tcpServer->sendData(clientId, jsonDoc.toJson());
}

void JsonRPCServer::sendErrorResponse(const QUuid &clientId, int commandId, const QString &error)
{
    QVariantMap rsp;
    rsp.insert("id", commandId);
    rsp.insert("status", "error");
    rsp.insert("error", error);

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(rsp);
    m_tcpServer->sendData(clientId, jsonDoc.toJson());
}


