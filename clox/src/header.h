#pragma once

#include "utils/common.h"
#include "utils/serde.h"

/*
-disassemble  test.cloxbin -run
-code "var a=1; print a;" -compile -output test.cloxbin
*/
struct Version
{
    uint8_t major = 0;
    uint8_t minor = 0;
    uint8_t build = 0;
    char    tag   = '-';

    bool operator==(const Version& other) const { return major == other.major && minor == other.minor; }

    bool operator<=(const Version& other) const
    {
        return major < other.major || (major == other.major && minor <= other.minor);
    }
};

[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, const Version& v)
{
    using write_t = int;
    os << (write_t)v.major << "." << (write_t)v.minor << v.tag << (write_t)v.build;
    return os;
}

static constexpr Version VERSION{0, 0, 1, 'a'};

struct Header
{
    static constexpr char    magic[] = {'_', 'C', 'O', 'D', 'E', '4', '2', '_'};
    static constexpr Version version  = VERSION;
};

static Result<void> writeHeader(std::ostream& o_stream)
{
    ASSERT_MSG(serde::isLittleEndian(), "not supported, need reverting bytes");

    serde::SerializeN(o_stream, Header::magic, ARRAY_COUNT(Header::magic));
    serde::Serialize(o_stream, Header::version);
    return Result<void>();
}

static Result<void> readHeader(std::istream& i_stream)
{
    ASSERT_MSG(serde::isLittleEndian(), "not supported, need reverting bytes");

    char   tempStr[32];
    serde::DeserializeN(i_stream, tempStr, ARRAY_COUNT(Header::magic));
    if (0 != strncmp(Header::magic, tempStr, ARRAY_COUNT(Header::magic)))
    {
        FAIL();
        return Result<void>::error_t(format("Invalid MagicID: %s != %s\n", tempStr, Header::magic));
    }

    Version version;
    serde::Deserialize(i_stream, version);
    if (Header::version != version)
    {
        FAIL();
        return Result<void>::error_t(format("Invalid version %d.%d != %d.%d expected\n", version, Header::version));
    }

    //LOG_INFO("Code ver: ");
    //std::cout << version << std::endl;
    return Result<void>();
}