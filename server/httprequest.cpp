/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "httprequest.h"
#include "loggingcategories.h"

#include <QUrlQuery>


HttpRequest::HttpRequest() :
    m_rawData(QByteArray()),
    m_valid(false),
    m_isComplete(false)
{
}

HttpRequest::HttpRequest(QByteArray rawData) :
    m_rawData(rawData),
    m_valid(false),
    m_isComplete(false)
{
    validate();
}

QByteArray HttpRequest::rawHeader() const
{
    return m_rawHeader;
}

QHash<QByteArray, QByteArray> HttpRequest::rawHeaderList() const
{
    return m_rawHeaderList;
}

HttpRequest::RequestMethod HttpRequest::method() const
{
    return m_method;
}

QString HttpRequest::methodString() const
{
    return m_methodString;
}

QByteArray HttpRequest::httpVersion() const
{
    return m_httpVersion;
}

QUrl HttpRequest::url() const
{
    return m_url;
}

QUrlQuery HttpRequest::urlQuery() const
{
    return m_urlQuery;
}

QByteArray HttpRequest::payload() const
{
    return m_payload;
}

bool HttpRequest::isValid() const
{
    return m_valid;
}

bool HttpRequest::isComplete() const
{
    return m_isComplete;
}

bool HttpRequest::hasPayload() const
{
    return !m_payload.isEmpty();
}

void HttpRequest::appendData(const QByteArray &data)
{
    m_rawData.append(data);
    validate();
}

void HttpRequest::validate()
{
    m_isComplete = true; m_valid = false;

    // Parese the HTTP request. The request is invalid, until the end of the parse process.
    if (m_rawData.isEmpty())
        return;

    // split the data into header and payload
    int headerEndIndex = m_rawData.indexOf("\r\n\r\n");
    if (headerEndIndex < 0) {
        qCWarning(dcWebServer) << "Could not parse end of HTTP header (empty line between header and body):" << m_rawData;
        return;
    }
    m_rawHeader = m_rawData.left(headerEndIndex);
    m_payload = m_rawData.right(m_rawData.length() - headerEndIndex).simplified();

    // parse status line
    QStringList headerLines = QString(m_rawHeader).split(QRegExp("\r\n"));
    QString statusLine = headerLines.takeFirst();
    QStringList statusLineTokens = statusLine.split(QRegExp("[ \r\n][ \r\n]*"));
    if (statusLineTokens.count() != 3) {
        qCWarning(dcWebServer) << "Could not parse HTTP status line:" << statusLine;
        return;
    }

    // verify http version
    m_httpVersion = statusLineTokens.at(2).toUtf8().simplified();
    if (!m_httpVersion.contains("HTTP")) {
        qCWarning(dcWebServer) << "Unknown HTTP version:" << m_httpVersion;
        return;
    }
    m_methodString = statusLineTokens.at(0).simplified();
    m_method = getRequestMethodType(m_methodString);

    m_url = QUrl("http://example.com" + statusLineTokens.at(1).simplified());

    if (m_url.hasQuery())
        m_urlQuery = QUrlQuery(m_url.query());

    // verify header formating
    foreach (const QString &line, headerLines) {
        if (!line.contains(":")) {
            qCWarning(dcWebServer) << "Invalid HTTP header:" << line;
            return;
        }
        int index = line.indexOf(":");
        QByteArray key = line.left(index).toUtf8().simplified();
        QByteArray value = line.right(line.count() - index - 1).toUtf8().simplified();
        m_rawHeaderList.insert(key, value);
    }

    // check User-Agent
    if (!m_rawHeaderList.contains("User-Agent")) {
        qWarning() << "User-Agent header is missing";
        return;
    }

    // verify content length with actual payload
    if (m_rawHeaderList.contains("Content-Length")) {
        bool ok = false;
        int contentLength = m_rawHeaderList.value("Content-Length").toInt(&ok);
        if (!ok) {
            qCWarning(dcWebServer) << "Could not parse Content-Length.";
            return;
        }
        // check if we have all data
        if (m_payload.size() < contentLength) {
            qCDebug(dcWebServer) << "Request incomplete:";
            qCDebug(dcWebServer) << "   -> Content-Length:" << contentLength;
            qCDebug(dcWebServer) << "   -> Payload size  :" << payload().size();
            m_isComplete = false;
            return;
        }
        // check if the content lenght bigger than header Content-Length
        if (m_payload.size() > contentLength) {
            qCWarning(dcWebServer) << "Payload size greater than header Content-Length:";
            qCWarning(dcWebServer) << "   -> Content-Length:" << contentLength;
            qCWarning(dcWebServer) << "   -> Payload size  :" << payload().size();
            m_isComplete = true;
            return;
        }

    }
    m_valid = true;
}

HttpRequest::RequestMethod HttpRequest::getRequestMethodType(const QString &methodString)
{
    if (methodString == "GET") {
        return RequestMethod::Get;
    } else if (methodString == "POST") {
        return RequestMethod::Post;
    } else if (methodString == "PUT") {
        return RequestMethod::Put;
    } else if (methodString == "DELETE") {
        return RequestMethod::Delete;
    }
    qCWarning(dcWebServer) << "Method" << methodString << "will not be handled.";
    return RequestMethod::Unhandled;
}

QDebug operator<<(QDebug debug, const HttpRequest &httpRequest)
{
    debug << "===================================" << "\n";
    debug << "  HTTP version: " << httpRequest.httpVersion() << "\n";
    debug << "        method: " << httpRequest.methodString() << "\n";
    debug << "      URL path: " << httpRequest.url().path() << "\n";
    debug << "     URL query: " << httpRequest.urlQuery().query() << "\n";
    debug << "      is valid: " << httpRequest.isValid() << "\n";
    debug << "-----------------------------------" << "\n";
    debug << httpRequest.rawHeader() << "\n";
    debug << "-----------------------------------" << "\n";
    debug << httpRequest.payload() << "\n";
    debug << "-----------------------------------" << "\n";
    return debug;
}
