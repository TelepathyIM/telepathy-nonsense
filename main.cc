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

#include <QCoreApplication>

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>

#include "debug.hh"
#include "protocol.hh"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("telepathy-nonsense"));

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);
    DebugInterface debug;

    Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<Protocol>(QStringLiteral("xmpp"));
    Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(QStringLiteral("nonsense"));

    cm->addProtocol(proto);
    cm->registerObject();

    return app.exec();
}
