#include "value.h"
#include "object.h"
#include "utils/serde.h"

Result<void> Value::serialize(std::ostream &o_stream) const
{
    static_assert(sizeof(type) == 1, "Check enum type");
    serde::Serialize(o_stream, type);
    switch (type)
    {
        case Type::Null: break;
        case Type::Bool: serde::Serialize(o_stream, as.boolean); break;
        case Type::Number: serde::Serialize(o_stream, as.number); break;
        case Type::Integer: serde::Serialize(o_stream, as.integer); break;
        case Type::Object: return as.object->serialize(o_stream); break;
        case Type::Undefined: break;
        default: FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}
Result<void> Value::deserialize(std::istream &i_stream)
{
    serde::Deserialize(i_stream, type);
    switch (type)
    {
        case Type::Bool: serde::Deserialize(i_stream, as.boolean); break;
        case Type::Null:
        case Type::Undefined: break;
        case Type::Number: serde::Deserialize(i_stream, as.number); break;
        case Type::Integer: serde::Deserialize(i_stream, as.integer); break;
        case Type::Object:
        {
            auto result = Object::deserialize(i_stream);
            ASSERT(result.isOk());
            if (result.isOk())
            {
                as.object = result.extract();
            }
            else
            {
                return result.error();
            }
            break;
        }
        default: FAIL_MSG("Unsupported type: %d\n", type);
    }
    return Result<void>();
}

const char *Value::getTypeName(Type type)
{
    switch (type)
    {
        case Type::Bool: return "Boolean";
        case Type::Null: return "Null";
        case Type::Number: return "Number";
        case Type::Integer: return "Integer";
        case Type::Object: return "Object";
        default: return "Undefined type";
    }
}
Value::operator bool() const
{
    ASSERT(type == Type::Bool);
    return as.boolean;
}
Value::operator int() const
{
    ASSERT(type == Type::Integer);
    return as.integer;
}
Value::operator double() const
{
    ASSERT(type == Type::Number);
    return as.number;
}
Value::operator char *() const
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

Value Value::Create() { return Value{.type = Type::Undefined}; }
Value Value::Create(NullType)
{
    return Value{
        .as{.integer = (int)0xDEADBEEF},
        .type = Type::Null,
    };
}
Value Value::Create(bool value)
{
    return Value{
        .as{.boolean = value},
        .type = Type::Bool,
    };
}
Value Value::Create(int value)
{
    return Value{
        .as{.integer = value},
        .type = Type::Integer,
    };
}
Value Value::Create(double value)
{
    return Value{
        .as{.number = value},
        .type = Type::Number,
    };
}

Value Value::Create(Object* object) {
    return Value{
        .as{
            .object = object,
        },
        .type = Type::Object,
    };
}

Value Value::CreateConcat(const char *str1, size_t len1, const char *str2, size_t len2)
{
    return Value{
        .as{
            .object = ObjectString::CreateConcat(str1, len1, str2, len2),
        },
        .type = Type::Object,
    };
}
Value Value::CreateByCopy(const char *str, size_t length)
{
    return Value{
        .as{
            .object = ObjectString::CreateByCopy(str, length),
        },
        .type = Type::Object,
    };
}

Value Value::operator-() const
{
    switch (type)
    {
        case Value::Type::Number: return Create(-as.number);
        case Value::Type::Integer: return Create(-as.integer);
        default: ASSERT(false); return Create();
    }
}

Value Value::operator-(const Value &a)
{
    switch (a.type)
    {
        case Value::Type::Number: return Create(-a.as.number);
        case Value::Type::Integer: return Create(-a.as.integer);
        default: ASSERT(false); return Create();
    }
}

bool operator==(const Value &a, const Value &b)
{
    if (a.type != b.type)
    {
        return false;
    }
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean == b.as.boolean;
        case Value::Type::Number: return a.as.number == b.as.number;
        case Value::Type::Integer: return a.as.integer == b.as.integer;
        case Value::Type::Object: return Object::compare(a.as.object, b.as.object);
        case Value::Type::Null: return true;
        default: ASSERT(false); return false;
    }
}
bool operator<(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean < b.as.boolean;
        case Value::Type::Number: return a.as.number < b.as.number;
        case Value::Type::Integer: return a.as.integer < b.as.integer;
        case Value::Type::Null: return false;
        case Value::Type::Object:
            switch (a.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strA = static_cast<char *>(a);
                    auto strB = static_cast<char *>(b);
                    return strcmp(strA, strB) < 0;
                }
                default: FAIL_MSG("Not implemented"); return false;
            }
        default: ASSERT(false); return false;
    }
}
bool operator>(const Value &a, const Value &b)
{
    ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Bool: return a.as.boolean > b.as.boolean;
        case Value::Type::Number: return a.as.number > b.as.number;
        case Value::Type::Integer: return a.as.integer > b.as.integer;
        case Value::Type::Object:
            switch (a.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strA = static_cast<char *>(a);
                    auto strB = static_cast<char *>(b);
                    return strcmp(strA, strB) > 0;
                }
                default: FAIL_MSG("Not implemented"); return false;
            }
        case Value::Type::Null: return false;
        default: ASSERT(false); return false;
    }
}

