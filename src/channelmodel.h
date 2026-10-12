#ifndef CHANNELMODEL_H
#define CHANNELMODEL_H

#include "epgstore.h"

#include <QAbstractListModel>
#include <QUrl>
#include <qqmlregistration.h>

class ChannelModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by Tvh.channels")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        UuidRole = Qt::UserRole + 1,
        NumberRole,
        NameRole,
        IconUrlRole,
        NowIdRole,
        NowTitleRole,
        NowSubtitleRole,
        NowSummaryRole,
        NowStartRole,
        NowStopRole,
        NowProgressRole,
        NowGenreRole,
        NowRecordingRole,
        NextTitleRole,
        NextStartRole,
    };
    Q_ENUM(Role)

    explicit ChannelModel(const EpgStore *store, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChannels(const QVector<Channel> &channels);
    const QVector<Channel> &channels() const
    {
        return m_channels;
    }
    void setBaseUrl(const QUrl &url);
    // Re-evaluates now/next information; call when time advances or EPG changes.
    void refreshNow();

    Q_INVOKABLE int indexOfUuid(const QString &uuid) const;
    Q_INVOKABLE int indexOfNumber(int number) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QUrl iconUrl(int row) const;

Q_SIGNALS:
    void countChanged();

private:
    struct NowNext {
        const EpgEvent *now = nullptr;
        const EpgEvent *next = nullptr;
    };

    const EpgStore *m_store;
    QVector<Channel> m_channels;
    QVector<NowNext> m_nowNext;
    QUrl m_baseUrl;
};

#endif // CHANNELMODEL_H
