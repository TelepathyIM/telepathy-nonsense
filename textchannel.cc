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

#include "textchannel.hh"
#include "common.hh"
#include "connection.hh"

QString xmppConditionToStr(QXmppStanza::Error::Condition condition)
{
    switch (condition) {
    case QXmppStanza::Error::BadRequest:
        return QStringLiteral("Bad request");
    case QXmppStanza::Error::Conflict:
        return QStringLiteral("Conflict");
    case QXmppStanza::Error::FeatureNotImplemented:
        return QStringLiteral("Feature is not implemented");
    case QXmppStanza::Error::Forbidden:
        return QStringLiteral("Forbidden");
    case QXmppStanza::Error::Gone:
        return QStringLiteral("Gone");
    case QXmppStanza::Error::InternalServerError:
        return QStringLiteral("Internal server error");
    case QXmppStanza::Error::ItemNotFound:
        return QStringLiteral("Item not found");
    case QXmppStanza::Error::JidMalformed:
        return QStringLiteral("Jid is malformed");
    case QXmppStanza::Error::NotAcceptable:
        return QStringLiteral("Not acceptable");
    case QXmppStanza::Error::NotAllowed:
        return QStringLiteral("Not allowed");
    case QXmppStanza::Error::NotAuthorized:
        return QStringLiteral("Not authorized");
    case QXmppStanza::Error::PaymentRequired:
        return QStringLiteral("Payment required");
    case QXmppStanza::Error::RecipientUnavailable:
        return QStringLiteral("Recipient unavailable");
    case QXmppStanza::Error::Redirect:
        return QStringLiteral("Redirect");
    case QXmppStanza::Error::RegistrationRequired:
        return QStringLiteral("Registration required");
    case QXmppStanza::Error::RemoteServerNotFound:
        return QStringLiteral("Remote server not found");
    case QXmppStanza::Error::RemoteServerTimeout:
        return QStringLiteral("Remote server timeout");
    case QXmppStanza::Error::ResourceConstraint:
        return QStringLiteral("Resource constraint");
    case QXmppStanza::Error::ServiceUnavailable:
        return QStringLiteral("Service unavailable");
    case QXmppStanza::Error::SubscriptionRequired:
        return QStringLiteral("Subscription required");
    case QXmppStanza::Error::UndefinedCondition:
        return QStringLiteral("Undefined condition");
    case QXmppStanza::Error::UnexpectedRequest:
        return QStringLiteral("Unexpected request");
    default:
        return QStringLiteral("Unknown error");
    }
}

TextChannel::TextChannel(Connection *connection, Tp::BaseChannel *baseChannel)
    : Tp::BaseChannelTextType(baseChannel),
      m_connection(connection),
      m_targetHandle(baseChannel->targetHandle()),
      m_targetJid(baseChannel->targetID())
{
    DBG;
    QStringList supportedContentTypes = QStringList() << QStringLiteral("text/plain");
    Tp::UIntList messageTypes = Tp::UIntList() << Tp::ChannelTextMessageTypeNormal
                                               << Tp::ChannelTextMessageTypeDeliveryReport;

    uint messagePartSupportFlags = Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead;
    uint deliveryReportingSupport = Tp::DeliveryReportingSupportFlagReceiveSuccesses | Tp::DeliveryReportingSupportFlagReceiveRead;

    setMessageAcknowledgedCallback(Tp::memFun(this, &TextChannel::messageAcknowledged));

    m_messagesIface = Tp::BaseChannelMessagesInterface::create(this,
                                                               supportedContentTypes,
                                                               messageTypes,
                                                               messagePartSupportFlags,
                                                               deliveryReportingSupport);

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_messagesIface));
    m_messagesIface->setSendMessageCallback(Tp::memFun(this, &TextChannel::sendMessage));

    m_chatStateIface = Tp::BaseChannelChatStateInterface::create();
    m_chatStateIface->setSetChatStateCallback(Tp::memFun(this, &TextChannel::setChatState));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_chatStateIface));
}

