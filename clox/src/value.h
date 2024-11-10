#pragma once

#include "utils/common.h"

struct Object;

struct Value
{
    union
    {
        bool    boolean;
        double  number;
        int     integer;
        Object *object;
    } as;

    enum class Type : uint8_t
    {
        Null,
        Bool,
        Number,
        Integer,
        Object,
        Undefined,
        COUNT = Undefined
    };
    Type               type = Type::Undefined;
    static const char *getTypeName(Type type);

    static constexpr struct NullType
    {
    } Null = NullType{};

    Result<void> serialize(std::ostream &o_stream) const;
    Result<void> deserialize(std::istream &i_stream);

    bool is(Type t) const { return t == type; }

    bool isNumber() const { return type == Type::Integer || type == Type::Number; }

    bool isFalsey() const { return type == Type::Null || (type == Type::Bool && as.boolean == false); }

    explicit operator bool() const;
    explicit operator int() const;
    explicit operator double() const;
    explicit operator char *() const;

    static Value Create();
    static Value Create(NullType);
    static Value Create(bool value);
    static Value Create(int value);
    static Value Create(double value);
    // Object
    static Value Create(Object *object);
    static Value CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2);
    static Value CreateByCopy(const char *begin, size_t length);

    Value operator-() const;
    Value operator-(const Value &a);
};

bool operator==(const Value &a, const Value &b);
bool operator<(const Value &a, const Value &b);
bool operator>(const Value &a, const Value &b);

#define DECL_OPERATOR(OP) Value operator OP(const Value &a, const Value &b);
DECL_OPERATOR(+)
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)
#undef DECL_OPERATOR

////////////////////////////

void printValue(const Value &value);
void printValueDebug(const Value &value);
