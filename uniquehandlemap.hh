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

#ifndef UNIQUEHANDLEMAP_HH
#define UNIQUEHANDLEMAP_HH

#include <QString>
#include <TelepathyQt/Types>

class UniqueHandleMap
{
public:
    UniqueHandleMap();

    const QString operator[] (const uint handle);
    uint operator[] (const QString& bareJid);

private:
    QStringList m_knownHandles;
};

#endif // UNIQUEHANDLEMAP_HH
