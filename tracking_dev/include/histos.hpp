////////////////////////////////////////////////////////////////////////////////
//  histogram manager                                                         //
//  read a config file, build histograms as described in the config file      //
//                                                                            //
//  in order to make this file as independent as possible,                    //
//  a txt parser was also implemented in this file                            //
//  Xinzhan Bai                                                               //
////////////////////////////////////////////////////////////////////////////////

#ifndef HISTOS_HPP
#define HISTOS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <type_traits>

#include <TH1F.h>
#include <TH2F.h>
#include <TCanvas.h>
#include <TObject.h>
#include <TFile.h>

namespace histos
{
    using std::string;
    using std::vector;
    using std::unordered_map;
    using std::map;
    using std::istringstream;
    using std::fstream;
    using std::cout;
    using std::endl;

    // a class to parse text file
    template<typename T=string> class TextParser
    {
        public:
            TextParser(){}
            ~TextParser()
            {
                __cache.clear();
                __variable_def.clear();
            }

            void SetConfigFilePath(const char* _path)
            {
                path = _path;
            }

            void Load()
            {
                fstream input_file(path, fstream::in);
                if(!input_file.is_open())
                    std::cout<<"ERROR:: histo manager cannot open file: "<< path<<std::endl;
                string line;

                while(std::getline(input_file, line))
                {
                    auto entry = __parse_line(line);
                    if(entry.size() <= 0)
                        continue;

                    __substitute_var(entry);

                    // expand all histo entries
                    if(entry.size() <= 2)
                        __cache.push_back(entry);
                    else {
                        try {
                            int nhistos = stoi(entry[1]);
                            if(nhistos <= 1)
                                __cache.push_back(entry);
                            else {
                                string orig = entry[2];
                                string title = entry[3];
                                for(int i=0; i<nhistos; i++) {
                                    entry[2] = orig + std::to_string(i);
                                    entry[3] = title + std::to_string(i);
                                    __cache.push_back(entry);
                                }
                            }
                        } catch(...){
                            cout<<"ERROR: failed to convert string to int."<<endl;
                        }
                    }
                }

                //for(auto &i: __cache)
                //    __print(i);
            }

            // @param: type = TH1F/TH2F; key=histo_name
            vector<string> GetEntry(const string &type, const string &key)
            {
                vector<string> res;

                int count = 0;
                for(auto &i: __cache){
                    if(i[0] != type)
                        continue;

                    if(i.size() < 3) // avoid memory issue
                        return res;

                    if(i[2] == key) {
                        count++;
                        res = i;
                    }
                }

                if(count != 1) {
                    cout<<"ERROR: found "<<count<<" entries for histo: type = "<<type<<", name="<<key<<endl;
                    cout<<"       possible duplicated histo names in configuration file."<<endl;
                    cout<<"       please check your configuration file."<<endl;
                    exit(0);
                }

                return res;
            }

            // parse line
            vector<string> __parse_line(string &line)
            {
                __remove_comments(line);
                vector<string> res;
                if(line.size() <= 0)
                    return res;

                __trim_space(line);
                res = __separate_token(line);

                return res;
            }

            // remove trailing and leading white spaces
            void __trim_space(string &line)
            {
                if(line.size() <= 0)
                    line.clear();

                size_t p1 = line.find_first_not_of(" ");
                size_t p2 = line.find_last_not_of(" ");

                if(p2 < p1)
                    line.clear();

                size_t length = p2 - p1;

                line = line.substr(p1, length + 1);
            }

            // remove trailing and leading tokens
            void __trim_token(string &line)
            {
                if(line.size() <= 0)
                    line.clear();

                size_t p1 = line.find_first_not_of(token);
                size_t p2 = line.find_last_not_of(token);

                if(p2 < p1)
                    line.clear();

                size_t length = p2 - p1;

                line = line.substr(p1, length + 1);
            }

            // separate fields in string by tokens
            vector<string> __separate_token(string &line)
            {
                vector<string> res;

                size_t length = line.size();
                if(length <= 0)
                    return res;

                vector<size_t> tmp;
                for(size_t i=0; i<line.size(); ++i) {
                    if(token.find(line[i]) != string::npos) {
                        tmp.push_back(i);
                    }
                }

                size_t N = tmp.size();
                if(N <= 0) {
                    res.push_back(line);
                    return res;
                }

                if(tmp[0] != 0)
                    tmp.insert(tmp.begin(), 0);
                if(tmp.back() != length-1)
                    tmp.push_back(length-1);

                for(size_t pos=0; pos<tmp.size()-1; ++pos) {
                    string _t;
                    if(pos == 0)
                        _t = line.substr(tmp[pos], tmp[pos+1] - tmp[pos]);
                    else if(pos == tmp.size() - 2)
                        _t = line.substr(tmp[pos]+1, tmp[pos+1] - tmp[pos]+1);
                    else
                        _t = line.substr(tmp[pos]+1, tmp[pos+1] - tmp[pos]);

                    __trim_token(_t);
                    __trim_space(_t);

                    if(_t.size() > 0)
                        res.push_back(_t);
                }

                return res;
            }

            // remove comments
            void __remove_comments(string &line)
            {
                size_t pos = line.find_first_of("#");
                line = line.substr(0, pos);
            }

            // substitute variables
            void __substitute_var(vector<string> &line)
            {
                for(size_t i=0; i<line.size(); ++i){
                    if(line[i].find("$") != string::npos) {
                        if(line.size() == 2 && i == 0) {
                            // definition encountered
                            if(__variable_def.find(line[i]) != __variable_def.end()) {
                                cout<<"duplicated variable definition in entry: "<<endl;
                                __print(line);
                                return;
                            }

                            __variable_def[line[i]] = line[i+1];
                        }
                        else {
                            if(__variable_def.find(line[i]) == __variable_def.end()){
                                cout<<"undefined variable in entry: "<<endl;
                                __print(line);
                                return;
                            }

                            line[i] = __variable_def[line[i]];
                        }
                    }
                }
            }

            // debug
            void __print(const vector<string> &line)
            {
                cout<<"size: "<<line.size()<<", ";
                for(auto &i: line)
                    cout<<"|"<<i;
                cout<<"|"<<endl;
            }

        private:
            vector<vector<string>> __cache;

            // white space can't be a token, white space is considered meaningful
            // in histogram title discription
            string token = ",;=|";

            string path = "config/histo.conf";
            unordered_map<string, string> __variable_def;
    };

