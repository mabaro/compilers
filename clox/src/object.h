#pragma once

#include "value.h"
#include "chunk.h"
#include "utils/memory.h"

struct ObjectString;
struct ObjectFunction;

struct ObjectFunction
{
    Chunk* chunk;
};

struct Object
{
#if USING(DEBUG_BUILD)
    union
    {
        Object         *obj;
        ObjectString   *string;
        ObjectFunction *function;
    } as;
#endif  // #if USING(DEBUG_BUILD)

    enum class Type : uint8_t
    {
        String,
        Function,
        //
        Undefined,
        COUNT = Undefined,
    };
    Type               type = Type::Undefined;
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
        else if (std::is_same_v<ObjectFunction, ObjectT>)
        {
            newObject       = ALLOCATE_FLEX(ObjectT, flexibleSize);
            newObject->type = ObjectT::Type::Function;
#if USING(DEBUG_BUILD)
            newObject->as.obj = newObject;
#endif  // #if USING(DEBUG_BUILD)
        }
        else
        {
            FAIL_MSG("Unsupported type: %s", typeid(ObjectT()).name());
        }

        ////////////////////////////////////////////////////////////////////////////////
        if (newObject)
        {
            newObject->_allocatedNext = s_allocatedList;
            s_allocatedList           = newObject;
        }  ////////////////////////////////////////////////////////////////////////////////
        return newObject;
    }

    inline ObjectString *asString()
    {
        ASSERT(type == Type::String);
        return (ObjectString *)this;
    }

    inline const ObjectString *asString() const
    {
        ASSERT(type == Type::String);
        return (ObjectString *)this;
    }
    inline ObjectFunction* asFunction()
    {
        ASSERT(type == Type::Function);
        return (ObjectFunction*)this;
    }

    inline const ObjectFunction* asFunction() const
    {
        ASSERT(type == Type::Function);
        return (ObjectFunction*)this;
    }

    static void FreeObjects();

    static bool compare(const Object *a, const Object *b);
    Object* operator+(const Object& other) const;

   protected:
    static void FreeObject(Object *obj);

    Object        *_allocatedNext = nullptr;
    static Object *s_allocatedList;
};

///////////////////////////////////////////////////////////////////////////////////////

struct ObjectString : public Object
{
    size_t length = 0;
    char  *chars  = nullptr;

    Result<void> serialize(std::ostream &o_stream) const;
    Result<void> deserialize(std::istream &i_stream);

    static ObjectString *CreateEmpty();
    static ObjectString *CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2);
    static ObjectString *CreateByCopy(const char *str, size_t length);

    static bool compare(const ObjectString &a, const ObjectString &b);
};