#define DECL_OPERATOR(OP)                                                                  \
    Value operator OP(const Value &a, const Value &b)                                      \
    {                                                                                      \
        switch (a.type)                                                                    \
        {                                                                                  \
            case Value::Type::Number: return Value::Create(a.as.number OP b.as.number);    \
            case Value::Type::Integer: return Value::Create(a.as.integer OP b.as.integer); \
            default: ASSERT(false); return Value::Create();                                \
        }                                                                                  \
    }
DECL_OPERATOR(-)
DECL_OPERATOR(*)
DECL_OPERATOR(/)
#undef DECL_OPERATOR

Value operator+(const Value&a, const Value &b)
{
    //ASSERT(a.type == b.type);
    switch (a.type)
    {
        case Value::Type::Number: 
            if (b.is(Value::Type::Number))
            {
                return Value::Create(a.as.number + b.as.number);
            }
            break;
        case Value::Type::Integer:
            if (b.is(Value::Type::Integer))
            {
                return Value::Create(a.as.integer + b.as.integer);
            }
            break;
        case Value::Type::Object:
        {
            if (b.is(Value::Type::Object))
            {
                auto result = *a.as.object + *b.as.object;
                if (result != nullptr)
                {
                    return Value::Create(result);
                }
            }
            return Value{};
            break;
        }
        default: FAIL();
    }
 
    FAIL_MSG("Undefined '%s' for Values of types: %s and %s", __FUNCTION__, Value::getTypeName(a.type),
             Value::getTypeName(b.type));
    return Value{};
}

////////////////////////////////////////////////////////////////////////////////////

[[maybe_unused]] static void print(const Object *obj)
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
        default: FAIL_MSG("Unexpected Object type(%d)", obj->type);
    }
}

void printValue(const Value &value)
{
    switch (value.type)
    {
        case Value::Type::Bool: printf("%s", value.as.boolean ? "true" : "false"); break;
        case Value::Type::Null: printf("%s", "null"); break;
        case Value::Type::Number: printf("%.2f", value.as.number); break;
        case Value::Type::Integer: printf("%d", value.as.integer); break;
        case Value::Type::Object:
            switch (value.as.object->type)
            {
                case Object::Type::String:
                {
                    auto strObj = *value.as.object->asString();
                    printf("%.*s", (int)strObj.length, strObj.chars);
                    break;
                }
                default: FAIL_MSG("UNDEFINED OBJECT TYPE(%d)", value.as.object->type);
            }
            break;
        default: printf("UNDEF"); break;
    }
}

void printValueDebug(const Value &value)
{
    const bool isString = value.type == Value::Type::Object && value.as.object->type == Object::Type::String;
    if (!isString)
    {
        printValue(value);
    }
    else
    {
        auto        strObj     = *value.as.object->asString();
        const char *charPtr    = strObj.chars;
        const char *charPtrEnd = strObj.chars + strObj.length;
        while (charPtr != charPtrEnd)
        {
            if (*charPtr != '\n')
            {
                putc(*charPtr, stdout);
            }
            else
            {
                putc('\\', stdout);
                putc('n', stdout);
            }
            ++charPtr;
        }
    }
}
