#ifndef LOGOIMAGEPROVIDER_H
#define LOGOIMAGEPROVIDER_H

#include <QQuickAsyncImageProvider>

// Serves channel logos to QML as "image://logo/<percent-encoded URL>".
//
// Logos are kept in a disk cache so they show instantly and survive server
// hiccups: TVHeadend sometimes answers image requests with 403 instead of
// asking to authenticate again, and plain QML image loading gives up then.
// A refused download is retried up to twice on a fresh connection, and an outdated
// cached copy is used if the server still refuses.
class LogoImageProvider : public QQuickAsyncImageProvider
{
public:
    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;
};

#endif // LOGOIMAGEPROVIDER_H
