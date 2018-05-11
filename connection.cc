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

#include <QXmppRosterManager.h>
#include <QXmppVCardManager.h>
#include <QXmppVersionManager.h>
#include <QXmppUtils.h>
#include <QXmppPresence.h>
#include <QXmppTransferManager.h>
#include <QXmppMucManager.h>

#include "connection.hh"
#include "muctextchannel.hh"
#include "filetransferchannel.hh"
#include "common.hh"
#include "telepathy-nonsense-config.h"

Tp::RequestableChannelClass createRequestableChannelClassText()
{
    Tp::RequestableChannelClass text;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    text.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    text.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    return text;
}

Tp::RequestableChannelClass createRequestableChannelClassGroupChat()
{
    Tp::RequestableChannelClass groupChat;
    groupChat.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    groupChat.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeRoom;
    groupChat.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    groupChat.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    return groupChat;
}

Tp::RequestableChannelClass createRequestableChannelClassFileTransfer()
{
    Tp::RequestableChannelClass fileTransfer;
    fileTransfer.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER;
    fileTransfer.fixedProperties[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")]  = Tp::HandleTypeContact;
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL + QLatin1String(".ContentHashType"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType")); /* TODO: QXmpp only supports SI file transfers with MD5 hash */
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date"));
    fileTransfer.allowedProperties.append(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".URI"));
    return fileTransfer;
}

static const Tp::RequestableChannelClass requestableChannelClassText = createRequestableChannelClassText();
static const Tp::RequestableChannelClass requestableChannelClassGroupChat = createRequestableChannelClassGroupChat();
static const Tp::RequestableChannelClass requestableChannelClassFileTransfer = createRequestableChannelClassFileTransfer();

Connection::Connection(const QDBusConnection &dbusConnection, const QString &cmName, const QString &protocolName, const QVariantMap &parameters) :
    Tp::BaseConnection(dbusConnection, cmName, protocolName, parameters), m_client (0)
{
    DBG;

    /* Connection.Interface.Contacts */
    m_contactsIface = Tp::BaseConnectionContactsInterface::create();
    m_contactsIface->setGetContactAttributesCallback(Tp::memFun(this, &Connection::getContactAttributes));
    m_contactsIface->setContactAttributeInterfaces(QStringList()
                                                   << TP_QT_IFACE_CONNECTION
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING
                                                   << TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES
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

    /* Connection.Interface.ContactGroups */
    m_contactGroupsIface = Tp::BaseConnectionContactGroupsInterface::create();
    m_contactGroupsIface->setDisjointGroups(false);
    m_contactGroupsIface->setSetContactGroupsCallback(Tp::memFun(this, &Connection::setContactGroups));
    m_contactGroupsIface->setSetGroupMembersCallback(Tp::memFun(this, &Connection::setGroupMembers));
    m_contactGroupsIface->setAddToGroupCallback(Tp::memFun(this, &Connection::addToGroup));
    m_contactGroupsIface->setRemoveFromGroupCallback(Tp::memFun(this, &Connection::removeFromGroup));
    m_contactGroupsIface->setRemoveGroupCallback(Tp::memFun(this, &Connection::removeGroup));
    m_contactGroupsIface->setRenameGroupCallback(Tp::memFun(this, &Connection::renameGroup));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactGroupsIface));

    /* Connection.Interface.Aliasing */
    m_aliasingIface = Tp::BaseConnectionAliasingInterface::create();
    m_aliasingIface->setGetAliasesCallback(Tp::memFun(this, &Connection::getAliases));
    m_aliasingIface->setSetAliasesCallback(Tp::memFun(this, &Connection::setAliases));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_aliasingIface));

    /* Connection.Interface.ClientTypes */
    m_clientTypesIface = Tp::BaseConnectionClientTypesInterface::create();
    m_clientTypesIface->setGetClientTypesCallback(Tp::memFun(this, &Connection::getClientTypes));
    m_clientTypesIface->setRequestClientTypesCallback(Tp::memFun(this, &Connection::requestClientTypes));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_clientTypesIface));

    /* Connection.Interface.ContactCapabilities */
    m_contactCapabilitiesIface = Tp::BaseConnectionContactCapabilitiesInterface::create();
    m_contactCapabilitiesIface->setGetContactCapabilitiesCallback(Tp::memFun(this, &Connection::getContactCapabilities));
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_contactCapabilitiesIface));

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
    m_requestsIface->requestableChannelClasses << requestableChannelClassText;
    m_requestsIface->requestableChannelClasses << requestableChannelClassGroupChat;
    m_requestsIface->requestableChannelClasses << requestableChannelClassFileTransfer;
    plugInterface(Tp::AbstractConnectionInterfacePtr::dynamicCast(m_requestsIface));

    QString myJid = parameters.value(QStringLiteral("account")).toString();
    QString server = parameters.value(QStringLiteral("server")).toString();
    QString resource = parameters.value(QStringLiteral("resource")).toString();
    if (resource.isEmpty()) {
        /* Make sure that the resource is a non-empty string */
        resource = QUuid::createUuid().toString();
    }
    uint priority = parameters.value(QStringLiteral("priority")).toUInt();
    bool requireEncryption = parameters.value(QStringLiteral("require-encryption")).toBool();
    bool ignoreSslErrors = parameters.value(QStringLiteral("ignore-ssl-errors")).toBool();
    m_clientConfig.setJid(myJid);
    if (!server.isEmpty()) {
        m_clientConfig.setHost(server);
    }
    m_clientConfig.setResource(resource);
    m_clientConfig.setAutoAcceptSubscriptions(false);
    m_clientConfig.setAutoReconnectionEnabled(false);
    m_clientConfig.setIgnoreSslErrors(ignoreSslErrors);
    if (requireEncryption) {
        m_clientConfig.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSRequired);
    } else {
        m_clientConfig.setStreamSecurityMode(QXmppConfiguration::StreamSecurityMode::TLSEnabled);
    }
    m_clientPresence.setPriority(priority);
    setSelfContact(m_uniqueContactHandleMap[myJid], myJid);

    setConnectCallback(Tp::memFun(this, &Connection::doConnect));
    setInspectHandlesCallback(Tp::memFun(this, &Connection::inspectHandles));
    setCreateChannelCallback(Tp::memFun(this, &Connection::createChannelCB));
    setRequestHandlesCallback(Tp::memFun(this, &Connection::requestHandles));
    connect(this, &Connection::disconnected, this, &Connection::doDisconnect);
}

