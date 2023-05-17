#include "SerializeBuffer.h"
#include <stdio.h>

SerializeBuffer::SerializeBuffer()
: _iBufferSize(eBUFFER_DEFAULT), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializeBuffer::SerializeBuffer(int iBufferSize)
    : _iBufferSize(iBufferSize), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializeBuffer::~SerializeBuffer()
{
    delete[] _chpBuffer;
}

void SerializeBuffer::Clear(void)
{
    _iReadPos = 0;
    _iWritePos = 0;
}

int SerializeBuffer::Resize(int iBufferSize)
{
    if (iBufferSize > eBUFFER_MAX)
    {
        printf("Requested Resize Size is too Big!! %d -> %d\n", _iBufferSize, iBufferSize);
        return -1;
    }

    char* chpNewBuffer = new char[iBufferSize];
    memcpy_s(chpNewBuffer, iBufferSize, _chpBuffer, _iBufferSize);
    delete[] _chpBuffer;

    _chpBuffer = chpNewBuffer;
    printf("Resize!! %d -> %d\n", _iBufferSize, iBufferSize);
    _iBufferSize = iBufferSize;

    return _iBufferSize;
}

int	SerializeBuffer::GetDataSize(void)
{ 
    _iDataSize = _iWritePos - _iReadPos;
    return _iDataSize; 
}

int SerializeBuffer::MoveWritePos(int iSize)
{
    if (iSize < 0) return -1;
    _iWritePos += iSize;
    return iSize;
}

int SerializeBuffer::MoveReadPos(int iSize)
{
    if (iSize < 0) return -1;
    _iReadPos += iSize;
    return iSize;
}

SerializeBuffer& SerializeBuffer::operator=(SerializeBuffer& clSrSerializeBuffer)
{
    *this = clSrSerializeBuffer;
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(unsigned char byValue)
{
    if (_iBufferSize - _iWritePos < sizeof(byValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos], 
            _iBufferSize - _iWritePos,
             &byValue, sizeof(byValue));

    _iWritePos += sizeof(byValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(char chValue)
{
    if (_iBufferSize - _iWritePos < sizeof(chValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos], 
            _iBufferSize - _iWritePos,
            &chValue, sizeof(chValue));

    _iWritePos += sizeof(chValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(short shValue)
{
    if (_iBufferSize - _iWritePos < sizeof(shValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos], 
            _iBufferSize - _iWritePos,
            &shValue, sizeof(shValue));

    _iWritePos += sizeof(shValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(unsigned short wValue)
{
    if (_iBufferSize - _iWritePos < sizeof(wValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &wValue, sizeof(wValue));

    _iWritePos += sizeof(wValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(int iValue)
{
    if (_iBufferSize - _iWritePos < sizeof(iValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &iValue, sizeof(iValue));

    _iWritePos += sizeof(iValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(long lValue)
{
    if (_iBufferSize - _iWritePos < sizeof(lValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &lValue, sizeof(lValue));

    _iWritePos += sizeof(lValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(float fValue)
{
    if (_iBufferSize - _iWritePos < sizeof(fValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &fValue, sizeof(fValue));

    _iWritePos += sizeof(fValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(__int64 iValue)
{
    if (_iBufferSize - _iWritePos < sizeof(iValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &iValue, sizeof(iValue));

    _iWritePos += sizeof(iValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator<<(double dValue)
{
    if (_iBufferSize - _iWritePos < sizeof(dValue))
        Resize(_iBufferSize * 1.5f);

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &dValue, sizeof(dValue));

    _iWritePos += sizeof(dValue);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(BYTE& byValue)
{
    if (_iWritePos - _iReadPos < sizeof(BYTE))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(BYTE));
        return *this;
    }

    memcpy_s(&byValue, sizeof(BYTE),
        &_chpBuffer[_iReadPos], sizeof(BYTE));

    _iReadPos += sizeof(BYTE);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(char& chValue)
{
    if (_iWritePos - _iReadPos < sizeof(char))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(char));
        return *this;
    }

    memcpy_s(&chValue, sizeof(char),
        &_chpBuffer[_iReadPos], sizeof(char));

    _iReadPos += sizeof(char);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(short& shValue)
{
    if (_iWritePos - _iReadPos < sizeof(short))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(short));
        return *this;
    }

    memcpy_s(&shValue, sizeof(short),
        &_chpBuffer[_iReadPos], sizeof(short));

    _iReadPos += sizeof(short);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(WORD& wValue)
{
    if (_iWritePos - _iReadPos < sizeof(WORD))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(WORD));
        return *this;
    }

    memcpy_s(&wValue, sizeof(WORD),
        &_chpBuffer[_iReadPos], sizeof(WORD));

    _iReadPos += sizeof(WORD);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(int& iValue)
{
    if (_iWritePos - _iReadPos < sizeof(int))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(int));
        return *this;
    }

    memcpy_s(&iValue, sizeof(int),
        &_chpBuffer[_iReadPos], sizeof(int));

    _iReadPos += sizeof(int);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(DWORD& dwValue)
{
    if (_iWritePos - _iReadPos < sizeof(DWORD))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(DWORD));
        return *this;
    }

    memcpy_s(&dwValue, sizeof(DWORD),
        &_chpBuffer[_iReadPos], sizeof(DWORD));

    _iReadPos += sizeof(DWORD);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(float& fValue)
{
    if (_iWritePos - _iReadPos < sizeof(float))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(float));
        return *this;
    }

    memcpy_s(&fValue, sizeof(float),
        &_chpBuffer[_iReadPos], sizeof(float));

    _iReadPos += sizeof(float);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(__int64& iValue)
{
    if (_iWritePos - _iReadPos < sizeof(__int64))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(__int64));
        return *this;
    }

    memcpy_s(&iValue, sizeof(__int64),
        &_chpBuffer[_iReadPos], sizeof(__int64));

    _iReadPos += sizeof(__int64);
    return *this;
}

SerializeBuffer& SerializeBuffer::operator>>(double& dValue)
{
    if (_iWritePos - _iReadPos < sizeof(double))
    {
        printf("Used Size(%d) is small than Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(double));
        return *this;
    }

    memcpy_s(&dValue, sizeof(double),
        &_chpBuffer[_iReadPos], sizeof(double));

    _iReadPos += sizeof(double);
    return *this;
}

int SerializeBuffer::GetData(char* chpDest, int iSize)
{
    if (_iWritePos - _iReadPos < iSize)
    {
        printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iWritePos - _iReadPos, iSize);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iReadPos], iSize);

    _iReadPos += iSize;
    return iSize;
}

int SerializeBuffer::PutData(char* chpSrc, int iSrcSize)
{
    if (_iBufferSize - _iWritePos < iSrcSize)
        Resize(_iBufferSize + (iSrcSize * 2));

    memcpy_s(&_chpBuffer[_iReadPos],
            _iBufferSize - _iWritePos,
            chpSrc, iSrcSize);

    _iWritePos += iSrcSize;
    return iSrcSize;
}
