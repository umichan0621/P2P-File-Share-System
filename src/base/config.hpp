/***********************************************************************/
/* 名称:配置														   */
/* 说明:设定和获取程序内的固定参数和可调节参数						   */
/* 创建时间:2021/11/11												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <atomic>
#include <string>
#include <base/singleton.hpp>
using std::string;
//测试用
#ifndef _DEBUG
constexpr int TEST_PORT = 2345;
constexpr int TEST_PORT6 = 6789;
#else
//constexpr int TEST_PORT = 32542;//NAT C
constexpr int TEST_PORT = 42543;//NAT B+
constexpr int TEST_PORT6 = 3424;
#endif



//测试用

enum FILE_STATUS
{
	STATUS_NULL = 0,
	STATUS_DOWNLOAD,	//正在下载的文件
	STATUS_PAUSE,		//暂停下载的任务
	STATUS_COMPLETE,	//下载完成的文件，同时自动作为分享中的文件存在
	STATUS_SHARE,		//分享中的文件
	STATUS_ONLINE,		//本地数据库中记录且本地有的文件
	STATUS_OFFLINE,		//本地数据库中记录但本地没有的文件
	STATUS_FOLDER,		//数据库逻辑文件的文件夹
	STATUS_FOLDER_SHARE //数据库逻辑文件分享中的文件夹
};

//允许连接数 = 65534(1-65534)
constexpr uint16_t	MAX_CONNECTION_NUM = 1024;
constexpr int32_t	MIN_MTU = 1400;
constexpr int32_t	DEFAULT_MTU = 1400;
constexpr int32_t	ADDR_LEN_IPV6 = 28;				//IPv6 Sockaddr数据结构长度
constexpr uint32_t	HEARTBEAT_CLOCK = 10*1000 ;		//心跳包频率
constexpr uint32_t	PING_CLOCK = 500;				//ping的频率
constexpr uint32_t	PING_TIMEOUT_COUNT = 10;		//ping的次数

constexpr uint64_t	PROGRESS_REFRESH_FREQUENCY = 1000;
constexpr uint32_t	DOWNLOAD_RATE = 10;				//30MB/s
constexpr uint32_t	PING_RTT = 200;					//连接时ping和connect时发送的频率
constexpr uint16_t	HEARTBEAT_TIMEOUT_COUNT = 3;
constexpr uint16_t	CONNECT_TIMEOUT_COUNT = 6;		//连接无NAT设备时超时的次数
constexpr uint16_t	NAT_PROBE_TIMEOUT_COUNT = 10;
//文件各个组成部分的大小
constexpr uint16_t		FULL_BLOCK_SIZE		= 0x00000200;		//块   = 512B = 2^9B
constexpr uint16_t		FULL_FRAGMENT_SIZE	= 0x00008000;		//分片 = 32KB = 2^15B
constexpr uint32_t		FULL_SEGMENT_SIZE	= 0x00200000;		//段   = 2MB  = 2^21B

namespace base
{
	class Config
	{
	public:
		void set_port(uint16_t Port, uint16_t Port6) {
			m_Port = Port;
			m_Port6 = Port6;
		}
		void set_max_connection_num(uint16_t MaxConnectionNum) { 
			m_MaxConnectionNum = MaxConnectionNum; }
		void set_max_upload(uint64_t MaxUpLoad) { m_MaxUpLoad = MaxUpLoad; }
		void set_max_download(uint64_t MaxDownLoad) { m_MaxDownLoad = MaxDownLoad; }
		void set_path_download(std::string DownloadPath) {m_DownloadPath = DownloadPath;}
		void set_path_share(std::string SharePath) { m_SharePath = SharePath; }
	public:
		void port(uint16_t& Port, uint16_t& Port6) { 
			Port = m_Port;
			Port6 = m_Port6;
		};
		uint16_t max_connection_num() { return m_MaxConnectionNum; }
		uint64_t max_upload() { return m_MaxUpLoad; }
		uint64_t max_download() { return m_MaxDownLoad; }
		const string& path_download() {return m_DownloadPath;}
		const string& path_share() { return m_SharePath; }
	private:
		uint16_t				m_Port = 0;
		uint16_t				m_Port6 = 0;
		uint16_t				m_MaxConnectionNum = 0;
		std::atomic<uint64_t>	m_MaxUpLoad;
		std::atomic<uint64_t>	m_MaxDownLoad;
		string					m_DownloadPath;
		string					m_SharePath;
	};
}
#define g_pConfig base::Singleton<base::Config>::get_instance()