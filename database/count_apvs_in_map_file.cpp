#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

//# CrateID  : the Unique ID for the VME Crate
//# MPDID    : MPD address in VME Crate 
//# GEMID    : detector ID
//# dimension: x/y 0/1
//# ADCId    : the ADCId address of APV
//# I2C      : the I2C address of APV
//# Pos	   : Position
//# Invert   : how the detector is positioned in the layer First two are normal (1) last two are inverted (0)
//# other    : backup param used for large scare spectrometer
//#
//# notes    : MPD CrateID and MPDID  uniquely define the MPD address
//#	     APV crateID, MPDID ADCId uniquely define the APV address	


// define an apv struct
struct APV {
    int crate_id, layer, mpd_id, gem_id, dimension, adc_ch, i2c_ch, pos, invert;
    string discriptor;
    int back_plane, gem_pos;

    APV() {
    }

    APV(const vector<string> & vec) 
    {
        crate_id = stoi(vec[1]);        layer  = stoi(vec[2]);
        mpd_id   = stoi(vec[3]);        gem_id = stoi(vec[4]);
        dimension= stoi(vec[5]);        adc_ch = stoi(vec[6]);
        i2c_ch   = stoi(vec[7]);        pos    = stoi(vec[8]);
        invert   = stoi(vec[9]);        discriptor = vec[10];
        back_plane = stoi(vec[11]);     gem_pos = stoi(vec[12]);
    }
};

// remove leading and trailing white spaces
void trim(string &str)
{
    if(str.size() <= 0) return;

    size_t start = str.find_first_not_of(' ');
    size_t end = str.find_last_of(' ');

    str = str.substr(start, end - start - 1);
}

// parse apv
vector<string> parse_line(const string &str)
{
    vector<string> res;

    istringstream iss(str);
    string tmp;
    while(iss >> tmp)
    {
        res.push_back(tmp.substr(0, tmp.size()-1));
        tmp.clear();
    }

    return res;
}

// print out 1 layer apv
void print_layer(const vector<APV> &apvs, int layer)
{
    cout<<setfill(' ')<<setw(8)<<"crate"
        <<setfill(' ')<<setw(8)<<"mpd"
        <<setfill(' ')<<setw(8)<<"adc"
        <<setfill(' ')<<setw(8)<<"gem_id"
        <<setfill(' ')<<setw(8)<<"x/y"
        <<setfill(' ')<<setw(8)<<"i2c"
        <<setfill(' ')<<setw(8)<<"pos"
        <<setfill(' ')<<setw(8)<<"invert"
        <<endl;

    for(auto &apv: apvs) {
        if(apv.layer != layer) 
            continue;

        cout<<setfill(' ')<<setw(8)<<apv.crate_id
            <<setfill(' ')<<setw(8)<<apv.mpd_id
            <<setfill(' ')<<setw(8)<<apv.adc_ch
            <<setfill(' ')<<setw(8)<<apv.gem_id
            <<setfill(' ')<<setw(8)<<apv.dimension
            <<setfill(' ')<<setw(8)<<apv.i2c_ch
            <<setfill(' ')<<setw(8)<<apv.pos
            <<setfill(' ')<<setw(8)<<apv.invert
            <<endl;
    }
}

// main
int main(int argc, char* argv[])
{
    fstream f("gem_map.txt", fstream::in);
    if(!f.is_open()){
        cout<<"cannot open file."<<endl;
    }

    vector<APV> apvs;

    string line;
    int count  = 0;
    while(getline(f, line))
    {
        if(static_cast<char>(line[0]) == '#') continue;
        if(line.find("APV") != string::npos){
            auto r = parse_line(line);

            apvs.emplace_back(r);

            count++;
        }
    }

    cout<<"total apvs: "<<count<<"/"<<apvs.size()<<endl;

    // print out 1 layer
    print_layer(apvs, 1);
    print_layer(apvs, 3);

    return 0;
}
