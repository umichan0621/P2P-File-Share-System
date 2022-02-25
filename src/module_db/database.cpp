#include "database.h"
#include <base/config.hpp>
#include <base/logger/logger.h>
using namespace std;
#pragma warning(disable:4267)

namespace database
{
	DataBaseManager::DataBaseManager() :
		m_pSqliteDatabase(nullptr) ,
		m_UnusedFileSeq(0)
	{}

	DataBaseManager::~DataBaseManager()
	{
		sqlite3_close(m_pSqliteDatabase);
	}

	bool DataBaseManager::open(string strPath)
	{
		int32_t Res = sqlite3_open(strPath.c_str(), &m_pSqliteDatabase);//打开

		if (Res)
		{
			LOG_ERROR << "Can't open database:" << sqlite3_errmsg(m_pSqliteDatabase);
			return false;
		}
		create_table_file_info();
		create_table_file_info_append();
		m_UnusedFileSeq = max_file_seq();
		return true;
	}

	int32_t DataBaseManager::file_seq()
	{
		return (++m_UnusedFileSeq);
	}


	//增删
	bool DataBaseManager::insert_file_info(int32_t FileSeq, const std::string& strSHA1,
		int8_t FileType, const std::string& strFilePath, uint64_t FileSize)
	{
		string strSql = "INSERT INTO FILE_INFO(FILE_SEQ, SHA1, FILE_TYPE, FILE_PATH, \
						 FILE_SIZE, FILE_PARENT, CREATE_TIME)\
                         SELECT " + to_string(FileSeq) + ",'" + strSHA1 + "'," +
			to_string(FileType) + ",'" + strFilePath + "'," +
			to_string((int64_t)FileSize) + ",0,STRFTIME('%Y-%m-%d %H:%M','now','localtime')";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to insert data, FileSeq = " << FileSeq;
			return false;
		}
		strSql = "INSERT INTO FILE_INFO_APPEND(FILE_SEQ) SELECT " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		sqlite3_step(pStmt);
		return true;
	}

	bool DataBaseManager::insert_file_info(int32_t FileSeq, const std::string& strSHA1, 
		uint64_t FileSize,int32_t FileParent)
	{
		int8_t FileType = STATUS_OFFLINE;
		string strSql = "INSERT INTO FILE_INFO(FILE_SEQ, SHA1, FILE_TYPE, FILE_PATH, \
						 FILE_SIZE, FILE_PARENT, CREATE_TIME)\
                         SELECT " + to_string(FileSeq) + ",'" + strSHA1 + "'," +
			to_string(FileType) + ",'NULL'," +to_string((int64_t)FileSize) + 
			","+ to_string(FileParent) +",STRFTIME('%Y-%m-%d %H:%M', 'now', 'localtime')";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to insert data, FileSeq = " << FileSeq;
			return false;
		}
		strSql = "INSERT INTO FILE_INFO_APPEND(FILE_SEQ) SELECT " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		sqlite3_step(pStmt);
		return true;
	}

	bool DataBaseManager::delete_file_info(int32_t FileSeq)
	{
		string strSql = "DELETE FROM FILE_INFO WHERE FILE_SEQ = " + to_string(FileSeq);

		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to delete file info, FileSeq = " << FileSeq;
			return false;
		}

		strSql = "DELETE FROM FILE_INFO_APPEND WHERE FILE_SEQ = " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		sqlite3_step(pStmt);
		return true;
	}

	bool DataBaseManager::create_new_folder(int32_t FileSeq, int32_t FileParent,const std::string& strFileName)
	{
		int8_t FileType = STATUS_FOLDER;
		string strSql = "INSERT INTO FILE_INFO(FILE_SEQ, SHA1, FILE_TYPE, FILE_PATH, \
						 FILE_SIZE, FILE_PARENT, CREATE_TIME)\
                         SELECT " + to_string(FileSeq) + ",'NULL'," +to_string(FileType) + 
						",'NULL',0,"+ to_string(FileParent) +", STRFTIME('%Y-%m-%d %H:%M', 'now', 'localtime')";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to insert data, FileSeq = " << FileSeq;
			return false;
		}
		strSql = "INSERT INTO FILE_INFO_APPEND(FILE_SEQ, FILE_NAME) SELECT " 
			+ to_string(FileSeq)+",'"+ strFileName +"'";
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		sqlite3_step(pStmt);
		return true;
	}

		

	//增删

	//改
	bool DataBaseManager::update_file_info(int32_t FileSeq, const std::string& strFileName, const std::string& strRemark)
	{
		string strSql = "UPDATE FILE_INFO_APPEND SET \
                    FILE_NAME = '" + strFileName + "',\
					UPLOAD_DATA = 0,\
					FILE_REMARK='" + strRemark + "'\
					WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update FileType, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	bool DataBaseManager::update_file_type(int32_t FileSeq, int8_t FileType)
	{
		string strSql = "UPDATE FILE_INFO SET \
                    FILE_TYPE = " + to_string(FileType) +
			" WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update FileType, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	bool DataBaseManager::update_file_info(int32_t FileSeq, const std::string& strFilePath)
	{
		string strSql = "UPDATE FILE_INFO SET \
                    FILE_PATH = '" + strFilePath + "' " +
			"WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update FilePath, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	bool DataBaseManager::update_file_info(int32_t FileSeq, uint64_t WriteTime)
	{
		string strSql = "UPDATE FILE_INFO SET \
        WRITE_TIME = " + to_string((int64_t)WriteTime) +
			" WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update WriteTime, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	bool DataBaseManager::update_file_name(int32_t FileSeq, const std::string& strFileName)
	{
		string strSql = "UPDATE FILE_INFO_APPEND SET \
                    FILE_NAME = '" + strFileName + "'\
					WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update FileType, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	bool DataBaseManager::update_file_parent(int32_t FileSeq, int32_t FileParent)
	{
		string strSql = "UPDATE FILE_INFO SET \
                    FILE_PARENT = " + std::to_string(FileParent) + "\
					WHERE FILE_SEQ = " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_DONE != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to update File Parent, FileSeq = " << FileSeq;
			return false;
		}
		return true;
	}

	//改

	//查
	

	bool DataBaseManager::select_file_info(std::vector<int32_t>& VecFileSeq, int8_t FileType)
	{
		string strSql = "SELECT FILE_SEQ FROM FILE_INFO WHERE FILE_TYPE =  " + to_string(FileType);

		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		while (SQLITE_ROW == sqlite3_step(pStmt))
		{
			int32_t FileSeq = sqlite3_column_int(pStmt, 0);
			VecFileSeq.emplace_back(FileSeq);
		}
		return true;
	}
	
	//MyFile查询用
	bool DataBaseManager::select_file_info(int32_t FileSeq, int32_t& FileParent, uint8_t& FileType,
		uint64_t& FileSize, std::string& FileName, std::string& WriteTime)
	{
		string strSql = "SELECT FILE_PARENT, FILE_TYPE, FILE_SIZE, \
						CREATE_TIME FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileParent = sqlite3_column_int(pStmt, 0);
		FileType= sqlite3_column_int(pStmt, 1);
		FileSize = (uint64_t)sqlite3_column_int64(pStmt, 2);
		WriteTime = (const char*)sqlite3_column_text(pStmt, 3);

		strSql = "SELECT FILE_NAME FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileName = (const char*)sqlite3_column_text(pStmt, 0);

		return true;
	}
	//MyFile查询用

	//Download查询用
	bool DataBaseManager::select_file_info(int32_t FileSeq, uint8_t& FileType,
		uint64_t& FileSize, std::string& FileName)
	{
		string strSql = "SELECT FILE_TYPE, FILE_SIZE\
						 FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileType = sqlite3_column_int(pStmt, 0);
		FileSize = (uint64_t)sqlite3_column_int64(pStmt, 1);

		strSql = "SELECT FILE_NAME FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileName = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}
	//Download查询用

	//Share查询用
	bool DataBaseManager::select_file_info(int32_t FileSeq, std::string& FileName, uint64_t& UploadData,
		std::string& FileRemark, std::string& CreateTime)
	{
		string strSql = "SELECT FILE_NAME, UPLOAD_DATA, FILE_REMARK FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileName = (const char*)sqlite3_column_text(pStmt, 0);
		UploadData = (uint64_t)sqlite3_column_int64(pStmt, 1);
		FileRemark = (const char*)sqlite3_column_text(pStmt, 2);

		strSql = "SELECT CREATE_TIME FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		CreateTime = (const char*)sqlite3_column_text(pStmt, 0);
		return true;

	}
	//Share查询用

	bool DataBaseManager::select_file_info(int32_t FileSeq, std::string& strSHA1)
	{
		string strSql = "SELECT SHA1 FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		strSHA1 = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_file_info(int32_t FileSeq, std::string& FilePath, std::string& FileName, uint64_t& FileSize)
	{
		string strSql = "SELECT FILE_PATH, FILE_SIZE FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FilePath = (const char*)sqlite3_column_text(pStmt, 0);
		FileSize = (uint64_t)sqlite3_column_int64(pStmt, 1);
		strSql = "SELECT FILE_NAME FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileName = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}



	bool DataBaseManager::select_file_parent(int32_t FileSeq, int32_t& FileParent)
	{
		string strSql = "SELECT FILE_PARENT FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileParent = (uint64_t)sqlite3_column_int(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_file_path(int32_t FileSeq, std::string& FilePath)
	{
		string strSql = "SELECT FILE_PATH FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FilePath = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_file_size(int32_t FileSeq, uint64_t& FileSize)
	{
		string strSql = "SELECT FILE_SIZE FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileSize = (uint64_t)sqlite3_column_int64(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_file_name(int32_t FileSeq, std::string& FileName)
	{
		string strSql = "SELECT FILE_NAME FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		FileName = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_upload_data(int32_t FileSeq, uint64_t& UploadData)
	{
		string strSql = "SELECT UPLOAD_DATA FROM FILE_INFO_APPEND WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		UploadData = (uint64_t)sqlite3_column_int64(pStmt, 0);
		return true;
	}

	bool DataBaseManager::select_create_time(int32_t FileSeq, std::string& CreateTime)
	{
		string strSql = "SELECT CREATE_TIME FROM FILE_INFO WHERE FILE_SEQ =  " + to_string(FileSeq);
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			LOG_ERROR << "Fail to select file info, FileSeq = " << FileSeq;
			return false;
		}
		CreateTime = (const char*)sqlite3_column_text(pStmt, 0);
		return true;
	}

	int32_t DataBaseManager::select_file_seq(const std::string& strSHA1)
	{
		string strSql = "SELECT FILE_SEQ FROM FILE_INFO WHERE SHA1 = " +strSHA1;
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			return -1;
		}
		return sqlite3_column_int(pStmt, 0);
	}

	int32_t DataBaseManager::max_file_seq()
	{
		string strSql = "SELECT FILE_SEQ FROM FILE_INFO ORDER BY FILE_SEQ DESC LIMIT 1";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, strSql.c_str(), -1, &pStmt, NULL);
		if (SQLITE_ROW != sqlite3_step(pStmt))
		{
			return 1;
		}
		//sqlite3_column
		return sqlite3_column_int(pStmt, 0);
	}
	//查

	//建表  
	void DataBaseManager::create_table_file_info()
	{
		const char* pSql = "CREATE TABLE FILE_INFO(\
                    FILE_SEQ     INT PRIMARY KEY    NOT NULL,\
					SHA1		 TEXT			    NOT NULL,\
                    FILE_TYPE    INT8               NOT NULL,\
                    FILE_PATH    TEXT               NOT NULL,\
                    FILE_SIZE    BIG INT            NOT NULL,\
					FILE_PARENT	 INT				NOT NULL,\
                    CREATE_TIME  TEXT               NOT NULL);";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, pSql, -1, &pStmt, NULL);//检查SQL语句
		sqlite3_step(pStmt);//执行
	}

	void DataBaseManager::create_table_file_info_append()
	{
		const char* pSql = "CREATE TABLE FILE_INFO_APPEND(\
                    FILE_SEQ     INT PRIMARY KEY    NOT NULL,\
                    FILE_NAME    TEXT                       ,\
                    UPLOAD_DATA  BIG INT                    ,\
                    FILE_REMARK  TEXT                       );";
		sqlite3_stmt* pStmt = NULL;
		sqlite3_prepare(m_pSqliteDatabase, pSql, -1, &pStmt, NULL);//检查SQL语句
		sqlite3_step(pStmt);//执行
	}
	//建表
}