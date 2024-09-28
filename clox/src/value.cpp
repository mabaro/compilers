#pragma once

#include "value.h"

#include <cstdlib>
#include <vector>

ObjectString *ObjectString::Create(const char *begin, const char *end)
{
    const size_t length = end - begin;
    char *newString = ALLOCATE(char, length + 1);
    ASSERT(newString);
    memcpy(newString, begin, length);
    newString[length] = 0;

    ObjectString *newStringObj = Object::allocate<ObjectString>();
    newStringObj->chars = newString;
    newStringObj->length = length;
    return newStringObj;
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