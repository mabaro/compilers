#pragma once

#include <iostream>

namespace serialize
{
using size_t = uint8_t;

template <typename T, size_t TSIZE=sizeof(T)>
void Serialize(std::ostream& ostr, const T& i_value)
{
    ostr.write((char*)&i_value, TSIZE);
}
template <typename AS_T, typename T, size_t TSIZE=sizeof(AS_T)>
void SerializeAs(std::ostream& ostr, const T& i_value)
{
    AS_T temp = static_cast<AS_T>(i_value);
    ostr.write((char*)&temp, TSIZE);
}
template <typename T, typename COUNT_T=uint32_t>
void SerializeN(std::ostream& ostr, const T* i_value, COUNT_T count)
{
    ostr.write((char*)i_value, count * sizeof(T));
}

template <typename T, size_t TSIZE=sizeof(T)>
void Deserialize(std::istream& istr, T& o_value)
{
    istr.read((char*)&o_value, TSIZE);
}
template <typename AS_T, typename T, size_t TSIZE=sizeof(AS_T)>
void DeserializeAs(std::istream& istr, T& o_value)
{
    AS_T temp;
    istr.read((char*)&temp, TSIZE);
    o_value = static_cast<T>(temp);
}
template <typename T, typename COUNT_T=uint32_t>
void DeserializeN(std::istream& istr, T* o_value, COUNT_T count)
{
    istr.read((char*)o_value, count * sizeof(T));
}

}  // namespace serialize