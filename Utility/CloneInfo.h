#ifndef CLONEINFO_H
#define CLONEINFO_H

#include <string>
#include "./XMLParser/XMLNode.h"

class CloneInfo
{
public:
    CloneInfo():
      part_id_(0), partition_len_(0),
      start_addr_(0), chksum_value_(0)
    {
    }

    ~CloneInfo(){
    }

    void setPartitionName(const std::string &name){
        this->partition_name_ = name;
    }

    void setPartID(unsigned int part_id){
        this->part_id_ = part_id;
    }

    void setPartitionLen(unsigned long long len){
        this->partition_len_ = len;
    }

    void setStartAddress(unsigned long long addr){
        this->start_addr_ = addr;
    }

    void setImageLocation(const std::string &location){
        this->image_path_ = location;
    }

    void setChksumValue(unsigned char chksum_value){
        chksum_value_ = chksum_value;
    }

    unsigned long long partition_len(){
        return partition_len_;
    }

    unsigned long long start_addr(){
        return start_addr_;
    }

    std::string image_path(){
        return image_path_;
    }

    std::string partition_name(){
        return partition_name_;
    }

    unsigned int part_id(){
        return part_id_;
    }

    unsigned char chksum_value(){
        return chksum_value_;
    }

    void LoadXML(const XML::Node &node);
    void SaveXML(XML::Node &node) const;

    bool verify(const std::string &dir);

private:
    std::string partition_name_;
    unsigned int part_id_;
    std::string image_path_;
    unsigned long long   partition_len_;
    unsigned long long   start_addr_;
    unsigned char chksum_value_;
};

typedef QSharedPointer<CloneInfo> CLONE_ITEM;

#endif // CLONEINFO_H
