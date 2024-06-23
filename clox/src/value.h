#pragma once
#include "common.h"
#include <vector>

struct Object
{
    enum class Type
    {
        String,
        COUNT
    };
    Type type = Type::COUNT;
};

struct ObjectString : public Object
{
    int length = 0;
    char *chars = nullptr;
};

struct Value
{
    union
    {
        bool boolean;
        double number;
        int integer;
        Object *object;
    } valueAs;

    enum class Type
    {
        Bool,
        Null,
        Number,
        Integer,
        Undefined,
        COUNT = Undefined
    };
    Type type = Type::Undefined;


    static constexpr struct NullType {} Null = NullType{};

    bool is(Type t) const { return t == type; }
    bool isNumber() const { return type == Type::Integer || type == Type::Number; }
    bool isFalsey() const { return type == Type::Null || (type == Type::Bool && as<bool>() == false); }

    explicit operator bool() const
    {
        ASSERT(type == Type::Bool);
        return valueAs.boolean;
    }
    explicit operator int() const
    {
        ASSERT(type == Type::Integer);
        return valueAs.integer;
    }
    explicit operator double() const
    {
        ASSERT(type == Type::Number);
        return valueAs.number;
    }

    template <typename T>
    T as() const
    {
        ASSERT(type != Type::Undefined);
        if (std::is_same_v<T, bool>)
        {
            ASSERT(type == Type::Bool);
            return (T)valueAs.boolean;
        }
        else if (std::is_integral_v<T>)
        {
            ASSERT(type == Type::Integer);
            return (T)valueAs.integer;
        }
        else if (std::is_floating_point_v<T>)
        {
            ASSERT(type == Type::Number);
            return (T)valueAs.number;
        }

        ASSERT(type == Type::Null);
        return (T)0xDEADBEEF;
    }

    static Value CreateValue()
    {
        return Value{.type = Type::Undefined};
    }
    static Value CreateValue(NullType)
    {
        return Value{
            .valueAs{.integer = (int)0xDEADBEEF},
            .type = Type::Null,
        };
    }
    static Value CreateValue(bool value)
    {
        return Value{
            .valueAs{.boolean = value},
            .type = Type::Bool,
        };
    }
    static Value CreateValue(int value)
    {
        return Value{
            .valueAs{.integer = value},
            .type = Type::Integer,
        };
    }
    static Value CreateValue(double value)
    {
        return Value{
            .valueAs{.number = value},
            .type = Type::Number,
        };
    }

    Value operator-() const
    {
        switch (type)
        {
        case Value::Type::Number:
            return CreateValue(-valueAs.number);
        case Value::Type::Integer:
            return CreateValue(-valueAs.integer);
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
            return CreateValue(-a.as<double>());
        case Value::Type::Integer:
            return CreateValue(-a.as<int>());
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
        return a.as<bool>() == b.as<bool>();
    case Value::Type::Number:
        return a.as<double>() == b.as<double>();
    case Value::Type::Integer:
        return a.as<int>() == b.as<int>();
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
        return a.as<bool>() < b.as<bool>();
    case Value::Type::Number:
        return a.as<double>() < b.as<double>();
    case Value::Type::Integer:
        return a.as<int>() < b.as<int>();
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
        return a.as<bool>() > b.as<bool>();
    case Value::Type::Number:
        return a.as<double>() > b.as<double>();
    case Value::Type::Integer:
        return a.as<int>() > b.as<int>();
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
        case Value::Type::Number:                                        \
            return Value::CreateValue(a.as<double>() OP b.as<double>()); \
        case Value::Type::Integer:                                       \
            return Value::CreateValue(a.as<int>() OP b.as<int>());       \
        default:                                                         \
            ASSERT(false);                                               \
            return Value::CreateValue();                                 \
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

    void write(T value)
    {
        _values.push_back(value);
    }

protected:
    std::vector<T> _values;
};
using ValueArray = RandomAccessContainer<Value>;

void print(const Value &value)
{
    switch (value.type)
    {
    case Value::Type::Bool:
        printf(value.as<bool>() ? "true" : "false");
        break;
    case Value::Type::Null:
        printf("null");
        break;
    case Value::Type::Number:
        printf("%.2f", value.as<double>());
        break;
    case Value::Type::Integer:
        printf("%d", value.as<int>());
        break;
    default:
        ASSERT(false);
        printf("UNDEFINED");
        break;
    }
}
