#include "SerializePacket.h"
#include <stdio.h>

SerializePacket::SerializePacket()
    : _iBufferSize(eBUFFER_DEFAULT), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializePacket::SerializePacket(int iBufferSize)
    : _iBufferSize(iBufferSize), _iDataSize(0), _iReadPos(0), _iWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

SerializePacket::~SerializePacket()
{
    delete[] _chpBuffer;
}

void SerializePacket::Clear(void)
{
    _iReadPos = 0;
    _iWritePos = 0;
}

int SerializePacket::Resize(int iBufferSize)
{
    if (iBufferSize > eBUFFER_MAX)
    {
        ::printf("Requested Resize Size is too Big!! %d -> %d\n", _iBufferSize, iBufferSize);
        return -1;
    }

    char* chpNewBuffer = new char[iBufferSize];
    memcpy_s(chpNewBuffer, iBufferSize, _chpBuffer, _iBufferSize);
    delete[] _chpBuffer;

    _chpBuffer = chpNewBuffer;
    ::printf("Resize!! %d -> %d\n", _iBufferSize, iBufferSize);
    _iBufferSize = iBufferSize;

    return _iBufferSize;
}

int	SerializePacket::GetDataSize(void)
{
    _iDataSize = _iWritePos - _iReadPos;
    return _iDataSize;
}

int SerializePacket::MoveWritePos(int iSize)
{
    if (iSize < 0) return -1;
    _iWritePos += iSize;
    return iSize;
}

int SerializePacket::MoveReadPos(int iSize)
{
    if (iSize < 0) return -1;
    _iReadPos += iSize;
    return iSize;
}

SerializePacket& SerializePacket::operator=(SerializePacket& clSrSerializePacket)
{
    *this = clSrSerializePacket;
    return *this;
}


