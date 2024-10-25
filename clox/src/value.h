#pragma once

#include "utils/common.h"

#define ALLOCATE(Type) (Type *)malloc(sizeof(Type))
#define ALLOCATE_FLEX(Type, ExtraSize) (Type *)malloc(sizeof(Type) + ExtraSize)
#define ALLOCATE_N(Type, count) (Type *)malloc(sizeof(Type) * count)
#define DEALLOCATE(Type, pointer) free(pointer)
#define DEALLOCATE_N(Type, pointer, N) free(pointer)

struct ObjectString;

struct Object
{
#if USING(DEBUG_BUILD)
    union
    {
        Object* obj;
        ObjectString *string;
    } as;
#endif  // #if USING(DEBUG_BUILD)

    enum class Type : uint8_t
    {
        String,
        COUNT
    };
    Type               type = Type::COUNT;
    static const char *getTypeName(Type type);

    Result<void>            serialize(std::ostream &o_stream) const;
    static Result<Object *> deserialize(std::istream &i_stream);

    template <typename ObjectT>
    static ObjectT *allocate(size_t flexibleSize = 0)
    {
        static_assert(std::is_same_v<ObjectT, ObjectString>, "ObjectT not supported");

        ObjectT *newObject = nullptr;
        if (std::is_same_v<ObjectString, ObjectT>)
        {
            newObject       = ALLOCATE_FLEX(ObjectT, flexibleSize);
            newObject->type = ObjectT::Type::String;
#if USING(DEBUG_BUILD)
            newObject->as.obj = newObject;
#endif  // #if USING(DEBUG_BUILD)
        }
        ////////////////////////////////////////////////////////////////////////////////
        if (newObject)
        {
            newObject->_allocatedNext = s_allocatedList;
            s_allocatedList           = newObject;
        }  ////////////////////////////////////////////////////////////////////////////////
        return newObject;
    }

    ObjectString *asString()
    {
        ASSERT(type == Type::String);
        return (ObjectString *)this;
    }
    const ObjectString *asString() const
    {
        ASSERT(type == Type::String);
        return (ObjectString *)this;
    }

    static void FreeObjects();

    static bool compare(const Object *a, const Object *b);

   protected:
    static void FreeObject(Object *obj);

    Object        *_allocatedNext = nullptr;
    static Object *s_allocatedList;
};

struct ObjectString : public Object
{
    size_t length = 0;
    char  *chars  = nullptr;

    Result<void> serialize(std::ostream &o_stream) const;
    Result<void> deserialize(std::istream &i_stream);

    static ObjectString *CreateEmpty();
    static ObjectString* CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2);
    static ObjectString *CreateByCopy(const char *str, size_t length);

    static bool compare(const ObjectString &a, const ObjectString &b);
};

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

Result<Value> operator+(const Object &a, const Object &b);

////////////////////////////

void printValue(const Value &value);
void printValueDebug(const Value &value);
