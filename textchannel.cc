/*
    Copyright (C) 2015 Niels Ole Salscheider <niels_ole@salscheider-online.de>
    Copyright (C) 2014 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "textchannel.hh"
#include "common.hh"
#include "connection.hh"

TextChannel::TextChannel(Connection *connection, Tp::BaseChannel *baseChannel, uint selfHandle, const QString &selfJid)
    : Tp::BaseChannelTextType(baseChannel),
      m_connection(connection),
      m_contactHandle(baseChannel->targetHandle()),
      m_contactJid(baseChannel->targetID()),
      m_selfHandle(selfHandle),
      m_selfJid(selfJid)
{
    DBG;

    QStringList supportedContentTypes = QStringList() << QLatin1String("text/plain");
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

TextChannelPtr TextChannel::create(Connection *connection, Tp::BaseChannel *baseChannel, uint selfHandle, const QString &selfJid)
{
    return TextChannelPtr(new TextChannel(connection, baseChannel, selfHandle, selfJid));
}

QString TextChannel::sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error)
{
    uint outFlags = 0;
    QUuid messageToken;
    QXmppMessage message;
    message.setTo(m_contactJid + m_connection->lastResourceForJid(m_contactJid));
    message.setFrom(m_selfJid + QStringLiteral("/") + m_connection->qxmppClient()->configuration().resource());
    message.setId(messageToken.toString());

    if (flags & (Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead)) {
        message.setReceiptRequested(true);
        message.setMarkable(true);
        outFlags |= Tp::MessageSendingFlagReportDelivery | Tp::MessageSendingFlagReportRead;
    }

    QString content;
    for (auto &part : messageParts) {
        // TODO handle other parts?
        if(part.count(QLatin1String("content-type")) && part.value(QLatin1String("content-type")).variant().toString() == QLatin1String("text/plain") && part.count(QLatin1String("content"))) {
            content = part.value(QLatin1String("content")).variant().toString();
            break;
        }
    }
    message.setBody(content);

    m_connection->qxmppClient()->sendPacket(message);
    return messageToken.toString();
}

void TextChannel::onMessageReceived(const QXmppMessage &message)
{
    Tp::MessagePart header;
    header[QLatin1String("message-token")] = QDBusVariant(message.id());
    header[QLatin1String("message-sent")]  = QDBusVariant(message.stamp());
    header[QLatin1String("message-received")]  = QDBusVariant(QDateTime::currentMSecsSinceEpoch() / 1000);
    header[QLatin1String("message-sender")]    = QDBusVariant(m_contactHandle);
    header[QLatin1String("message-sender-id")] = QDBusVariant(m_contactJid);

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
        m_chatStateIface->chatStateChanged(m_contactHandle, state);
    }

    /* Handle chat markers */
    if (message.marker() != QXmppMessage::NoMarker) {
        Tp::MessagePartList partList;
        header[QLatin1String("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeDeliveryReport);
        partList << header;

        switch (message.marker()) {
        case QXmppMessage::Acknowledged:
            header[QLatin1String("delivery-status")] = QDBusVariant(Tp::DeliveryStatusRead);
            break;
        case QXmppMessage::Displayed:
        case QXmppMessage::Received:
            header[QLatin1String("delivery-status")] = QDBusVariant(Tp::DeliveryStatusDelivered);
            break;
        default:
            Q_ASSERT(0);
        }
        addReceivedMessage(partList);
    }

    /* Send receipt */
    if (message.isReceiptRequested()) {
        QUuid outMessageToken;
        QXmppMessage outMessage;
        outMessage.setMarker(QXmppMessage::Received);
        outMessage.setTo(m_contactJid + m_connection->lastResourceForJid(m_contactJid));
        outMessage.setFrom(m_selfJid + QStringLiteral("/") + m_connection->qxmppClient()->configuration().resource());
        outMessage.setMarkerId(message.id());
        outMessage.setId(outMessageToken.toString());

        m_connection->qxmppClient()->sendPacket(outMessage);
    }

    /* Text message */
    if (!message.body().isEmpty()) {
        Tp::MessagePartList body;
        Tp::MessagePart text;
        text[QLatin1String("content-type")] = QDBusVariant(QLatin1String("text/plain"));
        text[QLatin1String("content")]      = QDBusVariant(message.body());
        body << text;

        Tp::MessagePartList partList;
        header[QLatin1String("message-type")]  = QDBusVariant(Tp::ChannelTextMessageTypeNormal);
        partList << header << body;
        addReceivedMessage(partList);
    }
}

void TextChannel::messageAcknowledged(const QString &messageId)
{
    QUuid messageToken;
    QXmppMessage message;
    message.setMarker(QXmppMessage::Displayed);
    message.setTo(m_contactJid + m_connection->lastResourceForJid(m_contactJid)); //TODO: Should we make sure that we send the "displayed" ack to the same resource as the "received" ack?
    message.setFrom(m_selfJid + QStringLiteral("/") + m_connection->qxmppClient()->configuration().resource());
    message.setId(messageToken.toString());
    message.setMarkerId(messageId);

    m_connection->qxmppClient()->sendPacket(message);
}

void TextChannel::setChatState(uint state, Tp::DBusError *error)
{
    Q_UNUSED(error);

    QUuid messageToken;
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
    message.setTo(m_contactJid + m_connection->lastResourceForJid(m_contactJid));
    message.setFrom(m_selfJid + QStringLiteral("/") + m_connection->qxmppClient()->configuration().resource()); //TODO: read XEP wrt resource
    message.setId(messageToken.toString());

    m_connection->qxmppClient()->sendPacket(message);
}
