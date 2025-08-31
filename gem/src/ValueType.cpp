#include "ValueType.h"

ValueType::ValueType()
{
    // default ctor
}

ValueType::ValueType(const std::vector<std::string> &v)
{
    __contents.clear();
    for(auto &i: v)
        __contents.push_back(i);
}

ValueType::~ValueType()
{
    __contents.clear();
}
