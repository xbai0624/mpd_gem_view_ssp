#ifndef VALUETYPE_H
#define VALUETYPE_H

#include <vector>
#include <string>

class ValueType
{
    public:
        ValueType();
        ValueType(const std::vector<std::string> &);
        ~ValueType();

        template<typename T>
            typename std::enable_if<std::is_same<T, int>::value, T>::type val() const
            {
                int res = 0;
                if(__contents.size() <= 0)
                    return res;

                std::string tmp = __contents[0];
                res = stoi(tmp);
                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, bool>::value, T>::type val() const
            {
                bool res = false;
                if(__contents.size() <= 0)
                    return res;

                std::string tmp = __contents[0];
                if(tmp == "true")
                    res = true;

                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, float>::value, T>::type val() const
            {
                float res = 0;
                if(__contents.size() <= 0)
                    return res;

                std::string tmp = __contents[0];
                res = stod(tmp);
                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, double>::value, T>::type val() const
            {
                double res = 0;
                if(__contents.size() <= 0)
                    return res;

                std::string tmp = __contents[0];
                res = stod(tmp);
                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, float>::value, std::vector<float>>::type arr() const
            {
                std::vector<float> res;
                if(__contents.size() <= 0)
                    return res;

                for(auto &i: __contents)
                    res.push_back(stod(i));
                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, double>::value, std::vector<double>>::type arr() const
            {
                std::vector<double> res;
                if(__contents.size() <= 0)
                    return res;

                for(auto &i: __contents)
                    res.push_back(stod(i));
                return res;
            }

        template<typename T>
            typename std::enable_if<std::is_same<T, int>::value, std::vector<int>>::type arr() const
            {
                std::vector<int> res;
                if(__contents.size() <= 0)
                    return res;

                for(auto &i: __contents)
                    res.push_back(stod(i));
                return res;
            }

    private:
        std::vector<std::string> __contents;
};

#endif