void Connection::doConnect(Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(error);

    setStatus(Tp::ConnectionStatusConnecting, Tp::ConnectionStatusReasonRequested);

    m_client = new QXmppClient();
    m_client->versionManager().setClientName(qAppName());
    m_client->versionManager().setClientVersion(telepathy_nonsense_VERSION_STRING);
    m_client->versionManager().setClientOs(QSysInfo::prettyProductName());

    QXmppLogger *logger = new QXmppLogger();
    logger->setLoggingType(QXmppLogger::SignalLogging);
    logger->setMessageTypes(QXmppLogger::AnyMessage);
    connect(logger, &QXmppLogger::message, this, &Connection::onLogMessage);
    m_client->setLogger(logger);

    /* Try to set the device type. For now, we will try to query the hostnamed
     * dbus interface and assume "pc" if that fails. It would be great if
     * QSysInfo would provide this functionality. */
    QString clientType = QLatin1String("pc");
    QDBusInterface hostnameInterface(QStringLiteral("org.freedesktop.hostname1"),
                                     QStringLiteral("/org/freedesktop/hostname1"),
                                     QStringLiteral("org.freedesktop.hostname1"),
                                     QDBusConnection::systemBus());
    QVariant chassisReply = hostnameInterface.property("Chassis");
    if (chassisReply.isValid()) {
        auto chassisType = chassisReply.toString();
        /* This is the best mapping that I can think of... */
        if (chassisType == QLatin1String("tablet")) {
            clientType = QStringLiteral("handheld");
        } else if (chassisType == QLatin1String("handset")) {
            clientType = QStringLiteral("phone");
        }
    }

    /* Enable extensions */
    m_discoveryManager = m_client->findExtension<QXmppDiscoveryManager>();
    m_discoveryManager->setClientType(clientType);
    connect(m_discoveryManager, &QXmppDiscoveryManager::infoReceived, this, &Connection::onDiscoveryInfoReceived);
    connect(m_discoveryManager, &QXmppDiscoveryManager::itemsReceived, this, &Connection::onDiscoveryItemsReceived);

    m_mucManager = new QXmppMucManager();
    m_client->addExtension(m_mucManager);

    QXmppTransferManager *transferManager = new QXmppTransferManager;
    m_client->addExtension(transferManager);
    connect(transferManager, &QXmppTransferManager::fileReceived, this, &Connection::onFileReceived);

#if QXMPP_VERSION >= 0x000905
    m_carbonManager = new QXmppCarbonManager;
    m_client->addExtension(m_carbonManager);
    connect(m_carbonManager, &QXmppCarbonManager::messageReceived, this, &Connection::onCarbonMessageReceived);
    connect(m_carbonManager, &QXmppCarbonManager::messageSent, this, &Connection::onCarbonMessageSent);
#endif

    /* The features for ourself must only be added after adding all QXmpp
     * extensions - we would miss features otherwise */
    m_contactsFeatures[m_clientConfig.jid()] = m_discoveryManager->capabilities().features();

    connect(m_client, &QXmppClient::connected, this, &Connection::onConnected);
    connect(m_client, &QXmppClient::error, this, &Connection::onError);
    connect(m_client, &QXmppClient::messageReceived, this, &Connection::onMessageReceived);
    connect(m_client, &QXmppClient::presenceReceived, this, &Connection::onPresenceReceived);

    connect(&m_client->rosterManager(), &QXmppRosterManager::rosterReceived, this, &Connection::onRosterReceived);

    connect(&m_client->vCardManager(), &QXmppVCardManager::vCardReceived, this, &Connection::onVCardReceived);
    connect(&m_client->vCardManager(), &QXmppVCardManager::clientVCardReceived, this, &Connection::onClientVCardReceived);

    //SASL channel registration
    Tp::DBusError saslError;
    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, TP_QT_IFACE_CHANNEL_TYPE_SERVER_AUTHENTICATION);
    Tp::BaseChannelServerAuthenticationTypePtr authType = Tp::BaseChannelServerAuthenticationType::create(TP_QT_IFACE_CHANNEL_INTERFACE_SASL_AUTHENTICATION);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(authType));

    m_saslIface = Tp::BaseChannelSASLAuthenticationInterface::create(QStringList() << QStringLiteral("X-TELEPATHY-PASSWORD"),
                                                                     /* hasInitialData */ false,
                                                                     /* canTryAgain */ true,
                                                                     /* authorizationIdentity */ m_client->configuration().jid(),
                                                                     /* defaultUsername */ QString(),
                                                                     /* defaultRealm */ m_client->configuration().domain(),
                                                                     /* maySaveResponse */ true);

    m_saslIface->setStartMechanismWithDataCallback(Tp::memFun(this, &Connection::saslStartMechanismWithData));

    baseChannel->setRequested(false);
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(m_saslIface));

    baseChannel->registerObject(&saslError);
    if (!saslError.isValid()) {
        addChannel(baseChannel);
    }
}

