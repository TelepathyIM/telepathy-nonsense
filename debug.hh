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

#ifndef DEBUG_HH
#define DEBUG_HH

#include <TelepathyQt/BaseDebug>
#include <QScopedPointer>

class DebugInterface
{
public:
    DebugInterface();
    ~DebugInterface();

    bool isActive();

protected:
    static void outputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    QScopedPointer<Tp::BaseDebug> m_debugInterfacePtr;
    QtMessageHandler m_defaultMessageHandler;
};

#endif // DEBUG_HH
