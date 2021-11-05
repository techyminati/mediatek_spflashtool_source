#include "DLWithoutScatterCommand.h"
#include "../Logger/Log.h"
#include "../Err/Exception.h"
#include "../BootRom/flashtoolex_api.h"
#include "../BootRom/type_define.h"
#include "../BootRom/mtk_status.h"

namespace APCore
{

DLWithoutScatterCommand::DLWithoutScatterCommand(APKey key):
    ICommand(key),
    cb_op_progress_(NULL),
    cb_stage_message_(NULL)
{
}

DLWithoutScatterCommand::~DLWithoutScatterCommand()
{

}

void DLWithoutScatterCommand::get_partition_list(op_part_list_t* list, int count)
{
    if (list){
        for(int i = 0; i < count; i++){
            op_part_list_t item = part_list_.at(i);

            strncpy(list[i].part_name, item.part_name, strlen(item.part_name) + 1);
            strncpy(list[i].file_path, item.file_path, strlen(item.file_path) + 1);
        }
    }
}

void DLWithoutScatterCommand::exec(const QSharedPointer<Connection> &conn){
    Q_UNUSED(conn);

    HSESSION hs;

    int status = STATUS_OK;

    int count = part_list_.size();

    op_part_list_t *flist = new op_part_list_t[count];

    get_partition_list(flist, count);

    callbacks_struct_t cbs;
    memset(&cbs, 0, sizeof(cbs));

    cbs.cb_op_progress = cb_op_progress_ == NULL ? cb_dl_operation_progress : cb_op_progress_;

    cbs.cb_stage_message = cb_stage_message_ == NULL ? cb_dl_stage_message: cb_stage_message_;

    if(connect(&hs) != STATUS_OK){
        goto exit;
    }

    LOGI("Begin executing download without scatter command...");

    status = flashtool_download(
                hs,
                NULL,
                flist,
                part_list_.size(),
                &cbs);

    LOGI("Download result: %s(%d)", StatusToString(status), status);

    if(status != STATUS_OK)
    {
        goto exit;
    }

    LOGI("Download Succeeded.");

exit:
    delete[] flist;
    flist = NULL;

    reboot_device(hs);

    flashtool_destroy_session(hs);

    if(status != STATUS_OK)
    {
       THROW_BROM_EXCEPTION(status,0);
    }
}

void DLWithoutScatterCommand::cb_dl_operation_progress(void* arg, enum transfer_phase phase,
                                                       unsigned int progress,
                                                       unsigned long long data_xferd,
                                                       unsigned long long data_total,
                                                       const struct cbs_additional_info * cbs_add_info)
{
    Q_UNUSED(arg);
    Q_UNUSED(data_xferd);
    Q_UNUSED(data_total);
    Q_UNUSED(cbs_add_info);
   // unsigned long long  total_bytes = static_cast<unsigned long long>(static_cast<float>(data_xferd)*progress/100);
    if(phase == TPHASE_LOADER)
    {
        if(progress == 0)
        {
            LOGI("Download Boot Loader 0%");
        }
        else
        {
            LOGI("Download Boot Loader %d", progress);
        }
    }
    else
    {
        if(progress == 0)
        {
            LOGI("Download Flash 0%");
        }
        else
        {
            LOGI("Download Flash %d", progress);
        }
    }
}

void DLWithoutScatterCommand::cb_dl_stage_message(void *usr_arg, const char *message)
{
    Q_UNUSED(usr_arg);

    std::cout<< "Stage: " << message << std::endl;
}

}

