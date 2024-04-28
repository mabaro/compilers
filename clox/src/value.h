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
        Float,
        Integer,
        Undefined,
        COUNT = Undefined
    };
    Type type = Type::Undefined;

    bool isNumber() const { return type != Type::Bool; }

public:
    Value() : type(Type::Undefined) {}
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
    T getValue() const {
        assert(type != Type::Undefined);
        if (std::is_integral_v<T>)
        {
            assert(type == Type::Integer);
            return (T)integer;
        }
        if (std::is_floating_point_v<T>)
        {
            assert(type == Type::Float);
            return (T)number;
        }

        assert(type == Type::Bool);
        return (T)boolean;
    }
};
Value operator-(const Value& a)
{
    switch (a.type)
    {
    case Value::Type::Float:
        return Value(-a.getValue<double>());
    case Value::Type::Integer:
        return Value(-a.getValue<int>());
    default:
        assert(false);
        return Value();
    }
}

#define DECL_OPERATOR(OP)                                               \
    Value operator OP(const Value &a, const Value &b)                  \
    {                                                                   \
        switch (a.type)                                                 \
        {                                                               \
        case Value::Type::Float:                                        \
            return Value(a.getValue<double>() OP b.getValue<double>()); \
        case Value::Type::Integer:                                      \
            return Value(a.getValue<int>() OP b.getValue<int>());       \
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

void print(Value value) {
    printf("%g", value);
}
