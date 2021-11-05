#ifndef ASYNCUARTCOMPORTINFO_H
#define ASYNCUARTCOMPORTINFO_H

#include <QThread>

class AsyncUARTComPortInfo: public QThread
{
    Q_OBJECT

public:
    AsyncUARTComPortInfo();
    virtual ~AsyncUARTComPortInfo();

    virtual void run();

signals:
    void signal_send_uart_infos(const std::list<std::string> &uart_port_list);
};

#endif // ASYNCUARTCOMPORTINFO_H

