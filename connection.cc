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

#include <QXmppRosterManager.h>
#include <QXmppVCardManager.h>
#include <QXmppVersionManager.h>
#include <QXmppUtils.h>
#include <QXmppPresence.h>

#include "connection.hh"
#include "textchannel.hh"
#include "common.hh"
#include "telepathy-qxmpp-config.h"

Connection::Connection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters), m_client (0)
{
    DBG;

    /* Connection.Interface.Contacts */
    m_contactsIface = Tp::BaseConnectionContactsInterface::create();
    m_contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &Connection::getContactAttributes));
    m_contactsIface->setContactAttributeInterfaces(QStringList()
                                                   << TP_QT_IFACE_CONNECTION
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_REQUESTS
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS);
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactsIface));

    /* Connection.Interface.SimplePresence */
    m_simplePresenceIface = Tp::BaseConnectionSimplePresenceInterface::create();
    m_simplePresenceIface->setStatuses(Common::getSimpleStatusSpecMap());
    m_simplePresenceIface->setSetPresenceCallback(Tp::memFun(this, &Connection::setPresence));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_simplePresenceIface));

    /* Connection.Interface.ContactList */
    m_contactListIface = Tp::BaseConnectionContactListInterface::create();
    m_contactListIface->setContactListPersists(true);
    m_contactListIface->setCanChangeContactList(true);
    m_contactListIface->setDownloadAtConnection(true);
    m_contactListIface->setGetContactListAttributesCallback(Tp::memFun(this, &Connection::getContactListAttributes));
    m_contactListIface->setRequestSubscriptionCallback(Tp::memFun(this, &Connection::requestSubscription));
    m_contactListIface->setAuthorizePublicationCallback(Tp::memFun(this, &Connection::authorizePublication));
    m_contactListIface->setRemoveContactsCallback(Tp::memFun(this, &Connection::removeContacts));
    m_contactListIface->setUnsubscribeCallback(Tp::memFun(this, &Connection::unsubscribe));
    m_contactListIface->setUnpublishCallback(Tp::memFun(this, &Connection::unpublish));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactListIface));

    /* Connection.Interface.Aliasing */
    m_aliasingIface = Tp::BaseConnectionAliasingInterface::create();
    m_aliasingIface->setGetAliasesCallback(Tp::memFun(this, &Connection::getAliases));
    m_aliasingIface->setSetAliasesCallback(Tp::memFun(this, &Connection::setAliases));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_aliasingIface));

    /* Connection.Interface.Avatars */
    m_avatarsIface = Tp::BaseConnectionAvatarsInterface::create();
    m_avatarsIface->setAvatarDetails(Common::getAvatarSpec());
    m_avatarsIface->setGetKnownAvatarTokensCallback(Tp::memFun(this, &Connection::getKnownAvatarTokens));
    m_avatarsIface->setRequestAvatarsCallback(Tp::memFun(this, &Connection::requestAvatars));
    m_avatarsIface->setClearAvatarCallback(Tp::memFun(this, &Connection::clearAvatar));
    m_avatarsIface->setSetAvatarCallback(Tp::memFun(this, &Connection::setAvatar));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_avatarsIface));

    /* Connection.Interface.Requests */
    m_requestsIface = Tp::BaseConnectionRequestsInterface::create(this);
    /* Fill requestableChannelClasses */
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    m_requestsIface->requestableChannelClasses << text;
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_requestsIface));

    QString myJid = parameters.value(QLatin1String("account")).toString();
    //QString server = parameters.value(QLatin1String("server")).toString();
    QString resource = parameters.value(QLatin1String("resource")).toString();
    uint priority = parameters.value(QLatin1String("priority")).toUInt();
    m_clientConfig.setJid(myJid);
    m_clientConfig.setResource(resource);
    m_clientConfig.setAutoAcceptSubscriptions(false);
    m_clientConfig.setAutoReconnectionEnabled(false);
    m_clientConfig.setIgnoreSslErrors(false);
    m_clientPresence.setPriority(priority);
    setSelfContact(m_uniqueHandleMap[myJid], myJid);

    setConnectCallback(Tp::memFun(this, &Connection::doConnect));
    setInspectHandlesCallback(Tp::memFun(this, &Connection::inspectHandles));
    setCreateChannelCallback(Tp::memFun(this, &Connection::createChannel));
    setRequestHandlesCallback(Tp::memFun(this, &Connection::requestHandles));
    connect(this, SIGNAL(disconnected()), SLOT(doDisconnect()));
}