void Connection::saslStartMechanismWithData(const QString &mechanism, const QByteArray &data, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(error);
    Q_ASSERT(mechanism == QLatin1String("X-TELEPATHY-PASSWORD"));

    m_saslIface->setSaslStatus(Tp::SASLStatusInProgress, QStringLiteral("InProgress"), QVariantMap());

    m_clientConfig.setPassword(QString::fromUtf8(data));

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
        m_avatarTokens[m_uniqueContactHandleMap[selfHandle()]] = QString::fromLatin1(m_clientPresence.photoHash());
    }

    m_discoveryManager->requestInfo(m_clientConfig.domain());
    m_serverEntities.push_back(m_clientConfig.domain());
    m_discoveryManager->requestItems(m_clientConfig.domain());
}

void Connection::onError(QXmppClient::Error error)
{
    DBG;

    //TODO
    if (error == QXmppClient::SocketError || error == QXmppClient::KeepAliveError) {
        setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNetworkError);
    } else if (error == QXmppClient::XmppStreamError) {
        QXmppStanza::Error::Condition xmppStreamError = m_client->xmppStreamError();
        if (xmppStreamError == QXmppStanza::Error::NotAuthorized) {
            m_saslIface->setSaslStatus(Tp::SASLStatusServerFailed, QStringLiteral("ServerFailed"), QVariantMap());
        } else {
            setStatus(Tp::ConnectionStatusDisconnected, Tp::ConnectionStatusReasonNoneSpecified);
        }
    } else {
        Q_ASSERT(0);
    }
}

void Connection::onLogMessage(QXmppLogger::MessageType type, const QString &text)
{
    switch(type) {
    case QXmppLogger::ReceivedMessage:
    case QXmppLogger::SentMessage:
        qCDebug(qxmppStanza) << text;
        break;
    case QXmppLogger::InformationMessage:
        qCInfo(qxmppGeneric) << text;
        break;
    case QXmppLogger::WarningMessage:
        qCCritical(qxmppGeneric) << text;
        break;
    default:
        qCDebug(qxmppGeneric) << text;
    }
}

void Connection::onRosterReceived()
{
    DBG;

    updateGroups();
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

    if (jid == m_clientConfig.jidBare()) {
        return; // Ignore presence update from another resource of current account.
    }

    if (m_uniqueRoomHandleMap.contains(jid)) {
        jid = presence.from();
        updateMucParticipantInfo(jid, presence);
        // TODO: If presence.mucItem().nick() is not empty, then the presence stanza is "Changing nickname". Look at XEP-0042, section 7.6 for details.
    }

    updateJidPresence(jid, presence);
}

void Connection::updateJidPresence(const QString &jid, const QXmppPresence &presence)
{
    QMap<QString, QXmppPresence> receivedPresence;
    receivedPresence.insert(jid, presence);

    Tp::SimpleContactPresences presences;
    presences[m_uniqueContactHandleMap[jid]] = toTpPresence(receivedPresence);
    m_simplePresenceIface->setPresences(presences);

    if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
        m_avatarTokens[jid] = QString::fromLatin1(presence.photoHash());
    }

    qCDebug(general) << "capability hash:" << presence.capabilityHash();
    qCDebug(general) << "capability node and verification:" << presence.capabilityNode() << presence.capabilityVer().toBase64();
    qCDebug(general) << "capability extensions:" << presence.capabilityExt();

    if (!presence.capabilityVer().isEmpty()) {
//        QString nodeWithVerification = presence.capabilityNode() + QLatin1Char('#') + QString::fromLatin1(presence.capabilityVer().toBase64());

        qCDebug(general) << Q_FUNC_INFO << "Request info from" << presence.from();
        m_discoveryManager->requestInfo(presence.from());
    }
}

void Connection::onDiscoveryInfoReceived(const QXmppDiscoveryIq &iq)
{
    DBG;

    qCDebug(general) << "Received the following discovery info:";
    qCDebug(general) << iq.queryNode();
    qCDebug(general) << iq.type() << iq.queryType();
    qCDebug(general) << iq.from() << iq.to();
    qCDebug(general) << iq.features();
    qCDebug(general).noquote() << iq.verificationString().toBase64();

    for (auto &identity : iq.identities()) {
        if (identity.category() == QLatin1String("client")) {
            m_contactsFeatures[iq.from()] = iq.features();
            m_clientTypes[iq.from()] = identity.type();

            QString bareJid = QXmppUtils::jidToBareJid(iq.from());
            uint handle = m_uniqueContactHandleMap[bareJid];
            if (bareJid + lastResourceForJid(bareJid, /* force */ true) == iq.from()) {
                Tp::DBusError error;
                Tp::ContactCapabilitiesMap caps = getContactCapabilities(Tp::UIntList() << handle, &error);
                m_contactCapabilitiesIface->contactCapabilitiesChanged(caps);
                m_clientTypesIface->clientTypesUpdated(handle, getClientType(handle));
            }
        } else if (identity.category() == QLatin1String("proxy")) {
            if (identity.type() == QLatin1String("bytestreams") && m_serverEntities.contains(iq.from())) {
                qCDebug(general) << "Found proxy with JID" << iq.from();
                QXmppTransferManager *transferManager = m_client->findExtension<QXmppTransferManager>();
                transferManager->setProxy(iq.from());
            }
        }
    }

#if QXMPP_VERSION >= 0x000905
    bool carbonFeaturesAvailable = true;
    for (auto &carbonFeature : m_carbonManager->discoveryFeatures()) {
        if (!iq.features().contains(carbonFeature)) {
            carbonFeaturesAvailable = false;
            break;
        }
    }
    if (carbonFeaturesAvailable) {
        m_carbonManager->setCarbonsEnabled(true);
    }
#endif
}

void Connection::onDiscoveryItemsReceived(const QXmppDiscoveryIq &iq)
{
    DBG;
    for (auto &item : iq.items()) {
        if (m_serverEntities.contains(iq.from())) {
            m_serverEntities.push_back(item.jid());
            m_discoveryManager->requestInfo(item.jid(), item.node());
        }
    }
}

