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

#ifndef PROTOCOL_HH
#define PROTOCOL_HH

#include <TelepathyQt/BaseProtocol>

class Protocol : public Tp::BaseProtocol
{
    Q_OBJECT
    Q_DISABLE_COPY(Protocol)

public:
    Protocol(const QDBusConnection &dbusConnection, const QString &name);

private:
    Tp::BaseConnectionPtr createConnection(const QVariantMap &parameters, Tp::DBusError *error);
    QString identifyAccount(const QVariantMap &parameters, Tp::DBusError *error);
    QString normalizeContact(const QString &contactId, Tp::DBusError *error);

    /* Protocol.I.Addressing */
    QString normalizeVCardAddress(const QString &vCardField, const QString vCardAddress,
            Tp::DBusError *error);
    QString normalizeContactUri(const QString &uri, Tp::DBusError *error);

private:
    Tp::BaseProtocolAddressingInterfacePtr m_addrIface;
    Tp::BaseProtocolAvatarsInterfacePtr m_avatarsIface;
    Tp::BaseProtocolPresenceInterfacePtr m_presenceIface;

};

#endif // PROTOCOL_HH
