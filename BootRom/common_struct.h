#ifndef _COMMON_STRUCT_H_
#define _COMMON_STRUCT_H_

#include "type_define.h"

#define MAX_PARTITION_COUNT		128

/*-----------USER SETTINGS-------------------*/
#define POWER_BATTERY            (0)
#define POWER_USB                (1)
#define POWER_AUTO_DECT          (2)

#define RESET_KEY_DEFAULT         (0)
#define RESET_KEY_ONE             (0x50)
#define RESET_KEY_TWO             (0x68)

/*--------------------------------------------*/
#define CHKSUM_LEVEL_NONE        (0)
#define CHKSUM_LEVEL_USB         (1<<0)
#define CHKSUM_LEVEL_STORAGE     (1<<1)
#define CHKSUM_LEVEL_PC_IMAGE   (1<<2)
#define CHKSUM_LEVEL_SKIP_PC_DRAM  (1<<3)
#define CHKSUM_LEVEL_USB_STORAGE (CHKSUM_LEVEL_USB | CHKSUM_LEVEL_STORAGE)


/*---------------------------enum----------------------------*/
struct reboot_option
{
   uint32 is_dev_reboot; //! 1: below arg is vaild; 0: below arg is invalid
   uint32 timeout_ms;
   uint32 async;       //!< 1: disconnect before watchdog timeout; 0: disconnect after watchdog timeout
   uint32 bootup;      //!< 1: enter charging after download; 0: auto boot up after download;2:enter fastboot mode.
   /* 1: Make PreLoader enter download mode without timeout;
       * 0: PreLoader with default timeout for download */
   uint32 dlbit;
   uint32 bNotResetRTCTime;
};

typedef enum reset_boot_mode
{
	BM_FASTBOOT = 1,
}reset_boot_mode_e;

typedef enum otp_action
{
   OTP_ZONE_READ = 0x1,
   OTP_ZONE_WRITE = 0x2,
   OTP_ZONE_GET_ADDR = 0x4,
   OTP_ZONE_GET_SIZE = 0x8,
   OTP_ZONE_GET_LOCK_STATUS = 0x10,
   //LOCK can be added to WRITE as addititional flag.
   //eg: (OTP_WRITE | OTP_LOCK)
   OTP_ZONE_LOCK = 0x10000,
}otp_action_e;

typedef enum storage_type
{
   STORAGE_UNKNOW = 0x0,
   STORAGE_NONE = STORAGE_UNKNOW,
   STORAGE_EMMC = 0x1,
   STORAGE_SDMMC,
   STORAGE_EMMC_END = 0xF,
   STORAGE_NAND = 0x10,
   STORAGE_NAND_SLC,
   STORAGE_NAND_MLC,
   STORAGE_NAND_TLC,
   STORAGE_NAND_AMLC,
   STORAGE_NAND_SPI,
   STORAGE_NAND_3DMLC,
   STORAGE_NAND_END = 0x1F,
   STORAGE_NOR = 0x20,
   STORAGE_NOR_SERIAL,
   STORAGE_NOR_PARALLEL,
   STORAGE_NOR_END = 0x2F,
   STORAGE_UFS = 0x30,
}storage_type_e;

typedef enum emmc_section{
   EMMC_UNKNOWN = 0
   ,EMMC_BOOT1
   ,EMMC_BOOT2
   ,EMMC_RPMB
   ,EMMC_GP1
   ,EMMC_GP2
   ,EMMC_GP3
   ,EMMC_GP4
   ,EMMC_USER
   ,EMMC_END
   ,EMMC_BOOT1_BOOT2
} emmc_section_e;

typedef enum ufs_section{
   UFS_SECTION_UNKONWN = 0
   ,UFS_SECTION_LU0
   ,UFS_SECTION_LU1
   ,UFS_SECTION_LU2
   ,UFS_SECTION_END
   ,UFS_SECTION_LU0_LU1
} ufs_section_e;

typedef enum normal_section{
   MONODY_SECTION = 0,
   BOOT1_SECTION=0x20,
   BOOT2_SECTION,
   USER_SECTION,
   NORMAL_SECTION_END
}normal_section_e;

/** DA-refactory nand section define**/
typedef enum nand_section{
   NAND_NONE   = 0,
   NAND_BOOT1 = 1,	/*preloader*/
   NAND_GP1	   = 2,	/*raw partition*/
   NAND_GP2       = 3,          /*mntl partition*/
   NAND_GP3       = 4,	/*ubifs partition*/
   NAND_NCT       = 5,	/*pmt, bmt*/
} nand_section_e;

typedef enum nand_cell_usage
{
   CELL_UNI = 0,		// SLC
   CELL_MULTI
}nand_cell_usage_e;

typedef enum nand_addr_type
{
   ADDR_LOGICAL = 0,
   ADDR_PHYSICAL,
   ADDR_PHYSICAL_PMT,
   ADDR_FLAG_END,
}nand_addr_type;

typedef enum nand_bin_type
{
   BIN_RAW = 0,
   BIN_UBI_IMG,
   BIN_FTL_IMG,
   BIN_FLAG_END,
}nand_bin_type;

typedef enum nand_format_level
{
   FORMAT_NORMAL = 0,
   FORMAT_FORCE,
   FORMAT_MARK_BAD_BLOCK,
   FORMAT_LEVEL_END
}nand_format_level;