Tp::ContactAttributesMap Connection::getContactListAttributes(const QStringList &interfaces, bool hold, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(hold);

    Tp::UIntList handles;

    for (auto jid : m_client->rosterManager().getRosterBareJids()) {
        handles.append(m_uniqueContactHandleMap[jid]);
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
        QString contactJid = m_uniqueContactHandleMap[handle];
        QXmppRosterIq::Item rosterIq = m_client->rosterManager().getRosterEntry(contactJid);
        QMap<QString, QXmppPresence> contactPresences;
        QVariantMap attributes;

        attributes[TP_QT_IFACE_CONNECTION + QLatin1String("/contact-id")] = contactJid;

        if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_CAPABILITIES + QLatin1String("/capabilities")] = QVariant::fromValue(getContactCapabilities(Tp::UIntList() << handle, error).value(handle));
        }
        if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_ALIASING + QLatin1String("/alias")] = QVariant::fromValue(getAlias(handle, error));
        }

        if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CLIENT_TYPES + QLatin1String("/client-types")] = QVariant::fromValue(getClientType(handle));
        }

        if (contactJid == m_clientConfig.jidBare()) {
            if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST)) {
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/subscribe")] = Tp::SubscriptionStateYes;
                attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_LIST + QLatin1String("/publish")] = Tp::SubscriptionStateYes;
            }

            contactPresences.insert(contactJid, m_client->clientPresence());

            if (m_client && m_client->isConnected()) {
                if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS)) {
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token")] = QVariant::fromValue(m_avatarTokens[contactJid]);
                }
            }
        } else if (bareJids.contains(contactJid)) {
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

            contactPresences = m_client->rosterManager().getAllPresencesForBareJid(contactJid);

            if (m_client && m_client->isConnected()) {
                if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS)) {
                    attributes[TP_QT_IFACE_CONNECTION_INTERFACE_AVATARS + QLatin1String("/token")] = QVariant::fromValue(m_avatarTokens[contactJid]);
                }
            }
        } else if (m_mucParticipants.contains(contactJid)) {
            contactPresences.insert(contactJid, m_mucParticipants.value(contactJid));
        }

        if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS)) {
            QStringList groups = rosterIq.groups().toList();
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS + QLatin1String("/groups")] = QVariant::fromValue(groups);
        }

        if (interfaces.contains(TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE)) {
            attributes[TP_QT_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE + QLatin1String("/presence")] = QVariant::fromValue(toTpPresence(contactPresences));
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
            tpPresence.status = QStringLiteral("available");
            break;
        case QXmppPresence::Away:
            tpPresence.type = Tp::ConnectionPresenceTypeAway;
            tpPresence.status = QStringLiteral("away");
            break;
        case QXmppPresence::XA:
            tpPresence.type = Tp::ConnectionPresenceTypeExtendedAway;
            tpPresence.status = QStringLiteral("xa");
            break;
        case QXmppPresence::DND:
            tpPresence.type = Tp::ConnectionPresenceTypeBusy;
            tpPresence.status = QStringLiteral("dnd");
            break;
        case QXmppPresence::Chat:
            tpPresence.type = Tp::ConnectionPresenceTypeAvailable;
            tpPresence.status = QStringLiteral("chat");
            break;
        case QXmppPresence::Invisible:
            tpPresence.type = Tp::ConnectionPresenceTypeHidden;
            tpPresence.status = QStringLiteral("hidden");
            break;
        }
    } else {
        tpPresence.type = Tp::ConnectionPresenceTypeOffline;
        tpPresence.status = QStringLiteral("offline");
    }

    return tpPresence;
}

QString Connection::getAlias(uint handle, Tp::DBusError *error)
{
    Q_UNUSED(error);

    if (handle == selfHandle()) {
        return m_clientConfig.jidBare();
    }

    const QString jid = m_uniqueContactHandleMap[handle];

    if (m_mucParticipants.contains(jid)) {
        return QXmppUtils::jidToResource(jid);
    } else {
        return m_client->rosterManager().getRosterEntry(jid).name();
    }
}

Tp::AliasMap Connection::getAliases(const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;

    Tp::AliasMap aliases;

    for (uint handle : handles) {
        aliases[handle] = getAlias(handle, error);
    }

    return aliases;
}

void Connection::setAliases(const Tp::AliasMap &aliases, Tp::DBusError *error)
{
    DBG;
    for(Tp::AliasMap::const_iterator it = aliases.begin(); it != aliases.end(); it++) {
        m_client->rosterManager().getRosterEntry(m_uniqueContactHandleMap[it.key()]).setName(it.value());
    }
}

QStringList Connection::inspectHandles(uint handleType, const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;

    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return QStringList();
    }

    switch (handleType) {
    case Tp::HandleTypeContact:
    case Tp::HandleTypeRoom:
        break;
    default:
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QStringLiteral("Unsupported handle type"));
        return QStringList();
    }

    QStringList result;

    UniqueHandleMap &map = handleType == Tp::HandleTypeContact ? m_uniqueContactHandleMap : m_uniqueRoomHandleMap;
    for (uint handle : handles) {
        QString bareJid = map[handle];
        if (bareJid.isEmpty()) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
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
    switch (handleType) {
    case Tp::HandleTypeContact:
        for (const QString &identifier : identifiers) {
            result.append(m_uniqueContactHandleMap[identifier]);
        }
        break;
    case Tp::HandleTypeRoom:
        for (const QString &identifier : identifiers) {
            result.append(m_uniqueRoomHandleMap[identifier]);
        }
        break;
    default:
        error->set(TP_QT_ERROR_INVALID_ARGUMENT, QStringLiteral("Connection::requestHandles - Unsupported handle type"));
        break;
    }

    return result;
}

//TODO m_contactListIface->contactsChangedWithID for all of these and for remote actions:

void Connection::requestSubscription(const Tp::UIntList &handles, const QString &message, Tp::DBusError *error)
{
    DBG;
    Q_UNUSED(message);

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
    }

    for (auto handle : handles) {
        m_client->rosterManager().addItem(m_uniqueContactHandleMap[handle]); //TODO: Is adding the contact here the right thing to do?
        m_client->rosterManager().subscribe(m_uniqueContactHandleMap[handle], message);
    }
}

void Connection::removeContacts(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
    }

    Tp::ContactSubscriptionMap changes;
    Tp::HandleIdentifierMap identifiers;
    Tp::HandleIdentifierMap removals;
    for (uint handle : handles) {
        m_client->rosterManager().removeItem(m_uniqueContactHandleMap[handle]);
        removals[handle] = m_uniqueContactHandleMap[handle];
    }
    m_contactListIface->contactsChangedWithID(changes, identifiers, removals);
}

void Connection::authorizePublication(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().acceptSubscription(m_uniqueContactHandleMap[handle]);
    }
}

void Connection::unsubscribe(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().unsubscribe(m_uniqueContactHandleMap[handle]);
    }
}

void Connection::unpublish(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return;
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
    }

    for (uint handle : handles) {
        m_client->rosterManager().refuseSubscription(m_uniqueContactHandleMap[handle]);
    }
}

Tp::BaseChannelPtr Connection::createChannelCB(const QVariantMap &request, Tp::DBusError *error)
{
    DBG;

    const QString channelType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")).toString();

    bool requested = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".Requested")).toBool();
    uint targetHandleType = Tp::HandleTypeNone;
    uint targetHandle = 0;
    QString targetID;

    if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType"))) {
        targetHandleType = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")).toUInt();
    }

    switch (targetHandleType) {
    case Tp::HandleTypeContact:
        if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
            targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
            targetID = m_uniqueContactHandleMap[targetHandle];
        } else if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
            targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            targetHandle = m_uniqueContactHandleMap[targetID];
        }
        break;
    case Tp::HandleTypeRoom:
        if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle"))) {
            targetHandle = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")).toUInt();
            targetID = m_uniqueRoomHandleMap[targetHandle];
        } else if (request.contains(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID"))) {
            targetID = request.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();
            targetHandle = m_uniqueRoomHandleMap[targetID];
        }
        break;
    default:
        if (error) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QStringLiteral("Unknown target handle type"));
        }
        return Tp::BaseChannelPtr();
    }

    if (targetHandleType == Tp::HandleTypeNone) {
        if (error) {
            error->set(TP_QT_ERROR_INVALID_ARGUMENT, QStringLiteral("Target handle type is not present in the request details."));
        }
        return Tp::BaseChannelPtr();
    }

    if (targetID.isEmpty()) {
        if (error) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Target handle is unknown."));
        }
        return Tp::BaseChannelPtr();
    }

    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(this, channelType, Tp::HandleType(targetHandleType), targetHandle);
    baseChannel->setTargetID(targetID);
    baseChannel->setRequested(requested);

    if (channelType == TP_QT_IFACE_CHANNEL_TYPE_TEXT) {
        if (targetHandleType == Tp::HandleTypeContact) {
            TextChannelPtr textChannel = TextChannel::create(this, baseChannel.data());
            baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textChannel));
        } else if (targetHandleType == Tp::HandleTypeRoom) {
            MucTextChannelPtr textChannel;
            textChannel = MucTextChannel::create(this, baseChannel.data());
            baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(textChannel));
        } else {
            Q_ASSERT(0);
        }
    } else if (channelType == TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER) {
        FileTransferChannelPtr fileTransferChannel = FileTransferChannel::create(this, baseChannel.data(), request);
        baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(fileTransferChannel));
    }

    return baseChannel;
}

TextChannelPtr Connection::getTextChannel(const QString &contactJid, bool ensure, bool mucInvitation)
{
    uint initiatorHandle, targetHandle;
    Tp::HandleType handleType;

    const QString bareJid = QXmppUtils::jidToBareJid(contactJid);

    // This check (if rooms map contains jid) prevents us from repeated invitation.
    if (m_uniqueRoomHandleMap.contains(bareJid)) {
        qCDebug(general) << Q_FUNC_INFO << "message from room" << bareJid;
        return TextChannelPtr();
    }

    if (mucInvitation) {
        initiatorHandle = targetHandle = m_uniqueRoomHandleMap[bareJid];
        handleType = Tp::HandleTypeRoom;
    } else {
        initiatorHandle = targetHandle = m_uniqueContactHandleMap[bareJid];
        handleType = Tp::HandleTypeContact;
    }

    if (handleType == Tp::HandleTypeContact) {
        setLastResource(QXmppUtils::jidToBareJid(contactJid), QXmppUtils::jidToResource(contactJid));
    }

    //TODO: initiator should be group creator
    Tp::DBusError error;
    bool yours;

    QVariantMap request;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_TEXT;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = targetHandle;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = handleType;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = initiatorHandle;

    Tp::BaseChannelPtr channel;
    if (ensure) {
        channel = ensureChannel(request, yours, /* suppressHandler */ false, &error);
    } else {
        channel = getExistingChannel(request, &error);
    }

    if (error.isValid()) {
        qCWarning(general) << "ensureChannel failed:" << error.name() << " " << error.message();
    }
    if (!channel) {
        return TextChannelPtr();
    }

    return TextChannelPtr::dynamicCast(channel->interface(TP_QT_IFACE_CHANNEL_TYPE_TEXT));
}

void Connection::onMessageReceived(const QXmppMessage &message)
{
    // GroupChat messages processed in the MucTextChannel itself.
    if (message.type() == QXmppMessage::GroupChat) {
        return;
    }

    TextChannelPtr textChannel = getTextChannel(message.from(), true, !message.mucInvitationJid().isEmpty());
    if (!textChannel) {
        qCDebug(general) << "Error, channel is not a TextChannel?";
        return;
    }

    textChannel->onMessageReceived(message);
}

