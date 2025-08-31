#ifndef CONFIG_ARGS_H
#define CONFIG_ARGS_H

#include "ConfigValue.h"
#include <vector>
#include <string>
#include <unordered_map>

class ConfigArgs
{
public:
    struct ArgData
    {
        std::string name;
        bool take_value;
        ConfigValue def;
    };

    struct ArgHelp
    {
        std::string name,def, message;
        bool take_value;
        std::vector<std::string> marks;
    };

    ConfigArgs(bool flexible_positionals = false);
    virtual ~ConfigArgs();

    // add an argument
    void Add(const std::string &mark, const std::string &dest, bool take_value,
             const std::string &help, ConfigValue def);
    // add multiple arguments
    void Add(const std::vector<std::string> &marks, const std::string &dest, bool take_value,
             const std::string &help, ConfigValue def);


    // add arg without value
    void AddSwitch(const std::string &mark, const std::string &dest, const std::string &help)
    {
        Add(mark, dest, false, help, ConfigValue("NO_DEFAULT"));
    }
    void AddSwitches(const std::vector<std::string> &marks, const std::string &dest, const std::string &help)
    {
        Add(marks, dest, false, help, ConfigValue("NO_DEFAULT"));
    }

    // add arg with value
    template<typename T>
    void AddArg(const std::string &mark, const std::string &dest, const std::string &help, T def)
    {
        Add(mark, dest, true, help, ConfigValue(def));
    }
    template<typename T>
    void AddArgs(const std::vector<std::string> &marks, const std::string &dest, const std::string &help, T def)
    {
        Add(marks, dest, true, help, ConfigValue(def));
    }

    // add positional arguments
    void AddPositional(const std::string &name, const std::string &help = "");
    void AddPositionals(const std::vector<std::string> &names, const std::string &help = "")
    {
        for (auto &name : names) { AddPositional(name, help); }
    }
    void SetFlexiblePositionals(const std::string &help) { flexible = true; flexible_help = help; }

    // add help marks
    void AddHelp(const std::string &h) { helps.push_back(h); };
    void AddHelps(const std::vector<std::string> &hs) { helps.insert(helps.end(), hs.begin(), hs.end()); };

    // parse
    std::unordered_map<std::string, ConfigValue> ParseArgs(int argc, char *argv[]);

    const std::vector<ConfigValue> &GetPositionals() const { return positionals; }
    void Clear()
    {
        helps.clear(); flexible_help.clear();
        argv0.clear(); positionals.clear(); pos_helps.clear();
        arg_map.clear(); arg_helps.clear();
    }

    void PrintInstruction();
    void PrintHelp();

private:
    void add_arg(const std::string &mark, const std::string &dest, bool take_value, ConfigValue def);

    bool flexible;
    std::string argv0, flexible_help;
    std::vector<std::string> helps;
    std::vector<ArgHelp> pos_helps;
    std::vector<ConfigValue> positionals;
    std::unordered_map<std::string, ArgData> arg_map;
    std::vector<ArgHelp> arg_helps;
};


#endif // CONFIG_ARGS_H
