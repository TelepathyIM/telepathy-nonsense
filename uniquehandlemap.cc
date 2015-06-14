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

#include "uniquehandlemap.hh"

UniqueHandleMap::UniqueHandleMap()
{

}

const QString UniqueHandleMap::operator[] (const uint handle)
{
    if (handle > static_cast<uint>(m_knownHandles.size())) {
        return QString();
    }
    return m_knownHandles[handle - 1];
}

uint UniqueHandleMap::operator[] (const QString &bareJid)
{
    int idx = m_knownHandles.indexOf(bareJid);
    if (idx != -1) {
        return idx + 1;
    }

    m_knownHandles.append(bareJid);
    return m_knownHandles.size();
}
