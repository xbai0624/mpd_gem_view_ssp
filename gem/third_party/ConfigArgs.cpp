//============================================================================//
// A class to simplify the argument parsing procedure                         //
//                                                                            //
// Chao Peng                                                                  //
// 09/03/2020                                                                 //
//============================================================================//

#include "ConfigArgs.h"
#include "ConfigParser.h"
#include <iostream>
#include <algorithm>



ConfigArgs::ConfigArgs(bool f)
: flexible(f)
{
    // place holder
}

ConfigArgs::~ConfigArgs()
{
    // place holder
}

// add an argument
void ConfigArgs::Add(const std::string &mark, const std::string &dest, bool take_value,
                     const std::string &help, ConfigValue def)
{
    // sanity check
    if (mark.empty()) { return; }

    // auto fill dest name
    std::string name = dest;
    if (name.empty()) {
        const auto nhyphen = mark.find_first_not_of('-');
        name = ConfigParser::str_replace(mark.substr(nhyphen), "-", "_");
    }

    // boolean switch
    if (!take_value) { def = "false"; }

    add_arg(mark, name, take_value, def);
    arg_helps.push_back({name, def.String(), help, take_value, {mark}});
}

// add multiple arguments
void ConfigArgs::Add(const std::vector<std::string> &marks, const std::string &dest, bool take_value,
                     const std::string &help, ConfigValue def)
{
    // sanity check
    if (marks.empty()) { return; }

    // auto fill dest name
    std::string name = dest;
    if (name.empty()) {
        const auto nhyphen = marks.back().find_first_not_of('-');
        name = ConfigParser::str_replace(marks.back().substr(nhyphen), "-", "_");
    }

    // boolean switch
    if (!take_value) { def = "false"; }

    for (auto &mark : marks) { add_arg(mark, name, take_value, def); }
    arg_helps.push_back({name, def.String(), help, take_value, marks});
}

void ConfigArgs::AddPositional(const std::string &name, const std::string &help)
{
    pos_helps.push_back({name, "NO_DEFAULT", help, false, {}});
}

// main function to parse arguments
std::unordered_map<std::string, ConfigValue> ConfigArgs::ParseArgs(int argc, char *argv[])
{
    std::unordered_map<std::string, ConfigValue> res;
    positionals.clear();

    if (argc < 1) {
        throw(std::runtime_error("no argument"));
    }

    // default values
    for (auto &it : arg_map) {
        auto &data = it.second;
        if (data.def.String() != "NO_DEFAULT") {
            res[data.name] = data.def;
        }
    }

    argv0 = argv[0];

    // try to split
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.empty()) {
            continue;
        }

        if ((arg[0] == '-') && (arg.find('=') != std::string::npos)) {
            auto split = ConfigParser::split(arg, "=");
            if (split[0].size()) {  args.emplace_back(std::move(split[0])); }
            if (split[1].size()) {  args.emplace_back(std::move(split[1])); }
        } else {
            args.emplace_back(std::move(arg));
        }
    }

    // parse arguments
    for(size_t i = 0; i < args.size(); ++i) {
        auto &arg = args[i];
        // argument mark
        if (arg[0] == '-') {
            // help message
            if (std::find(helps.begin(), helps.end(), arg) != helps.end()) {
                PrintHelp();
                exit(0);
            }
            // unknown argument
            auto it = arg_map.find(arg);
            if (it == arg_map.end()) {
                std::cerr << "unknown argument " + arg << std::endl;
                PrintInstruction();
                exit(-1);
            }
            auto &data = it->second;
            // take the next value
            if (data.take_value) {
                if (i + 1 >= args.size()) {
                    std::cerr << "argument " + arg + " requires value but no one found" << std::endl;
                    PrintInstruction();
                    exit(-1);
                }
                i++;
                res[data.name] = ConfigValue(args[i]);
                if (arg_map.find(args[i]) != arg_map.end()) {
                    std::cout << "warning: " << arg << " takes a following value "
                              << args[i] << ", which seems to be another argument." << std::endl;
                }
            // switch type
            } else {
                res[data.name] = "true";
            }
        // positional
        } else {
            int i_pos = positionals.size();
            positionals.emplace_back(arg);
            if (i_pos < pos_helps.size()) {
                res[pos_helps[i_pos].name] = arg;
            }
        }
    }

    if (positionals.size() < pos_helps.size()) {
        std::cerr << "lack of positional arguments:";
        for (size_t i = positionals.size(); i < pos_helps.size(); ++i) {
            std::cerr << " [" << pos_helps[i].name << "]";
        }
        std::cerr << "\n";
        PrintInstruction();
        exit(-1);
    }

    if (!flexible && positionals.size() > pos_helps.size()) {
        std::cerr << "required " << pos_helps.size() << " positional arguments, but received "
                  << positionals.size() << ":\n\t[" << positionals[0].String() << "]";
        for (size_t i = 1; i < positionals.size(); ++i) {
            std::cerr << ", [" << positionals[i].String() << "]";
        }
        std::cerr << "\n";
        PrintInstruction();
        exit(-1);
    }

    return res;
}

void ConfigArgs::add_arg(const std::string &mark, const std::string &dest, bool take_value, ConfigValue def)
{
    // check existence
    if (arg_map.find(mark) != arg_map.end()) {
        std::cerr << "ConfigArgs Error: repeated argument mark " << mark << ", abort adding it." << std::endl;
        return;
    }

    // check format
    const auto nhyphen = mark.find_first_not_of('-');
    if (nhyphen >= mark.size() || nhyphen == 0) {
        std::cerr << "ConfigArgs Error: only supports mark begin with -, "
                  << mark << " is not a supported argument mark"
                  << std::endl;
        return;
    }

    arg_map[mark] = ArgData{dest, take_value, def};
}

void ConfigArgs::PrintInstruction()
{
    std::cout << "usage:\n\t" << argv0;

    for (auto &help : arg_helps) {
        std::cout << " [" << ConfigParser::str_join(help.marks, ", ");
        if (help.take_value) { std::cout << " " << ConfigParser::str_upper(help.name); }
        std::cout << "]";
    }

    for (auto &pos : pos_helps) {
        std::cout << " " << ConfigParser::str_upper(pos.name);
    }

    if (flexible) { std::cout << " ..."; }

    if (helps.size()) {
        std::cout << "\n\t" << ConfigParser::str_join(helps, ", ") << ": print help message";
    }

    std::cout << std::endl;
}

void ConfigArgs::PrintHelp()
{
    PrintInstruction();
    std::cout << "arguments: \n";
    for (auto &help: pos_helps) {
        std::cout << "\t" << ConfigParser::str_upper(help.name);
        if (help.message.size()) { std::cout << ": " << help.message; }
        std::cout << "\n";
    }

    if (flexible) {
        std::cout << "\t...";
        if (flexible_help.size()) { std::cout << ": " << flexible_help; }
        std::cout << "\n";
    }

    for (auto &help : arg_helps) {
        std::cout << "\t" << ConfigParser::str_join(help.marks, ", ");
        if (help.take_value) {
            std::cout << " [" << ConfigParser::str_upper(help.name);
            if (help.def != "NO_DEFAULT") {
                std::cout << " (default: " << help.def << ")";
            }
            std::cout << "]";
        }
        std::cout << ": " << help.message << "\n";
    }
}

