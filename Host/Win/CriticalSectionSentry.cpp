#include "../Inc/CriticalSectionSentry.h"

#include <windows.h>
#include <QSharedPointer>

class CriticalSectionImpl
{
public:
    CriticalSectionImpl() { InitializeCriticalSection(&m_cs); }
    virtual ~CriticalSectionImpl() { DeleteCriticalSection(&m_cs); }

    void lock() { EnterCriticalSection(&m_cs); }
    void unlock() { LeaveCriticalSection(&m_cs); }

private:
    CRITICAL_SECTION m_cs;
};

CriticalSection::CriticalSection(): m_cs_impl(new CriticalSectionImpl())
{

}

CriticalSection::~CriticalSection()
{

}

void CriticalSection::lock()
{
    if (m_cs_impl)
        m_cs_impl->lock();
}

void CriticalSection::unlock()
{
    if (m_cs_impl)
        m_cs_impl->unlock();
}

CriticalSectionSentry::CriticalSectionSentry(CriticalSection &lock): m_lock(lock)
{
    m_lock.lock();
}

CriticalSectionSentry::~CriticalSectionSentry()
{
    m_lock.unlock();
}