void Connection::onCarbonMessageReceived(const QXmppMessage &message)
{
    /* There must not be any carbon copies for group chat messages */
    if (message.type() == QXmppMessage::GroupChat) {
        return;
    }

    TextChannelPtr textChannel = getTextChannel(message.from(), false, !message.mucInvitationJid().isEmpty());
    if (textChannel) {
        textChannel->onMessageReceived(message);
    }
}

void Connection::onCarbonMessageSent(const QXmppMessage &message)
{
    /* There must not be any carbon copies for group chat messages */
    if (message.type() == QXmppMessage::GroupChat) {
        return;
    }

    TextChannelPtr textChannel = getTextChannel(message.to(), false, !message.mucInvitationJid().isEmpty());
    if (textChannel) {
        textChannel->onCarbonMessageSent(message);
    }
}

void Connection::onFileReceived(QXmppTransferJob *job)
{
    DBG;
    uint initiatorHandle, targetHandle;

    initiatorHandle = targetHandle = m_uniqueContactHandleMap[QXmppUtils::jidToBareJid(job->jid())];
    setLastResource(QXmppUtils::jidToBareJid(job->jid()), QXmppUtils::jidToResource(job->jid()));

    Tp::DBusError error;

    QVariantMap request;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".ChannelType")] = TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandle")] = targetHandle;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".TargetHandleType")] = Tp::HandleTypeContact;
    request[TP_QT_IFACE_CHANNEL + QLatin1String(".InitiatorHandle")] = initiatorHandle;
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentType")] = QStringLiteral("application/octet-stream"); /* QXmpp does not report the mime type (yet) */
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Filename")] = job->fileName();
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Size")] = job->fileSize();
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHashType")] = Tp::FileHashTypeMD5; /* QXmpp only supports SI file transfers with MD5 hash */
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".ContentHash")] = job->fileHash();
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Description")] = job->fileInfo().description();
    request[TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER + QLatin1String(".Date")] = job->fileDate();
    request[NONSENSE_XMPP_TRANSFER_JOB] = QVariant::fromValue(job);

    Tp::BaseChannelPtr channel = createChannel(request, false /* suppressHandler */, &error);

    if (error.isValid()) {
        qCWarning(general) << "createChannel failed:" << error.name() << " " << error.message();
        return;
    }

    FileTransferChannelPtr fileTransferChannel = FileTransferChannelPtr::dynamicCast(channel->interface(TP_QT_IFACE_CHANNEL_TYPE_FILE_TRANSFER));

    if (!fileTransferChannel) {
        qCDebug(general) << "Error, channel is not a FileTransferChannel?";
        return;
    }
}

void Connection::requestAvatars(const Tp::UIntList &handles, Tp::DBusError *error)
{
    DBG;
    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    for (uint handle : handles) {
        m_client->vCardManager().requestVCard(m_uniqueContactHandleMap[handle]);
    }
}

void Connection::onVCardReceived(QXmppVCardIq iq)
{
    DBG;
    updateAvatar(iq.photo(), QXmppUtils::jidToBareJid(iq.from()), iq.photoType());
}

void Connection::onClientVCardReceived()
{
    DBG;
    updateAvatar(m_client->vCardManager().clientVCard().photo(), m_clientConfig.jidBare(), m_client->vCardManager().clientVCard().photoType());
}

void Connection::updateAvatar(const QByteArray &photo, const QString &jid, const QString &type)
{
    QString photoHash = QString::fromLatin1(QCryptographicHash::hash(photo, QCryptographicHash::Sha1));
    m_avatarTokens[jid] = photoHash;
    m_avatarsIface->avatarRetrieved(m_uniqueContactHandleMap[jid], photoHash, photo, type);
}

Tp::AvatarTokenMap Connection::getKnownAvatarTokens(const Tp::UIntList &handles, Tp::DBusError *error)
{
    if (error->isValid()) {
        return Tp::AvatarTokenMap();
    }

    if (!m_client || !m_client->isConnected()) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return Tp::AvatarTokenMap();
    }

    Tp::AvatarTokenMap result;
    for (uint handle : handles) {
        if (!m_avatarTokens[m_uniqueContactHandleMap[handle]].isEmpty()) {
            result.insert(handle, m_avatarTokens[m_uniqueContactHandleMap[handle]]);
        }
    }

    return result;
}

void Connection::clearAvatar(Tp::DBusError *error)
{
    if (!m_client || !m_client->isConnected() || !m_client->vCardManager().isClientVCardReceived()) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, QStringLiteral("Disconnected"));
    }

    QXmppVCardIq clientVCard = m_client->vCardManager().clientVCard();
    clientVCard.setPhoto(QByteArray());
    clientVCard.setPhotoType(QString());
    m_client->vCardManager().setClientVCard(clientVCard);

    QXmppPresence presence;
    presence.setVCardUpdateType(QXmppPresence::VCardUpdateNoPhoto);
    presence.setFrom(m_clientConfig.jid());
    m_client->sendPacket(presence);

    m_avatarTokens[m_clientConfig.jidBare()] = QStringLiteral("");
}

