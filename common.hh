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

#ifndef COMMON_HH
#define COMMON_HH

#include <QLoggingCategory>
#include <TelepathyQt/ProtocolInterface>

Q_DECLARE_LOGGING_CATEGORY(qxmppGeneric)
Q_DECLARE_LOGGING_CATEGORY(qxmppStanza)
Q_DECLARE_LOGGING_CATEGORY(tracing)

#define DBG qCDebug(tracing) << "ENTERING " << Q_FUNC_INFO

class Common {
public:
    static Tp::SimpleStatusSpecMap getSimpleStatusSpecMap();
    static Tp::AvatarSpec getAvatarSpec();
};

#endif // COMMON_HH
