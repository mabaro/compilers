#include "value.h"

#include <cstring>

Object *Object::s_allocatedList = nullptr;

const char *Object::getString(Type type)
{
    switch (type)
    {
        case Type::String:
            return "String";
        default:
            return "Undefined type";
    }
}
void Object::FreeObject(Object *obj)
{
    switch (obj->type)
    {
        case Type::String:
        {
            ObjectString *strObj = as<ObjectString>(obj);
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

ObjectString *ObjectString::Create(char *ownedStr, size_t length)
{
    ObjectString *newStringObj = Object::allocate<ObjectString>();
    newStringObj->chars = ownedStr;
    newStringObj->length = length;
    return newStringObj;
}

ObjectString *ObjectString::Create(const char *begin, const char *end)
{
    const size_t length = end - begin;
    char *newString = ALLOCATE_N(char, length + 1);
    ASSERT(newString);
    memcpy(newString, begin, length);
    newString[length] = 0;

    return Create(newString, length);
}

///////////////////////////////////////////////////////////////////////////////

const char *Value::getString(Type type)
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
Value Value::Create(char *ownedStr, size_t length)
{
    return Value{
        .as{
            .object = ObjectString::Create(ownedStr, length),
        },
        .type = Type::Object,
    };
}
Value Value::Create(const char *begin, const char *end)
{
    return Value{
        .as{
            .object = ObjectString::Create(begin, end),
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
            const ObjectString *aStr = Object::as<ObjectString>(&a);
            const ObjectString *bStr = Object::as<ObjectString>(&b);
            if (aStr && bStr)
            {
                const size_t newLength = aStr->length + bStr->length;
                char *newStr = (char *)malloc(newLength + 1);
                memcpy(newStr, aStr->chars, aStr->length);
                memcpy(newStr + aStr->length, bStr->chars, bStr->length);
                newStr[newLength] = 0;
                return Value::Create(newStr, newLength);
            }
        }
        break;
        default:
            FAIL();
    }
    return Error_t(buildMessage("Undefined operation for objects of types: %s and %s", Object::getString(a.type),
                                Object::getString(b.type)));
}

bool compareObject(const Object *a, const Object *b)
{
    ASSERT(b->type == a->type);

    if (a == nullptr || b == nullptr)
    {
        FAIL();
        return a == b;
    }

    switch (a->type)
    {
        case Object::Type::String:
        {
            const ObjectString *aStr = Object::as<ObjectString>(a);
            const ObjectString *bStr = Object::as<ObjectString>(b);
            return (aStr->length == bStr->length) && memcmp(aStr->chars, bStr->chars, aStr->length);
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
            return compareObject(a.as.object, b.as.object);
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

void printValue(std::string& oStr, const Value &value)
{
    oStr.resize(256);
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
            switch(value.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strObj = *value.as.object->asString();
                    snprintf(oStr.data(), oStr.capacity(), "%.*s", (int)strObj.length, strObj.chars);
                    break;
                }
                default:
                    FAIL_MSG("UNDEFINED OBJECT TYPE(%d)", value.as.object->type);
            }
            break;
        default:
            FAIL_MSG("UNDEFINED VALUE TYPE(%d)", value.type);
            break;
    }
    oStr.resize(strlen(oStr.data())+1);
}

void printValue(const Value &value)
{
    constexpr size_t SIZE = 256;
    std::string outStr;
    outStr.resize(SIZE);
    printValue(outStr, value);
    printf("%s", outStr.c_str());
}