QString Connection::setAvatar(const QByteArray &avatar, const QString &mimetype, Tp::DBusError *error)
{
    if (!m_client || !m_client->isConnected() || !m_client->vCardManager().isClientVCardReceived()) {
        error->set(TP_QT_ERROR_NOT_AVAILABLE, QStringLiteral("Disconnected"));
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

    m_avatarTokens[m_clientConfig.jidBare()] = QString::fromLatin1(hash);

    return QString::fromLatin1(hash);
}

void Connection::updateGroups()
{
    QSet<QString> groups;

    for (const QString &jid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(jid);
        groups.unite(item.groups());
    }

    m_contactGroupsIface->setGroups(groups.toList());
    m_contactGroupsIface->groupsCreated(m_contactGroupsIface->groups());
}

void Connection::setContactGroups(uint contact, const QStringList &groups, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    if (!m_uniqueContactHandleMap.contains(contact)) {
        error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
    }

    const QString contactJid = m_uniqueContactHandleMap[contact];

    QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);

    auto oldGroups = item.groups();

    item.setGroups(groups.toSet());

    if (oldGroups == item.groups()) {
        // NOP
        return;
    }

    QXmppRosterIq iq;
    iq.setType(QXmppIq::Set);
    iq.addItem(item);

    m_client->sendPacket(iq);

    auto groupsAddedToTheContact = item.groups().subtract(oldGroups);
    auto groupsRemovedFromTheContact = oldGroups.subtract(item.groups());

    auto newGroups = groupsAddedToTheContact;
    auto removedGroups = groupsRemovedFromTheContact;

    for (const QString &jid : m_client->rosterManager().getRosterBareJids()) {
        if (jid == contactJid) {
            // Skip the target contact
            continue;
        }

        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(jid);

        newGroups.subtract(item.groups());
        removedGroups.subtract(item.groups());
    }

    if (!newGroups.isEmpty()) {
        m_contactGroupsIface->groupsCreated(newGroups.toList());
    }
    if (!removedGroups.isEmpty()) {
        m_contactGroupsIface->groupsRemoved(removedGroups.toList());
    }
    m_contactGroupsIface->groupsChanged(Tp::UIntList() << contact, groupsAddedToTheContact.toList(), groupsRemovedFromTheContact.toList());
}

void Connection::setGroupMembers(const QString &group, const Tp::UIntList &members, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    QStringList memberJids;

    for (uint handle : members) {
        if (!m_uniqueContactHandleMap.contains(handle)) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
            return;
        }

        memberJids.append(m_uniqueContactHandleMap[handle]);
    }

    Tp::UIntList handlesRemovedFromTheGroup;
    Tp::UIntList handlesAddedToTheGroup;

    bool groupExisted = false;

    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);
        if (item.groups().contains(group)) {
            groupExisted = true;

            if (memberJids.contains(contactJid)) {
                // NOP
                continue;
            } else {
                auto groups = item.groups();
                groups.remove(group);
                item.setGroups(groups);

                handlesRemovedFromTheGroup.append(m_uniqueContactHandleMap[contactJid]);
            }
        } else {
            if (memberJids.contains(contactJid)) {
                auto groups = item.groups();
                groups.insert(group);
                item.setGroups(groups);

                handlesAddedToTheGroup.append(m_uniqueContactHandleMap[contactJid]);
            } else {
                // NOP
                continue;
            }
        }

        QXmppRosterIq iq;
        iq.setType(QXmppIq::Set);
        iq.addItem(item);
        m_client->sendPacket(iq);
    }

    if (!groupExisted && !members.isEmpty()) {
        m_contactGroupsIface->groupsCreated(QStringList() << group);
    }

    if (groupExisted && members.isEmpty()) {
        m_contactGroupsIface->groupsRemoved(QStringList() << group);
    }

    m_contactGroupsIface->groupsChanged(handlesRemovedFromTheGroup, QStringList(), QStringList() << group);
    m_contactGroupsIface->groupsChanged(handlesAddedToTheGroup, QStringList() << group, QStringList());
}

void Connection::addToGroup(const QString &group, const Tp::UIntList &members, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    if (members.isEmpty()) {
        // NOP
        return;
    }

    QStringList memberJids;

    for (uint handle : members) {
        if (!m_uniqueContactHandleMap.contains(handle)) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
            return;
        }

        memberJids.append(m_uniqueContactHandleMap[handle]);
    }

    Tp::UIntList handlesAddedToTheGroup;

    bool groupExists = false;

    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);
        if (item.groups().contains(group)) {
            groupExists = true;
            continue;
        }

        if (!memberJids.contains(contactJid)) {
            continue;
        }

        auto groups = item.groups();
        groups.insert(group);
        item.setGroups(groups);

        QXmppRosterIq iq;
        iq.setType(QXmppIq::Set);
        iq.addItem(item);
        m_client->sendPacket(iq);

        handlesAddedToTheGroup.append(m_uniqueContactHandleMap[contactJid]);
    }

    if (!groupExists) {
        m_contactGroupsIface->groupsCreated(QStringList() << group);
    }

    m_contactGroupsIface->groupsChanged(handlesAddedToTheGroup, QStringList() << group, QStringList());
}

void Connection::removeFromGroup(const QString &group, const Tp::UIntList &members, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    QStringList memberJids;

    for (uint handle : members) {
        if (!m_uniqueContactHandleMap.contains(handle)) {
            error->set(TP_QT_ERROR_INVALID_HANDLE, QStringLiteral("Unknown handle"));
            return;
        }

        memberJids.append(m_uniqueContactHandleMap[handle]);
    }

    Tp::UIntList handlesRemovedFromTheGroup;

    bool groupExisted = false;
    bool groupRemoved = true;

    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);

        if (item.groups().contains(group)) {
            groupExisted = true;
            if (memberJids.contains(contactJid)) {
                auto groups = item.groups();
                groups.remove(group);
                item.setGroups(groups);

                QXmppRosterIq iq;
                iq.setType(QXmppIq::Set);
                iq.addItem(item);
                m_client->sendPacket(iq);

                handlesRemovedFromTheGroup.append(m_uniqueContactHandleMap[contactJid]);
            } else {
                groupRemoved = false;
            }
        }
    }

    if (!groupExisted) {
        // NOP
        return;
    }

    if (groupRemoved) {
        m_contactGroupsIface->groupsRemoved(QStringList() << group);
    }

    m_contactGroupsIface->groupsChanged(handlesRemovedFromTheGroup, QStringList(), QStringList() << group);
}

