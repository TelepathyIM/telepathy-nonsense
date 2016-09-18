/*
 * This file is part of the telepathy-nonsense connection manager.
 * Copyright (C) 2015 Niels Ole Salscheider <niels_ole@salscheider-online.de>
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

#include "common.hh"

Q_LOGGING_CATEGORY(qxmppGeneric, "qxmpp.generic")
Q_LOGGING_CATEGORY(qxmppStanza, "qxmpp.stanza")
Q_LOGGING_CATEGORY(tracing, "nonsense.tracing")

Tp::SimpleStatusSpecMap Common::getSimpleStatusSpecMap()
{
    Tp::SimpleStatusSpec spOffline;
    spOffline.type = Tp::ConnectionPresenceTypeOffline;
    spOffline.maySetOnSelf = true;
    spOffline.canHaveMessage = false;

    Tp::SimpleStatusSpec spAvailable;
    spAvailable.type = Tp::ConnectionPresenceTypeAvailable;
    spAvailable.maySetOnSelf = true;
    spAvailable.canHaveMessage = true;

    Tp::SimpleStatusSpec spAway;
    spAway.type = Tp::ConnectionPresenceTypeAway;
    spAway.maySetOnSelf = true;
    spAway.canHaveMessage = true;

    Tp::SimpleStatusSpec spXa;
    spXa.type = Tp::ConnectionPresenceTypeExtendedAway;
    spXa.maySetOnSelf = true;
    spXa.canHaveMessage = true;

    Tp::SimpleStatusSpec spDnd;
    spDnd.type = Tp::ConnectionPresenceTypeBusy;
    spDnd.maySetOnSelf = true;
    spDnd.canHaveMessage = true;

    Tp::SimpleStatusSpec spChat;
    spChat.type = Tp::ConnectionPresenceTypeAvailable;
    spChat.maySetOnSelf = true;
    spChat.canHaveMessage = true;

    Tp::SimpleStatusSpec spHidden;
    spHidden.type = Tp::ConnectionPresenceTypeHidden;
    spHidden.maySetOnSelf = true;
    spHidden.canHaveMessage = true;

    Tp::SimpleStatusSpec spUnknown;
    spUnknown.type = Tp::ConnectionPresenceTypeUnknown;
    spUnknown.maySetOnSelf = false;
    spUnknown.canHaveMessage = false;

    Tp::SimpleStatusSpecMap specs;
    specs.insert(QStringLiteral("offline"), spOffline);
    specs.insert(QStringLiteral("available"), spAvailable);
    specs.insert(QStringLiteral("away"), spAway);
    specs.insert(QStringLiteral("xa"), spXa);
    specs.insert(QStringLiteral("dnd"), spDnd);
    specs.insert(QStringLiteral("chat"), spChat);
    specs.insert(QStringLiteral("hidden"), spHidden);
    specs.insert(QStringLiteral("unknown"), spUnknown);
    return specs;
}

Tp::AvatarSpec Common::getAvatarSpec()
{
    return Tp::AvatarSpec(QStringList() << QStringLiteral("image/png") << QStringLiteral("image/jpeg"),
                          0 /* minHeight */,
                          512 /* maxHeight */,
                          256 /* recommendedHeight */,
                          0 /* minWidth */,
                          512 /* maxWidth */,
                          256 /* recommendedWidth */,
                          1024 * 1024 /* maxBytes */);
}
