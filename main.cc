/*
    Copyright (C) 2015 Niels Ole Salscheider <niels_ole@salscheider-online.de>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <QCoreApplication>

#include <TelepathyQt/BaseConnectionManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Debug>

#include "protocol.hh"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("telepathy-qxmpp"));

    Tp::registerTypes();
    Tp::enableDebug(true);
    Tp::enableWarnings(true);

    Tp::BaseProtocolPtr proto = Tp::BaseProtocol::create<Protocol>(QLatin1String("xmpp"));
    Tp::BaseConnectionManagerPtr cm = Tp::BaseConnectionManager::create(QLatin1String("qxmpp"));

    cm->addProtocol(proto);
    cm->registerObject();

    return app.exec();
}
