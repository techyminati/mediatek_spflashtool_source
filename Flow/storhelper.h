#ifndef STORHELPER_H
#define STORHELPER_H

namespace APCore
{
    class Connection;
}

class CStorHelperImpl;

class CStorHelper
{
public:
    CStorHelper(APCore::Connection*);
    ~CStorHelper();

public:
    U64 AlignSize(void) const;
    U64 BlockSize(void) const;
    U64 LogicalSize(void) const;
    U64 PhysicalSize(void) const;
    U64 BootPartSize(void) const;
    U64 UserPartSize(void) const;

private:
    CStorHelper(const CStorHelper &);
    CStorHelper & operator=(const CStorHelper &);

private:
    CStorHelperImpl *my_impl_;
};

#endif // STORHELPER_H
