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

#include "muctextchannel.hh"

#include "common.hh"
#include "connection.hh"

#include <QXmppMucManager.h>
#include <QXmppUtils.h>

MucTextChannel::MucTextChannel(Connection *connection, Tp::BaseChannel *baseChannel) :
    TextChannel(connection, baseChannel)
{
    QXmppMucManager *mucManager = m_connection->qxmppClient()->findExtension<QXmppMucManager>();
    const QXmppConfiguration &clientConfig = m_connection->qxmppClient()->configuration();
    m_room = mucManager->addRoom(baseChannel->targetID());
    m_room->setNickName(clientConfig.user());

    if (!m_room->isJoined()) {
        m_room->join();
    }

    connect(m_room, SIGNAL(participantsChanged()), this, SLOT(onMucParticipantsChanged()));

    // Default flags:
    Tp::ChannelGroupFlags groupFlags = Tp::ChannelGroupFlagChannelSpecificHandles|Tp::ChannelGroupFlagHandleOwnersNotAvailable;
    // Permissions (not implemented yet):
    groupFlags |= Tp::ChannelGroupFlagCanAdd|Tp::ChannelGroupFlagCanRemove;

    m_groupIface = Tp::BaseChannelGroupInterface::create();
    m_groupIface->setGroupFlags(groupFlags);
    m_groupIface->setSelfHandle(connection->ensureContactHandle(selfJid()));
    m_groupIface->setAddMembersCallback(Tp::memFun(this, &MucTextChannel::addMembers));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_groupIface));

    const QString roomName = QXmppUtils::jidToUser(m_room->jid());
    const QString serverName = QXmppUtils::jidToDomain(m_room->jid());

    m_roomIface = Tp::BaseChannelRoomInterface::create(roomName, serverName, /* creator */ QString(), /* creatorHandle */ 0, /* creationTimestamp */ QDateTime());
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_roomIface));

    m_roomConfigIface = Tp::BaseChannelRoomConfigInterface::create();

    if (m_room->isJoined()) {
        m_roomConfigIface->setTitle(m_room->name());
        m_roomConfigIface->setDescription(m_room->subject());
    }

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_roomConfigIface));

    connect(m_room, SIGNAL(nameChanged(QString)), this, SLOT(onRoomNameChanged(QString)));
    connect(m_room, SIGNAL(messageReceived(QXmppMessage)), this, SLOT(onMessageReceived(QXmppMessage)));
}

void MucTextChannel::addMembers(const Tp::UIntList &contacts, const QString &reason, Tp::DBusError *error)
{
    QStringList jids;
    for (uint handle : contacts) {
        const QString jid = m_connection->getContactIdentifier(handle);
        if (jid.isEmpty()) {
            if (error) {
                error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
            }
            return;
        }

        jids.append(jid);
    }

    // Direct Invitation, XEP-0249
    for (const QString &jid : jids) {
        QUuid messageToken = QUuid::createUuid();
        QXmppMessage message;
        message.setType(QXmppMessage::Normal);
        message.setTo(jid); // contacts
        message.setId(messageToken.toString());
        message.setMucInvitationJid(m_room->jid());
        message.setMucInvitationReason(reason);
        m_connection->qxmppClient()->sendPacket(message);
    }
}

MucTextChannel::~MucTextChannel()
{
    m_room->leave();
}

void MucTextChannel::onMessageReceived(const QXmppMessage &message)
{
    uint sender = m_groupIface->memberIdentifiers().key(message.from());

    if (sender == 0) {
        qDebug() << Q_FUNC_INFO << "unknown sender" << message.from() << "body:" << message.body();
        return;
    }

    if (m_sentIds.contains(message.id())) {
        m_sentIds.removeOne(message.id());
        // TODO: Mark message as delivered
        return;
    }

    processReceivedMessage(message, sender, message.from());
}

MucTextChannelPtr MucTextChannel::create(Connection *connection, Tp::BaseChannel *baseChannel)
{
    return MucTextChannelPtr(new MucTextChannel(connection, baseChannel));
}

void MucTextChannel::onRoomNameChanged(const QString &newName)
{
    m_roomConfigIface->setTitle(newName);
}

void MucTextChannel::onMucParticipantsChanged()
{
    DBG;

    Tp::UIntList handles;

    for (const QString &participant : m_room->participants()) {
        handles << m_connection->ensureContactHandle(participant);

        const QXmppPresence &presence = m_room->participantPresence(participant);
        m_connection->updateMucParticipantInfo(participant, presence);
        m_connection->updateJidPresence(participant, presence);
    }

    m_groupIface->setMembers(handles, /* details */ QVariantMap());
}

bool MucTextChannel::sendQXmppMessage(QXmppMessage &message)
{
    if (message.state() != QXmppMessage::None) {
        return false;
    }

    m_sentIds << message.id();

    message.setType(QXmppMessage::GroupChat);
    return TextChannel::sendQXmppMessage(message);
}

QString MucTextChannel::targetJid() const
{
    return m_room->jid();
}

QString MucTextChannel::selfJid() const
{
    return m_connection->qxmppClient()->configuration().jid();
//    return m_room->jid() + QLatin1Char('/') + m_room->nickName();
}
