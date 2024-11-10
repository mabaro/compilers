#include "chunk.h"
#include "header.h"
#include "utils/serde.h"
#include <cstring>

static const char* CODE_SEG = ".CODE";
static const char* DATA_SEG = ".DATA";

Result<void> Chunk::serialize(std::ostream& o_stream) const
{
    ASSERT_MSG(serde::isLittleEndian(), "not supported, need reverting bytes");

    auto headerResult = writeHeader(o_stream);
    if (!headerResult.isOk())
    {
        return headerResult.error();
    }

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

    auto headerResult = readHeader(i_stream);
    if (!headerResult.isOk())
    {
        return headerResult.error();
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