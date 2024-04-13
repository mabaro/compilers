#pragma once
#include "common.h"
#include <vector>

using Value = double;

struct ValueArray {
    size_t getSize() const { return _values.size(); }
    const Value* getValues() const { return _values.data(); }
    const Value& getValue(size_t index) const { assert(index < _values.size()); return _values[index]; }

    void write(Value value) {
        _values.push_back(value);
    }
protected:
    std::vector<Value> _values;
};

void print(Value value) {
    printf("%g", value);
}
