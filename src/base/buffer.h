/***********************************************************************/
/* 名称:缓存区														   */
/* 说明:可重复读写的缓存			                                   */
/* 创建时间:2021/02/20												   */
/* Email:umichan0621@gmail.com		                                   */
/* Reference:https://github.com/chenshuo/muduo                         */
/***********************************************************************/
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
#pragma once
#include <atomic>
#include <vector>
#include <string>
#include <algorithm>
#include <assert.h>

namespace base
{
    class Buffer
    {
        static const size_t kPrepend = 8;
        static const size_t kInitialSize = 4096;
    public:
        explicit Buffer(size_t initialSize = kInitialSize);
    public://读
        int32_t readable_size() const;      //可读长度
        const char* begin_read() const;     //获取可读Buffer
        char* begin_read();
        void retrieve(int32_t Len);         //告知Buffer已经取走长度为Len的可读缓存
        void retrieve_until(const char* pEnd);
        void retrieve_all();
        std::string retrieve_string(int32_t Len);
        void read_over();                   //告知Buffer当前线程读取完毕可以销毁
        uint16_t reader_count();            //返回当前正在读取Buffer的数量
    public://写
        int32_t writable_size() const;      //可写长度
        void unwrite(int32_t Len);          //取消写入，返还长度为Len的内存
        void append(const std::string& strBuf);
        void append(const char* pBuf, int32_t Len);
    private://写
        void write(int32_t Len);            //告知Buffer已经写入长度为Len的缓存     
        char* begin_write();
        void make_space(int32_t Len);       //为Buffer开辟空间确保能够写入
    private:
        char* begin();                      //Buffer起始位置
        const char* begin() const;
        int32_t prependable_size() const;   //前面预留出来的字节数
    private:
        std::vector<char>       m_Buffer;
        int32_t                 m_ReadIndex;
        int32_t                 m_WriteIndex;
        std::atomic<uint16_t>   m_ReadingCount; //当前正在读取Buffer的数量
    };
}