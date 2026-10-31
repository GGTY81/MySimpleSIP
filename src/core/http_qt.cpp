/*
 * MySimpleSIP — requisições HTTP de URLGetAsync/URLGetSync (global.cpp) com Qt Network.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Substitui o corpo de URLGetAsyncThread (WinINet: CInternetSession/CHttpFile) com o
// mesmo comportamento: GET/POST/PUT (post = 0/1/2); POST sem corpo usa a query string
// como corpo; Content-Type padrão application/x-www-form-urlencoded; usuário/senha da
// URL ou dos parâmetros (Basic/Digest via authenticationRequired); erros de certificado
// ignorados (como as flags SECURITY_FLAG_IGNORE_* do original); tempo limite de 10 s;
// devolve status, corpo e cabeçalhos brutos ("Nome: valor\r\n").
// Requer uma QCoreApplication (a UI Qt) — sem ela, statusCode fica 0.

#include "global.h"

#include <QAuthenticator>
#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QThread>
#include <QUrl>

#include <functional>

void msip_http_request(URLGetAsyncData* data)
{
    data->body.Empty();
    data->statusCode = 0;
    if (data->url.IsEmpty() || !QCoreApplication::instance()) {
        data->headers.Empty();
        return;
    }

    QUrl url(QString::fromStdWString(data->url.str()));
    if (!url.isValid() || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))) {
        data->headers.Empty();
        return;
    }
    QString username = url.userName();
    QString password = url.password();
    if (username.isEmpty()) {
        username = QString::fromStdWString(data->username.str());
        password = QString::fromStdWString(data->password.str());
    }
    url.setUserInfo(QString());

    QByteArray body;
    if (!data->postData.IsEmpty()) {
        body = QByteArray(CStringA(data->postData).GetString());
    }
    CString requestHeaders = data->headers;
    data->headers.Empty();
    if (data->post) {
        if (data->postData.IsEmpty() && url.hasQuery()) {
            body = url.query(QUrl::FullyEncoded).toUtf8();
            url.setQuery(QString());
        }
        if (requestHeaders.IsEmpty()) {
            requestHeaders = _T("Content-Type: application/x-www-form-urlencoded");
        }
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    for (const QString& line : QString::fromStdWString(requestHeaders.str()).split(QRegularExpression(QStringLiteral("\r?\n")), Qt::SkipEmptyParts)) {
        int colon = line.indexOf(QLatin1Char(':'));
        if (colon > 0) {
            request.setRawHeader(line.left(colon).trimmed().toUtf8(), line.mid(colon + 1).trimmed().toUtf8());
        }
    }

    QNetworkAccessManager manager;
    QObject::connect(&manager, &QNetworkAccessManager::authenticationRequired,
                     [&](QNetworkReply*, QAuthenticator* auth) {
                         if (!username.isEmpty() && !password.isEmpty()) {
                             auth->setUser(username);
                             auth->setPassword(password);
                         }
                     });
    QNetworkReply* reply = data->post == 2 ? manager.put(request, body)
        : data->post ? manager.post(request, body)
                     : manager.get(request);
    QObject::connect(reply, &QNetworkReply::sslErrors, reply, [reply] { reply->ignoreSslErrors(); });
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if (status.isValid()) {
        data->statusCode = status.toUInt();
        QByteArray bytes = reply->readAll();
        data->body = CStringA(CStringA::string_type(bytes.constData(), bytes.size()));
        // MySimpleSIP: como HTTP_QUERY_RAW_HEADERS_CRLF do WinINet — linha de status e depois
        // os cabeçalhos com o nome em forma canônica (o Qt 6 entrega os nomes em minúsculas;
        // o código do MicroSIP procura "Content-Type: text/csv" e "\r\ncache-control:")
        CString headers;
        headers.Format(_T("HTTP/1.1 %u %s\r\n"), data->statusCode,
                       CString(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toByteArray().constData()).GetString());
        for (const auto& pair : reply->rawHeaderPairs()) {
            QByteArray name = pair.first.toLower();
            for (int i = 0; i < name.size(); i++) {
                if (i == 0 || name[i - 1] == '-') {
                    name[i] = QChar::toUpper(name[i]);
                }
            }
            headers.Append(CString(name.constData()));
            headers.Append(_T(": "));
            headers.Append(CString(pair.second.constData()));
            headers.Append(_T("\r\n"));
        }
        data->headers = headers;
    }
    reply->deleteLater();
}

// Executa fn numa QThread (precisa ser QThread para o QEventLoop do QNetworkAccessManager).
// Retorna false se não houver QCoreApplication; o chamador então executa na própria thread.
bool msip_run_in_thread(std::function<void()> fn)
{
    if (!QCoreApplication::instance()) {
        return false;
    }
    QThread* thread = QThread::create(std::move(fn));
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
    return true;
}
