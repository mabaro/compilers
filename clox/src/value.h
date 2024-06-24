#pragma once
#include <cstdlib>
#include <vector>

#include "common.h"

#define ALLOCATE(Obj, count) (Obj *)malloc(sizeof(Obj) * count)

struct Object
{
    enum class Type
    {
        String,
        COUNT
    };
    Type type = Type::COUNT;

    template <typename ObjectT>
    static ObjectT *allocate(Type type)
    {
        ObjectT *newObject = ALLOCATE(ObjectT, 1);
        newObject->type = type;
        return newObject;
    }
};

struct ObjectString : public Object
{
    size_t length = 0;
    char *chars = nullptr;
    static ObjectString *Create(const char *begin, const char *end)
    {
        const size_t length = end - begin;
        char *newString = ALLOCATE(char, length + 1);
        ASSERT(newString);
        memcpy(newString, begin, length);
        newString[length] = 0;

        ObjectString *newStringObj = Object::allocate<ObjectString>(Object::Type::String);
        newStringObj->chars = newString;
        newStringObj->length = length;
        return newStringObj;
    }
};

struct Value
{
    union
    {
        bool boolean;
        double number;
        int integer;
        Object *object;
    } as;

    enum class Type
    {
        Bool,
        Null,
        Number,
        Integer,
        Object,
        Undefined,
        COUNT = Undefined
    };
    Type type = Type::Undefined;

    static constexpr struct NullType
    {
    } Null = NullType{};

    bool is(Type t) const { return t == type; }
    bool isNumber() const { return type == Type::Integer || type == Type::Number; }
    bool isFalsey() const { return type == Type::Null || (type == Type::Bool && as.boolean == false); }

    explicit operator bool() const
    {
        ASSERT(type == Type::Bool);
        return as.boolean;
    }
    explicit operator int() const
    {
        ASSERT(type == Type::Integer);
        return as.integer;
    }
    explicit operator double() const
    {
        ASSERT(type == Type::Number);
        return as.number;
    }
    explicit operator char *() const
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

    template <typename T>
    T asT() const
    {
        ASSERT(type != Type::Undefined);
        if (std::is_same_v<T, bool>)
        {
            ASSERT(type == Type::Bool);
            return (T)as.boolean;
        }
        else if (std::is_integral_v<T>)
        {
            ASSERT(type == Type::Integer);
            return (T)as.integer;
        }
        else if (std::is_floating_point_v<T>)
        {
            ASSERT(type == Type::Number);
            return (T)as.number;
        }

        ASSERT(type == Type::Null);
        return (T)0xDEADBEEF;
    }

    static Value CreateValue() { return Value{.type = Type::Undefined}; }
    static Value CreateValue(NullType)
    {
        return Value{
            .as{.integer = (int)0xDEADBEEF},
            .type = Type::Null,
        };
    }
    static Value CreateValue(bool value)
    {
        return Value{
            .as{.boolean = value},
            .type = Type::Bool,
        };
    }
    static Value CreateValue(int value)
    {
        return Value{
            .as{.integer = value},
            .type = Type::Integer,
        };
    }
    static Value CreateValue(double value)
    {
        return Value{
            .as{.number = value},
            .type = Type::Number,
        };
    }
    static Value CreateValue(const char *begin, const char *end)
    {
        const size_t length = end - begin;
        return Value{
            .as{
                .object = ObjectString::Create(begin, end),
            },
            .type = Type::Number,
        };
    }

    Value operator-() const
    {
        switch (type)
        {
            case Value::Type::Number:
                return CreateValue(-as.number);
            case Value::Type::Integer:
                return CreateValue(-as.integer);
            default:
                ASSERT(false);
                return CreateValue();
        }
        return *this;
    }

    Value operator-(const Value &a)
    {
        switch (a.type)
        {
            case Value::Type::Number:
                return CreateValue(-a.as.number);
            case Value::Type::Integer:
                return CreateValue(-a.as.integer);
            default:
                ASSERT(false);
                return CreateValue();
        }
    }
};
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
        case Value::Type::Null:
            return false;
        default:
            ASSERT(false);
            return false;
    }
}

#define DECL_OPERATOR(OP)                                                \
    Value operator OP(const Value &a, const Value &b)                    \
    {                                                                    \
        switch (a.type)                                                  \
        {                                                                \
            case Value::Type::Number:                                    \
                return Value::CreateValue(a.as.number OP b.as.number);   \
            case Value::Type::Integer:                                   \
                return Value::CreateValue(a.as.integer OP b.as.integer); \
            default:                                                     \
                ASSERT(false);                                           \
                return Value::CreateValue();                             \
        }                                                                \
    }

DECL_OPERATOR(+)
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)

////////////////////////////

template <typename T>
struct RandomAccessContainer
{
    size_t getSize() const { return _values.size(); }
    const T *getValues() const { return _values.data(); }
    const T &getValue(size_t index) const
    {
        ASSERT(index < _values.size());
        return _values[index];
    }

    const T &operator[](size_t index) const { return getValue(index); }

    void write(T value) { _values.push_back(value); }

   protected:
    std::vector<T> _values;
};
using ValueArray = RandomAccessContainer<Value>;

void print(const Object *obj)
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

void print(const Value &value)
{
    switch (value.type)
    {
        case Value::Type::Bool:
            printf(value.as.boolean ? "true" : "false");
            break;
        case Value::Type::Null:
            printf("null");
            break;
        case Value::Type::Number:
            printf("%.2f", value.as.number);
            break;
        case Value::Type::Integer:
            printf("%d", value.as.integer);
            break;
        case Value::Type::Object:
            print(value.as.object);
            break;
        default:
            ASSERT(false);
            printf("UNDEFINED");
            break;
    }
}
