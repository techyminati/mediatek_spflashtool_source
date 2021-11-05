#include "DeviceScan.h"
#include "Logger/Log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>
#include <netinet/in.h>
#include <QTime>

#include "../../BootRom/brom.h"

#define UEVENT_BUFFER_SIZE 2048

DeviceScan::DeviceScan(USB_DEVICE_INFO* info) :
    usb_info_(info),
    start_time_(0),
    end_time_(0),
    hotplug_sock_(0)
{
}

DeviceScan::~DeviceScan()
{
    if(hotplug_sock_ > 0)
       close(hotplug_sock_);
}

int DeviceScan::init_hotplug_sock()
{
    const int bufferSize = 2048;

    struct sockaddr_nl snl;
    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

    int s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);

    if(s == -1)
    {
        LOGI("create socket error.\n");

        return -1;
    }

    int tmp = 1;

    int ret = setsockopt(s, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret == -1)
    {
        LOGI("set socket receive buffer failed!\n");
        close(s);
        return -1;
    }

    ret = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));
    if (ret == -1)
    {
        LOGI("set socket reuse addr option fail!\n");
        close(s);
        return -1;
    }

    ret = bind(s, (struct sockaddr*)&snl, sizeof(struct sockaddr_nl));

    if(ret < 0)
    {
        LOGI("bind socket error.\n");
        close(s);
        return -1;
    }

    return s;
}

bool DeviceScan::VerifyDeviceInfo(size_t ports_count, const USB_DEVICE_INFO *inst)
{
    for (size_t i=0; i < ports_count; ++i)
    {
        if (usb_info_[i].vid == inst->vid &&
                usb_info_[i].pid == inst->pid )
        {
            start_time_ = time(NULL);
            return true;
        }
    }

    return false;
}

bool DeviceScan::WaitForDeviceReady(const char *path)
{
#if 1
    //int fd = -1;

    // UEVENT is earlier than create tty node
    // so sleep some time...
    LOGI("<%s>: waiting...\n", path);

    if(-1 == chown(path, 0, 0))
    {
        LOGI("change owner as root: <%s>: failed %d, %s~\n", path, errno, strerror(errno));
    }

    if(-1 == chmod(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH))
    {
        LOGI("change owner as root: <%s>: failed %d, %s~\n", path, errno, strerror(errno));
    }
#if 0
    for (int i=0; i<10000; ++i)
    {
        if ((fd=open(path, O_RDWR|O_NONBLOCK|O_NOCTTY, 0)) > 0)
        {
            LOGI("round:%d <%s>: ready~\n", i, path);
            close(fd);
            sleep(1);

            end_time_ = time(NULL);
           return true;
        }
        else
        {
            LOGI("[%d]open %s failed: %d, %s\n",
                i, path, errno, strerror(errno));
        }
        usleep(10);
    }

#endif

    LOGI("<%s>: not ready!\n", path);

    return false;
#else
    usleep(500000);
    return true;
#endif
}

bool DeviceScan::USBPathMatch(const char *buf, const char *preferComPort) const
{
    LOGI("source usb_path: %s, search usb com port: %s", buf, preferComPort);
    std::string usb_path = std::string(buf);
    size_t npos = usb_path.rfind(':');
    std::string pre_usb_path = usb_path.substr(0, npos);
    npos = pre_usb_path.rfind('/');
    std::string usb_com_port = pre_usb_path.substr(npos + 1);
    return usb_com_port == std::string(preferComPort);
}

