#include "CPacket.h"
#include <stdio.h>

CPacket::CPacket()
    : _iBufferSize(eBUFFER_DEFAULT), _iPayloadSize(0), _iHeaderSize(dfHEADER_LEN),
    _iPayloadReadPos(dfHEADER_LEN), _iPayloadWritePos(dfHEADER_LEN),
    _iHeaderReadPos(0), _iHeaderWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

CPacket::CPacket(int headerLen)
    : _iBufferSize(eBUFFER_DEFAULT), _iPayloadSize(0), _iHeaderSize(headerLen),
    _iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
    _iHeaderReadPos(0), _iHeaderWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

CPacket::CPacket(int headerLen, int iBufferSize)
    : _iBufferSize(iBufferSize), _iPayloadSize(0), _iHeaderSize(headerLen),
    _iPayloadReadPos(headerLen), _iPayloadWritePos(headerLen),
    _iHeaderReadPos(0), _iHeaderWritePos(0)
{
    _chpBuffer = new char[_iBufferSize];
}

CPacket::~CPacket()
{
    delete[] _chpBuffer;
}

void CPacket::Clear(void)
{
    _iPayloadReadPos = _iHeaderSize;
    _iPayloadWritePos = _iHeaderSize;
    _iHeaderReadPos = 0;
    _iHeaderWritePos = 0;
}

int CPacket::Resize(int iBufferSize)
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

int CPacket::MoveHeaderWritePos(int iSize)
{
    if (iSize < 0)
    {
        ::printf("Error! Func %s Line %d (req size - %d)\n",
            __func__, __LINE__, iSize);
        return -1;
    }

    _iHeaderWritePos += iSize;

    if (_iHeaderWritePos > _iHeaderSize)
    {
        ::printf("Error! Func %s Line %d (write pos - %d, req size - %d)\n",
            __func__, __LINE__, _iHeaderWritePos, iSize);
        return -1;
    }

    return iSize;
}

int CPacket::MoveHeaderReadPos(int iSize)
{
    if (iSize < 0)
    {
        ::printf("Error! Func %s Line %d (req size - %d)\n",
            __func__, __LINE__, iSize);
        return -1;
    }

    _iHeaderReadPos += iSize;

    if (_iHeaderReadPos > _iHeaderSize)
    {
        ::printf("Error! Func %s Line %d (read pos - %d, req size - %d)\n",
            __func__, __LINE__, _iHeaderReadPos, iSize);
        return -1;
    }

    return iSize;
}

int CPacket::MovePayloadWritePos(int iSize)
{
    if (iSize < 0)
    {
        ::printf("Error! Func %s Line %d (req size - %d)\n",
            __func__, __LINE__, iSize);
        return -1;
    }

    if (_iPayloadWritePos + iSize > _iBufferSize)
    {
        ::printf("Error! Func %s Line %d (write pos - %d, req size - %d)\n",
            __func__, __LINE__, _iPayloadWritePos, iSize);
        return -1;
    }

    _iPayloadWritePos += iSize;

    return iSize;
}

int CPacket::MovePayloadReadPos(int iSize)
{
    if (iSize < 0)
    {
        ::printf("Error! Func %s Line %d (req size - %d)\n",
            __func__, __LINE__, iSize);
        return -1;
    }

    if (_iPayloadReadPos + iSize > _iBufferSize)
    {
        ::printf("Error! Func %s Line %d (read pos - %d, req size - %d)\n",
            __func__, __LINE__, _iPayloadReadPos, iSize);
        return -1;
    }

    _iPayloadReadPos += iSize;

    return iSize;
}

CPacket& CPacket::operator=(CPacket& clSrCPacket)
{
    *this = clSrCPacket;
    return *this;
}

CPacket& CPacket::operator<<(float fValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(fValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &fValue, sizeof(fValue));

    _iPayloadWritePos += sizeof(fValue);
    return *this;
}

CPacket& CPacket::operator<<(double dValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(dValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &dValue, sizeof(dValue));

    _iPayloadWritePos += sizeof(dValue);
    return *this;
}

CPacket& CPacket::operator>>(float& fValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(float))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(float), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&fValue, sizeof(float),
        &_chpBuffer[_iPayloadReadPos], sizeof(float));

    _iPayloadReadPos += sizeof(float);
    return *this;
}

CPacket& CPacket::operator>>(double& dValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(double))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(double), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&dValue, sizeof(double),
        &_chpBuffer[_iPayloadReadPos], sizeof(double));

    _iPayloadReadPos += sizeof(double);
    return *this;
}

CPacket& CPacket::operator<<(unsigned char byValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(byValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &byValue, sizeof(byValue));

    _iPayloadWritePos += sizeof(byValue);
    return *this;
}

CPacket& CPacket::operator<<(char chValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(chValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &chValue, sizeof(chValue));

    _iPayloadWritePos += sizeof(chValue);
    return *this;
}

CPacket& CPacket::operator<<(short shValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(shValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &shValue, sizeof(shValue));

    _iPayloadWritePos += sizeof(shValue);
    return *this;
}

CPacket& CPacket::operator<<(unsigned short wValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(wValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &wValue, sizeof(wValue));

    _iPayloadWritePos += sizeof(wValue);
    return *this;
}

CPacket& CPacket::operator<<(int iValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &iValue, sizeof(iValue));

    _iPayloadWritePos += sizeof(iValue);
    return *this;
}

CPacket& CPacket::operator<<(long lValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(lValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &lValue, sizeof(lValue));

    _iPayloadWritePos += sizeof(lValue);
    return *this;
}

