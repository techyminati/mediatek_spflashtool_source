#include "ConnSetting.h"
#include "EfuseSetting.h"
#include "../Cmd/EfuseCommand.h"
#include "../XMLParser/XMLNode.h"
#include "../Host/Inc/RuntimeMemory.h"

#include <time.h>
#include <QtGlobal>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>

using std::string;

#ifdef _WIN32
#define _STRICMP(s, c)  \
   (_stricmp((s).c_str(), (c))==0)
#else
#define _STRICMP(s, c)  \
   (strcasecmp((s).c_str(), (c))==0)
#endif

#define Text2Bool(txt)  \
   _STRICMP(txt, "true")

#define Bool2EfuseOpt(txt)  \
   (Text2Bool(txt) ? EFUSE_ENABLE : EFUSE_DISABLE)

#define _sbc_key_E sbc_pub_key_u.w_sbc_pub_key.key_e
#define _sbc_key_N sbc_pub_key_u.w_sbc_pub_key.key_n

#define _sbc_key1_E sbc_pub_key1_u.w_sbc_pub_key1.key_e
#define _sbc_key1_N sbc_pub_key1_u.w_sbc_pub_key1.key_n

#define _sbc_key2_E sbc_pub_key2_u.w_sbc_pub_key2.key_e
#define _sbc_key2_N sbc_pub_key2_u.w_sbc_pub_key2.key_n

#define _sbc_key3_E sbc_pub_key3_u.w_sbc_pub_key3.key_e
#define _sbc_key3_N sbc_pub_key3_u.w_sbc_pub_key3.key_n


static bool Str2Buf(_BUFFER *buf, const string &str)
{
   const U32 length = str.length();

   if (length > 0 && length%2 == 0)
   {
      buf->buf_len = length/2;
      buf->buf = new char[length+1];
      strcpy(buf->buf, str.c_str());
      return true;
   }
   return false;
}

