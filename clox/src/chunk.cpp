#include "chunk.h"

#include <cstring>

#include "utils/serde.h"


struct Version
{
    uint8_t major;
    uint8_t minor;
    bool    operator==(const Version& other) const { return major == other.major && minor == other.minor; }
    bool    operator<=(const Version& other) const
    {
        return major < other.major || (major == other.major && minor <= other.minor);
    }
};
static const Version VERSION{0, 0};

static const char* MAGIC_ID = "CODE42";
static const char* CODE_SEG = ".CODE";
static const char* DATA_SEG = ".DATA";

Result<void> Chunk::serialize(std::ostream& o_stream) const
{
    ASSERT_MSG(serde::isLittleEndian(), "not supported, need reverting bytes");

    serde::SerializeN(o_stream, MAGIC_ID, strlen(MAGIC_ID));
    serde::Serialize(o_stream, VERSION);
    serde::SerializeN(o_stream, DATA_SEG, strlen(DATA_SEG));

    serde::SerializeAs<serde::constants_len_t>(o_stream, _constants.size());
    for (auto constantIt = _constants.cbegin(); constantIt != _constants.cend(); ++constantIt)
    {
        constantIt->serialize(o_stream);
    }

    serde::SerializeN(o_stream, CODE_SEG, strlen(CODE_SEG));
    serde::SerializeAs<serde::code_len_t>(o_stream, _code.size());
    if (!_code.empty())
    {
        serde::SerializeN(o_stream, _code.data(), _code.size());
    }
    return Result<void>();
}
Result<void> Chunk::deserialize(std::istream& i_stream)
{
    using len_t = serde::size_t;
    len_t len   = 0;
    char  tempStr[32];

    serde::DeserializeN(i_stream, &tempStr[0], strlen(MAGIC_ID));
    if (0 != strncmp(MAGIC_ID, tempStr, strlen(MAGIC_ID)))
    {
        FAIL();
        return Result<void>::error_t(format("Invalid MagicID: %s != %s\n", tempStr, MAGIC_ID));
    }

    std::remove_const<decltype(VERSION)>::type version;
    serde::Deserialize(i_stream, version);
    if (VERSION != version)
    {
        FAIL();
        return Result<void>::error_t(format("Invalid version %d.%d != %d.%d expected\n", version, VERSION));
    }

    serde::DeserializeN(i_stream, tempStr, strlen(DATA_SEG));
    if (0 != strncmp(DATA_SEG, tempStr, strlen(DATA_SEG)))
    {
        FAIL();
        return Result<void>::error_t(format("DATA segment not present\n"));
    }

    serde::DeserializeAs<serde::constants_len_t>(i_stream, len);
    _constants.resize(len);
    for (auto constantIt = _constants.begin(); constantIt != _constants.end(); ++constantIt)
    {
        constantIt->deserialize(i_stream);
    }

    serde::DeserializeN(i_stream, tempStr, strlen(CODE_SEG));
    if (0 != strncmp(CODE_SEG, tempStr, strlen(CODE_SEG)))
    {
        FAIL();
        return Result<void>::error_t(format("CODE segment not present\n"));
    }
    serde::DeserializeAs<serde::code_len_t>(i_stream, len);
    _code.resize(len);
    serde::DeserializeN(i_stream, _code.data(), len);

    return Result<void>();
}