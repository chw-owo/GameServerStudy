#include "Typedef.h"
#include "SerializeBuffer.h"

#include <windows.h>
#include <stdio.h>

SerializeBuffer& operator << (SerializeBuffer& out, int8 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, int16 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, int32 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, int64 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, uint8 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, uint16 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, uint32 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator << (SerializeBuffer& out, uint64 iValue)
{
    if (out._iBufferSize - out._iWritePos < sizeof(iValue))
        out.Resize(out._iBufferSize * 1.5f);

    memcpy_s(&out._chpBuffer[out._iWritePos],
        out._iBufferSize - out._iWritePos,
        &iValue, sizeof(iValue));

    out._iWritePos += sizeof(iValue);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, int8& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(int8))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(int8), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(int8),
        &out._chpBuffer[out._iReadPos], sizeof(int8));

    out._iReadPos += sizeof(int8);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, int16& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(int16))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(int16), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(int16),
        &out._chpBuffer[out._iReadPos], sizeof(int16));

    out._iReadPos += sizeof(int16);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, int32& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(int32))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(int32), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(int32),
        &out._chpBuffer[out._iReadPos], sizeof(int32));

    out._iReadPos += sizeof(int32);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, int64& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(int64))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(int64), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(int64),
        &out._chpBuffer[out._iReadPos], sizeof(int64));

    out._iReadPos += sizeof(int64);
    return out;
}


SerializeBuffer& operator >> (SerializeBuffer& out, uint8& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(uint8))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(uint8), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(uint8),
        &out._chpBuffer[out._iReadPos], sizeof(uint8));

    out._iReadPos += sizeof(uint8);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, uint16& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(uint16))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(uint16), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(uint16),
        &out._chpBuffer[out._iReadPos], sizeof(uint16));

    out._iReadPos += sizeof(uint16);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, uint32& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(uint32))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(int), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(uint32),
        &out._chpBuffer[out._iReadPos], sizeof(uint32));

    out._iReadPos += sizeof(uint32);
    return out;
}

SerializeBuffer& operator >> (SerializeBuffer& out, uint64& iValue)
{
    if (out._iWritePos - out._iReadPos < sizeof(uint64))
    {
        printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n", 
            out._iWritePos - out._iReadPos, sizeof(uint64), __func__, __LINE__);
        return out;
    }

    memcpy_s(&iValue, sizeof(uint64),
        &out._chpBuffer[out._iReadPos], sizeof(uint64));

    out._iReadPos += sizeof(uint64);
    return out;
}