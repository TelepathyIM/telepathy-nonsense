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

#ifndef QXMPP_TEXTCHANNEL_HPP
#define QXMPP_TEXTCHANNEL_HPP

#include <TelepathyQt/BaseChannel>

#include <QXmppClient.h>
#include <QXmppMessage.h>

class TextChannel;

typedef Tp::SharedPtr<TextChannel> TextChannelPtr;

class TextChannel : public Tp::BaseChannelTextType
{
    Q_OBJECT
public:
    static TextChannelPtr create(QXmppClient *client, Tp::BaseChannel *baseChannel, uint contactHandle, const QString &contactJid, uint selfHandle, const QString &selfJid);

public slots:
    void onMessageReceived(const QXmppMessage &message);

private:
    TextChannel(QXmppClient *client, Tp::BaseChannel *baseChannel, uint targetHandle, const QString &jid, uint selfHandle, const QString &selfJid);
    QString sendMessage(const Tp::MessagePartList &messageParts, uint flags, Tp::DBusError *error);
    void setChatState(uint state, Tp::DBusError *error);
    void messageAcknowledged(const QString &messageId);

private:
    Tp::BaseChannelMessagesInterfacePtr m_messagesIface;
    Tp::BaseChannelChatStateInterfacePtr m_chatStateIface;

    QPointer<QXmppClient> m_client;
    uint m_contactHandle;
    QString m_contactJid;
    uint m_selfHandle;
    QString m_selfJid;
};

#endif // QXMPP_TEXTCHANNEL_HPP
