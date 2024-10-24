#pragma once

#include "utils/common.h"
#include "value.h"

struct Environment
{
    bool Init(Environment *parentEnvironment)
    {
        ASSERT(_dict.empty());
        _parentEnvironment = parentEnvironment;
        return true;
    }
    bool Reset()
    {
        _dict.clear();
        _parentEnvironment = nullptr;
        return true;
    }
    size_t getVariableCount() const {
        return _dict.size();
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
        if (_parentEnvironment != nullptr)
        {
            return _parentEnvironment->findVariable(varName);
        }
        return nullptr;
    }
    void print() const
    {
        for (const auto &it : _dict)
        {
            printf("%s=[", it.first.c_str());
            printValue(it.second);
            printf("]");
        }
        printf("\n");
    }

    std::unordered_map<std::string, Value> _dict;
    Environment                           *_parentEnvironment = nullptr;
};
