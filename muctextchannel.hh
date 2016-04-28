/*
 * This file is part of the telepathy-nonsense connection manager.
 * Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef MUCTEXTCHANNEL_HPP
#define MUCTEXTCHANNEL_HPP

#include "textchannel.hh"

class QXmppMucRoom;

class MucTextChannel;

typedef Tp::SharedPtr<MucTextChannel> MucTextChannelPtr;

class MucTextChannel : public TextChannel
{
    Q_OBJECT
public:
    static MucTextChannelPtr create(Connection *connection, Tp::BaseChannel *baseChannel);
    ~MucTextChannel();

public slots:
    void onMessageReceived(const QXmppMessage &message) override;

protected slots:
    void onRoomNameChanged(const QString &newName);
    void onMucParticipantsChanged();

protected:
    MucTextChannel(Connection *connection, Tp::BaseChannel *baseChannel);
    void setChatState(uint state, Tp::DBusError *error);
    void messageAcknowledged(const QString &messageId);

    void addMembers(const Tp::UIntList &contacts, const QString &reason, Tp::DBusError *error);

    bool sendQXmppMessage(QXmppMessage &message) override;
    QString targetJid() const override;
    QString selfJid() const override;

protected:
    Tp::BaseChannelGroupInterfacePtr m_groupIface;
    Tp::BaseChannelRoomInterfacePtr m_roomIface;
    Tp::BaseChannelRoomConfigInterfacePtr m_roomConfigIface;

    QXmppMucRoom *m_room;

    QStringList m_sentIds;

};

#endif // MUCTEXTCHANNEL_HPP
