/***********************************************************************/
/* 名称:DB模块													       */
/* 说明:持久化文件共享系统的一些信息								   */
/* 创建时间:2021/11/30												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <string>
#include <vector>
#include <third/sqlite/sqlite3.h>
#include <base/singleton.hpp>
#include <atomic>
#ifdef _WIN64
#pragma comment(lib,"sqlite3.lib")
#else
#pragma comment(lib,"sqlite3_32.lib")
#endif

namespace database
{
    class DataBaseManager
    {
    public:
        DataBaseManager();
        ~DataBaseManager();
    public:
        bool open(std::string strPath);
        int32_t file_seq();
    public://增删
        bool insert_file_info(int32_t FileSeq, const std::string& strSHA1, int8_t FileType, 
            const std::string& strFilePath, uint64_t FileSize);
        bool insert_file_info(int32_t FileSeq, const std::string& strSHA1, uint64_t FileSize,
            int32_t FileParent);
        bool delete_file_info(int32_t FileSeq);

        //创建一个文件夹
        bool create_new_folder(int32_t FileSeq, int32_t FileParent, const std::string& strFileName);
    public://改
        bool update_file_info(int32_t FileSeq, const std::string& strFileName, const std::string& strRemark);
        bool update_file_info(int32_t FileSeq, const std::string& strFilePath);
        bool update_file_info(int32_t FileSeq, uint64_t WriteTime);
        bool update_file_info(int32_t FileSeq, int8_t FileType);
        bool update_file_name(int32_t FileSeq, const std::string& strFileName);

    public://查
        bool select_file_info(std::vector<int32_t>& VecFileSeq, int8_t FileType);

        //MyFile查询用
        bool select_file_info(int32_t FileSeq, int32_t& FileParent, uint8_t& FileType,
            uint64_t& FileSize, std::string& FileName, std::string& WriteTime);

        //Download查询用
        bool select_file_info(int32_t FileSeq, uint8_t& FileType, 
            uint64_t& FileSize, std::string& FileName);

        //Share查询用
        bool select_file_info(int32_t FileSeq, std::string& FileName, uint64_t& UploadData, 
            std::string& FileRemark, std::string& CreateTime);

        bool select_file_info(int32_t FileSeq, std::string& strSHA1);
        bool select_file_info(int32_t FileSeq, std::string& FilePath, std::string& FileName, uint64_t& FileSize);
        bool select_file_parent(int32_t FileSeq, int32_t& FileParent);
        bool select_file_path(int32_t FileSeq, std::string& FilePath);
        bool select_file_size(int32_t FileSeq, uint64_t& FileSize);
        bool select_file_name(int32_t FileSeq, std::string& FileName);
        bool select_upload_data(int32_t FileSeq, uint64_t& UploadData);
        bool select_create_time(int32_t FileSeq, std::string& CreateTime);
        //检查当前SHA1的文件是否存在
        int32_t select_file_seq(const std::string& strSHA1);


    private://建表
        void create_table_file_info();
        void create_table_file_info_append();

        //获取一个还没使用过的FileSeq
        int32_t max_file_seq();
    private:
        sqlite3*                m_pSqliteDatabase;
        std::atomic<uint32_t>	m_UnusedFileSeq;

    };
}

#define g_pDataBaseManager base::Singleton<database::DataBaseManager>::get_instance()