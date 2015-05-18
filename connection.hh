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

#ifndef QXMPP_CONNECTION_HPP
#define QXMPP_CONNECTION_HPP

#include <TelepathyQt/BaseConnection>
#include <TelepathyQt/BaseChannel>

#include <QXmppClient.h>
#include <QXmppVCardIq.h>
#include <QXmppMessage.h>

#include "uniquehandlemap.hh"

class Connection : public Tp::BaseConnection
{
    Q_OBJECT
public:
    Connection(const QDBusConnection &dbusConnection,
            const QString &cmName, const QString &protocolName,
            const QVariantMap &parameters);

private:
    void doConnect(Tp::DBusError *error);

    uint setPresence(const QString &status, const QString &message, Tp::DBusError *error);
    Tp::ContactAttributesMap getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error);
    Tp::ContactAttributesMap getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error);
    Tp::AliasMap getAliases(const Tp::UIntList &handles, Tp::DBusError *error);
    void setAliases(const Tp::AliasMap &aliases, Tp::DBusError *error);
    QStringList inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error);
    Tp::UIntList requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error);
    void requestSubscription(const Tp::UIntList &handles, const QString &message, Tp::DBusError *error);
    void authorizePublication(const Tp::UIntList &handles, Tp::DBusError *error);
    void removeContacts(const Tp::UIntList &handles, Tp::DBusError *error);
    void unsubscribe(const Tp::UIntList &handles, Tp::DBusError *error);
    void unpublish(const Tp::UIntList &handles, Tp::DBusError *error);
    void saslStartMechanismWithData(const QString &mechanism, const QByteArray &data, Tp::DBusError *error);
    Tp::BaseChannelPtr createChannel(const QString &channelType, uint targetHandleType, uint targetHandle, const QVariantMap &request, Tp::DBusError *error);
    void requestAvatars(const Tp::UIntList &handles, Tp::DBusError *error);
    Tp::AvatarTokenMap getKnownAvatarTokens(const Tp::UIntList &contacts, Tp::DBusError *error);
    void clearAvatar(Tp::DBusError *error);
    QString setAvatar(const QByteArray &avatar, const QString &mimetype, Tp::DBusError *error);

    Tp::SimplePresence toTpPresence(QMap<QString, QXmppPresence> presences);

private slots:
    void doDisconnect();

    void onConnected();
    void onDisconnected();
    void onError(QXmppClient::Error error);
    void onMessageReceived(const QXmppMessage &message);
    void onPresenceReceived(const QXmppPresence &presence);
//     void onIqReceived(const QXmppIq &iq);
//     void onStateChanged(QXmppClient::State state);

    void onRosterReceived();

    void onVCardReceived(QXmppVCardIq);
    void onClientVCardReceived();

private:
    Tp::BaseConnectionContactsInterfacePtr m_contactsIface;
    Tp::BaseConnectionSimplePresenceInterfacePtr m_simplePresenceIface;
    Tp::BaseConnectionContactListInterfacePtr m_contactListIface;
    Tp::BaseConnectionAliasingInterfacePtr m_aliasingIface;
    Tp::BaseConnectionAvatarsInterfacePtr m_avatarsIface;
    Tp::BaseConnectionRequestsInterfacePtr m_requestsIface;
    Tp::BaseChannelSASLAuthenticationInterfacePtr m_saslIface;

    QPointer<QXmppClient> m_client;
    QXmppPresence m_clientPresence;
    QXmppConfiguration m_clientConfig;
    UniqueHandleMap m_uniqueHandleMap;
    QMap<QString, QString> m_avatarTokens;
};

#endif // QXMPP_CONNECTION_HPP
