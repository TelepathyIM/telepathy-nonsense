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

#ifndef UNIQUEHANDLEMAP_HH
#define UNIQUEHANDLEMAP_HH

#include <QStringList>

class UniqueHandleMap
{
public:
    UniqueHandleMap();

    const QString operator[] (const uint handle) const;
    uint operator[] (const QString &bareJid);

    bool contains(const uint handle) const;
    bool contains(const QString &bareJid) const;

private:
    QStringList m_knownHandles;
};

#endif // UNIQUEHANDLEMAP_HH
