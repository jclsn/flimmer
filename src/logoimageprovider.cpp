// SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
//
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "logoimageprovider.h"

#include "networkauth.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QQuickTextureFactory>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThreadStorage>
#include <QTimer>

namespace
{
constexpr int s_timeoutMs = 15000;
constexpr qint64 s_refreshAfterSecs = 7 * 24 * 3600;
constexpr int s_maxAttempts = 3;
constexpr int s_retryDelayMs = 1000;

QString cacheFilePath(const QUrl &url)
{
    static const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/logos");

    QDir().mkpath(dir);
    const QByteArray hash =
        QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Sha1).toHex();

    return dir + QLatin1Char('/') + QString::fromLatin1(hash);
}

// Image requests run on QML's image loader thread, which needs its own manager.
QNetworkAccessManager *threadNetworkManager()
{
    static QThreadStorage<QNetworkAccessManager *> storage;
    if (!storage.hasLocalData()) {
        auto *networkManager = new QNetworkAccessManager;
        NetworkAuth::install(networkManager);
        storage.setLocalData(networkManager);
    }

    return storage.localData();
}

class LogoResponse : public QQuickImageResponse
{
public:
    LogoResponse(const QUrl &url, const QSize &requestedSize)
        : m_url(url)
        , m_requestedSize(requestedSize)
        , m_cacheFile(cacheFilePath(url))
    {
        const QFileInfo cached(m_cacheFile);

        if (cached.exists()
            && cached.lastModified().secsTo(QDateTime::currentDateTime()) < s_refreshAfterSecs
            && loadFromCache()) {
            // finished() must not be emitted before the response is returned.
            QMetaObject::invokeMethod(this, &LogoResponse::finished, Qt::QueuedConnection);
            return;
        }

        download(threadNetworkManager(), 1);
    }

    QQuickTextureFactory *textureFactory() const override
    {
        QMutexLocker lock(&m_mutex);
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    QString errorString() const override
    {
        QMutexLocker lock(&m_mutex);
        return m_error;
    }

    void cancel() override
    {
        m_cancelled = true;

        if (m_reply) {
            m_reply->abort();
        }
    }

private:
    bool loadFromCache()
    {
        QImage image(m_cacheFile);

        if (image.isNull()) {
            return false;
        }

        setImage(image);
        return true;
    }

    void setImage(QImage image)
    {
        // Only scale down; QML asks for a height (sourceSize) on channel logos.
        const int w = m_requestedSize.width();
        const int h = m_requestedSize.height();
        if (w > 0 && h > 0 && (image.width() > w || image.height() > h)) {
            image = image.scaled(m_requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else if (h > 0 && w <= 0 && image.height() > h) {
            image = image.scaledToHeight(h, Qt::SmoothTransformation);
        } else if (w > 0 && h <= 0 && image.width() > w) {
            image = image.scaledToWidth(w, Qt::SmoothTransformation);
        }

        QMutexLocker lock(&m_mutex);
        m_image = image;
    }

    void download(QNetworkAccessManager *networkManager, int attempt)
    {
        QNetworkRequest request(m_url);
        request.setTransferTimeout(s_timeoutMs);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        m_reply = networkManager->get(request);
        connect(m_reply, &QNetworkReply::finished, this, [this, networkManager, attempt]() {
            QNetworkReply *reply = m_reply;
            m_reply = nullptr;
            reply->deleteLater();

            if (attempt > 1) {
                networkManager->deleteLater();
            }

            if (reply->error() == QNetworkReply::NoError) {
                const QByteArray data = reply->readAll();

                QImage image;
                if (image.loadFromData(data)) {
                    QSaveFile file(m_cacheFile);

                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(data);
                        file.commit();
                    }

                    setImage(image);
                    Q_EMIT finished();

                    return;
                }
            }

            // An outdated copy beats no logo at all.
            if (m_cancelled || loadFromCache()) {
                Q_EMIT finished();

                return;
            }

            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const bool authProblem = status == 401 || status == 403
                || reply->error() == QNetworkReply::AuthenticationRequiredError;

            if (authProblem && attempt < s_maxAttempts) {
                // A fresh manager starts a new connection and a new digest handshake.
                QTimer::singleShot(s_retryDelayMs * attempt, this, [this, attempt]() {
                    if (m_cancelled) {
                        Q_EMIT finished();
                        return;
                    }

                    auto *fresh = new QNetworkAccessManager;
                    NetworkAuth::install(fresh);
                    download(fresh, attempt + 1);
                });

                return;
            }

            QMutexLocker lock(&m_mutex);
            m_error = reply->errorString();
            lock.unlock();

            Q_EMIT finished();
        });
    }

    const QUrl m_url;
    const QSize m_requestedSize;
    const QString m_cacheFile;
    QPointer<QNetworkReply> m_reply;
    mutable QMutex m_mutex;
    QImage m_image;
    QString m_error;
    bool m_cancelled = false;
};
}

QQuickImageResponse *LogoImageProvider::requestImageResponse(const QString &id,
                                                             const QSize &requestedSize)
{
    return new LogoResponse(QUrl(QUrl::fromPercentEncoding(id.toUtf8())), requestedSize);
}