void Connection::doConnect(Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(error);

    setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

    m_client = new QXmppClient();
    m_client->versionManager().setClientName(qAppName());
    m_client->versionManager().setClientVersion(telepathy_qxmpp_VERSION_STRING);
#if QT_VERSION >= 0x050000
    m_client->versionManager().setClientOs(QSysInfo::prettyProductName());
#endif

    connect(m_client, SIGNAL(connected()), this, SLOT(onConnected()));
 //   connect(m_client, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(m_client, SIGNAL(error(QXmppClient::Error)), this, SLOT(onError(QXmppClient::Error)));
    connect(m_client, SIGNAL(messageReceived(const QXmppMessage &)), this, SLOT(onMessageReceived(const QXmppMessage &)));
    connect(m_client, SIGNAL(presenceReceived(const QXmppPresence &)), this, SLOT(onPresenceReceived(const QXmppPresence &)));
 //   connect(m_client, SIGNAL(iqReceived(const QXmppIq &)), this, SLOT(onIqReceived(const QXmppIq &)));
//     connect(m_client, SIGNAL(stateChanged(QXmppClient::State)), this, SLOT(onStateChanged(QXmppClient::State)));

    connect(&m_client->rosterManager(), SIGNAL(rosterReceived()), this, SLOT(onRosterReceived()));

    connect(&m_client->vCardManager(), SIGNAL(vCardReceived(QXmppVCardIq)), this, SLOT(onVCardReceived(QXmppVCardIq)));
    connect(&m_client->vCardManager(), SIGNAL(clientVCardReceived()), this, SLOT(onClientVCardReceived()));

    //SASL channel registration
    Tp::DBusError saslError;
    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION);
    Tp::BaseChannelServerAuthenticationTypePtr authType = Tp::BaseChannelServerAuthenticationType::create(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(authType));

    m_saslIface = Tp::BaseChannelSASLAuthenticationInterface::create(QStringList() << QLatin1String("X-TELEPATHY-PASSWORD"), false, true, QString(), QString(), QString(), false);
    m_saslIface->setStartMechanismWithDataCallback( Tp::memFun(this, &Connection::saslStartMechanismWithData));

    baseChannel->setRequested(false);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_saslIface));

    baseChannel->registerObject(&saslError);
    if (!saslError.isValid())
        addChannel(baseChannel);
}

void Connection::saslStartMechanismWithData(const QString &mechanism, const QByteArray &data, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(error);
    Q_ASSERT(mechanism == QLatin1String("X-TELEPATHY-PASSWORD"));

    m_saslIface->setSaslStatus(Tp::SASLStatusInProgress, QLatin1String("InProgress"), QVariantMap());

    m_clientConfig.setPassword(data);

    m_client->connectToServer(m_clientConfig, m_clientPresence);
}

void Connection::doDisconnect()
{
    DBG;

    m_client->disconnectFromServer();
    setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonRequested);
}

void Connection::onConnected()
{
    DBG;

    setStatus(Tp::ConnectionStatusConnected, Tp::ConnectionStatusReasonRequested);
    m_saslIface->setSaslStatus(Tp::SASLStatusSucceeded, QLatin1String("Succeeded"), QVariantMap());

    Tp::SimpleContactPresences presences;
    QMap<QString, QXmppPresence> clientPresence;
    clientPresence.insert(m_clientConfig.jidBare(), m_client->clientPresence());
    presences[selfHandle()] = toTpPresence(clientPresence);
    m_simplePresenceIface->setPresences(presences);

    m_contactListIface->setContactListState(Tp::ContactListStateWaiting);
    m_clientPresence = m_client->clientPresence();
    m_client->vCardManager().requestClientVCard();
    if (m_clientPresence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
        m_avatarTokens[m_uniqueHandleMap[selfHandle()]] = m_clientPresence.photoHash();
    }
}

void Connection::onDisconnected()
{
//     DBG;
// 
//     //TODO reason?
//     setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonRequested);
}

void Connection::onError(QXmppClient::Error error)
{
    DBG;

    //TODO
    if (error == QXmppClient::SocketError || QXmppClient::KeepAliveError) {
        setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
    } else if (error == QXmppClient::XmppStreamError) {
        QXmppStanza::Error::Condition xmppStreamError = m_client->xmppStreamError();
        if (xmppStreamError == QXmppStanza::Error::NotAuthorized)
            setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonAuthenticationFailed);
        else
            setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNoneSpecified);
    } else
        Q_ASSERT(0);
}

void Connection::onRosterReceived()
{
    DBG;

    m_contactListIface->setContactListState(Tp::ContactListStateSuccess);
}