CPacket& CPacket::operator<<(__int64 iValue)
{
    if (_iBufferSize - _iPayloadWritePos < sizeof(iValue))
        Resize((int)(_iBufferSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        &iValue, sizeof(iValue));

    _iPayloadWritePos += sizeof(iValue);
    return *this;
}

CPacket& CPacket::operator>>(BYTE& byValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(BYTE))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(BYTE), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&byValue, sizeof(BYTE),
        &_chpBuffer[_iPayloadReadPos], sizeof(BYTE));

    _iPayloadReadPos += sizeof(BYTE);
    return *this;
}

CPacket& CPacket::operator>>(char& chValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(char))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(char), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&chValue, sizeof(char),
        &_chpBuffer[_iPayloadReadPos], sizeof(char));

    _iPayloadReadPos += sizeof(char);
    return *this;
}

CPacket& CPacket::operator>>(wchar_t& szValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(wchar_t))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(wchar_t), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&szValue, sizeof(wchar_t),
        &_chpBuffer[_iPayloadReadPos], sizeof(wchar_t));

    _iPayloadReadPos += sizeof(wchar_t);
    return *this;
}

CPacket& CPacket::operator>>(short& shValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(short))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(short), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&shValue, sizeof(short),
        &_chpBuffer[_iPayloadReadPos], sizeof(short));

    _iPayloadReadPos += sizeof(short);
    return *this;
}

CPacket& CPacket::operator>>(WORD& wValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(WORD))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(WORD), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&wValue, sizeof(WORD),
        &_chpBuffer[_iPayloadReadPos], sizeof(WORD));

    _iPayloadReadPos += sizeof(WORD);
    return *this;
}

CPacket& CPacket::operator>>(int& iValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(int))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(int), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&iValue, sizeof(int),
        &_chpBuffer[_iPayloadReadPos], sizeof(int));

    _iPayloadReadPos += sizeof(int);
    return *this;
}

CPacket& CPacket::operator>>(DWORD& dwValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(DWORD))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(DWORD), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&dwValue, sizeof(DWORD),
        &_chpBuffer[_iPayloadReadPos], sizeof(DWORD));

    _iPayloadReadPos += sizeof(DWORD);
    return *this;
}

CPacket& CPacket::operator>>(__int64& iValue)
{
    if (_iPayloadWritePos - _iPayloadReadPos < sizeof(__int64))
    {
        ::printf("Used Size(%d) < Requested Size(%llu)!: %s %d\n",
            _iPayloadWritePos - _iPayloadReadPos, sizeof(__int64), __func__, __LINE__);
        return *this;
    }

    memcpy_s(&iValue, sizeof(__int64),
        &_chpBuffer[_iPayloadReadPos], sizeof(__int64));

    _iPayloadReadPos += sizeof(__int64);
    return *this;
}

int CPacket::GetPayloadData(char* chpDest, int iSize)
{
    if (_iPayloadWritePos - _iPayloadReadPos < iSize)
    {
        ::printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iPayloadWritePos - _iPayloadReadPos, iSize);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);

    _iPayloadReadPos += iSize;
    return iSize;
}

int CPacket::PeekPayloadData(char* chpDest, int iSize)
{
    if (_iPayloadWritePos - _iPayloadReadPos < iSize)
    {
        ::printf("Used Size(%d) is small than Requested Size(%d)!\n",
            _iPayloadWritePos - _iPayloadReadPos, iSize);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iPayloadReadPos], iSize);
    return iSize;
}

int CPacket::PutPayloadData(char* chpSrc, int iSrcSize)
{
    if (_iBufferSize - _iPayloadWritePos < iSrcSize)
        Resize(_iBufferSize + (int)(iSrcSize * 1.5f));

    memcpy_s(&_chpBuffer[_iPayloadWritePos],
        _iBufferSize - _iPayloadWritePos,
        chpSrc, iSrcSize);

    _iPayloadWritePos += iSrcSize;
    return iSrcSize;
}

int CPacket::GetHeaderData(char* chpDest, int iSize)
{
    if (_iHeaderSize < iSize)
    {
        ::printf("Header(%d) is small than Requested Size(%d)! (read: %d)\n",
            _iHeaderSize, iSize, _iHeaderReadPos);
        return -1;
    }

    if (_iHeaderWritePos - _iHeaderReadPos < iSize)
    {
        ::printf("Header(%d) is small than Requested Size(%d)! (read: %d)\n",
            _iHeaderSize, iSize, _iHeaderReadPos);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);

    _iHeaderReadPos += iSize;
    return iSize;
}

int CPacket::PeekHeaderData(char* chpDest, int iSize)
{
    if (_iHeaderWritePos - _iHeaderReadPos < iSize)
    {
        ::printf("Usable Size(%d) is small than Requested Size(%d)! (header read pos: %d)\n",
            _iHeaderWritePos - _iHeaderReadPos, iSize, _iHeaderReadPos);
        return -1;
    }

    memcpy_s(chpDest, iSize, &_chpBuffer[_iHeaderReadPos], iSize);
    return iSize;
}

int CPacket::PutHeaderData(char* chpSrc, int iSrcSize)
{
    if (_iHeaderSize < _iHeaderWritePos + iSrcSize)
    {
        ::printf("Usable size(%d) is small than Requested Size(%d)! (header write pos: %d)\n",
            _iHeaderSize - _iHeaderWritePos, iSrcSize, _iHeaderWritePos);
        return -1;
    }

    memcpy_s(&_chpBuffer[_iHeaderWritePos],
        _iHeaderSize, chpSrc, iSrcSize);

    _iHeaderWritePos += iSrcSize;

    return iSrcSize;
}

