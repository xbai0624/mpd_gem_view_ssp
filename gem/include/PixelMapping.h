#ifndef PIXEL_MAP_H
#define PIXEL_MAP_H

#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

namespace pixel {

    // this is a temporary solution
    const std::string apv_name[] = {"APV-A","APV-B","APV-C","APV-D","APV-E","APV-F","APV-G","APV-H"};
    const int apv_plane[]        = {      0,      0,      0,      0,      1,      1,      1,      1};
    const int apv_pos[]          = {      0,      1,      2,      3,      0,      1,      2,      3};
 
    class PixelMapping {
        public:
            PixelMapping(const PixelMapping &) = delete;
            PixelMapping & operator=(const PixelMapping&) = delete;

            static PixelMapping& Instance() {
                static PixelMapping instance;
                instance.Load();
                return instance;
            }

            void Load(const char* path = "database/strip_to_pixel.csv")
            {
                if(is_loaded) return; // no need to reload the text file

                // init the map
                for(auto &i: apv_name)
                {
                    socket_pin_to_pixel[i] = std::vector<std::pair<int, int>>(128, std::pair<int, int>(0, 0));
                }

                std::fstream input(path, std::fstream::in);
                std::string line;

                auto trim = [&](std::string &line)
                {
                    size_t first = line.find_first_not_of(' ');
                    if( first == std::string::npos)
                        line="";

                    size_t last = line.find_last_not_of(' ');
                    line = line.substr(first, (last-first+1));
                };

                auto parse_line = [&](const std::string &line) -> std::vector<std::string>
                {
                    std::vector<std::string> res;
                    std::istringstream iss(line);
                    std::string tmp;

                    while(iss >> tmp) {
                        trim(tmp);
                        res.push_back(tmp);
                    }

                    return res;
                };

                auto string_to_pair = [&](std::string &line) -> std::pair<int, int>
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

                    int r = std::stoi(tmp[0])-1, c = std::stoi(tmp[1])-1;

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

                        (socket_pin_to_pixel[apv_name[i-1]])[channel] = p;
                    }
                }

                // implement the inverse conversion, pixel to panasonic socket connector pin
                for(auto &i: socket_pin_to_pixel) {
                    for(size_t ch=0; ch<i.second.size(); ch++)
                    {
                        int row = (i.second)[ch].first, col = (i.second)[ch].second;
                        if( row >= 0 &&  col >= 0) {
                            int key = row * 100000 + col;
                            pixel_to_socket_pin[key] = ch;
                        }
                    }
                }

                // finished reading text file
                is_loaded = true;
            }

            std::pair<int, int> GetCoordFromStripNo(const std::string &apv_type, const int ch)
            {
                return (socket_pin_to_pixel[apv_type])[ch];
            }

            int GetStripNoFromCoord(int row, int col)
            {
                if(row < 0 || col < 0)
                    return -1;

                int key = row * 100000 + col;
                return pixel_to_socket_pin[key];
            }

            std::string GetAPVNameFromPlanePos(int plane, int pos)
            {
                int index = plane > 0 ? 4 : 0;
                index += pos;

                return apv_name[index];
            }

        private:
            PixelMapping() {}
            std::unordered_map<std::string, std::vector<std::pair<int, int>>> socket_pin_to_pixel;
            std::unordered_map<int, int> pixel_to_socket_pin;

            bool is_loaded = false;
    };
}

#endif
