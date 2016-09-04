/*
    Copyright (C) 2016 Alexandr Akulich <akulichalexander@gmail.com>

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "debug.hh"

static DebugInterface *s_instance = nullptr;

void DebugInterface::outputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_ASSERT(s_instance);

    Tp::BaseDebug *interface = s_instance->m_debugInterfacePtr.data();
    const QtMessageHandler defaultHandler = s_instance->m_defaultMessageHandler;
    Q_ASSERT(interface);

    QString domain = QStringLiteral("%1:%2, %3");
    domain = domain.arg(QString::fromLatin1(context.file)).arg(context.line).arg(QString::fromLatin1(context.function));
    switch (type) {
    case QtDebugMsg:
        interface->newDebugMessage(domain, Tp::DebugLevelDebug, msg);
        break;
    case QtInfoMsg:
        interface->newDebugMessage(domain, Tp::DebugLevelInfo, msg);
        break;
    case QtWarningMsg:
        interface->newDebugMessage(domain, Tp::DebugLevelWarning, msg);
        break;
    case QtCriticalMsg:
        interface->newDebugMessage(domain, Tp::DebugLevelCritical, msg);
        break;
    case QtFatalMsg:
        interface->newDebugMessage(domain, Tp::DebugLevelError, msg);
        break;
    }

    if (defaultHandler) {
        defaultHandler(type, context, msg);
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    const QString logMessage = qFormatLogMessage(type, context, msg);

    if (logMessage.isNull()) {
        return;
    }
#else
    const QString logMessage = QString::number(type) + QLatin1Char('|') + msg;
#endif

    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

DebugInterface::DebugInterface() :
    m_debugInterfacePtr(new Tp::BaseDebug()),
    m_defaultMessageHandler(nullptr)
{
    m_debugInterfacePtr->setGetMessagesLimit(-1);

    if (!m_debugInterfacePtr->registerObject(TP_QT_CONNECTION_MANAGER_BUS_NAME_BASE + QStringLiteral("nonsense"))) {
        return;
    }

    s_instance = this;
    m_defaultMessageHandler = qInstallMessageHandler(outputHandler);
}

DebugInterface::~DebugInterface()
{
    qInstallMessageHandler(m_defaultMessageHandler);

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool DebugInterface::isActive()
{
    return m_debugInterfacePtr->isRegistered();
}
