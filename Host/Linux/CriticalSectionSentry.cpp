#include "../Inc/CriticalSectionSentry.h"

#include <pthread.h>
#include <QSharedPointer>

class CriticalSectionImpl
{
public:
    CriticalSectionImpl() { pthread_mutex_init(&m_mutex, NULL); }
    virtual ~CriticalSectionImpl() { pthread_mutex_destroy(&m_mutex); }

    void lock() { pthread_mutex_lock(&m_mutex); }
    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
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
