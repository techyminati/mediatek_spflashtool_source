#ifndef DEVICESCANTHREAD_H
#define DEVICESCANTHREAD_H

#include <time.h>

typedef struct __USB_DEVICE_INFO
{
    short vid;
    short pid;

} USB_DEVICE_INFO;

class DeviceScan
{
public:
    explicit DeviceScan(USB_DEVICE_INFO* info);
    ~DeviceScan();
    bool FindDeviceUSBPort(size_t ports_count, char *portName, int* p_stop_flag, const int& d_time_out);
    bool FindSpecialDeviceUSBPort(size_t ports_count, const char *sPreferComPort, char *portName, int* p_stop_flag, int d_time_out);

private:
    int init_hotplug_sock();
    bool GetDeviceInfo(size_t ports_count, const char *buf, size_t buf_size, char *portName);

    bool VerifyDeviceInfo(size_t ports_count, const USB_DEVICE_INFO *inst);

    bool WaitForDeviceReady(const char *path);

    void GetDeviceID();
    bool USBPathMatch(const char *buf, const char *preferComPort) const;

private:
    DeviceScan(const DeviceScan &rhs);
    DeviceScan & operator=(const DeviceScan &rhs);

private:
    USB_DEVICE_INFO* usb_info_;
    time_t start_time_;
    time_t end_time_;
    int hotplug_sock_;
};

#endif // DEVICESCANTHREAD_H
