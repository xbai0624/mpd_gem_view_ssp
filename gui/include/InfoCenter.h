#ifndef INFO_CENTER_H
#define INFO_CENTER_H

#include <string>

class InfoCenter
{
public:
    ~InfoCenter();

    static InfoCenter* Instance() {
        if(_instance == nullptr)
            _instance = new InfoCenter();
        return _instance;
    }

    int ParseRunNumber(const std::string &str);

private:
    static InfoCenter* _instance;
    InfoCenter(){}
};

#endif
