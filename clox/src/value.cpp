#include "value.h"

#include <cstring>

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
            ObjectString *newString = ObjectString::CreateEmpty();
            newString->deserialize(i_stream);
            return newString;
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
            ObjectString *strObj = obj->asString();
            DEALLOCATE(ObjectString, strObj);
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
Result<void> ObjectString::deserialize(std::istream &i_stream)
{
    this->length = 0;

    uint8_t byte;
    serde::Deserialize(i_stream, byte);
    this->length = byte;

    if ((byte & 0x03) == 2)
    {
        this->length = byte;
        serde::Deserialize(i_stream, byte);
        this->length = (byte << 8) | this->length;
    }
    else if ((byte & 0x03) == 0)
    {
        this->length = byte;
        serde::Deserialize(i_stream, byte);
        this->length = (byte << 8) | this->length;
        serde::Deserialize(i_stream, byte);
        this->length = (byte << 16) | this->length;
        serde::Deserialize(i_stream, byte);
        this->length = (byte << 24) | this->length;
    }
    this->length >>= 2;

    this->chars = ALLOCATE_N(char, this->length + 1);
    serde::DeserializeN(i_stream, this->chars, this->length);
    this->chars[this->length] = '\0';
    return Result<void>();
}

ObjectString *ObjectString::CreateEmpty()
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
    newStringObj->length           = newLength;
    newStringObj->chars[newLength] = '\0';
    return newStringObj;
}

ObjectString *ObjectString::CreateByCopy(const char *str, size_t length)
{
    ObjectString *newStringObj = Object::allocate<ObjectString>(length + 1);
    newStringObj->chars        = ((char *)newStringObj) + sizeof(ObjectString);
    newStringObj->length       = length;
    memcpy(newStringObj->chars, str, length);
    newStringObj->chars[length] = '\0';
    return newStringObj;
}

bool ObjectString::compare(const ObjectString &a, const ObjectString &b)
{
    return a.length == b.length && (0 == memcmp(a.chars, b.chars, a.length));
}

///////////////////////////////////////////////////////////////////////////////

Result<void> Value::serialize(std::ostream &o_stream) const
{
    static_assert(sizeof(type) == 1, "Check enum type");
    serde::Serialize(o_stream, type);
    switch (type)
    {
        case Type::Null: break;
        case Type::Bool: serde::Serialize(o_stream, as.boolean); break;
        case Type::Number: serde::Serialize(o_stream, as.number); break;
        case Type::Integer: serde::Serialize(o_stream, as.integer); break;
        case Type::Object: return as.object->serialize(o_stream); break;
        case Type::Undefined: break;
        default: FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}
Result<void> Value::deserialize(std::istream &i_stream)
{
    serde::Deserialize(i_stream, type);
    switch (type)
    {
        case Type::Bool: serde::Deserialize(i_stream, as.boolean); break;
        case Type::Null:
        case Type::Undefined: break;
        case Type::Number: serde::Deserialize(i_stream, as.number); break;
        case Type::Integer: serde::Deserialize(i_stream, as.integer); break;
        case Type::Object:
        {
            auto result = Object::deserialize(i_stream);
            ASSERT(result.isOk());
            if (result.isOk())
            {
                as.object = result.extract();
            }
            else
            {
                return result.error();
            }
            break;
        }
        default: FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}

const char *Value::getTypeName(Type type)
{
    switch (type)
    {
        case Type::Bool: return "Boolean";
        case Type::Null: return "Null";
        case Type::Number: return "Number";
        case Type::Integer: return "Integer";
        case Type::Object: return "Object";
        default: return "Undefined type";
    }
}
Value::operator bool() const
{
    ASSERT(type == Type::Bool);
    return as.boolean;
}
Value::operator int() const
{
    ASSERT(type == Type::Integer);
    return as.integer;
}
Value::operator double() const
{
    ASSERT(type == Type::Number);
    return as.number;
}
Value::operator char *() const
{
    ASSERT(type == Type::Object && as.object->type == Object::Type::String);
    if (as.object->type == Object::Type::String)
    {
        ObjectString *strObj = static_cast<ObjectString *>(as.object);
        return strObj->chars;
    }
    FAIL();
    return (char *)"Invalid ObjectString";
}

Value Value::Create() { return Value{.type = Type::Undefined}; }
Value Value::Create(NullType)
{
    return Value{
        .as{.integer = (int)0xDEADBEEF},
        .type = Type::Null,
    };
}
Value Value::Create(bool value)
{
    return Value{
        .as{.boolean = value},
        .type = Type::Bool,
    };
}
Value Value::Create(int value)
{
    return Value{
        .as{.integer = value},
        .type = Type::Integer,
    };
}
Value Value::Create(double value)
{
    return Value{
        .as{.number = value},
        .type = Type::Number,
    };
}
Value Value::CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2)
{
    return Value{
        .as{
            .object = ObjectString::CreateConcat(str1, len1, str2, len2),
        },
        .type = Type::Object,
    };
}
Value Value::CreateByCopy(const char *str, size_t length)
{
    return Value{
        .as{
            .object = ObjectString::CreateByCopy(str, length),
        },
        .type = Type::Object,
    };
}

Value Value::operator-() const
{
    switch (type)
    {
        case Value::Type::Number: return Create(-as.number);
        case Value::Type::Integer: return Create(-as.integer);
        default: ASSERT(false); return Create();
    }
}

Value Value::operator-(const Value &a)
{
    switch (a.type)
    {
        case Value::Type::Number: return Create(-a.as.number);
        case Value::Type::Integer: return Create(-a.as.integer);
        default: ASSERT(false); return Create();
    }
}

Result<Value> operator+(const Object &a, const Object &b)
{
    switch (a.type)
    {
        case Object::Type::String:
        {  // concatenate
            const ObjectString *aStr = a.asString();
            const ObjectString *bStr = b.asString();
            if (aStr && bStr)
            {
                return Value::CreateConcat(aStr->chars, aStr->length, bStr->chars, bStr->length);
            }
        }
        break;
        default: FAIL();
    }
    return Error_t(buildMessage("Undefined operation for objects of types: %s and %s", Object::getTypeName(a.type),
                                Object::getTypeName(b.type)));
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

bool operator==(const Value &a, const Value &b)
{
    if (a.type != b.type)
    {
        return false;
    }
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean == b.as.boolean;
        case Value::Type::Number: return a.as.number == b.as.number;
        case Value::Type::Integer: return a.as.integer == b.as.integer;
        case Value::Type::Object: return Object::compare(a.as.object, b.as.object);
        case Value::Type::Null: return true;
        default: ASSERT(false); return false;
    }
}
bool operator<(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean < b.as.boolean;
        case Value::Type::Number: return a.as.number < b.as.number;
        case Value::Type::Integer: return a.as.integer < b.as.integer;
        case Value::Type::Null: return false;
        case Value::Type::Object:
            switch (a.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strA = static_cast<char *>(a);
                    auto strB = static_cast<char *>(b);
                    return strcmp(strA, strB) < 0;
                }
                default: FAIL_MSG("Not implemented"); return false;
            }
        default: ASSERT(false); return false;
    }
}
bool operator>(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean > b.as.boolean;
        case Value::Type::Number: return a.as.number > b.as.number;
        case Value::Type::Integer: return a.as.integer > b.as.integer;
        case Value::Type::Object:
            switch (a.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strA = static_cast<char *>(a);
                    auto strB = static_cast<char *>(b);
                    return strcmp(strA, strB) > 0;
                }
                default: FAIL_MSG("Not implemented"); return false;
            }
        case Value::Type::Null: return false;
        default: ASSERT(false); return false;
    }
}

