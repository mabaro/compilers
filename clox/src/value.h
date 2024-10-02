#pragma once

#include "common.h"

#define ALLOCATE(Type) (Type *)malloc(sizeof(Type))
#define ALLOCATE_N(Type, count) (Type *)malloc(sizeof(Type) * count)
#define DEALLOCATE(Type, pointer) free(pointer)
#define DEALLOCATE_N(Type, pointer, N) free(pointer)

struct ObjectString;

struct Object
{
#if USING(DEBUG_BUILD)
    union
    {
        ObjectString *string;
    } as;
#endif // #if USING(DEBUG_BUILD)

    enum class Type
    {
        String,
        COUNT
    };
    Type type = Type::COUNT;
    static const char *getTypeName(Type type);

    template <typename ObjectT>
    static ObjectT *allocate()
    {
        static_assert(std::is_same_v<ObjectT, ObjectString>, "ObjectT not supported");

        ObjectT *newObject = nullptr;
        if (std::is_same_v<ObjectString, ObjectT>)
        {
            newObject = ALLOCATE(ObjectT);
            newObject->type = ObjectT::Type::String;
        }
        ////////////////////////////////////////////////////////////////////////////////
        if (newObject)
        {
            newObject->_allocatedNext = s_allocatedList;
            s_allocatedList = newObject;
        }  ////////////////////////////////////////////////////////////////////////////////
        return newObject;
    }

    template <typename T>
    static const T *castTo(const Object *obj)
    {
        ASSERT(obj);
        if (obj && T::obj_type == obj->type)
        {
            return static_cast<const T *>(obj);
        }
        FAIL_MSG("Cannot cast %s -> %s", getTypeName(obj->type), getTypeName(T::obj_type));
        return nullptr;
    }
    template <typename T>
    static T *castTo(Object *obj)
    {
        ASSERT(obj);
        if (obj && T::obj_type == obj->type)
        {
            return static_cast<T *>(obj);
        }
        FAIL_MSG("Cannot cast %s -> %s", getTypeName(obj->type), getTypeName(T::obj_type));
        return nullptr;
    }
    ObjectString *asString()
    {
        ASSERT(type == Type::String);
        if (type == Type::String)
        {
            return (ObjectString *)this;
        }
        return nullptr;
    }

    static void FreeObjects();

    static bool compare(const Object *a, const Object *b);

   protected:
    static void FreeObject(Object *obj);

    Object *_allocatedNext = nullptr;
    static Object *s_allocatedList;
};

struct ObjectString : public Object
{
    static constexpr Type obj_type = Type::String;

    size_t length = 0;
    char *chars = nullptr;
    static ObjectString *CreateByMove(char *str, size_t length);
    static ObjectString *CreateByCopy(const char *str, size_t length);

    static bool compare(const ObjectString &a, const ObjectString &b);
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
    static const char *getTypeName(Type type);

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
    static Value CreateByMove(char *ownedStr, size_t length);
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

Result<Value> operator+(const Object &a, const Object &b);

////////////////////////////

void printValue(std::string &oStr, const Value &value);
void printValue(const Value &value);
