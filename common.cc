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
    specs.insert(QLatin1String("offline"), spOffline);
    specs.insert(QLatin1String("available"), spAvailable);
    specs.insert(QLatin1String("away"), spAway);
    specs.insert(QLatin1String("xa"), spXa);
    specs.insert(QLatin1String("dnd"), spDnd);
    specs.insert(QLatin1String("chat"), spChat);
    specs.insert(QLatin1String("hidden"), spHidden);
    specs.insert(QLatin1String("unknown"), spUnknown);
    return specs;
}

Tp::AvatarSpec Common::getAvatarSpec()
{
    return Tp::AvatarSpec(QStringList() << QLatin1String("image/png") << QLatin1String("image/jpeg"),
                          0 /* minHeight */,
                          512 /* maxHeight */,
                          256 /* recommendedHeight */,
                          0 /* minWidth */,
                          512 /* maxWidth */,
                          256 /* recommendedWidth */,
                          1024 * 1024 /* maxBytes */);
}