void Connection::removeGroup(const QString &group, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }

    bool groupExisted = false;
    Tp::UIntList handlesRemovedFromTheGroup;

    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);

        if (item.groups().contains(group)) {
            groupExisted = true;

            auto groups = item.groups();
            groups.remove(group);
            item.setGroups(groups);

            QXmppRosterIq iq;
            iq.setType(QXmppIq::Set);
            iq.addItem(item);
            m_client->sendPacket(iq);

            handlesRemovedFromTheGroup.append(m_uniqueContactHandleMap[contactJid]);
        }
    }

    if (groupExisted) {
        m_contactGroupsIface->groupsRemoved(QStringList() << group);
        m_contactGroupsIface->groupsChanged(handlesRemovedFromTheGroup, QStringList(), QStringList() << group);
    }
}

void Connection::renameGroup(const QString &oldName, const QString &newName, Tp::DBusError *error)
{
    if ((!m_client) || (!m_client->isConnected())) {
        error->set(TP_QT_ERROR_DISCONNECTED, QStringLiteral("Disconnected"));
        return;
    }


    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);
        if (item.groups().contains(newName)) {
            error->set(TP_QT_ERROR_NOT_AVAILABLE, QStringLiteral("There is already a group with the new name"));
            return;
        }
    }

    Tp::UIntList affectedHandles;

    for (const QString &contactJid : m_client->rosterManager().getRosterBareJids()) {
        QXmppRosterIq::Item item = m_client->rosterManager().getRosterEntry(contactJid);

        if (item.groups().contains(oldName)) {
            auto groups = item.groups();
            groups.remove(oldName);
            groups.insert(newName);
            item.setGroups(groups);

            QXmppRosterIq iq;
            iq.setType(QXmppIq::Set);
            iq.addItem(item);
            m_client->sendPacket(iq);

            affectedHandles.append(m_uniqueContactHandleMap[contactJid]);
        }
    }

    if (affectedHandles.isEmpty()) {
        error->set(TP_QT_ERROR_DOES_NOT_EXIST, QStringLiteral("The group does not exist"));
        return;
    }

    m_contactGroupsIface->groupsRemoved(QStringList() << oldName);
    m_contactGroupsIface->groupsChanged(affectedHandles, QStringList() << newName, QStringList() << oldName);
}

QStringList Connection::getClientType(uint handle) const
{
    if (handle == selfHandle()) {
        return QStringList() << m_discoveryManager->clientType();
    }

    QString jid = m_uniqueContactHandleMap[handle];
    if (jid.isEmpty()) {
        return QStringList();
    }

    QString fullJid = jid + bestResourceForJid(jid);
    if (!m_clientTypes.contains(fullJid)) {
        return QStringList();
    }

    return QStringList() << m_clientTypes.value(fullJid);
}

Tp::ContactClientTypes Connection::getClientTypes(const Tp::UIntList &contacts, Tp::DBusError *error)
{
    Tp::ContactClientTypes result;

    for (uint handle : contacts) {
        result.insert(handle, getClientType(handle));
    }

    return result;
}

QStringList Connection::requestClientTypes(uint contact, Tp::DBusError *error)
{
    return getClientType(contact);
}

Tp::ContactCapabilitiesMap Connection::getContactCapabilities(const Tp::UIntList &contacts, Tp::DBusError *error)
{
    Tp::ContactCapabilitiesMap capabilities;

    const QStringList contactJids = inspectHandles(Tp::HandleTypeContact, contacts, error);
    if (error->isValid()) {
        return capabilities;
    }

    for (int i = 0; i < contacts.count(); ++i) {
        Tp::RequestableChannelClassList channelClassList;
        channelClassList << requestableChannelClassText; // Text channels supported by everyone.

        const QString fullJid = contactJids.at(i) + lastResourceForJid(contactJids.at(i), /* force */ true);
        const QStringList caps = m_contactsFeatures.value(fullJid);

        if (caps.contains(QStringLiteral("http://jabber.org/protocol/si/profile/file-transfer"))) {
            channelClassList << requestableChannelClassFileTransfer;
        } else {
            qCDebug(general) << "Contact" << fullJid << "has these caps:" << caps;
        }
        capabilities[contacts.at(i)] = channelClassList;
    }

    return capabilities;
}

QPointer<QXmppClient> Connection::qxmppClient() const
{
    return m_client;
}

QString Connection::lastResourceForJid(const QString &jid, bool force)
{
    QString resource = m_lastResources[jid];
    if (resource.isEmpty() || !m_client->rosterManager().getResources(jid).contains(resource)) {
        if (force) {
            return bestResourceForJid(jid);
        } else {
            m_lastResources.remove(jid);
            return QString();
        }
    }

    return QLatin1Char('/') + resource;
}

QString Connection::bestResourceForJid(const QString &jid) const
{
    const QStringList resources = m_client->rosterManager().getResources(jid);
    if (resources.isEmpty()) {
        return QString();
    }

    QString resource = resources.first();
    int highestPriority = m_client->rosterManager().getPresence(jid, resource).priority();
    for (int i = 1; i < resources.count(); ++i) {
        const int priority = m_client->rosterManager().getPresence(jid, resources.at(i)).priority();
        if (priority > highestPriority) {
            highestPriority = priority;
            resource = resources.at(i);
        }
    }

    return QLatin1Char('/') + resource;
}

void Connection::setLastResource(const QString &jid, const QString &resource)
{
    m_lastResources[jid] = resource;
}

uint Connection::ensureContactHandle(const QString &id)
{
    return m_uniqueContactHandleMap[id];
}

QString Connection::getContactIdentifier(uint handle) const
{
    return m_uniqueContactHandleMap[handle];
}

void Connection::updateMucParticipantInfo(const QString &participant, const QXmppPresence &presense)
{
    m_mucParticipants.insert(participant, presense);
}