SerializePacket& SerializePacket::operator<<(float fValue)
{
    if (_iBufferSize - _iWritePos < sizeof(fValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &fValue, sizeof(fValue));

    _iWritePos += sizeof(fValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(double dValue)
{
    if (_iBufferSize - _iWritePos < sizeof(dValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &dValue, sizeof(dValue));

    _iWritePos += sizeof(dValue);
    return *this;
}

SerializePacket& SerializePacket::operator>>(float& fValue)
{
    if (_iWritePos - _iReadPos < sizeof(float))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(float), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&fValue, sizeof(float),
        &_chpBuffer[_iReadPos], sizeof(float));

    _iReadPos += sizeof(float);
    return *this;
}

SerializePacket& SerializePacket::operator>>(double& dValue)
{
    if (_iWritePos - _iReadPos < sizeof(double))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(double), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&dValue, sizeof(double),
        &_chpBuffer[_iReadPos], sizeof(double));

    _iReadPos += sizeof(double);
    return *this;
}

SerializePacket& SerializePacket::operator<<(unsigned char byValue)
{
    if (_iBufferSize - _iWritePos < sizeof(byValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &byValue, sizeof(byValue));

    _iWritePos += sizeof(byValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(char chValue)
{
    if (_iBufferSize - _iWritePos < sizeof(chValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &chValue, sizeof(chValue));

    _iWritePos += sizeof(chValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(short shValue)
{
    if (_iBufferSize - _iWritePos < sizeof(shValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &shValue, sizeof(shValue));

    _iWritePos += sizeof(shValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(unsigned short wValue)
{
    if (_iBufferSize - _iWritePos < sizeof(wValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &wValue, sizeof(wValue));

    _iWritePos += sizeof(wValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(int iValue)
{
    if (_iBufferSize - _iWritePos < sizeof(iValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &iValue, sizeof(iValue));

    _iWritePos += sizeof(iValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(long lValue)
{
    if (_iBufferSize - _iWritePos < sizeof(lValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &lValue, sizeof(lValue));

    _iWritePos += sizeof(lValue);
    return *this;
}

SerializePacket& SerializePacket::operator<<(__int64 iValue)
{
    if (_iBufferSize - _iWritePos < sizeof(iValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        &iValue, sizeof(iValue));

    _iWritePos += sizeof(iValue);
    return *this;
}

SerializePacket& SerializePacket::operator>>(BYTE& byValue)
{
    if (_iWritePos - _iReadPos < sizeof(BYTE))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!\n",
            _iWritePos - _iReadPos, sizeof(BYTE));
        return *this;
    }

    memcpy_s(&byValue, sizeof(BYTE),
        &_chpBuffer[_iReadPos], sizeof(BYTE));

    _iReadPos += sizeof(BYTE);
    return *this;
}

SerializePacket& SerializePacket::operator>>(char& chValue)
{
    if (_iWritePos - _iReadPos < sizeof(char))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(char), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&chValue, sizeof(char),
        &_chpBuffer[_iReadPos], sizeof(char));

    _iReadPos += sizeof(char);
    return *this;
}

SerializePacket& SerializePacket::operator>>(wchar_t& szValue)
{
    if (_iWritePos - _iReadPos < sizeof(wchar_t))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(wchar_t), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&szValue, sizeof(wchar_t),
        &_chpBuffer[_iReadPos], sizeof(wchar_t));

    _iReadPos += sizeof(wchar_t);
    return *this;
}

SerializePacket& SerializePacket::operator>>(short& shValue)
{
    if (_iWritePos - _iReadPos < sizeof(short))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(short), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&shValue, sizeof(short),
        &_chpBuffer[_iReadPos], sizeof(short));

    _iReadPos += sizeof(short);
    return *this;
}

SerializePacket& SerializePacket::operator>>(WORD& wValue)
{
    if (_iWritePos - _iReadPos < sizeof(WORD))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(WORD), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&wValue, sizeof(WORD),
        &_chpBuffer[_iReadPos], sizeof(WORD));

    _iReadPos += sizeof(WORD);
    return *this;
}

SerializePacket& SerializePacket::operator>>(int& iValue)
{
    if (_iWritePos - _iReadPos < sizeof(int))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(int), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&iValue, sizeof(int),
        &_chpBuffer[_iReadPos], sizeof(int));

    _iReadPos += sizeof(int);
    return *this;
}

SerializePacket& SerializePacket::operator>>(DWORD& dwValue)
{
    if (_iWritePos - _iReadPos < sizeof(DWORD))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(DWORD), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&dwValue, sizeof(DWORD),
        &_chpBuffer[_iReadPos], sizeof(DWORD));

    _iReadPos += sizeof(DWORD);
    return *this;
}

SerializePacket& SerializePacket::operator>>(__int64& iValue)
{
    if (_iWritePos - _iReadPos < sizeof(__int64))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iWritePos - _iReadPos, sizeof(__int64), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&iValue, sizeof(__int64),
        &_chpBuffer[_iReadPos], sizeof(__int64));

    _iReadPos += sizeof(__int64);
    return *this;
}

int SerializePacket::GetData(char* chpDest, int iSize)
{
    if (_iWritePos - _iReadPos < iSize)
    {
        ::printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iWritePos - _iReadPos, iSize);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iReadPos], iSize);

    _iReadPos += iSize;
    return iSize;
}

int SerializePacket::PeekData(char* chpDest, int iSize)
{
    if (_iWritePos - _iReadPos < iSize)
    {
        ::printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iWritePos - _iReadPos, iSize);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iReadPos], iSize);
    return iSize;
}

int SerializePacket::CheckData(int iSize)
{
    if (_iWritePos - _iReadPos < iSize)
    {
        ::printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iWritePos - _iReadPos, iSize);
        return -1;
    }

    ::printf("SerializePacket: ");
    for (int i = 0; i < iSize; i++)
    {
        ::printf("%x ", _chpBuffer[_iReadPos + i]);
    }
    ::printf("\n");
}

int SerializePacket::PutData(char* chpSrc, int iSrcSize)
{
    if (_iBufferSize - _iWritePos < iSrcSize)
        Resize(_iBufferSize + (int)(iSrcSize * 1.5f));

    memcpy_s(&_chpBuffer[_iWritePos],
        _iBufferSize - _iWritePos,
        chpSrc, iSrcSize);

    _iWritePos += iSrcSize;
    return iSrcSize;
}