uint Connection::setPresence(const QString &status, const QString &message, Tp::DBusError *error)
{
    DBG;
    //TODO check

    if (m_client && m_client->isConnected()) {
        m_clientPresence = m_client->clientPresence();
    }

    if (status == QLatin1String("available")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::Online);
    } else if (status == QLatin1String("away")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::Away);
    } else if (status == QLatin1String("xa")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::XA);
    } else if (status == QLatin1String("dnd")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::DND);
    } else if (status == QLatin1String("chat")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::Chat);
    } else if (status == QLatin1String("hidden")) {
        m_clientPresence.setAvailableStatusType(QXmppPresence::Invisible);
    } else {
        Q_ASSERT(0);
    }
    m_clientPresence.setStatusText(message);

    if (m_client && m_client->isConnected()) {
        m_client->setClientPresence(m_clientPresence);
    }

    return selfHandle();
}

void Connection::onPresenceReceived(const QXmppPresence &presence)
{
    QString jid = QXmppUtils::jidToBareJid(presence.from());
    QMap<QString, QXmppPresence> receivedPresence;
    receivedPresence.insert(jid, presence);

    Tp::SimpleContactPresences presences;
    presences[m_uniqueHandleMap[jid]] = toTpPresence(receivedPresence);
    m_simplePresenceIface->setPresences(presences);

    if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
        m_avatarTokens[jid] = presence.photoHash();
    }
}

// void Connection::onStateChanged(QXmppClient::State state)
// {
// }

Tp::ContactAttributesMap Connection::getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(hold);

    Tp::UIntList handles;

    for (auto jid : m_client->rosterManager().getRosterBareJids()) {
        handles.append(m_uniqueHandleMap[jid]);
    }

    return getContactAttributes(handles, interfaces, error);
}

Tp::ContactAttributesMap Connection::getContactAttributes(const Tp::UIntList &handles, const QStringList &interfaces, Tp::DBusError *error)
{
    DBG << "- Handles: " << handles;
    //TODO check

    Tp::ContactAttributesMap contactAttributes;
    QStringList bareJids = m_client->rosterManager().getRosterBareJids();

    for (auto handle : handles) {
        QString bareJid = m_uniqueHandleMap[handle];
        QXmppRosterIq::Item rosterIq = m_client->rosterManager().getRosterEntry(bareJid);
        QVariantMap attributes;

        if (bareJid == m_clientConfig.jidBare()) {
            attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = bareJid;

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")] = QVariant::fromValue(toTpPresence(QMap<QString, QXmppPresence>({{bareJid, m_client->clientPresence()}})));
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")] = QVariant::fromValue(bareJid);
            }

            if (m_client && m_client->isConnected()) {
                if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS)) {
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token")] = QVariant::fromValue(m_avatarTokens[bareJid]);
                }
            }
        } else if (bareJids.contains(bareJid)) {
            attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = bareJid;

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
                switch (rosterIq.subscriptionType()) {
                case QXmppRosterIq::Item::None:
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateNo;
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateNo;
                    break;
                case QXmppRosterIq::Item::From:
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateNo;
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
                    break;
                case QXmppRosterIq::Item::To:
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateNo;
                    break;
                case QXmppRosterIq::Item::Both:
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
                    break;
                case QXmppRosterIq::Item::Remove:
                    break;
                case QXmppRosterIq::Item::NotSet:
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateUnknown;
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateUnknown;
                    break;
                }
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")] = QVariant::fromValue(toTpPresence(m_client->rosterManager().getAllPresencesForBareJid(bareJid)));
            }

            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")] = QVariant::fromValue(rosterIq.name());
            }

            if (m_client && m_client->isConnected()) {
                if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS)) {
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token")] = QVariant::fromValue(m_avatarTokens[bareJid]);
                }
            }
        }

        contactAttributes[handle] = attributes;
    }
    return contactAttributes;
}

