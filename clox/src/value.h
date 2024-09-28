#pragma once

#include <vector>

#include "common.h"

#define ALLOCATE(Obj, count) (Obj *)malloc(sizeof(Obj) * count)

struct ObjectString;

struct Object
{
    enum class Type
    {
        String,
        COUNT
    };
    Type type = Type::COUNT;

    template <typename ObjectT>
    static ObjectT *allocate()
    {
        static_assert(std::is_same_v<ObjectT, ObjectString>, "ObjectT not supported");

        ObjectT *newObject = nullptr;
        if (std::is_same_v<ObjectString, ObjectT>)
        {
            newObject = ALLOCATE(ObjectT, 1);
            newObject->type = Type::String;
        }
        return newObject;
    }

    ObjectString* asString() {
        ASSERT( type == Type::String);
        if ( type == Type::String)
        {
            return (ObjectString*)this;
        }
        return nullptr;
    }
};

struct ObjectString : public Object
{
    size_t length = 0;
    char *chars = nullptr;
    static ObjectString *Create(const char *begin, const char *end);
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

    explicit operator bool() const;
    explicit operator int() const;
    explicit operator double() const;
    explicit operator char *() const;

    static Value Create();
    static Value Create(NullType);
    static Value Create(bool value);
    static Value Create(int value);
    static Value Create(double value);
    static Value Create(const char *begin, const char *end);

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

void print(const Object *obj);

void printValue(std::string& oStr, const Value &value);
void printValue(const Value &value);
