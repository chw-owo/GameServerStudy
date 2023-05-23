#pragma once

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

class SerializeBuffer;

SerializeBuffer& operator << (SerializeBuffer& out, int8 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, int16 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, int32 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, int64 iValue);

SerializeBuffer& operator << (SerializeBuffer& out, uint8 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, uint16 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, uint32 iValue);
SerializeBuffer& operator << (SerializeBuffer& out, uint64 iValue);

SerializeBuffer& operator >> (SerializeBuffer& out, int8& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, int16& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, int32& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, int64& iValue);

SerializeBuffer& operator >> (SerializeBuffer& out, uint8& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, uint16& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, uint32& iValue);
SerializeBuffer& operator >> (SerializeBuffer& out, uint64& iValue);