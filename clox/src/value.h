#pragma once

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
    static const char *getString(Type type);

    template <typename ObjectT>
    static ObjectT *allocate()
    {
        ObjectT *newObject = ALLOCATE(ObjectT, 1);
        newObject->type = ObjectT::obj_type;
        return newObject;
    }

    template <typename T>
    static const T *as(const Object *obj)
    {
        ASSERT(obj);
        if (obj && T::obj_type == obj->type)
        {
            return static_cast<const T *>(obj);
        }
        FAIL_MSG("Cannot cast %s -> %s", getString(obj->type), getString(T::obj_type));
        return nullptr;
    }
};

struct ObjectString : public Object
{
    static constexpr Type obj_type = Type::String;

    size_t length = 0;
    char *chars = nullptr;
    static ObjectString *Create(const char *begin, const char *end);
    static ObjectString *Create(char *ownedStr, size_t length);
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
    static const char* getString(Type type);

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

    static Value Create();
    static Value Create(NullType);
    static Value Create(bool value);
    static Value Create(int value);
    static Value Create(double value);
    static Value Create(const char *begin, const char *end);
    static Value Create(char *ownedStr, size_t length);

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

Result<Value> operator+(const Object &a, const Object &b);

////////////////////////////

void printValue(const Value &value);
