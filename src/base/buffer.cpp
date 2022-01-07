#include "buffer.h"

namespace base
{
    Buffer::Buffer(size_t initialSize) :
        m_Buffer(kPrepend + initialSize),
        m_ReadIndex(kPrepend),
        m_WriteIndex(kPrepend),
        m_ReadingCount(0)
    {
        assert(readable_size() == 0);
        assert(writable_size() == initialSize);
        assert(prependable_size() == kPrepend);
    }

    //读
    int32_t Buffer::readable_size() const
    {
        return m_WriteIndex - m_ReadIndex;
    }

    const char* Buffer::begin_read() const
    {
        return begin() + m_ReadIndex;
    }
    
    char* Buffer::begin_read()
    {
        return begin() + m_ReadIndex;
    }

    void Buffer::retrieve(int32_t Len)
    {
        assert(Len <= readable_size());
        if (Len < readable_size())
        {
            m_ReadIndex += Len;
            ++m_ReadingCount;
        }
        else
        {
            retrieve_all();
        }
    }

    void Buffer::retrieve_until(const char* pEnd)
    {
        assert(begin_read() <= pEnd);
        assert(pEnd <= begin_write());
        retrieve(pEnd - begin_read());
    }

    void Buffer::retrieve_all()
    {
        m_ReadIndex = kPrepend;
        m_WriteIndex = kPrepend;
        ++m_ReadingCount;
    }

    std::string Buffer::retrieve_string(int32_t Len)
    {
        assert(Len <= readable_size());
        std::string strRes(begin_read(), Len);
        retrieve(Len);
        return strRes;
    }

    void Buffer::read_over()
    {
        --m_ReadingCount;
    }

    uint16_t Buffer::reader_count()
    {
        return m_ReadingCount;
    }
    //读

    //写
    int32_t Buffer::writable_size() const
    {
        return m_Buffer.size() - m_WriteIndex;
    }

    void Buffer::unwrite(int32_t Len)
    {
        assert(Len <= readable_size());
        m_WriteIndex -= Len;
    }

    void Buffer::append(const std::string& strBuf)
    {
        append(strBuf.c_str(), strBuf.size());
    }

    void Buffer::append(const char* pBuf, int32_t Len)
    {
        if (writable_size() < Len)
        {
            make_space(Len);
        }
        assert(writable_size() >= Len);
        std::copy(pBuf, pBuf + Len, begin_write());
        write(Len);
    }

    char* Buffer::begin_write()
    {
        return begin() + m_WriteIndex;
    }

    void Buffer::write(int32_t Len)
    {
        assert(Len <= writable_size());
        m_WriteIndex += Len;
    }
    //写

    int32_t Buffer::prependable_size() const
    {
        return m_ReadIndex;
    }

    char* Buffer::begin()
    {
        return &*m_Buffer.begin();
    }

    const char* Buffer::begin() const
    {
        return &*m_Buffer.begin();
    }

    void Buffer::make_space(int32_t Len)
    {
        //可用空间不够，只能扩容
        if (writable_size() + prependable_size() < Len + kPrepend)
        {
            m_Buffer.resize(m_WriteIndex + Len);
        }
        //可用空间足够，向前移动
        else
        {
            // move readable data to the front, make space inside buffer
            assert(kPrepend < m_ReadIndex);
            size_t readable = readable_size();
            std::copy(begin() + m_ReadIndex, begin() + m_WriteIndex, begin() + kPrepend);
            m_ReadIndex = kPrepend;
            m_WriteIndex = m_ReadIndex + readable;
            assert(readable == readable_size());
        }
    }
}