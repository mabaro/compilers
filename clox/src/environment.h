#pragma once

#include "utils/common.h"
#include "value.h"

struct Environment
{
    bool Init()
    {
        ASSERT(_dict.empty());
        return true;
    }
    bool Reset()
    {
        _dict.clear();
        return true;
    }
    Value *addVariable(const char *varName)
    {
        auto itPair = _dict.insert({varName, Value{}});
        ASSERT_MSG(itPair.second == true, "Trying to add variable(%s) twice", varName);
        return &(itPair.first->second);
    }
    bool removeVariable(const char *varName)
    {
        auto varIt = _dict.find(varName);
        if (varIt != _dict.end())
        {
            _dict.erase(varIt);
            return true;
        }

        FAIL_MSG("Variable '%s' not defined in this environment", varName);
        return false;
    }
    Value *findVariable(const char *varName)
    {
        auto varIt = _dict.find(varName);
        if (varIt != _dict.end())
        {
            return &varIt->second;
        }
        return nullptr;
    }
    void print() const
    {
        for (const auto it : _dict)
        {
            printf("%s=[", it.first.c_str());
            printValue(it.second);
            printf("]");
        }
        printf("\n");
    }
    std::unordered_map<std::string, Value> _dict;
};
