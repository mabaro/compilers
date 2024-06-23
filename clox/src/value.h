#pragma once
#include "common.h"
#include <vector>

struct Value
{
protected:
    union {
        bool boolean;
        double number;
        int integer;
    };

public:
    enum class Type {
        Bool,
        Null,
        Float,
        Integer,
        Undefined,
        COUNT = Undefined
    };
    Type type = Type::Undefined;

    static constexpr struct NullType {} Null = NullType{};
    bool isNumber() const { return type != Type::Bool && type != Type::Null; }
    bool isFalsey() const { return type == Type::Null || (type == Type::Bool && as<bool>() == false); }

public:
    Value() : type(Type::Undefined) {}
    explicit Value(NullType) : type(Type::Null) {}
    explicit Value(bool value) : boolean(value), type(Type::Bool) {}
    explicit Value(int value) : integer(value), type(Type::Integer) {}
    explicit Value(double value) : number(value), type(Type::Float) {}

    explicit operator int() const { 
        assert(type == Type::Integer);
        return integer;
    }
    explicit operator bool() const { 
        assert(type == Type::Bool);
        return boolean;
    }
    explicit operator double() const { 
        assert(type == Type::Float);
        return number;
    }

    template<typename T>
    T as() const {
        assert(type != Type::Undefined);
        if (std::is_same_v<T, bool>)
        {
            assert(type == Type::Bool);
            return (T)boolean;
        }
        else if (std::is_integral_v<T>)
        {
            assert(type == Type::Integer);
            return (T)integer;
        }
        else if (std::is_floating_point_v<T>)
        {
            assert(type == Type::Float);
            return (T)number;
        }

        assert(type == Type::Null);
        return (T)0xDEADBEEF;
    }
};
Value operator-(const Value& a)
{
    switch (a.type)
    {
    case Value::Type::Float:
        return Value(-a.as<double>());
    case Value::Type::Integer:
        return Value(-a.as<int>());
    default:
        assert(false);
        return Value();
    }
}

bool operator==(const Value& a, const Value& b)
{
    if (a.type != b.type)
    {
        return false;
    }
    switch(a.type) {
        case Value::Type::Bool: return a.as<bool>() == b.as<bool>();
        case Value::Type::Float: return a.as<double>() == b.as<double>();
        case Value::Type::Integer: return a.as<int>() == b.as<int>();
        case Value::Type::Null: return true;
        default: assert(false); return false;
    }
}
bool operator<(const Value& a, const Value& b)
{
    assert(a.type == b.type);
    switch(a.type) {
        case Value::Type::Bool: return a.as<bool>() < b.as<bool>();
        case Value::Type::Float: return a.as<double>() < b.as<double>();
        case Value::Type::Integer: return a.as<int>() < b.as<int>();
        case Value::Type::Null: return false;
        default: assert(false); return false;
    }
}
bool operator>(const Value& a, const Value& b)
{
    assert(a.type == b.type);
    switch(a.type) {
        case Value::Type::Bool: return a.as<bool>() > b.as<bool>();
        case Value::Type::Float: return a.as<double>() > b.as<double>();
        case Value::Type::Integer: return a.as<int>() > b.as<int>();
        case Value::Type::Null: return false;
        default: assert(false); return false;
    }
}

#define DECL_OPERATOR(OP)                                               \
    Value operator OP(const Value &a, const Value &b)                  \
    {                                                                   \
        switch (a.type)                                                 \
        {                                                               \
        case Value::Type::Float:                                        \
            return Value(a.as<double>() OP b.as<double>()); \
        case Value::Type::Integer:                                      \
            return Value(a.as<int>() OP b.as<int>());       \
        default:                                                        \
            assert(false);                                              \
            return Value();                                             \
        }                                                               \
    }

DECL_OPERATOR(+)
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)

template<typename T>
struct RandomAccessContainer {
    size_t getSize() const { return _values.size(); }
    const T* getValues() const { return _values.data(); }
    const T& getValue(size_t index) const { assert(index < _values.size()); return _values[index]; }

    const T& operator[](size_t index) const { return getValue(index); }

    void write(T value) {
        _values.push_back(value);
    }
protected:
    std::vector<T> _values;
};
using ValueArray = RandomAccessContainer<Value>;

void print(const Value& value) {
    switch(value.type)
    {
        case Value::Type::Bool: printf(value.as<bool>() ? "true" : "false" ); break;
        case Value::Type::Null: printf("null"); break;
        case Value::Type::Float: printf("%.2f", value.as<double>()); break;
        case Value::Type::Integer: printf("%d", value.as<int>()); break;
        default: assert(false); printf("UNDEFINED"); break;
    }
}
