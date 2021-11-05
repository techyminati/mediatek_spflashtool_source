#ifndef CRITICALSECTIONSENTRY
#define CRITICALSECTIONSENTRY

#include <QSharedPointer>

class CriticalSectionImpl;

class CriticalSection
{
public:
    CriticalSection();
    virtual ~CriticalSection();

    void lock();
    void unlock();

private:
    CriticalSection(const CriticalSection &);
    void operator=(const CriticalSection &);

private:
    QSharedPointer<CriticalSectionImpl> m_cs_impl;
};

class CriticalSectionSentry
{
public:
    CriticalSectionSentry(CriticalSection &lock);
    virtual ~CriticalSectionSentry();

private:
    CriticalSection	&m_lock;
};

#endif // CRITICALSECTIONSENTRY

