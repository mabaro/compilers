#include "value.h"

#include <cstring>

#include "utils/serialize.h"

Object *Object::s_allocatedList = nullptr;

const char *Object::getTypeName(Type type)
{
    switch (type)
    {
        case Type::String:
            return "String";
        default:
            return "Undefined type";
    }
}

Result<void> Object::serialize(std::ostream &o_stream) const
{
    serialize::Serialize(o_stream, type);
    switch (type)
    {
        case Type::String:
          return asString()->serialize(o_stream);
        default:
            FAIL();
    }
    return Error<>(format("Unsupported type: %d\n", type));
}

Result<Object *> Object::deserialize(std::istream &i_stream)
{
    Object::Type type;
    serialize::Deserialize(i_stream, type);
    switch (type)
    {
        case Type::String:
        {
            serialize::size_t len;
            serialize::Deserialize(i_stream, len);
            char *newString = ALLOCATE_N(char, len+1);
            serialize::DeserializeN(i_stream, newString, len);
            newString[len] = '\0';
            return ObjectString::CreateByMove(newString, len);
        }
        default:
            FAIL();
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
            DEALLOCATE_N(char, strObj->chars, strObj->length);
            DEALLOCATE(ObjectString, strObj);
            break;
        }
        default:
            FAIL_MSG("Unsupported type: %d", obj->type);
    }
}
void Object::FreeObjects()
{
    Object *ptr = s_allocatedList;
    while (ptr)
    {
        Object *next = ptr->_allocatedNext;
        DEALLOCATE(Object, ptr);
        ptr = next;
    }
}

////////////////////////////////////////////////////////////////////////////////

Result<void> ObjectString::serialize(std::ostream &o_stream) const
{
    serialize::SerializeAs<serialize::size_t>(o_stream, this->length);
    serialize::SerializeN(o_stream, this->chars, this->length);
    return Result<void>();
}
Result<void> ObjectString::deserialize(std::istream &i_stream)
{
    serialize::DeserializeAs<serialize::size_t>(i_stream, this->length);
    serialize::DeserializeN(i_stream, this->chars, this->length);
    return Result<void>();
}

ObjectString *ObjectString::CreateByMove(char *str, size_t length)
{
    ObjectString *newStringObj = Object::allocate<ObjectString>();
    newStringObj->chars = str;
    newStringObj->length = length;
#if USING(DEBUG_BUILD)
    newStringObj->as.string = newStringObj;
#endif  // #if USING(DEBUG_BUILD)
    return newStringObj;
}

ObjectString *ObjectString::CreateByCopy(const char *str, size_t length)
{
    char *newString = ALLOCATE_N(char, length + 1);
    ASSERT(newString);
    memcpy(newString, str, length);
    newString[length] = 0;

    return CreateByMove(newString, length);
}

bool ObjectString::compare(const ObjectString &a, const ObjectString &b)
{
    return a.length == b.length && (0 == memcmp(a.chars, b.chars, a.length));
}

///////////////////////////////////////////////////////////////////////////////