    // histo manager class
    template<typename T=string> class HistoManager
    {
        public:
            HistoManager() {}
            HistoManager(const char* path)
            {
                config_path = path;
            }
            ~HistoManager() {
                __histos.clear();
            }

            void init() 
            {
                text_parser.SetConfigFilePath(config_path.c_str());
                text_parser.Load();
            }

            template<typename H>
                typename std::enable_if<std::is_same<H, float>::value, TH1F*>::type histo_1d(const char* name)
                {
                    if(__histos.find(name) != __histos.end())
                        return (TH1F*)__histos[name];

                    vector<string> entry = text_parser.GetEntry("TH1F", name);
                    __histos[name] = __build_th1f(entry);

                    return (TH1F*)__histos[name];
                }

            template<typename H>
                typename std::enable_if<std::is_same<H, float>::value, TH2F*>::type histo_2d(const char* name)
                {
                    if(__histos.find(name) != __histos.end())
                        return (TH2F*)__histos[name];

                    vector<string> entry = text_parser.GetEntry("TH2F", name);
                    __histos[name] = __build_th2f(entry);

                    return (TH2F*)__histos[name];
                }

            void reset()
            {
                for(auto &i: __histos)
                    i.second -> Reset("ICESM");
            }

            void save(const char* path)
            {
                TFile *f = new  TFile(path, "recreate");
                for(auto &i: __histos)
                    i.second -> Write();
                f->Close();
            }

            const map<string, TH1*> &get_histos_1d() const
            {
                return __histos;
            }

            // build 1d histogram
            TH1F* __build_th1f(const vector<string> &entry)
            {
                int nbins = 100;
                float low = 0, high = 0;
                try {
                    nbins = stoi(entry[4]);
                    low = stod(entry[5]);
                    high = stod(entry[6]);
                }catch(...){
                    cout<<"ERROR: failed to convert string to int/float"<<endl;
                }

                TH1F *h = new TH1F(entry[2].c_str(), entry[3].c_str(), nbins, low, high);
                h -> GetXaxis() -> SetTitle(entry[7].c_str());
                h -> GetYaxis() -> SetTitle(entry[8].c_str());
                __format_histo(h);
                return h;
            }

            // build 2d histogram
            TH2F* __build_th2f(const vector<string> &entry)
            {
                int xbins=100, ybins=100;
                float xlow=0, ylow=0, xhigh=0, yhigh=0;
                try{
                    xbins = stoi(entry[4]); ybins = stoi(entry[7]);
                    xlow = stod(entry[5]); ylow = stod(entry[8]);
                    xhigh = stod(entry[6]); yhigh = stod(entry[9]);
                }catch(...){
                    cout<<"ERROR: failed to convert string to int/float"<<endl;
                }

                TH2F *h = new TH2F(entry[2].c_str(), entry[3].c_str(), xbins, xlow, xhigh, ybins, ylow, yhigh);
                h -> GetXaxis() -> SetTitle(entry[10].c_str());
                h -> GetYaxis() -> SetTitle(entry[11].c_str());
                __format_histo(h);
                return h;
            }

            // format tcanvas
            void __format_canvas(TCanvas *c)
            {
                c->SetTitle(""); // no title
                //c->SetGridx();
                //c->SetGridy();
                c->SetBottomMargin(0.12);
                c->SetLeftMargin(0.12);
                c->SetRightMargin(0.05);
                c->SetTopMargin(0.05);

                gPad->SetLeftMargin(0.15); // gPad exists after creating TCanvas
                gPad->SetBottomMargin(0.15); // gPad exists after creating TCanvas
                gPad->SetFrameLineWidth(2);
            }

            // format TGraph, TGraphErrors
            template<typename Graph> void __format_graph(Graph* g)
            {
                g->SetTitle(""); // no title                                                     
                g->SetMarkerStyle(20);                                                           
                g->SetMarkerSize(1.0);                                                           
                g->SetMarkerColor(1);                                                            

                g->SetLineWidth(2); // xb                                                        
                g->SetLineColor(4); // xb                                                        

                double label_size = 0.045;                                                       
                double title_size = 0.055;
                //g->GetXaxis()->SetTitle(x_title.c_str());                                        
                g->GetXaxis()->SetLabelSize(label_size);                                         
                g->GetXaxis()->SetTitleSize(title_size);                                         
                g->GetXaxis()->SetLabelFont(62);                                                 
                g->GetXaxis()->SetTitleFont(62);
                g->GetXaxis()->SetTitleOffset(1.0);                                              
                g->GetXaxis()->CenterTitle();                                                    

                //g->GetYaxis()->SetTitle(y_title.c_str());                                        
                g->GetYaxis()->SetLabelSize(label_size);                                         
                g->GetYaxis()->SetTitleSize(title_size);                                         
                g->GetYaxis()->SetLabelFont(62);                                                 
                g->GetYaxis()->SetTitleFont(62);
                g->GetYaxis()->SetTitleOffset(1.1);                                              
                g->GetYaxis()->SetNdivisions(505);                                               
                g->GetYaxis()->CenterTitle();   
            }

            // format TH1F*, TH2F*
            template<typename Histo> void __format_histo(Histo* g)
            {
                double label_size = 0.045;                                                       
                double title_size = 0.055;
                //g->GetXaxis()->SetTitle(x_title.c_str());                                        
                g->GetXaxis()->SetLabelSize(label_size);                                         
                g->GetXaxis()->SetTitleSize(title_size);                                         
                g->GetXaxis()->SetLabelFont(62);                                                 
                g->GetXaxis()->SetTitleFont(62);
                g->GetXaxis()->SetTitleOffset(1.0);                                              
                g->GetXaxis()->CenterTitle();                                                    

                //g->GetYaxis()->SetTitle(y_title.c_str());                                        
                g->GetYaxis()->SetLabelSize(label_size);                                         
                g->GetYaxis()->SetTitleSize(title_size);                                         
                g->GetYaxis()->SetLabelFont(62);                                                 
                g->GetYaxis()->SetTitleFont(62);
                g->GetYaxis()->SetTitleOffset(1.1);                                              
                g->GetYaxis()->SetNdivisions(505);                                               
                g->GetYaxis()->CenterTitle();   

                g->SetLineWidth(2);
            }

        private:
            string config_path = "config/histo.conf";
            map<string, TH1*> __histos;
            TextParser<> text_parser;
    };
};

#endif