#define DECL_OPERATOR(OP)                                                                  \
    Value operator OP(const Value &a, const Value &b)                                      \
    {                                                                                      \
        switch (a.type)                                                                    \
        {                                                                                  \
            case Value::Type::Number: return Value::Create(a.as.number OP b.as.number);    \
            case Value::Type::Integer: return Value::Create(a.as.integer OP b.as.integer); \
            default: ASSERT(false); return Value::Create();                                \
        }                                                                                  \
    }

DECL_OPERATOR(+)
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)

////////////////////////////////////////////////////////////////////////////////////

[[maybe_unused]] static void print(const Object *obj)
{
    ASSERT(obj);

    switch (obj->type)
    {
        case Object::Type::String:
        {
            const ObjectString &strObj = *static_cast<const ObjectString *>(obj);
            printf("%.*s", (int)strObj.length, strObj.chars);
        }
        break;
        default: FAIL_MSG("Unexpected Object type(%d)", obj->type);
    }
}

void printValue(const Value &value)
{
    switch (value.type)
    {
        case Value::Type::Bool: printf("%s", value.as.boolean ? "true" : "false"); break;
        case Value::Type::Null: printf("%s", "null"); break;
        case Value::Type::Number: printf("%.2f", value.as.number); break;
        case Value::Type::Integer: printf("%d", value.as.integer); break;
        case Value::Type::Object:
            switch (value.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strObj = *value.as.object->asString();
                    printf("%.*s", (int)strObj.length, strObj.chars);
                    break;
                }
                default: FAIL_MSG("UNDEFINED OBJECT TYPE(%d)", value.as.object->type);
            }
            break;
        default: printf("UNDEF"); break;
    }
}

void printValueDebug(const Value &value)
{
    const bool isString = value.type == Value::Type::Object && value.as.object->type == Object::Type::String;
    if (!isString)
    {
        printValue(value);
    }
    else
    {
        auto        strObj     = *value.as.object->asString();
        const char *charPtr    = strObj.chars;
        const char *charPtrEnd = strObj.chars + strObj.length;
        while (charPtr != charPtrEnd)
        {
            if (*charPtr != '\n')
            {
                putc(*charPtr, stdout);
            }
            else
            {
                putc('\\', stdout);
                putc('n', stdout);
            }
            ++charPtr;
        }
    }
}
