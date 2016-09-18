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

#include "filetransferchannel.hh"
#include "common.hh"
#include "connection.hh"

#include <TelepathyQt/IODevice>

FileTransferChannel::FileTransferChannel(Connection *connection, Tp::BaseChannel *baseChannel, const QVariantMap &request)
    : Tp::BaseChannelFileTransferType(request), m_localAbort(false)
{
    DBG;

    QXmppTransferFileInfo fileInfo;
    fileInfo.setDate(date());
    fileInfo.setHash(contentHash().toLatin1());
    fileInfo.setName(filename());
    fileInfo.setDescription(description());
    fileInfo.setSize(size());

    m_ioChannel = new Tp::IODevice();
    /* Open the QBuffer - QXmpp requires this */
    m_ioChannel->open(QIODevice::ReadWrite);

    connect(this, &FileTransferChannel::stateChanged, this, &FileTransferChannel::onStateChanged);

    if (direction() == FileTransferChannel::Incoming) {
        m_transferJob = request.value(NONSENSE_XMPP_TRANSFER_JOB).value<QXmppTransferJob *>();
    } else { // Outgoing
        QXmppTransferManager *transferManager = connection->qxmppClient()->findExtension<QXmppTransferManager>();
        Q_ASSERT(transferManager);
        m_transferJob = transferManager->sendFile(baseChannel->targetID() + connection->lastResourceForJid(baseChannel->targetID(), true), m_ioChannel, fileInfo);
    }

    Q_ASSERT(m_transferJob);
    Q_ASSERT(m_transferJob->error() == QXmppTransferJob::NoError);
    connect(m_transferJob, &QXmppTransferJob::stateChanged, this, &FileTransferChannel::onQxmppTransferStateChanged);
    /* Unfortunately, there are two error() functions (but only one signal) in QXmppTransferJob.
     * Use the old syntax for now */
    connect(m_transferJob, SIGNAL(error(QXmppTransferJob::Error)), this, SLOT(onTransferError(QXmppTransferJob::Error)));
    m_ioChannel->setParent(m_transferJob);
}

FileTransferChannelPtr FileTransferChannel::create(Connection *connection, Tp::BaseChannel *baseChannel, const QVariantMap &request)
{
    return FileTransferChannelPtr(new FileTransferChannel(connection, baseChannel, request));
}

/* The state of the CM changes */
void FileTransferChannel::onStateChanged(uint state, uint reason)
{
    DBG << state;
    switch (state) {
    case Tp::FileTransferStateAccepted:
        if (direction() == FileTransferChannel::Incoming) {
            m_transferJob->accept(m_ioChannel);
            remoteProvideFile(m_ioChannel); /* The initial offset is always 0 for QXmpp */
        }
        break;
    case Tp::FileTransferStateCancelled:
        if (reason == Tp::FileTransferStateChangeReasonLocalStopped) {
            m_localAbort = true;
            m_transferJob->abort();
        }
        break;
    case Tp::FileTransferStateNone:
    case Tp::FileTransferStatePending:
    case Tp::FileTransferStateOpen:
    case Tp::FileTransferStateCompleted:
        break;
    default:
        Q_ASSERT(0);
    }
}

/* QXmpp's state changes */
void FileTransferChannel::onQxmppTransferStateChanged(QXmppTransferJob::State state)
{
    DBG << state;

    switch (state) {
    case QXmppTransferJob::TransferState:
        if (direction() == FileTransferChannel::Outgoing) {
            remoteAcceptFile(m_ioChannel, 0); /* The initial offset is always 0 for QXmpp */
            connect(m_transferJob, &QXmppTransferJob::progress, this, &FileTransferChannel::onOutgoingTransferProgressChanged);
        }
    case QXmppTransferJob::StartState:
    case QXmppTransferJob::FinishedState:
        /* Nothing to do */
        break;
    default:
        Q_ASSERT(0);
    }
}

void FileTransferChannel::onTransferError(QXmppTransferJob::Error error)
{
    switch (error) {
    case QXmppTransferJob::AbortError:
        if (!m_localAbort) {
            setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonRemoteStopped);
        }
        break;
    case QXmppTransferJob::FileAccessError:
        setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonLocalError);
        break;
    case QXmppTransferJob::FileCorruptError:
    case QXmppTransferJob::ProtocolError:
        /* It's hard to say if the error is local or remote. */
        setState(Tp::FileTransferStateCancelled, Tp::FileTransferStateChangeReasonRemoteError);
        break;
    default:
        Q_ASSERT(0);
    }
}

void FileTransferChannel::onOutgoingTransferProgressChanged(qint64 transferred)
{
    setTransferredBytes(transferred);
}