bool DeviceScan::GetDeviceInfo(size_t ports_count,
        const char *buf, size_t buf_size,
        char *portName)
{
    const char *prefix = "add@";
    const char *temp1 = "/tty/ttyACM";
    const char *sysPrefix ="/sys";
    const char *PID ="idProduct";
    const char *VID ="idVendor";
    const char *removePrefix = "remove@";

    char mybuf[UEVENT_BUFFER_SIZE * 2]={0};

    char *firstPosition;
    char *lastPosition;

    USB_DEVICE_INFO newdev = { 0, 0 };

    memcpy(mybuf, buf, buf_size);

    firstPosition = strstr(mybuf,prefix);
    lastPosition = strstr(mybuf, temp1);
    if(!firstPosition)
        firstPosition = strstr(mybuf, removePrefix);


    int prefixLen = strlen(prefix);
    int length = lastPosition - firstPosition - prefixLen;
    char path_without_sys_prefix [UEVENT_BUFFER_SIZE]={0};
    char path_without_sys_prefix_up_dir[UEVENT_BUFFER_SIZE]={0};
    char path_with_sys_prefix[UEVENT_BUFFER_SIZE]={0};
    char path_of_pid[UEVENT_BUFFER_SIZE]={0};
    char path_of_vid[UEVENT_BUFFER_SIZE]={0};

    if(firstPosition && lastPosition)
    {
        memcpy(path_without_sys_prefix,firstPosition+prefixLen,length);

        char* lastToken=rindex(path_without_sys_prefix,'/');
        int len = strlen(lastToken);

        memcpy(path_without_sys_prefix_up_dir,path_without_sys_prefix,length-len);

        sprintf(path_with_sys_prefix,"%s%s",sysPrefix,path_without_sys_prefix_up_dir);

        sprintf(path_of_vid,"%s/%s",path_with_sys_prefix,VID);
        sprintf(path_of_pid,"%s/%s",path_with_sys_prefix,PID);

        //read file
        char tmpbuf[5] = {0};

        int fd = open(path_of_vid,O_RDONLY);
        if (fd < 0)
        {
            LOGI("open VID device failed!\n");
            return false;
        }
        size_t readSize = read(fd,tmpbuf,sizeof(tmpbuf)-1);
        Q_UNUSED(readSize);
        close(fd);

        LOGI("vid is %s\n",tmpbuf);

        LOGI("device vid = %04x\n", strtol(tmpbuf, NULL, 16));

        newdev.vid = strtol(tmpbuf, NULL, 16);

        fd = open(path_of_pid,O_RDONLY);
        if (fd < 0)
        {
            LOGI("open PID device failed!\n");
            return false;
        }
        readSize = read(fd,tmpbuf,sizeof(tmpbuf)-1);
        Q_UNUSED(readSize);
        close(fd);

        LOGI("pid is %s\n", tmpbuf);

        newdev.pid = strtol(tmpbuf, NULL, 16);

        LOGI("device pid = %04x\n", strtol(tmpbuf, NULL, 16));

        char portNameTmp[UEVENT_BUFFER_SIZE]={0};
        memcpy(portNameTmp,rindex(lastPosition,'/'),strlen(lastPosition));
        sprintf(portName,"/dev%s",portNameTmp);
        LOGI("com portName is: %s\n",portName);

        if(VerifyDeviceInfo(ports_count, &newdev))// && WaitForDeviceReady(portName))
        {
            double  totalT = difftime(end_time_, start_time_);
            LOGI("Total wait time = %f", totalT);
            return true;
        }
    }

    return false;
}

bool DeviceScan::FindDeviceUSBPort(size_t ports_count, char *portName, int* p_stop_flag,const int& d_time_out)
{
    hotplug_sock_ = init_hotplug_sock();

    struct timeval tv;
    int ret, recvLen;

    QTime time;
    time.start();

    while(BOOT_STOP != (*p_stop_flag))
    {
        char buf[UEVENT_BUFFER_SIZE * 2] = {0};

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(hotplug_sock_, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        ret = select(hotplug_sock_ + 1, &fds, NULL, NULL, &tv);

        if (time.elapsed() > d_time_out )
        {
            LOGE("Timeout(%u ms) for searching USB port!",
                 d_time_out);

            return false;
        }

        if(ret < 0)
            continue;

        if(!FD_ISSET(hotplug_sock_, &fds))
            continue;

        recvLen = recv(hotplug_sock_, &buf, sizeof(buf), 0);

        if(recvLen > 0)
        {
            LOGI("%s\n", buf);

            if(GetDeviceInfo(ports_count, buf, sizeof(buf), portName))
            {
                close(hotplug_sock_);
                return true;
            }
        }
    }
    return false;
}

bool DeviceScan::FindSpecialDeviceUSBPort(size_t ports_count, const char *sPreferComPort, char *portName, int *p_stop_flag, int d_time_out)
{
    hotplug_sock_ = init_hotplug_sock();

    struct timeval tv;
    int ret, recvLen;

    QTime time;
    time.start();

    while(BOOT_STOP != (*p_stop_flag))
    {
        char buf[UEVENT_BUFFER_SIZE * 2] = {0};

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(hotplug_sock_, &fds);

        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        ret = select(hotplug_sock_ + 1, &fds, NULL, NULL, &tv);

        if (time.elapsed() > d_time_out )
        {
            LOGE("Timeout(%u ms) for searching USB port!",
                 d_time_out);

            return false;
        }

        if(ret < 0)
            continue;

        if(!FD_ISSET(hotplug_sock_, &fds))
            continue;

        recvLen = recv(hotplug_sock_, &buf, sizeof(buf), 0);

        if(recvLen > 0)
        {
            LOGI("%s\n", buf);
            if (!USBPathMatch(buf, sPreferComPort))
            {
                LOGI("skip: %s\n", buf);
                continue;
            }

            if(GetDeviceInfo(ports_count, buf, sizeof(buf), portName))
            {
                close(hotplug_sock_);
                return true;
            }
        }
    }
    return false;
}