TextChannelPtr TextChannel::create(Connection *connection, Tp::BaseChannel *baseChannel)
{
    return TextChannelPtr(new TextChannel(connection, baseChannel));
}

QString TextChannel::sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
    QUuid messageToken = QUuid::createUuid();
    QXmppMessage message;
    message.setTo(targetJid());
    message.setFrom(selfJid());
    message.setId(messageToken.toString());

    if (flags & (Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead)) {
        message.setReceiptRequested(true);
        message.setMarkable(true);
    }

    QString content;
    for (auto &part : messageParts) {
        // TODO handle other parts?
        if(part.count(QStringLiteral("content-type")) && part.value(QStringLiteral("content-type")).variant().toString() == QLatin1String("text/plain") && part.count(QStringLiteral("content"))) {
            content = part.value(QStringLiteral("content")).variant().toString();
            break;
        }
    }
    message.setBody(content);

    sendQXmppMessage(message);
    return messageToken.toString();
}

void TextChannel::onMessageReceived(const QXmppMessage &message)
{
    processReceivedMessage(message, m_targetHandle, m_targetJid);
}

void TextChannel::processReceivedMessage(const QXmppMessage &message, uint senderHandle, const QString &senderID)
{
    Tp::MessagePart header;
    header[QStringLiteral("message-token")] = QDBusVariant(message.id());
    if (message.stamp().isValid()) {
        header[QStringLiteral("message-sent")]  = QDBusVariant(message.stamp().toMSecsSinceEpoch() / 1000);
    }
    header[QStringLiteral("message-received")]  = QDBusVariant(QDateTime::currentMSecsSinceEpoch() / 1000);
    header[QStringLiteral("message-sender")]    = QDBusVariant(senderHandle);
    header[QStringLiteral("message-sender-id")] = QDBusVariant(senderID);

    /* Handle chat states */
    if (message.state() != QXmppMessage::None) {
        Tp::ChannelChatState state = Tp::ChannelChatStateActive;
        switch(message.state()) {
        case QXmppMessage::Active:
            state = Tp::ChannelChatStateActive;
            break;
        case QXmppMessage::Composing:
            state = Tp::ChannelChatStateComposing;
            break;
        case QXmppMessage::Gone:
            state = Tp::ChannelChatStateGone;
            break;
        case QXmppMessage::Inactive:
            state = Tp::ChannelChatStateInactive;
            break;
        case QXmppMessage::Paused:
            state = Tp::ChannelChatStatePaused;
            break;
        default:
            Q_ASSERT(0);
        }
        m_chatStateIface->chatStateChanged(senderHandle, state);
    }

    if (message.type() == QXmppMessage::Error) {
        Tp::MessagePartList partList;
        header[QStringLiteral("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);

        switch (message.error().type()) {
        // It seems that there is no "continue" error type in the spec
        case QXmppStanza::Error::Cancel:
        case QXmppStanza::Error::Modify:
        case QXmppStanza::Error::Auth:
            header[QStringLiteral("delivery-status")] = QDBusVariant(Tp::DeliveryStatusPermanentlyFailed);
            break;
        case QXmppStanza::Error::Wait:
            header[QStringLiteral("delivery-status")] = QDBusVariant(Tp::DeliveryStatusTemporarilyFailed);
            break;
        default:
            break;
        }

        QString errorMessage = xmppConditionToStr(message.error().condition());

        if (message.error().code() != 0) {
            errorMessage.append(QString(QStringLiteral(" (code %1)")).arg(message.error().code()));
        }

        header[QStringLiteral("delivery-error-message")] = QDBusVariant(errorMessage);

        partList << header;
        addReceivedMessage(partList);
        return;
    }

    /* Handle chat markers */
    if (message.marker() != QXmppMessage::NoMarker) {
        Tp::MessagePartList partList;
        header[QStringLiteral("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);

        switch (message.marker()) {
        case QXmppMessage::Acknowledged:
            header[QStringLiteral("delivery-status")] = QDBusVariant(Tp::DeliveryStatusRead);
            break;
        case QXmppMessage::Displayed:
        case QXmppMessage::Received:
            header[QStringLiteral("delivery-status")] = QDBusVariant(Tp::DeliveryStatusDelivered);
            break;
        default:
            Q_ASSERT(0);
        }
        partList << header;
        addReceivedMessage(partList);
    }

    /* Send receipt */
    if (message.isReceiptRequested()) {
        QUuid outMessageToken = QUuid::createUuid();
        QXmppMessage outMessage;
        outMessage.setMarker(QXmppMessage::Received);
        outMessage.setTo(message.from());
        outMessage.setFrom(selfJid());
        outMessage.setMarkerId(message.id());
        outMessage.setId(outMessageToken.toString());

        sendQXmppMessage(outMessage);
    }

    /* Text message */
    if (!message.body().isEmpty()) {
        Tp::MessagePartList body;
        Tp::MessagePart text;
        text[QStringLiteral("content-type")] = QDBusVariant(QStringLiteral("text/plain"));
        text[QStringLiteral("content")]      = QDBusVariant(message.body());
        body << text;

        Tp::MessagePartList partList;
        header[QStringLiteral("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
        partList << header << body;
        addReceivedMessage(partList);
    }
}

void TextChannel::onCarbonMessageSent(const QXmppMessage &message)
{
    Tp::MessagePart header;
    header[QStringLiteral("message-token")] = QDBusVariant(message.id());
    if (message.stamp().isValid()) {
        header[QStringLiteral("message-sent")]  = QDBusVariant(message.stamp().toMSecsSinceEpoch() / 1000);
    }
    header[QStringLiteral("message-received")]  = QDBusVariant(QDateTime::currentMSecsSinceEpoch() / 1000);
    header[QStringLiteral("message-sender")]    = QDBusVariant(m_connection->ensureContactHandle(selfJid()));
    header[QStringLiteral("message-sender-id")] = QDBusVariant(selfJid());

    /* Text message */
    if (!message.body().isEmpty()) {
        Tp::MessagePartList body;
        Tp::MessagePart text;
        text[QStringLiteral("content-type")] = QDBusVariant(QStringLiteral("text/plain"));
        text[QStringLiteral("content")]      = QDBusVariant(message.body());
        body << text;

        Tp::MessagePartList partList;
        header[QStringLiteral("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
        partList << header << body;
        m_messagesIface->messageSent(partList, 0, message.id());
    }
}

bool TextChannel::sendQXmppMessage(QXmppMessage &message)
{
    return m_connection->qxmppClient()->sendPacket(message);
}

QString TextChannel::targetJid() const
{
    return m_targetJid + m_connection->lastResourceForJid(m_targetJid);
}

QString TextChannel::selfJid() const
{
    return m_connection->qxmppClient()->configuration().jid();
}

void TextChannel::messageAcknowledged(const QString &messageId)
{
    QUuid messageToken = QUuid::createUuid();
    QXmppMessage message;
    message.setMarker(QXmppMessage::Displayed);
    message.setTo(targetJid()); //TODO: Should we make sure that we send the "displayed" ack to the same resource as the "received" ack?
    message.setFrom(selfJid());
    message.setId(messageToken.toString());
    message.setMarkerId(messageId);

    sendQXmppMessage(message);
}

void TextChannel::setChatState(uint state, Tp::DBusError *error)
{
    Q_UNUSED(error);

    QUuid messageToken = QUuid::createUuid();
    QXmppMessage message;
    switch (state) {
    case Tp::ChannelChatStateActive:
        message.setState(QXmppMessage::Active);
        break;
    case Tp::ChannelChatStateComposing:
        message.setState(QXmppMessage::Composing);
        break;
    case Tp::ChannelChatStateGone:
        message.setState(QXmppMessage::Gone);
        break;
    case Tp::ChannelChatStateInactive:
        message.setState(QXmppMessage::Inactive);
        break;
    case Tp::ChannelChatStatePaused:
        message.setState(QXmppMessage::Paused);
        break;
    default:
        Q_ASSERT(0);
    }
    message.setTo(targetJid());
    message.setFrom(selfJid());
    message.setId(messageToken.toString());

    sendQXmppMessage(message);
}