Result<void> Value::serialize(std::ostream &o_stream) const
{
    static_assert(sizeof(type) == 1, "Check enum type");
    serialize::Serialize(o_stream, type);
    switch (type)
    {
        case Type::Bool:
            serialize::Serialize(o_stream, as.boolean);
            break;
        case Type::Null:
        case Type::Undefined:
            break;
        case Type::Number:
            serialize::Serialize(o_stream, as.number);
            break;
        case Type::Integer:
            serialize::Serialize(o_stream, as.integer);
            break;
        case Type::Object:
            return as.object->serialize(o_stream);
            break;
        default:
            FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}
Result<void> Value::deserialize(std::istream &i_stream)
{
    serialize::Deserialize(i_stream, type);
    switch (type)
    {
        case Type::Bool:
            serialize::Deserialize(i_stream, as.boolean);
            break;
        case Type::Null:
        case Type::Undefined:
            break;
        case Type::Number:
            serialize::Deserialize(i_stream, as.number);
            break;
        case Type::Integer:
            serialize::Deserialize(i_stream, as.integer);
            break;
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
        default:
            FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}

const char *Value::getTypeName(Type type)
{
    switch (type)
    {
        case Type::Bool:
            return "Boolean";
        case Type::Null:
            return "Null";
        case Type::Number:
            return "Number";
        case Type::Integer:
            return "Integer";
        case Type::Object:
            return "Object";
        default:
            return "Undefined type";
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
Value Value::CreateByMove(char *str, size_t length)
{
    return Value{
        .as{
            .object = ObjectString::CreateByMove(str, length),
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
        case Value::Type::Number:
            return Create(-as.number);
        case Value::Type::Integer:
            return Create(-as.integer);
        default:
            ASSERT(false);
            return Create();
    }
    return *this;
}

Value Value::operator-(const Value &a)
{
    switch (a.type)
    {
        case Value::Type::Number:
            return Create(-a.as.number);
        case Value::Type::Integer:
            return Create(-a.as.integer);
        default:
            ASSERT(false);
            return Create();
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
                const size_t newLength = aStr->length + bStr->length;
                char *newStr = (char *)malloc(newLength + 1);
                memcpy(newStr, aStr->chars, aStr->length);
                memcpy(newStr + aStr->length, bStr->chars, bStr->length);
                newStr[newLength] = 0;
                return Value::CreateByMove(newStr, newLength);
            }
        }
        break;
        default:
            FAIL();
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
        default:
            FAIL();
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
        case Value::Type::Bool:
            return a.as.boolean == b.as.boolean;
        case Value::Type::Number:
            return a.as.number == b.as.number;
        case Value::Type::Integer:
            return a.as.integer == b.as.integer;
        case Value::Type::Object:
            return Object::compare(a.as.object, b.as.object);
        case Value::Type::Null:
            return true;
        default:
            ASSERT(false);
            return false;
    }
}
bool operator<(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool:
            return a.as.boolean < b.as.boolean;
        case Value::Type::Number:
            return a.as.number < b.as.number;
        case Value::Type::Integer:
            return a.as.integer < b.as.integer;
        case Value::Type::Null:
            return false;
        default:
            ASSERT(false);
            return false;
    }
}
bool operator>(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool:
            return a.as.boolean > b.as.boolean;
        case Value::Type::Number:
            return a.as.number > b.as.number;
        case Value::Type::Integer:
            return a.as.integer > b.as.integer;
        case Value::Type::Object:
            switch (a.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strA = static_cast<char *>(a);
                    auto strB = static_cast<char *>(b);
                    return strcmp(strA, strB);
                }
                default:
                    FAIL_MSG("Not implemented");
                    return false;
            }
        case Value::Type::Null:
            return false;
        default:
            ASSERT(false);
            return false;
    }
}

#define DECL_OPERATOR(OP)                                           \
    Value operator OP(const Value &a, const Value &b)               \
    {                                                               \
        switch (a.type)                                             \
        {                                                           \
            case Value::Type::Number:                               \
                return Value::Create(a.as.number OP b.as.number);   \
            case Value::Type::Integer:                              \
                return Value::Create(a.as.integer OP b.as.integer); \
            default:                                                \
                ASSERT(false);                                      \
                return Value::Create();                             \
        }                                                           \
    }

DECL_OPERATOR(+)
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)

////////////////////////////////////////////////////////////////////////////////////

static void print(const Object *obj)
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
        default:
            FAIL_MSG("Unexpected Object type(%d)", obj->type);
    }
}

void printValue(std::string &oStr, const Value &value)
{
    oStr.resize(64);
    switch (value.type)
    {
        case Value::Type::Bool:
            snprintf(oStr.data(), oStr.capacity(), "%s", value.as.boolean ? "true" : "false");
            break;
        case Value::Type::Null:
            snprintf(oStr.data(), oStr.capacity(), "%s", "null");
            break;
        case Value::Type::Number:
            snprintf(oStr.data(), oStr.capacity(), "%.2f", value.as.number);
            break;
        case Value::Type::Integer:
            snprintf(oStr.data(), oStr.capacity(), "%d", value.as.integer);
            break;
        case Value::Type::Object:
            switch (value.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strObj = *value.as.object->asString();
                    if (oStr.size() < strObj.length)
                    {
                        oStr.resize(strObj.length);
                    }
                    snprintf(oStr.data(), oStr.capacity(), "%.*s", (int)strObj.length, strObj.chars);
                    break;
                }
                default:
                    FAIL_MSG("UNDEFINED OBJECT TYPE(%d)", value.as.object->type);
            }
            break;
        default:
            snprintf(oStr.data(), oStr.capacity(), "UNDEF");
            break;
    }
}

void printValue(const Value &value)
{
    std::string outStr;
    printValue(outStr, value);
    printf("%s", outStr.c_str());
}