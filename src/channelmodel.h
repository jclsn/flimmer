/*
 * SPDX-FileCopyrightText: 2026 Jan Claußen <jan.claussen10@web.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setChannels(const QVector<Channel> &channels);
    [[nodiscard]] const QVector<Channel> &channels() const
    {
        return m_channels;
    }
    void setBaseUrl(const QUrl &url);
    // Re-evaluates now/next information; call when time advances or EPG changes.
    void refreshNow();

    Q_INVOKABLE [[nodiscard]] int indexOfUuid(const QString &uuid) const;
    Q_INVOKABLE [[nodiscard]] int indexOfNumber(int number) const;
    Q_INVOKABLE [[nodiscard]] QVariantMap get(int row) const;
    Q_INVOKABLE [[nodiscard]] QUrl iconUrl(int row) const;

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
