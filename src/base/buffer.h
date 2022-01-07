/***********************************************************************/
/* ����:������														   */
/* ˵��:���ظ���д�Ļ���			                                   */
/* ����ʱ��:2021/02/20												   */
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
    public://��
        int32_t readable_size() const;      //�ɶ�����
        const char* begin_read() const;     //��ȡ�ɶ�Buffer
        char* begin_read();
        void retrieve(int32_t Len);         //��֪Buffer�Ѿ�ȡ�߳���ΪLen�Ŀɶ�����
        void retrieve_until(const char* pEnd);
        void retrieve_all();
        std::string retrieve_string(int32_t Len);
        void read_over();                   //��֪Buffer��ǰ�̶߳�ȡ��Ͽ�������
        uint16_t reader_count();            //���ص�ǰ���ڶ�ȡBuffer������
    public://д
        int32_t writable_size() const;      //��д����
        void unwrite(int32_t Len);          //ȡ��д�룬��������ΪLen���ڴ�
        void append(const std::string& strBuf);
        void append(const char* pBuf, int32_t Len);
    private://д
        void write(int32_t Len);            //��֪Buffer�Ѿ�д�볤��ΪLen�Ļ���     
        char* begin_write();
        void make_space(int32_t Len);       //ΪBuffer���ٿռ�ȷ���ܹ�д��
    private:
        char* begin();                      //Buffer��ʼλ��
        const char* begin() const;
        int32_t prependable_size() const;   //ǰ��Ԥ���������ֽ���
    private:
        std::vector<char>       m_Buffer;
        int32_t                 m_ReadIndex;
        int32_t                 m_WriteIndex;
        std::atomic<uint16_t>   m_ReadingCount; //��ǰ���ڶ�ȡBuffer������
    };
}