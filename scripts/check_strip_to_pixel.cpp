

unordered_map<string, vector<pair<int, int>>> apv_strip_to_pixel;
const string apv_name[] = {"APV-A","APV-B","APV-C","APV-D","APV-E","APV-F","APV-G","APV-H"};

std::ostream &operator<<(std::ostream &os, const std::pair<int, int> &p)
{
    os<<"("<<p.first<<","<<p.second<<"),";
    return os;
}

void check_strip_to_pixel(const char* path = "../database/strip_to_pixel.csv")
{
    // init the map
    for(auto &i: apv_name)
    {
        apv_strip_to_pixel[i] = std::vector<std::pair<int, int>>(128, std::pair<int, int>(0, 0));
    }

    fstream input(path, fstream::in);
    string line;

    auto trim = [&](string &line)
    {
        size_t first = line.find_first_not_of(' ');
        if( first == std::string::npos)
            line="";

        size_t last = line.find_last_not_of(' ');
        line = line.substr(first, (last-first+1));
    };

    auto parse_line = [&](const string &line) -> vector<string>
    {
        vector<string> res;
        istringstream iss(line);
        string tmp;

        while(iss >> tmp) {
            trim(tmp);
            res.push_back(tmp);
        }

        return res;
    };

    auto string_to_pair = [&](string &line) -> std::pair<int, int>
    {
        // (34,45),
        const std::string delim = "(),";
        for(const char &c: delim) {
            std::replace(line.begin(), line.end(), c, ' ');
        }

        auto tmp = parse_line(line);

        if(tmp.size() != 2) {
            std::cout<<"ERROR:: Failed parsing strip to pixel map."<<std::endl;
            exit(0);
        }

        int r = std::stoi(tmp[0]), c = std::stoi(tmp[1]);

        return std::pair<int,int>(r, c);
    };

    while(std::getline(input, line))
    {
        trim(line);
        if(line[0] == '#')
            continue;
        auto v = parse_line(line);

        if(v.size() != 9) {
            std::cout<<"-ERROR- Failed parsing strip to pixel map: "<<v.size()<<" [] "<<line<<std::endl;
            exit(0);
        }

        std::string ch_t = v[0].substr(0, v[0].size() - 1);
        int channel = std::stoi(ch_t);

        for(size_t i=1; i<v.size(); i++) {

            auto p = string_to_pair(v[i]);

            (apv_strip_to_pixel[apv_name[i-1]])[channel] = p;
        }
    }

    for(auto &i: apv_strip_to_pixel)
    {
        cout<<i.first<<endl;
        for(auto &j: i.second)
            cout<<j;
        cout<<endl;
    }
}
