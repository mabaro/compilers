#include "object.h"

#include <cstring>

#include "utils/memory.h"
#include "utils/serde.h"

Object *Object::s_allocatedList = nullptr;

const char *Object::getTypeName(Type type)
{
    switch (type)
    {
        case Type::String: return "String";
        default: return "Undefined type";
    }
}

Result<void> Object::serialize(std::ostream &o_stream) const
{
    serde::Serialize(o_stream, type);
    switch (type)
    {
        case Type::String: return asString()->serialize(o_stream);
        default: FAIL();
    }
    return Error<>(format("Unsupported type: %d\n", type));
}

Result<Object *> Object::deserialize(std::istream &i_stream)
{
    Object::Type type;
    serde::Deserialize(i_stream, type);
    switch (type)
    {
        case Type::String:
        {
            auto result = ObjectString::deserialize(i_stream);
            if (result.isOk())
            {
                return result.extract();
            }
            return result.error();
        }
        case Type::Function:
        {
            ObjectFunction *newFunction = ObjectFunction::Create();
            newFunction->deserialize(i_stream);
            return newFunction;
        }
        default: FAIL();
    }
    return Error<>(format("Unsupported type: %d\n", type));
}

void Object::FreeObject(Object *obj)
{
    switch (obj->type)
    {
        case Type::String:
        {
            ObjectString *str = obj->asString();
            DEALLOCATE(ObjectString, str);
            break;
        }
        case Type::Function:
        {
            ObjectFunction *func = obj->asFunction();
            func->chunk.~Chunk();
            DEALLOCATE(ObjectFunction, func);
            break;
        }
        default: FAIL_MSG("Unsupported type: %d", obj->type);
    }
}

void Object::FreeObjects()
{
    Object *object = s_allocatedList;
    while (object)
    {
        Object *next = object->_allocatedNext;
        FreeObject(object);
        object = next;
    }
}

Object *Object::operator+(const Object &other) const
{
    switch (type)
    {
        case Object::Type::String:
        {  // concatenate
            if (other.type == Object::Type::String)
            {
                const ObjectString *aStr = asString();
                const ObjectString *bStr = other.asString();
                if (aStr && bStr)
                {
                    return ObjectString::CreateConcat(aStr->chars, aStr->length, bStr->chars, bStr->length);
                }
            }
        }
        break;
        default: FAIL();
    }
    FAIL_MSG("Undefined '%s' for objects of types: %s and %s", __FUNCTION__, Object::getTypeName(type),
             Object::getTypeName(other.type));
    return nullptr;
}

bool Object::compare(const Object *a, const Object *b)
{
    ASSERT(a != nullptr && b != nullptr);
    ASSERT(b->type == a->type);

    switch (a->type)
    {
        case Object::Type::String:
        {
            const ObjectString *aStr = a->asString();
            const ObjectString *bStr = b->asString();
            return ObjectString::compare(*aStr, *bStr);
        }
        default: FAIL();
    }
    return false;
}

void printObject(const Object &object)
{
    switch (object.type)
    {
        case Object::Type::String:
        {
            const auto &strObj = *object.asString();
            printf("%.*s", (int)strObj.length, strObj.chars);
            break;
        }
        case Object::Type::Function:
        {
            const auto &func = *object.asFunction();
            printf("<fn ");
            printObject(*func.name);
            printf(">");
            break;
        }
        default: FAIL_MSG("UNDEFINED OBJECT TYPE(%d)", object.type);
    }
}

////////////////////////////////////////////////////////////////////////////////

Result<void> ObjectString::serialize(std::ostream &o_stream) const
{
    if ((this->length < ((1L << 6) - 1)))
    {
        const uint8_t len = static_cast<uint8_t>(length << 2) | 0x01;
        serde::Serialize(o_stream, len);
    }
    else if ((this->length < ((1L << 14) - 1)))
    {
        const uint16_t len = static_cast<uint16_t>(length << 2) | 0x02;
        serde::Serialize(o_stream, len);
    }
    else
    {
        const uint32_t len = static_cast<uint32_t>(length << 2) | 0x00;
        serde::Serialize(o_stream, len);
    }

    serde::SerializeN(o_stream, this->chars, this->length);
    return Result<void>();
}