namespace APCore
{
   EfuseSetting::EfuseSetting() :
time_prefix_(false),
   only_output_(false),
   b_output_stb_lock_(false),
   b_output_stb_key_(false)
{
   memset(&common_arg_, 0, sizeof(common_arg_));
   memset(&secure_arg_, 0, sizeof(secure_arg_));
   memset(&lock_arg_, 0, sizeof(lock_arg_));
   memset(&stb_key_arg_, 0, sizeof(stb_key_arg_));
   memset(&extra_arg_, 0, sizeof(extra_arg_));
   memset(&bypass_rule_, 0, sizeof(bypass_rule_));
}

EfuseSetting::~EfuseSetting()
{
   if (common_arg_.spare.buf != NULL)
   {
      delete common_arg_.spare.buf;
      common_arg_.spare.buf = NULL;
   }

   if (secure_arg_.ac_key.buf != NULL)
   {
      delete secure_arg_.ac_key.buf;
      secure_arg_.ac_key.buf = NULL;
   }
   if (secure_arg_._sbc_key_E.buf != NULL)
   {
      delete secure_arg_._sbc_key_E.buf;
      secure_arg_._sbc_key_E.buf = NULL;
   }
   if (secure_arg_._sbc_key_N.buf != NULL)
   {
      delete secure_arg_._sbc_key_N.buf;
      secure_arg_._sbc_key_N.buf = NULL;
   }

   for(int i = 0; i < EFUSE_STK_KEY_NUMBER; i++)
   {
      if(stb_key_arg_.stb_blow_keys[i].key_name != NULL)
      {
         delete stb_key_arg_.stb_blow_keys[i].key_name;
         stb_key_arg_.stb_blow_keys[i].key_name = NULL;
      }

      if(stb_key_arg_.stb_blow_keys[i].stb_key.buf != NULL)
      {
         delete stb_key_arg_.stb_blow_keys[i].stb_key.buf;
         stb_key_arg_.stb_blow_keys[i].stb_key.buf = NULL;
      }
   }

   for(unsigned int i = 0; i < extra_arg_.item_count; i++)
   {
      if(extra_arg_.items[i].type == T_BUF &&
         extra_arg_.items[i].data.key_pair.key.buf != NULL)
      {
         delete extra_arg_.items[i].data.key_pair.key.buf;
         extra_arg_.items[i].data.key_pair.key.buf = NULL;
      }
   }
}

_SHARED_PTR<ICommand> EfuseSetting::CreateCommand(APKey key)
{
   QSharedPointer<APCore::EfuseCommand> cmd(new EfuseCommand(key, this));

   cmd->setreadonly(only_output_);
   cmd->setStbLockEnable(b_output_stb_lock_);
   cmd->setStbKeyEnable(b_output_stb_key_);

   return cmd;
}

void EfuseSetting::LoadXML(const XML::Node &node)
{
   string node_tag;
   XML::Node child_node = node.GetFirstChildNode();

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      if (_STRICMP(node_tag, "common-ctrl"))
      {
         LoadCommonCtrl(child_node);
      }
	  else if (_STRICMP(node_tag, "sbc-pubk-ctrl"))
	  {
	      //mt8168 added for parse sbc_pubk_ctrl node
	      LoadSbcPubkCtrl(child_node);
	  }
	  else if(_STRICMP(node_tag, "fa-ctrl"))
	  {
	      // mt8168 added for parse fa mode ctrl
	      LoadFaModeCtrl(child_node);
	  }
      else if (_STRICMP(node_tag, "custom-ctrl"))
      {
         //add for MT8695, HASH1~3 ctrl should be custom ctrl
         //but in order to not change interface, the ctrls would be taken by common ctrl.
         LoadCommonCtrl(child_node);
      }
      else if (_STRICMP(node_tag, "usb-id"))
      {
         LoadUsbID(child_node);
      }
      else if (_STRICMP(node_tag, "secure-ctrl"))
      {
         LoadSecureCtrl(child_node);
      }
      else if (_STRICMP(node_tag, "secure-ctrl2"))
      {
         //add for MT8695, Disable JTAG is writen in secure ctrl2
         //but in order to not change interface, the ctrls would be taken by secure ctrl.
         LoadSecureCtrl(child_node);
      }
      else if(_STRICMP(node_tag, "c_lock"))
      {
         LoadCLock(child_node);
      }
      else if(_STRICMP(node_tag, "M_Res"))
      {
         LoadMHWRes(child_node);
      }
      else if(_STRICMP(node_tag, "ctrl1"))
      {
         LoadCtrlKey(child_node);
      }
      else if(_STRICMP(node_tag, "sec_ctrl1"))
      {
         LoadSecCtrl1Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_ctrl0"))
      {
         LoadCCtrl0Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_ctrl_0"))
      {
         LoadCustomerCtrl0Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_ctrl1"))
      {
         LoadCCtrl1Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_ctrl2"))
      {
         LoadCCtrl2Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_ctrl3"))
      {
         LoadCCtrl3Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_0")||_STRICMP(node_tag, "c_data0"))
      {
         LoadCData0(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_1")||_STRICMP(node_tag, "c_data1"))
      {
         LoadCData1(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_2")||_STRICMP(node_tag, "c_data2"))
      {
         LoadCData2(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_3")||_STRICMP(node_tag, "c_data3"))
      {
         LoadCData3(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_4")||_STRICMP(node_tag, "c_data4"))
      {
         LoadCData4(child_node);
      }
      else if(_STRICMP(node_tag, "c_data_5")||_STRICMP(node_tag, "c_data5"))
      {
         LoadCData5(child_node);
      }
      else if (_STRICMP(node_tag, "sbc-pub-key"))
      {
         LoadSbcPubKey(child_node);
      }
      else if (_STRICMP(node_tag, "common-lock"))
      {
         LoadCommonLock(child_node);
      }
      else if (_STRICMP(node_tag, "secure-lock"))
      {
         LoadSecureLock(child_node);
      }
      else if (_STRICMP(node_tag, "ac-key"))
      {
         LoadAcKey(child_node);
      }
      else if (_STRICMP(node_tag, "spare"))
      {
         LoadSpare(child_node);
      }
      else if (_STRICMP(node_tag, "read-back"))
      {
         LoadReadBack(child_node);
      }
      else if(_STRICMP(node_tag, "stb-lock"))
      {
         LoadSTBLock(child_node);
         b_output_stb_lock_ = true;
      }
      else if(_STRICMP(node_tag, "stb-id"))
      {
         LoadSTBID(child_node);
      }
      else if(_STRICMP(node_tag, "stb-key-group"))
      {
         LoadSTBKey(child_node);
         b_output_stb_key_ = true;
      }
      else if(_STRICMP(node_tag, "sec_msc"))
      {
         LoadSecMsc(child_node);
      }
      else if(_STRICMP(node_tag, "cm_flag"))
      {
         LoadCmFlag(child_node);
      }
      else if(_STRICMP(node_tag, "custk"))
      {
         LoadCustk(child_node);
      }
      else if(_STRICMP(node_tag, "cust_data"))
      {
         LoadCustData(child_node);
      }
      else if(_STRICMP(node_tag, "cust_crypt_data"))
      {
         LoadCustCryptData(child_node);
      }
      else if(_STRICMP(node_tag, "c_sw_ver0"))
      {
	LoadSWVer0(child_node);
      }
      else if(_STRICMP(node_tag, "c_sw_ver1"))
      {
	LoadSWVer1(child_node);
      }
     else if(_STRICMP(node_tag, "c_sw_ver2"))
      {
	LoadSWVer2(child_node);
      }
     else if(_STRICMP(node_tag, "c_sw_ver3"))
      {
	LoadSWVer3(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_data"))
      {
	Load3PData(child_node);
      }
      else if(_STRICMP(node_tag, "run_time_blowing_config"))
      {
         LoadBypassItems(child_node);
      }
      else if (_STRICMP(node_tag, "sbc-pub-key1"))
      {
         LoadSbcPubKey1(child_node);
      }
      else if (_STRICMP(node_tag, "sbc-pub-key2"))
      {
         LoadSbcPubKey2(child_node);
      }
      else if (_STRICMP(node_tag, "sbc-pub-key3"))
      {
         LoadSbcPubKey3(child_node);
      }
      /*
      else
      {
      Q_ASSERT(0 && "unknown efuse setting.");
      }
      */
      child_node = child_node.GetNextSibling();
   }
}

void EfuseSetting::SaveXML(XML::Node &node) const
{
   Q_UNUSED(node);
}

void EfuseSetting::LoadSbcPubkCtrl(
	const XML::Node &node)
{
    string text;

	text = node.GetAttribute("Disable_SBC_PUBK_HASH");
	common_arg_.sbc_pub_hash_dis = Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_HASH1");
	common_arg_.sbc_pub_hash1_dis = Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_HASH2");
	common_arg_.sbc_pub_hash2_dis = Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_HASH3");
	common_arg_.sbc_pub_hash3_dis = Bool2EfuseOpt(text);


	text = node.GetAttribute("Disable_SBC_PUBK_HASH_FA");
	common_arg_.sbc_pubk_hash_fa_dis= Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_HASH1_FA");
	common_arg_.sbc_pubk_hash1_fa_dis= Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_HASH2_FA");
	common_arg_.sbc_pubk_hash2_fa_dis= Bool2EfuseOpt(text);	 
	text = node.GetAttribute("Disable_SBC_PUBK_HASH3_FA");
	common_arg_.sbc_pubk_hash3_fa_dis= Bool2EfuseOpt(text);
	text = node.GetAttribute("Disable_SBC_PUBK_MTK_FA");
	common_arg_.sbc_pubk_hash_mtk_fa_dis= Bool2EfuseOpt(text);

}

void EfuseSetting::LoadFaModeCtrl(
	const  XML::Node &node)
{
    string text;

	text = node.GetAttribute("Enable_FA");
	common_arg_.fa_mode_en = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadCommonCtrl(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("Disable_EMMC_boot");
   common_arg_.emmc_boot_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_NAND_boot");
   common_arg_.nand_boot_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_NAND_boot_speedup");
   common_arg_.nand_boot_speedup_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_EFUSE_USB_PID_VID_CUSTOM_EN");
   common_arg_.pid_vid_custom_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_UFS_boot");
   common_arg_.ufs_boot_dis= Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_SF_boot");
   common_arg_.efuse_sf_boot_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_SDMMC_boot");
   common_arg_.sdmmc_boot_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("USB_download_type");
   common_arg_.usbdl_type = atoi(text.c_str());

   if (common_arg_.usbdl_type != 0)
   {
      common_arg_.usbdl_type_blow = EFUSE_ENABLE;
   }
   else
   {
      common_arg_.usbdl_type_blow = EFUSE_DISABLE;
   }

   text = node.GetAttribute("Disable_SBC_PUBK_HASH");
   common_arg_.sbc_pub_hash_dis = Bool2EfuseOpt(text);
   text = node.GetAttribute("Disable_SBC_PUBK_HASH1");
   common_arg_.sbc_pub_hash1_dis = Bool2EfuseOpt(text);
   text = node.GetAttribute("Disable_SBC_PUBK_HASH2");
   common_arg_.sbc_pub_hash2_dis = Bool2EfuseOpt(text);
   text = node.GetAttribute("Disable_SBC_PUBK_HASH3");
   common_arg_.sbc_pub_hash3_dis = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadUsbID(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("vid");
   common_arg_.usb_vid = strtoul(text.c_str(), NULL, 16);

   text = node.GetAttribute("pid");
   common_arg_.usb_pid = strtoul(text.c_str(), NULL, 16);

   if (common_arg_.usb_vid != 0 &&
      common_arg_.usb_pid != 0 )
   {
      common_arg_.usb_id_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSpare(
   const XML::Node &node)
{
   if (Str2Buf(&common_arg_.spare, node.GetText()))
   {
      common_arg_.spare_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSecureCtrl(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("Enable_ACC");
   secure_arg_.acc_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_ACK");
   secure_arg_.ack_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_SBC");
   secure_arg_.sbc_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_SLA");
   secure_arg_.sla_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_DAA");
   secure_arg_.daa_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_JTAG");
   secure_arg_.jtag_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_Root_Cert");
   secure_arg_.root_cert_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_SW_JTAG_CON");
   secure_arg_.jtag_sw_con = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_Rom_Cmd");
   secure_arg_.rom_cmd_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Disable_DBGPORT_LOCK");
   secure_arg_.dbgport_lock_dis = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_JTAG_SEC_PASSWD");
   secure_arg_.jtag_sec_passwd_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_PL_AR");
   secure_arg_.pl_ar_en= Bool2EfuseOpt(text);

   text = node.GetAttribute("Enable_PK_CUS");
   secure_arg_.pk_cus_en= Bool2EfuseOpt(text);
}

void EfuseSetting::LoadSbcPubKey(
   const XML::Node &node)
{
   string keyE, keyN, key_type;
   string node_tag;

   XML::Node child_node = node.GetFirstChildNode();

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      if (_STRICMP(node_tag, "pub-key-e"))
      {
         Q_ASSERT(keyE.empty());
         keyE = child_node.GetText();
      }
      else if (_STRICMP(node_tag, "pub-key-n"))
      {
         Q_ASSERT(keyN.empty());
         keyN = child_node.GetText();
      }
      else if(_STRICMP(node_tag, "key-type"))
      {
         Q_ASSERT(keyN.empty());
         key_type = child_node.GetText();
         if(_STRICMP(key_type, "legacy"))
         {
            LOGD("legacy");
            secure_arg_.sbc_pub_key_u.w_sbc_pub_key.key_type= 0;
         }
         else if(_STRICMP(key_type, "pss"))
         {
            LOGD("pss");
            secure_arg_.sbc_pub_key_u.w_sbc_pub_key.key_type= 1;
         }
      }
      /*
      else
      {
      Q_ASSERT(0);
      break;
      }
      */
      child_node = child_node.GetNextSibling();
   }

   if (Str2Buf(&secure_arg_._sbc_key_E, keyE) &&
      Str2Buf(&secure_arg_._sbc_key_N, keyN))
   {
      secure_arg_.sbc_pubk_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSbcPubKey1(
   const XML::Node &node)
{
   string keyE, keyN, key_type;
   string node_tag;

   XML::Node child_node = node.GetFirstChildNode();

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      if (_STRICMP(node_tag, "pub-key-e"))
      {
         Q_ASSERT(keyE.empty());
         keyE = child_node.GetText();
      }
      else if (_STRICMP(node_tag, "pub-key-n"))
      {
         Q_ASSERT(keyN.empty());
         keyN = child_node.GetText();
      }
      else if(_STRICMP(node_tag, "key-type"))
      {
         Q_ASSERT(keyN.empty());
         key_type = child_node.GetText();
         if(_STRICMP(key_type, "legacy"))
         {
            LOGD("legacy");
            secure_arg_.sbc_pub_key1_u.w_sbc_pub_key1.key_type= 0;
         }
         else if(_STRICMP(key_type, "pss"))
         {
            LOGD("pss");
            secure_arg_.sbc_pub_key1_u.w_sbc_pub_key1.key_type= 1;
         }
      }
      /*
      else
      {
      Q_ASSERT(0);
      break;
      }
      */
      child_node = child_node.GetNextSibling();
   }

   if (Str2Buf(&secure_arg_._sbc_key1_E, keyE) &&
      Str2Buf(&secure_arg_._sbc_key1_N, keyN))
   {
      secure_arg_.sbc_pubk1_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSbcPubKey2(
   const XML::Node &node)
{
   string keyE, keyN, key_type;
   string node_tag;

   XML::Node child_node = node.GetFirstChildNode();

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      if (_STRICMP(node_tag, "pub-key-e"))
      {
         Q_ASSERT(keyE.empty());
         keyE = child_node.GetText();
      }
      else if (_STRICMP(node_tag, "pub-key-n"))
      {
         Q_ASSERT(keyN.empty());
         keyN = child_node.GetText();
      }
      else if(_STRICMP(node_tag, "key-type"))
      {
         Q_ASSERT(keyN.empty());
         key_type = child_node.GetText();
         if(_STRICMP(key_type, "legacy"))
         {
            LOGD("legacy");
            secure_arg_.sbc_pub_key2_u.w_sbc_pub_key2.key_type= 0;
         }
         else if(_STRICMP(key_type, "pss"))
         {
            LOGD("pss");
            secure_arg_.sbc_pub_key2_u.w_sbc_pub_key2.key_type= 1;
         }
      }
      /*
      else
      {
      Q_ASSERT(0);
      break;
      }
      */
      child_node = child_node.GetNextSibling();
   }

   if (Str2Buf(&secure_arg_._sbc_key2_E, keyE) &&
      Str2Buf(&secure_arg_._sbc_key2_N, keyN))
   {
      secure_arg_.sbc_pubk2_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSbcPubKey3(
   const XML::Node &node)
{
   string keyE, keyN, key_type;
   string node_tag;

   XML::Node child_node = node.GetFirstChildNode();

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      if (_STRICMP(node_tag, "pub-key-e"))
      {
         Q_ASSERT(keyE.empty());
         keyE = child_node.GetText();
      }
      else if (_STRICMP(node_tag, "pub-key-n"))
      {
         Q_ASSERT(keyN.empty());
         keyN = child_node.GetText();
      }
      else if(_STRICMP(node_tag, "key-type"))
      {
         Q_ASSERT(keyN.empty());
         key_type = child_node.GetText();
         if(_STRICMP(key_type, "legacy"))
         {
            LOGD("legacy");
            secure_arg_.sbc_pub_key3_u.w_sbc_pub_key3.key_type= 0;
         }
         else if(_STRICMP(key_type, "pss"))
         {
            LOGD("pss");
            secure_arg_.sbc_pub_key3_u.w_sbc_pub_key3.key_type= 1;
         }
      }
      /*
      else
      {
      Q_ASSERT(0);
      break;
      }
      */
      child_node = child_node.GetNextSibling();
   }

   if (Str2Buf(&secure_arg_._sbc_key3_E, keyE) &&
      Str2Buf(&secure_arg_._sbc_key3_N, keyN))
   {
      secure_arg_.sbc_pubk3_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadAcKey(
   const XML::Node &node)
{
   if (Str2Buf(&secure_arg_.ac_key, node.GetText()))
   {
      secure_arg_.ac_key_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadCommonLock(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("com_ctrl_lock");
   lock_arg_.common_ctrl_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("usb_id_lock");
   lock_arg_.usb_id_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("spare_lock");
   lock_arg_.spare_lock = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadSecureLock(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("sec_attr_lock");
   lock_arg_.sec_ctrl_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("ackey_lock");
   lock_arg_.ackey_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sbc_pubk_hash_lock");
   lock_arg_.sbc_pubk_hash_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sec_msc_lock");
   lock_arg_.sec_msc_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("custk_lock");
   lock_arg_.custk_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("oem_lock");
   lock_arg_.oem_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sbc_pubk_hash1_lock");
   lock_arg_.sbc_pubk_hash1_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sbc_pubk_hash2_lock");
   lock_arg_.sbc_pubk_hash2_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sbc_pubk_hash3_lock");
   lock_arg_.sbc_pubk_hash3_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("cust_crypt_data_lock");
   lock_arg_.cust_crypt_data_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("cust_data_lock");
   lock_arg_.cust_data_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("sbc_pubk_ctrl_lock");
   lock_arg_.sbc_pubk_ctrl_lock = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadMHWRes(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("DIS_SEC_CAP");
   extra_arg_.items[SEC_CAP].key = SEC_CAP;
   extra_arg_.items[SEC_CAP].type = T_BOOLEAN;
   extra_arg_.items[SEC_CAP].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("PROD_EN");
   extra_arg_.items[PROD_EN].key = PROD_EN;
   extra_arg_.items[PROD_EN].type = T_BOOLEAN;
   extra_arg_.items[PROD_EN].data.enable= Bool2EfuseOpt(text);
}

void EfuseSetting::LoadCLock(
   const XML::Node &node)
{
   string text;

   text = node.GetAttribute("c_ctrl3_lock");
   extra_arg_.items[C_CTRL3_LOCK].key = C_CTRL3_LOCK;
   extra_arg_.items[C_CTRL3_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_CTRL3_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_ctrl2_lock");
   extra_arg_.items[C_CTRL2_LOCK].key = C_CTRL2_LOCK;
   extra_arg_.items[C_CTRL2_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_CTRL2_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_ctrl1_lock");
   extra_arg_.items[C_CTRL1_LOCK].key = C_CTRL1_LOCK;
   extra_arg_.items[C_CTRL1_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_CTRL1_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_ctrl0_lock");
   extra_arg_.items[C_CTRL0_LOCK].key = C_CTRL0_LOCK;
   extra_arg_.items[C_CTRL0_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_CTRL0_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data0_1_lock");
   extra_arg_.items[C_DATA0_1_LOCK].key = C_DATA0_1_LOCK;
   extra_arg_.items[C_DATA0_1_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA0_1_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data0_lock");
   extra_arg_.items[C_DATA0_LOCK].key = C_DATA0_LOCK;
   extra_arg_.items[C_DATA0_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA0_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data1_lock");
   extra_arg_.items[C_DATA1_LOCK].key = C_DATA1_LOCK;
   extra_arg_.items[C_DATA1_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA1_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data2_lock");
   extra_arg_.items[C_DATA2_LOCK].key = C_DATA2_LOCK;
   extra_arg_.items[C_DATA2_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA2_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data3_lock");
   extra_arg_.items[C_DATA3_LOCK].key = C_DATA3_LOCK;
   extra_arg_.items[C_DATA3_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA3_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data4_lock");
   extra_arg_.items[C_DATA4_LOCK].key = C_DATA4_LOCK;
   extra_arg_.items[C_DATA4_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA4_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_data5_lock");
   extra_arg_.items[C_DATA5_LOCK].key = C_DATA5_LOCK;
   extra_arg_.items[C_DATA5_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_DATA5_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_pid_lock");
   extra_arg_.items[C_3P_PID_LOCK].key = C_3P_PID_LOCK;
   extra_arg_.items[C_3P_PID_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_PID_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_eppk_lock");
   extra_arg_.items[C_3P_EPPK_LOCK].key = C_3P_EPPK_LOCK;
   extra_arg_.items[C_3P_EPPK_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_EPPK_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_cpd_lock");
   extra_arg_.items[C_3P_CPD_LOCK].key = C_3P_CPD_LOCK;
   extra_arg_.items[C_3P_CPD_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_CPD_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_oid_lock");
   extra_arg_.items[C_3P_OID_LOCK].key = C_3P_OID_LOCK;
   extra_arg_.items[C_3P_OID_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_OID_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_sv0_key_lock");
   extra_arg_.items[C_3P_SV0_KEY_LOCK].key = C_3P_SV0_KEY_LOCK;
   extra_arg_.items[C_3P_SV0_KEY_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_SV0_KEY_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_sv1_key_lock");
   extra_arg_.items[C_3P_SV1_KEY_LOCK].key = C_3P_SV1_KEY_LOCK;
   extra_arg_.items[C_3P_SV1_KEY_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_SV1_KEY_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_jtag_unlock_key_lock");
   extra_arg_.items[C_3P_JTAG_UNLOCK_KEY_LOCK].key = C_3P_JTAG_UNLOCK_KEY_LOCK;
   extra_arg_.items[C_3P_JTAG_UNLOCK_KEY_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_JTAG_UNLOCK_KEY_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock0");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK0].key = C_3P_RSA_PUBK_LOCK0;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK0].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK0].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock1");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK1].key = C_3P_RSA_PUBK_LOCK1;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK1].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK1].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock2");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK2].key = C_3P_RSA_PUBK_LOCK2;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK2].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK2].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock3");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK3].key = C_3P_RSA_PUBK_LOCK3;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK3].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK3].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock4");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK4].key = C_3P_RSA_PUBK_LOCK4;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK4].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK4].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock5");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK5].key = C_3P_RSA_PUBK_LOCK5;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK5].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK5].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock6");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK6].key = C_3P_RSA_PUBK_LOCK6;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK6].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK6].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock7");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK7].key = C_3P_RSA_PUBK_LOCK7;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK7].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK7].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_rsa_pubk_lock_lock");
   extra_arg_.items[C_3P_RSA_PUBK_LOCK_LOCK].key = C_3P_RSA_PUBK_LOCK_LOCK;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_RSA_PUBK_LOCK_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_3p_data_lock_lock");
   extra_arg_.items[C_3P_DATA_LOCK_LOCK].key = C_3P_DATA_LOCK_LOCK;
   extra_arg_.items[C_3P_DATA_LOCK_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_3P_DATA_LOCK_LOCK].data.enable = Bool2EfuseOpt(text);
   
   text = node.GetAttribute("c_sw_ver_lock");
   extra_arg_.items[C_SW_VER_LOCK].key = C_SW_VER_LOCK;
   extra_arg_.items[C_SW_VER_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_SW_VER_LOCK].data.enable = Bool2EfuseOpt(text);

   text = node.GetAttribute("c_sw_ver_lock_lock");
   extra_arg_.items[C_SW_VER_LOCK_LOCK].key = C_SW_VER_LOCK_LOCK;
   extra_arg_.items[C_SW_VER_LOCK_LOCK].type = T_BOOLEAN;
   extra_arg_.items[C_SW_VER_LOCK_LOCK].data.enable = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadCtrlKey(
   const XML::Node &node)
{
   LoadUnitValue(CTRL1, node);
}

void EfuseSetting::LoadSecCtrl1Key(
   const XML::Node &node)
{
   LoadUnitValue(SEC_CTRL1, node);
}

void EfuseSetting::LoadCCtrl0Key(
   const XML::Node &node)
{
   LoadUnitValue(C_CTRL_0, node);
}

void EfuseSetting::LoadCustomerCtrl0Key(const XML::Node &node)
{
   string text;
   unsigned int value = 0;

   text = node.GetAttribute("c2k_sbc_en");
   if(Bool2EfuseOpt(text) == EFUSE_ENABLE)
      value += 0x01;

   text = node.GetAttribute("md1_sbc_en");
   if(Bool2EfuseOpt(text) == EFUSE_ENABLE)
      value += 0x01 << 1;

   extra_arg_.items[C_CTRL_0].key = C_CTRL_0;
   extra_arg_.items[C_CTRL_0].type = T_INT;
   extra_arg_.items[C_CTRL_0].data.iPair.value = value;

   if(value != 0)
      extra_arg_.items[C_CTRL_0].data.iPair.blow = EFUSE_ENABLE;
}

void EfuseSetting::LoadCCtrl1Key(
   const XML::Node &node)
{
   LoadUnitValue(C_CTRL_1, node);
}

void EfuseSetting::LoadCCtrl2Key(
   const XML::Node &node)
{
   LoadUnitValue(C_CTRL_2, node);
}

void EfuseSetting::LoadCCtrl3Key(
   const XML::Node &node)
{
   LoadUnitValue(C_CTRL_3, node);
}

void EfuseSetting::LoadCData0(
   const XML::Node &node)
{
   LoadKey(C_DATA_0, node);
}

void EfuseSetting::LoadCData1(
   const XML::Node &node)
{
   LoadKey(C_DATA_1, node);
}

void EfuseSetting::LoadCData2(
   const XML::Node &node)
{
   LoadKey(C_DATA_2, node);
}

void EfuseSetting::LoadCData3(
   const XML::Node &node)
{
   LoadKey(C_DATA_3, node);
}

void EfuseSetting::LoadCData4(
   const XML::Node &node)
{
   LoadKey(C_DATA_4, node);
}

void EfuseSetting::LoadCData5(
   const XML::Node &node)
{
   LoadKey(C_DATA_5, node);
}

void EfuseSetting::LoadUnitValue(EFUSE_KEY key,
   const XML::Node &node)
{
   std::string text;

   text = node.GetText();

   extra_arg_.items[key].key = key;
   extra_arg_.items[key].type = T_INT;
   extra_arg_.items[key].data.iPair.value = strtoul(text.c_str(), NULL, 16);

   if(extra_arg_.items[key].data.iPair.value != 0)
      extra_arg_.items[key].data.iPair.blow = EFUSE_ENABLE;
}

void EfuseSetting::LoadKey(EFUSE_KEY key,
   const XML::Node &node)
{
   std::string text;

   text = node.GetText();

   extra_arg_.items[key].key = key;
   extra_arg_.items[key].type = T_BUF;

   if(Str2Buf(&extra_arg_.items[key].data.key_pair.key, text))
   {
      extra_arg_.items[key].data.key_pair.key_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadReadBack(
   const XML::Node &node)
{
   output_dir_ = node.GetAttribute("dir");
   output_file_ = node.GetAttribute("name");
   time_prefix_ = Text2Bool(node.GetAttribute("time-prefix"));
}

void EfuseSetting::LoadSTBLock(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("stb_key_g7_lock");
   stb_key_arg_.stb_lock.stb_key_g7_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g6_lock");
   stb_key_arg_.stb_lock.stb_key_g6_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g5_lock");
   stb_key_arg_.stb_lock.stb_key_g5_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g4_lock");
   stb_key_arg_.stb_lock.stb_key_g4_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g3_lock");
   stb_key_arg_.stb_lock.stb_key_g3_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g2_lock");
   stb_key_arg_.stb_lock.stb_key_g2_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g1_lock");
   stb_key_arg_.stb_lock.stb_key_g1_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_g0_lock");
   stb_key_arg_.stb_lock.stb_key_g0_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_operatorid_lock");
   stb_key_arg_.stb_lock.stb_key_operatorid_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_chipid_lock");
   stb_key_arg_.stb_lock.stb_key_chipid_lock = Bool2EfuseOpt(text);

   text = node.GetAttribute("stb_key_sn_lock");
   stb_key_arg_.stb_lock.stb_key_sn_lock = Bool2EfuseOpt(text);
}

void EfuseSetting::LoadSTBID(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("stb_key_chipid");
   stb_key_arg_.stb_key_chipID = strtoul(text.c_str(), NULL, 16);

   if (stb_key_arg_.stb_key_chipID  != 0)
   {
      stb_key_arg_.stb_key_chipid_blow = EFUSE_ENABLE;
   }

   text = node.GetAttribute("stb_key_operatorid");
   stb_key_arg_.stb_key_operatorid = strtoul(text.c_str(), NULL, 16);

   if(stb_key_arg_.stb_key_operatorid != 0)
   {
      stb_key_arg_.stb_key_operatorid_blow = EFUSE_ENABLE;
   }
}

void EfuseSetting::LoadSTBKey(const XML::Node &node)
{
   string key;
   string node_tag;

   STB_KEY_PARAM param;

   XML::Node child_node = node.GetFirstChildNode();
   int i = 0;

   while (!child_node.IsEmpty())
   {
      node_tag = child_node.GetName();

      const U32 length = node_tag.length();

      if (length > 0 )
      {
         param.key_name = new char[length+1];
         strcpy(param.key_name, node_tag.c_str());
      }

      key = child_node.GetText();

      if(Str2Buf(&param.stb_key, key))
         param.key_blow = EFUSE_ENABLE;

      stb_key_arg_.stb_blow_keys[i++] = param;

      child_node = child_node.GetNextSibling();
   }
}

void EfuseSetting::LoadSecMsc(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("md1_sbc_en");
   secure_arg_.md1_sbc_en = Bool2EfuseOpt(text);

   text = node.GetAttribute("c2k_sbc_en");
   secure_arg_.c2k_sbc_en = Bool2EfuseOpt(text);
}
void EfuseSetting::LoadCmFlag(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("Enable_kcust");
   secure_arg_.dxcc_kcst_en = Bool2EfuseOpt(text);
}
void EfuseSetting::LoadCustk(
   const XML::Node &node)
{
   LoadKey(CUSTK, node);
}
void EfuseSetting::LoadCustCryptData(
   const XML::Node &node)
{
   LoadKey(CUST_CRYPT_DATA, node);
}
void EfuseSetting::LoadCustData(
   const XML::Node &node)
{
   LoadKey(CUST_DATA, node);
}
void EfuseSetting::LoadSWVer0(
   const XML::Node &node)
{
   LoadKey(C_SW_VER0, node);
}

void EfuseSetting::LoadSWVer1(
   const XML::Node &node)
{
   LoadKey(C_SW_VER1, node);
}

void EfuseSetting::LoadSWVer2(
   const XML::Node &node)
{
   LoadKey(C_SW_VER2, node);
}

void EfuseSetting::LoadSWVer3(
   const XML::Node &node)
{
   LoadKey(C_SW_VER3, node);
}

void EfuseSetting::Load3PData(const XML::Node &node)
{
   string node_tag;
   XML::Node child_node = node.GetFirstChildNode();

   while(!child_node.IsEmpty())
   {
      node_tag=child_node.GetName();

      if(_STRICMP(node_tag, "c_3p_pid"))
      {
         LoadPid(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_eppk"))
      {
         LoadEppk(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_cpd"))
      {
         LoadCpd(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_oid"))
      {
         LoadOid(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_sv0_key"))
      {
         LoadSV0Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_sv1_key"))
      {
         LoadSV1Key(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_jtag_unlock_key"))
      {
         LoadJtagUnlockKey(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk0"))
      {
         LoadRSAPubk0(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk1"))
      {
         LoadRSAPubk1(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk2"))
      {
         LoadRSAPubk2(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk3"))
      {
         LoadRSAPubk3(child_node);
      }      
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk4"))
      {
         LoadRSAPubk4(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk5"))
      {
         LoadRSAPubk5(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk6"))
      {
         LoadRSAPubk6(child_node);
      }
      else if(_STRICMP(node_tag, "c_3p_rsa_pubk7"))
      {
         LoadRSAPubk7(child_node);
      }

      child_node = child_node.GetNextSibling();
   }

}
void EfuseSetting::LoadPid(
   const XML::Node &node)
{
   LoadKey(C_3P_PID, node);
}
void EfuseSetting::LoadEppk(
   const XML::Node &node)
{
   LoadKey(C_3P_EPPK, node);
}
void EfuseSetting::LoadCpd(
   const XML::Node &node)
{
   LoadKey(C_3P_CPD, node);
}
void EfuseSetting::LoadOid(
   const XML::Node &node)
{
   LoadKey(C_3P_OID, node);
}
void EfuseSetting::LoadSV0Key(
   const XML::Node &node)
{
   LoadKey(C_3P_SV0_KEY, node);
}
void EfuseSetting::LoadSV1Key(
   const XML::Node &node)
{
   LoadKey(C_3P_SV1_KEY, node);
}
void EfuseSetting::LoadJtagUnlockKey(
   const XML::Node &node)
{
   LoadKey(C_3P_JTAG_UNLOCK_KEY, node);
}
void EfuseSetting::LoadRSAPubk0(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK0, node);
}
void EfuseSetting::LoadRSAPubk1(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK1, node);
}
void EfuseSetting::LoadRSAPubk2(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK2, node);
}
void EfuseSetting::LoadRSAPubk3(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK3, node);
}
void EfuseSetting::LoadRSAPubk4(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK4, node);
}

void EfuseSetting::LoadRSAPubk5(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK5, node);
}

void EfuseSetting::LoadRSAPubk6(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK6, node);
}

void EfuseSetting::LoadRSAPubk7(
   const XML::Node &node)
{
   LoadKey(C_RSA_PUBK7, node);
}

//to align with self_blow tool
void EfuseSetting::LoadBypassItems(const XML::Node &node)
{
   string text;

   text = node.GetAttribute("bypass_c_ctrl0");
   bypass_rule_.bypass_c_ctrl0 = Text2Bool(text);
   if(bypass_rule_.bypass_c_ctrl0)
   {
      BypassKeys(C_CTRL_0);
   }

   text = node.GetAttribute("bypass_c_ctrl1");
   bypass_rule_.bypass_c_ctrl1 = Text2Bool(text);
   if(bypass_rule_.bypass_c_ctrl1)
   {
      BypassKeys(C_CTRL_1);
   }

   text = node.GetAttribute("bypass_c_data0");
   bypass_rule_.bypass_c_data0 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data0)
   {
      BypassKeys(C_DATA_0);
   }

   text = node.GetAttribute("bypass_c_data1");
   bypass_rule_.bypass_c_data1 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data1)
   {
      BypassKeys(C_DATA_1);
   }

   text = node.GetAttribute("bypass_c_data2");
   bypass_rule_.bypass_c_data2 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data2)
   {
      BypassKeys(C_DATA_2);
   }

   text = node.GetAttribute("bypass_c_data3");
   bypass_rule_.bypass_c_data3 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data3)
   {
      BypassKeys(C_DATA_3);
   }

   text = node.GetAttribute("bypass_c_data4");
   bypass_rule_.bypass_c_data4 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data4)
   {
      BypassKeys(C_DATA_4);
   }

   text = node.GetAttribute("bypass_c_data5");
   bypass_rule_.bypass_c_data5 = Text2Bool(text);
   if(bypass_rule_.bypass_c_data5)
   {
      BypassKeys(C_DATA_5);
   }
}

void EfuseSetting::BypassKeys(EFUSE_KEY key)
{
   memset(&extra_arg_.items[key], 0, sizeof(extra_arg_.items[key]));
}

string EfuseSetting::ReadbackFile() const
{
   string rb_file;

   if (!output_file_.empty())
   {
      rb_file = output_dir_;

      if (!rb_file.empty() &&
         *(rb_file.rbegin())!= C_SEP_CHR)
      {
         rb_file += C_SEP_STR;
      }

      if (time_prefix_)
      {
         time_t _time;
         time(&_time);

         char prefix[20];
         struct tm *now = localtime(&_time);

         sprintf(prefix, "[%04d%02d%02d%02d%02d%02d]",
            now->tm_year+1900, now->tm_mon+1, now->tm_mday,
            now->tm_hour, now->tm_min, now->tm_sec);

         rb_file += prefix;
      }

      rb_file += output_file_;
   }
   return rb_file;
}

}