Tp::SimplePresence Connection::toTpPresence(QMap<QString, QXmppPresence> presences)
{
    QXmppPresence maxPresence(QXmppPresence::Error);

    for (const QXmppPresence &presence : presences) {
        if (presence.type() == QXmppPresence::Error) {
            continue;
        }

        if (maxPresence.type() == QXmppPresence::Error) {
            maxPresence = presence;
            continue;
        }

        if (maxPresence.availableStatusType() > presence.availableStatusType()) {
            maxPresence = presence;
        }
    }

    Tp::SimplePresence tpPresence;
    tpPresence.statusMessage = maxPresence.statusText();
    if (maxPresence.type() == QXmppPresence::Available) {
        switch (maxPresence.availableStatusType()) {
        case QXmppPresence::Online:
            tpPresence.type = Tp::ConnectionPresenceTypeAvailable;
            tpPresence.status = QLatin1String("available");
            break;
        case QXmppPresence::Away:
            tpPresence.type = Tp::ConnectionPresenceTypeAway;
            tpPresence.status = QLatin1String("away");
            break;
        case QXmppPresence::XA:
            tpPresence.type = Tp::ConnectionPresenceTypeExtendedAway;
            tpPresence.status = QLatin1String("xa");
            break;
        case QXmppPresence::DND:
            tpPresence.type = Tp::ConnectionPresenceTypeBusy;
            tpPresence.status = QLatin1String("dnd");
            break;
        case QXmppPresence::Chat:
            tpPresence.type = Tp::ConnectionPresenceTypeAvailable;
            tpPresence.status = QLatin1String("chat");
            break;
        case QXmppPresence::Invisible:
            tpPresence.type = Tp::ConnectionPresenceTypeHidden;
            tpPresence.status = QLatin1String("hidden");
            break;
        }
    } else {
        tpPresence.type = Tp::ConnectionPresenceTypeOffline;
        tpPresence.status = QLatin1String("offline");
    }

//    qDebug() << "TP presence: " << tpPresence.type << " "  << tpPresence.status;
    return tpPresence;
}

Tp::AliasMap Connection::getAliases(const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(error);

    Tp::AliasMap aliases;

    for (uint handle : handles) {
        aliases[handle] = m_client->rosterManager().getRosterEntry(m_uniqueHandleMap[handle]).name();
    }

    return aliases;
}

void Connection::setAliases(const Tp::AliasMap &aliases, Tp::DBusError *error)
{
    DBG;
    for(Tp::AliasMap::const_iterator it = aliases.begin(); it != aliases.end(); it++) {
        m_client->rosterManager().getRosterEntry(m_uniqueHandleMap[it.key()]).setName(it.value());
    }
}

QStringList Connection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;

    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
        return QStringList();
    }

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unsupported handle type"));
        return QStringList();
    }

    QStringList result;

    for (uint handle : handles) {
        QString bareJid = m_uniqueHandleMap[handle];
        if (bareJid.isEmpty()) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Unknown handle"));
            return QStringList();
        }

        result.append(bareJid);
    }

    return result;
}

Tp::UIntList Connection::requestHandles(uint handleType, const QStringList &identifiers, Tp::DBusError *error)
{
    DBG;

    Tp::UIntList result;

    if (handleType != Tp::HandleTypeContact) {
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Connection::requestHandles - Handle Type unknown"));
        return result;
    }

    for(auto &identifier : identifiers) {
        result.append(m_uniqueHandleMap[identifier]);
    }

    return result;
}

//TODO m_contactListIface->contactsChangedWithID for all of these and for remote actions:

void Connection::requestSubscription(const Tp::UIntList &handles, const QString &message, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(message);

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
    }

    for (auto handle : handles) {
        m_client->rosterManager().addItem(m_uniqueHandleMap[handle]); //TODO: Is adding the contact here the right thing to do?
        m_client->rosterManager().subscribe(m_uniqueHandleMap[handle], message);
    }
}

void Connection::removeContacts(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
    }

    Tp::ContactSubscriptionMap changes;
    Tp::HandleIdentifierMap identifiers;
    Tp::HandleIdentifierMap removals;
    for (uint handle : handles) {
        m_client->rosterManager().removeItem(m_uniqueHandleMap[handle]);
        removals[handle] = m_uniqueHandleMap[handle];
    }
    m_contactListIface->contactsChangedWithID(changes, identifiers, removals);
}

void Connection::authorizePublication(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().acceptSubscription(m_uniqueHandleMap[handle]);
    }
}

void Connection::unsubscribe(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().unsubscribe(m_uniqueHandleMap[handle]);
    }
}

void Connection::unpublish(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().refuseSubscription(m_uniqueHandleMap[handle]);
    }
}

Tp::BaseChannelPtr Connection::createChannel(const QVariantMap &request, Tp::DBusError *error)
{
    DBG;

    const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();

    uint targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
    uint targetHandle = 0;
    QString targetID;

    switch (targetHandleType) {
    case Tp::HandleTypeContact:
        if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
            targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
            targetID = m_uniqueHandleMap[targetHandle];
        } else if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
            targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            targetHandle = m_uniqueHandleMap[targetID];
        }
        break;
    default:
        if (error) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QLatin1String("Unknown target handle type"));
        }
        return Tp::BaseChannelPtr();
        break;
    }

    if (targetID.isEmpty()) {
        if (error) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QLatin1String("Target handle is unknown."));
        }
        return Tp::BaseChannelPtr();
    }


    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);
    baseChannel->setTargetID(targetID);

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        TextChannelPtr textChannel = TextChannel::create(m_client, baseChannel.data(), selfHandle(), m_clientConfig.jidBare());
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textChannel));
    }

    return baseChannel;
}

