#pragma once
#include "common.h"
#include <vector>

using Value = double;

template<typename T>
struct RandomAccessContainer {
    size_t getSize() const { return _values.size(); }
    const T* getValues() const { return _values.data(); }
    const T& getValue(size_t index) const { assert(index < _values.size()); return _values[index]; }

    const T& operator[](size_t index) const { return getValue(index); }

    void write(T value) {
        _values.push_back(value);
    }
protected:
    std::vector<T> _values;
};
using ValueArray = RandomAccessContainer<Value>;

void print(Value value) {
    printf("%g", value);
}