Result<ObjectString *> ObjectString::deserialize(std::istream &i_stream)
{
    uint32_t length = 0;

    uint8_t byte;
    serde::Deserialize(i_stream, byte);
    length = byte;

    if ((byte & 0x03) == 2)
    {
        length = byte;
        serde::Deserialize(i_stream, byte);
        length = (byte << 8) | length;
    }
    else if ((byte & 0x03) == 0)
    {
        length = byte;
        serde::Deserialize(i_stream, byte);
        length = (byte << 8) | length;
        serde::Deserialize(i_stream, byte);
        length = (byte << 16) | length;
        serde::Deserialize(i_stream, byte);
        length = (byte << 24) | length;
    }
    length >>= 2;

    constexpr size_t kSmallStringSize = 256;
    if (length < kSmallStringSize)
    {
        char tempStr[kSmallStringSize];
        serde::DeserializeN(i_stream, tempStr, length);
        return CreateByCopy(tempStr, length);
    }
    else
    {
        std::vector<char> tempStr(length);
        serde::DeserializeN(i_stream, tempStr.data(), length);
        return CreateByCopy(tempStr.data(), length);
    }
}

ObjectString *ObjectString::Create()
{
    ObjectString *newStringObj = Object::allocate<ObjectString>();
    newStringObj->chars        = nullptr;
    newStringObj->length       = 0;
    return newStringObj;
}

ObjectString *ObjectString::CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2)
{
    const size_t  newLength    = len1 + len2;
    ObjectString *newStringObj = Object::allocate<ObjectString>(newLength + 1);
    newStringObj->chars        = ((char *)newStringObj) + sizeof(ObjectString);
    memcpy(newStringObj->chars, str1, len1);
    memcpy(newStringObj->chars + len1, str2, len2);
    newStringObj->length           = static_cast<uint8_t>(newLength);
    newStringObj->length           = static_cast<decltype(length)>(newLength);
    newStringObj->chars[newLength] = '\0';
    return newStringObj;
}

ObjectString *ObjectString::CreateByCopy(const char *str, size_t length)
{
    ObjectString *newStringObj = Object::allocate<ObjectString>(length + 1);
    newStringObj->chars        = ((char *)newStringObj) + sizeof(ObjectString);
    newStringObj->length       = static_cast<uint8_t>(length);
    memcpy(newStringObj->chars, str, length);
    newStringObj->chars[length] = '\0';
    return newStringObj;
}

bool ObjectString::compare(const ObjectString &a, const ObjectString &b)
{
    return a.length == b.length && (0 == memcmp(a.chars, b.chars, a.length));
}

Result<void> ObjectFunction::serialize(std::ostream &o_stream) const
{
    this->name->serialize(o_stream);
    serde::SerializeAs<uint8_t>(o_stream, this->arity);
    this->chunk.serialize(o_stream);
    return Result<void>();
}

Result<void> ObjectFunction::deserialize(std::istream &i_stream)
{
    auto stringRes = ObjectString::deserialize(i_stream);
    if (!stringRes.isOk())
    {
        return stringRes.error();
    }
    this->name = stringRes.extract();
    serde::DeserializeAs<uint8_t>(i_stream, this->arity);
    this->chunk.deserialize(i_stream);

    return Result<void>();
}

ObjectFunction *ObjectFunction::Create(const char *name)
{
    ObjectFunction *function = Object::allocate<ObjectFunction>();
    function->arity          = 0;
    function->name           = ObjectString::CreateByCopy(name, strlen(name));
    new ((Chunk *)&function->chunk) Chunk(name);
    return function;
}
