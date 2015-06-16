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

#ifndef FILETRANSFERCHANNEL_HH
#define FILETRANSFERCHANNEL_HH

#include <TelepathyQt/BaseChannel>

#include <QXmppTransferManager.h>

class FileTransferChannel;
class Connection;

namespace Tp
{
class IODevice;
}

typedef Tp::SharedPtr<FileTransferChannel> FileTransferChannelPtr;

#define NONSENSE_XMPP_TRANSFER_JOB (QLatin1String("QXmpp.TransferJob"))

class FileTransferChannel : public Tp::BaseChannelFileTransferType
{
    Q_OBJECT
public:
    static FileTransferChannelPtr create(Connection *connection, Tp::BaseChannel *baseChannel, const QVariantMap &request);

private:
    FileTransferChannel(Connection *connection, Tp::BaseChannel *baseChannel, const QVariantMap &request);

private slots:
    void onStateChanged(uint state, uint reason);
    void onQxmppTransferStateChanged(QXmppTransferJob::State state);
    void onTransferError(QXmppTransferJob::Error error);
    void onOutgoingTransferProgressChanged(qint64 transferred);

private:
    Tp::IODevice *m_ioChannel;
    QXmppTransferJob *m_transferJob;
    bool m_localAbort;
};

#endif // FILETRANSFERCHANNEL_HH
