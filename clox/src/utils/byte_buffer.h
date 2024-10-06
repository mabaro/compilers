#pragma once

#include <istream>

// quick and dirty istream application

struct ByteBuffer : public std::basic_streambuf<char>
{
    ByteBuffer(const uint8_t* data, size_t count) { setg((char*)data, (char*)data, (char*)data + count); }
};
struct ByteStream : public std::istream
{
    ByteStream(const uint8_t* data, size_t count) : std::istream(&_buffer), _buffer(data, count) { rdbuf(&_buffer); }

   private:
    ByteBuffer _buffer;
};