typedef enum operation_type
{
   PAGE_SPARE = 0
   ,PAGE_ONLY
   ,SPARE_ONLY
   ,PAGE_WITH_ECC
   ,PAGE_SPARE_WITH_ECCDECODE
   ,VERIFY_AFTER_PROGRAM
   ,PAGE_SPARE_NORANDOM
   ,PAGE_FDM
   ,FLAG_END
}operation_type;

//memory test
typedef enum memory_type
{
   TEST_MEMORY
   ,TEST_STORAGE
}memory_type;

//RAM
typedef enum ram_type
{
   NONE = 0
  ,GENERAL_DRAM
  ,GENERAL_SRAM
}ram_type_e;

//storage life cycle status
enum life_cycle_status
{
     LIFE_CYCLE_NODEF = 0
    ,LIFE_CYCLE_NORMAL = 1
    ,LIFE_CYCLE_WARNING =2
    ,LIFE_CYCLE_EOL =3
};

//chip evolution
typedef enum chip_evolution_type
{
	CHIP_EVOLUTION_DEFAULT = 0,
	CHIP_EVOLUTION_KIBO_PLUS = 1,

	CHIP_EVOLULTION_NO_FOUND= 255

}chip_evolution_e;

typedef enum flash_hw_status
{
     //EMMC from 0x10
     EMMC_RPMB_KEY_EXIST=0x10,
     EMMC_RPMB_KEY_NOT_EXIST=0x11
     //UFS from 0x30
     //NAND from 0x50
}flash_hw_status_e;

/*-------------------------parameter struct------------------------------*/
struct nand_extension
{
   //enum nand_cell_usage  cell_usage;
   //enum nand_addr_type   addr_type;
   //enum nand_bin_type    bin_type;
   uint32 cell_usage;
   uint32 addr_type;
   uint32 bin_type;
   uint32 region;
   union
   {
      uint32 dummy;
      enum operation_type type;
      enum nand_format_level level;
   };
   uint32 sys_slc_percent;
   uint32 usr_slc_percent;
   /* the real physical size of partition, uint type is KB */
   uint32 phy_max_size;
};

struct emmc_extension
{
   //none.
};

struct nor_extension
{
   //none.
};

struct partition_extension_struct
{
   union
   {
      uint32 stub[6];//size alignment.
      struct nand_extension nand_ext;
      struct emmc_extension emmc_ext;
   };
};



/*--------------device info------------------------------*/
#pragma pack(push, 4)
struct mmc_info_struct
{
   uint32 type; //emmc or sdmmc or none.
   uint32 block_size;
   uint64 boot1_size;
   uint64 boot2_size;
   uint64 rpmb_size;
   uint64 gp1_size;
   uint64 gp2_size;
   uint64 gp3_size;
   uint64 gp4_size;
   uint64 user_size;
   uint32 cid[4];
   uint8 fwver[8];
   uint8 pre_eol_info;
   uint8 life_time_est_a;
   uint8 life_time_est_b;
   uint32 life_time_status;
};
#pragma pack(pop)

struct nand_info_struct
{
   uint32 type; //slc, mlc, spi, none
   uint32 page_size;
   uint32 block_size;
   uint32 spare_size;
   uint64 total_size;
   uint64 available_size;
   uint8  nand_bmt_exist;
   uint8  nand_id[12];
};

struct nor_info_struct
{
   uint32 type; //nor, none
   uint32 page_size;
   uint64 available_size;
};

//#pragma pack(push, 8)
struct ufs_info_struct
{
   uint32 type; //ufs or none.
   uint32 block_size;
   uint64 lu0_size;
   uint64 lu1_size;
   uint64 lu2_size;
   uint16 vendor_id;
   uint8 cid[16+4];
   uint8 fwver[4+4];
   uint8 sn[128+4];
   uint8 pre_eol_info;
   uint8 life_time_est_a;
   uint8 life_time_est_b;
   uint8 stub1;
   uint32 life_time_status;
};
//#pragma pack(pop)

struct ram_item_struct
{
   uint32 type; //general_sram, general_dram, none
   uint32 padding;
   uint64 base_address;
   uint64 size;
};
struct ram_info_struct
{
   struct ram_item_struct sram;
   struct ram_item_struct dram;
};

struct chip_id_struct
{
   uint16 hw_code;
   uint16 hw_sub_code;
   uint16 hw_version;
   uint16 sw_version;
   uint32 chip_evolution;
};

struct otp_parameter_struct
{
   uint32 offset_to_group;
   uint32 length;
};

struct register_unit
{
   uint32 address;
   uint32 value;
};

#define FULL_PRJ_NAME_LEN 60
#define OP_NAME_LEN 30

#define EMMC_ERR_TYPE_STR_MAXSIZE 31
struct emmc_error_detail
{
    uint8 emmc_err_type[EMMC_ERR_TYPE_STR_MAXSIZE+1];
    unsigned long lba;
};

struct da_error_detail
{
    union
    {
        uint8 buff[128];
        struct emmc_error_detail emmc_err_detail;
    };
};

struct ex_ufs_config
{
	uint32 force_provision; //default 0
	uint32 tw_size_gb; // default 0xFFFFFFFF
	uint32 tw_no_red;  // default 1
	uint32 hpb_count; //default 0xFFFFFFFF
};

#endif