void Connection::onMessageReceived(const QXmppMessage &message)
{
    uint initiatorHandle, targetHandle;

    initiatorHandle = targetHandle = m_uniqueHandleMap[QXmppUtils::jidToBareJid(message.from())];

    //TODO: initiator should be group creator
    Tp::DBusError error;
    bool yours;

    QVariantMap request;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = targetHandle;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = Tp::HandleTypeContact;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = initiatorHandle;

    Tp::BaseChannelPtr channel = ensureChannel(request, yours, /* suppressHandler */ false, &error);

    if (error.isValid()) {
        qWarning() << "ensureChannel failed:" << error.name() << " " << error.message();
        return;
    }

    TextChannelPtr textChannel = TextChannelPtr::dynamicCast(channel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));

    if (!textChannel) {
        qDebug() << "Error, channel is not a TextChannel?";
        return;
    }

    textChannel->onMessageReceived(message);
}

void Connection::requestAvatars(const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;
    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
        return;
    }

    for (uint handle : handles) {
        m_client->vCardManager().requestVCard(m_uniqueHandleMap[handle]);
    }
}

void Connection::onVCardReceived(QXmppVCardIq iq)
{
    DBG;
    QByteArray photoHash = QCryptographicHash::hash(iq.photo(), QCryptographicHash::Sha1);
    m_avatarTokens[QXmppUtils::jidToBareJid(iq.from())] = photoHash;
    m_avatarsIface->avatarRetrieved(m_uniqueHandleMap[QXmppUtils::jidToBareJid(iq.from())], photoHash, iq.photo(), iq.photoType());
}

void Connection::onClientVCardReceived()
{
    DBG;
    QByteArray photoHash = QCryptographicHash::hash(m_client->vCardManager().clientVCard().photo(), QCryptographicHash::Sha1);
    m_avatarTokens[m_clientConfig.jidBare()] = photoHash;
    m_avatarsIface->avatarRetrieved(selfHandle(), photoHash, m_client->vCardManager().clientVCard().photo(), m_client->vCardManager().clientVCard().photoType());
}

Tp::AvatarTokenMap Connection::getKnownAvatarTokens(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return Tp::AvatarTokenMap();
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QLatin1String("Disconnected"));
        return Tp::AvatarTokenMap();
    }

    Tp::AvatarTokenMap result;
    for (uint handle : handles) {
        if (!m_avatarTokens[m_uniqueHandleMap[handle]].isEmpty()) {
            result.insert(handle, m_avatarTokens[m_uniqueHandleMap[handle]]);
        }
    }

    return result;
}

void Connection::clearAvatar(Tp::DBusError *error)
{
    if (!m_client || !m_client->isConnected() || !m_client->vCardManager().isClientVCardReceived()) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String("Disconnected"));
    }

    QXmppVCardIq clientVCard = m_client->vCardManager().clientVCard();
    clientVCard.setPhoto(QByteArray());
    clientVCard.setPhotoType(QString());
    m_client->vCardManager().setClientVCard(clientVCard);

    QXmppPresence presence;
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNoPhoto);
    presence.setFrom(m_clientConfig.jid());
    m_client->sendPacket(presence);

    m_avatarTokens[m_clientConfig.jidBare()] = QString("");
}

QString Connection::setAvatar(const QByteArray &avatar, const QString &mimetype, Tp::DBusError *error)
{
    if (!m_client || !m_client->isConnected() || !m_client->vCardManager().isClientVCardReceived()) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, QLatin1String("Disconnected"));
    }

    QByteArray hash = QCryptographicHash::hash(avatar, QCryptographicHash::Sha1);
    QXmppVCardIq clientVCard = m_client->vCardManager().clientVCard();
    clientVCard.setPhoto(QByteArray());
    clientVCard.setPhotoType(mimetype);
    m_client->vCardManager().setClientVCard(clientVCard);

    QXmppPresence presence;
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateValidPhoto);
    presence.setFrom(m_clientConfig.jid());
    m_client->sendPacket(presence);

    m_avatarTokens[m_clientConfig.jidBare()] = hash;

    return hash;
}
