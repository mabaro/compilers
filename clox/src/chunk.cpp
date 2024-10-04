#include "chunk.h"
#include "utils/serialize.h"

static const int16_t VERSION = 0x01;

static const char* MAGIC_ID = "CODE42";
static const char* CODE_SEG = ".CODE";
static const char* DATA_SEG = ".DATA";

void Chunk::serialize(std::ostream& o_stream) const
{
    ASSERT_MSG(utils::isLittleEndian(), "not supported, need reverting bytes");
    using len_t = serialize::size_t;
    const size_t lenSize = sizeof(len_t);
    len_t len = 0;

    serialize::SerializeN(o_stream, MAGIC_ID, strlen(MAGIC_ID));
    serialize::Serialize(o_stream, VERSION);
    serialize::SerializeN(o_stream, DATA_SEG, strlen(DATA_SEG));

    serialize::SerializeAs<len_t>(o_stream, _constants.size());
    for(auto constantIt = _constants.cbegin(); constantIt != _constants.cend(); ++constantIt)
    {
        constantIt->serialize(o_stream);
    }

    serialize::SerializeN(o_stream, CODE_SEG, strlen(CODE_SEG));
    serialize::SerializeAs<len_t>(o_stream, _code.size());
    if (!_code.empty())
    {
        serialize::SerializeN(o_stream, _code.data(), _code.size());
    }
}
Result<void> Chunk::deserialize(std::istream& i_stream)
{
    using len_t = serialize::size_t;
    const size_t lenSize = sizeof(len_t);
    len_t len=0;
    char tempStr[32];

    serialize::DeserializeN(i_stream, &tempStr[0], strlen(MAGIC_ID));
    if (0 != strncmp(MAGIC_ID, tempStr, strlen(MAGIC_ID)))
    {
        return Result<void>::error_t(format("Invalid MagicID: %s != %s\n", tempStr, MAGIC_ID));
    }

    std::remove_const<decltype(VERSION)>::type version;
    serialize::Deserialize(i_stream, version);
    if (VERSION != version)
    {
        return Result<void>::error_t(format("Invalid version %d != %d expected\n", version, VERSION));
    }

    serialize::DeserializeN(i_stream, tempStr, strlen(DATA_SEG));
    if (0 != strncmp(DATA_SEG, tempStr, strlen(DATA_SEG)))
    {
        return Result<void>::error_t(format("DATA segment not present\n"));
    }

    serialize::DeserializeAs<len_t>(i_stream, len);
    _constants.resize(len);
    for(auto constantIt = _constants.begin(); constantIt != _constants.end(); ++constantIt)
    {
        constantIt->deserialize(i_stream);
    }
    
    serialize::DeserializeN(i_stream, tempStr, strlen(CODE_SEG));
    if (0 != strncmp(CODE_SEG, tempStr, strlen(CODE_SEG)))
    {
        return Result<void>::error_t(format("CODE segment not present\n"));
    }
    serialize::DeserializeAs<len_t>(i_stream, len);
    _code.resize(len);
    serialize::DeserializeN(i_stream, _code.data(), len);

    return Result<void>();
}