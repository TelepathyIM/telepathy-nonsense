/*
 * This file is part of the telepathy-nonsense connection manager.
 * Copyright (C) 2015 Niels Ole Salscheider <niels_ole@salscheider-online.de>
 * Copyright (C) 2014 Alexandr Akulich <akulichalexander@gmail.com>
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

#ifndef TEXTCHANNEL_HH
#define TEXTCHANNEL_HH

#include <TelepathyQt/BaseChannel>

#include <QXmppMessage.h>

class TextChannel;
class Connection;

typedef Tp::SharedPtr<TextChannel> TextChannelPtr;

class TextChannel : public Tp::BaseChannelTextType
{
    Q_OBJECT
public:
    static TextChannelPtr create(Connection *connection, Tp::BaseChannel *baseChannel);

public slots:
    virtual void onMessageReceived(const QXmppMessage &message);

protected:
    TextChannel(Connection *connection, Tp::BaseChannel *baseChannel);
    QString sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);
    void setChatState(uint state, Tp::DBusError *error);
    void messageAcknowledged(const QString &messageId);

    void processReceivedMessage(const QXmppMessage &message, uint senderHandle, const QString &senderID);

    virtual bool sendQXmppMessage(QXmppMessage &message);
    virtual QString targetJid() const;
    virtual QString selfJid() const;

protected:
    Tp::BaseChannelMessagesInterfacePtr m_messagesIface;
    Tp::BaseChannelChatStateInterfacePtr m_chatStateIface;

    Connection *m_connection;
    uint m_targetHandle;
    QString m_targetJid;
};

#endif // TEXTCHANNEL_HH
