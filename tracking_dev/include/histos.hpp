////////////////////////////////////////////////////////////////////////////////
//  histogram manager                                                         //
//  read a config file, build histograms as described in the config file      //
//                                                                            //
//  in order to make this file as independent as possible,                    //
//  a dedicated txt parser was also implemented in this file                  //
//  Xinzhan Bai                                                               //
//                                                                            //
//  Usage:                                                                    //
//      Both TxtParser and HistoManager has been implemented using template,  //
//      TxtParser is private, HistoManger use it internally                   //
//                                                                            //
//      1) declare the histogram manager tools:                               //
//         histos::HistoManager<> histo_manager;                              //
//         histo_manager.init(); or histo_manager.init("path/to/config/file");//
//                                                                            //
//      2) to fill a histogram:                                               //
//         histo_manager.hist_1d<float>("hist_name") -> Fill(0.9);            //
//         histo_manager.hist_2d<float>("hist_name") -> Fill(0.9, 0.9);       //
//                                                                            //
//  the default config file is "config/histo.conf". Config file format:       //
//                                                                            //
//  suppose I want generate 5 TH1F histos: h_pln0_t, h_pln1_t, ..., h_pln4_t: //
//                                                                            //
//  ${N} = 5                                                                  //
//  TH1F, h_pln${N}_t, hist title ${N}, 100, -30, 30, x title, y title        //
//  TH2F, h_name${N}, hist title ${N}, 100, 0, 2, 100, 0, 3, x title, ytitle  //
//                                                                            //
//  The program will search the place where ${N} variable holds, and replace  //
//  each ${N} by numbers from 0 to 5, expand it and save each entry to a map  //
//                                                                            //
//  If you only need one histogram, do:                                       //
//  TH1F, h_name, hist title, 100, -30, 30, x title, y title                  //
//  The programs won't attach anything if it doesn't find                     //
//  any declared variables, which is enclosed by ${ }                         //
//                                                                            //
//  For mulitple variables:                                                   //
//  ${P} = 2                                                                  //
//  ${M} = 4                                                                  //
//  TH1F, h_pln${P}_mod${M}, plane ${P} mod ${M}, ....., x title, y title     //
//  the program will expand all variables accordingly                         //
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

//#define HISTO_DEBUG

    // a class to parse text file
    template<typename TtextParser=std::string> class TextParser
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
            std::fstream input_file(path, std::fstream::in);
            if(!input_file.is_open())
                std::cout<<"ERROR:: histo manager cannot open file: "<< path<<std::endl;
            std::string line;

            while(std::getline(input_file, line))
            {
                auto entry = __parse_line(line);
                if(entry.size() <= 0)
                    continue;

                __substitute_var(entry);
            }
#ifdef HISTO_DEBUG
            for(auto &i: __cache)
                __print(i);
#endif
        }

        // @param: type = TH1F/TH2F; key=histo_name
        std::vector<std::string> GetEntry(const std::string &type, const std::string &key)
        {
            std::vector<std::string> res;

            int count = 0;
            for(auto &i: __cache){
                if(i[0] != type)
                    continue;

                if(i.size() < 3) // avoid memory issue
                    return res;

                if(i[1] == key) {
                    count++;
                    res = i;
                }
            }

            if(count != 1) {
                std::cout<<"ERROR: found "<<count<<" entries for histo: type = "<<type<<", name="<<key<<std::endl;
                std::cout<<"       possible duplicated histo names in configuration file."<<std::endl;
                std::cout<<"       please check your configuration file."<<std::endl;
                exit(0);
            }

            return res;
        }

        // parse line
        std::vector<std::string> __parse_line(std::string &line)
        {
            __remove_comments(line);
            std::vector<std::string> res;
            if(line.size() <= 0)
                return res;

            __trim_space(line);
            res = __separate_token(line);

            return res;
        }

        // remove trailing and leading white spaces
        void __trim_space(std::string &line)
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
        void __trim_token(std::string &line)
        {
            if(line.size() <= 0)
                line.clear();

            size_t p1 = line.find_first_not_of(token);
            size_t p2 = line.find_last_not_of(token);

            if(p2 < p1) {
                line.clear();
                return;
            }

            size_t length = p2 - p1;

            line = line.substr(p1, length + 1);
        }

        // separate fields in string by tokens
        std::vector<std::string> __separate_token(std::string &line)
        {
            std::vector<std::string> res;

            size_t length = line.size();
            if(length <= 0)
                return res;

            std::vector<size_t> tmp;
            for(size_t i=0; i<line.size(); ++i) {
                if(token.find(line[i]) != std::string::npos) {
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
                std::string _t;
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
        void __remove_comments(std::string &line)
        {
            size_t pos = line.find_first_of("#");
            line = line.substr(0, pos);
        }

        // substitute variables
        void __substitute_var(const std::vector<std::string> &line)
        {
            if(!__has_unexpanded_variable(line)) {
                __cache.push_back(line);
                return;
            }

            // definition encountered
            if(line.size() == 2) {
                if(__variable_def.find(line[0]) != __variable_def.end()) {
                    std::cout<<"duplicated variable definition in entry: "<<std::endl;
                    __print(line);
                    return;
                }

                __variable_def[line[0]] = line[1];
                return;
            }

            // normal variable, expand all occurences
            std::unordered_map<std::string, int> vars;
            __find_all_variables(line, vars);

            // each iteration only expand one variable
            // undefined variable
            if(__variable_def.find(vars.begin() -> first) == __variable_def.end()) {
                std::cout<<"ERROR: undefined variable: "<<vars.begin() -> first<<std::endl;
                std::cout<<"       variable declare must in format ${VAR} = val"<<std::endl;
                return;
            }

            std::vector<std::vector<std::string>> res;
            __expand_variable(line, vars.begin()->first, __variable_def.at(vars.begin()->first), res);
            for(auto &l: res)
                __substitute_var(l);
        }

        // check if a line has unexpanded variables
        bool __has_unexpanded_variable(const std::vector<std::string> &line)
        {
            for(auto &i: line)
                if(i.find("${") != std::string::npos) return true;
            return false;
        }

        // find all variables in one line
        void __find_all_variables(const std::vector<std::string> &line,
                std::unordered_map<std::string, int>& res)
        {
            for(auto &i: line)
                __find_all_variables(i, res);
        }

        // find all variables in one string
        void __find_all_variables(const std::string &element,
                std::unordered_map<std::string, int> &res)
        {
            std::string::size_type pos{};
            while( (pos = element.find("${", pos)) != element.npos) {
                std::string::size_type end{};
                if( (end = element.find("}", pos)) == element.npos ) {
                    std::cout<<"Error: variable does not have an ending enclosure: "
                        <<element<<std::endl;
                }

                std::string var = element.substr(pos, end-pos+1);
                if(res.find(var) != res.end())
                    res[var] += 1;
                else
                    res[var] = 1;
                pos = end;
            };
        }

        // expand for one variable, exhaust all ocurrences for this variable,
        // and put the expanded ones into res
        void __expand_variable(const std::vector<std::string> &elements,
                const std::string &var_key, const std::string &var_val,
                std::vector<std::vector<std::string>> &res)
        {
            try {
                int total = std::stoi(var_val);
                for(int i=0; i<total; i++) {
                    std::string s_i = std::to_string(i);

                    std::vector<std::string> temp = elements; // make a copy
                    for(auto &i_temp: temp)
                        __replace_string(i_temp, var_key, s_i);

                    res.push_back(temp);
                }
            } catch(...) {
                std::cout<<"ERROR: failed to convert string to int for variable: "
                    <<var_val<<std::endl;
            }
        }

        // replace all occurences of "what" to "with" in string "element"
        void __replace_string(std::string &element, const std::string &what, const std::string &with)
        {
            for(std::string::size_type pos{};
                    std::string::npos != (pos = element.find(what, pos));
                    pos += with.length())
                element.replace(pos, what.length(), with);
        }

        // print entries
        void __print(const std::vector<std::string> &line)
        {
            std::cout<<"size: "<<line.size()<<", ";
            for(auto &i: line)
                std::cout<<"|"<<i;
            std::cout<<"|"<<std::endl;
        }

    private:
        std::vector<std::vector<std::string>> __cache;

        // white space can't be a token, white space is considered meaningful
        // in histogram title discription
        std::string token = ",;=|";

        std::string path = "config/histo.conf";
        std::unordered_map<std::string, std::string> __variable_def;
    };

    // histo manager class
    template<typename ThistManager=std::string> class HistoManager
    {
    public:
        HistoManager() {}
        ~HistoManager() {
            __histos.clear();
        }

        void init(const char* _p = "config/histo.conf") 
        {
            config_path = _p;
            text_parser.SetConfigFilePath(config_path.c_str());
            text_parser.Load();

            TH1::AddDirectory(false);
        }

        template<typename H>
            typename std::enable_if<std::is_same<H, float>::value, TH1F*>::type histo_1d(const char* name)
            {
                if(__histos.find(name) != __histos.end())
                    return (TH1F*)__histos[name];

                std::vector<std::string> entry = text_parser.GetEntry("TH1F", name);
                __histos[name] = __build_th1f(entry);

                return (TH1F*)__histos[name];
            }

        template<typename H>
            typename std::enable_if<std::is_same<H, float>::value, TH2F*>::type histo_2d(const char* name)
            {
                if(__histos.find(name) != __histos.end())
                    return (TH2F*)__histos[name];

                std::vector<std::string> entry = text_parser.GetEntry("TH2F", name);
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

            // first do a sort
            std::map<std::string, TH1*> m_tmp;
            for(auto &i: __histos)
                m_tmp[i.first] = i.second;

            for(auto &i: m_tmp) {
                //i.second -> SetDirectory(f);
                i.second -> Write();
            }

            f->Close();
        }

        const std::unordered_map<std::string, TH1*> &get_histos_1d() const
        {
            return __histos;
        }

        // build 1d histogram
        // format in : "TH1F, name, title, bins, min, max, title, title"
        TH1F* __build_th1f(const std::vector<std::string> &entry)
        {
            int nbins = 100;
            float low = 0, high = 0;
            try {
                nbins = stoi(entry[3]);
                low = stod(entry[4]);
                high = stod(entry[5]);
            }catch(...){
                std::cout<<"ERROR: failed to convert string to int/float"<<std::endl;
            }

            TH1F *h = new TH1F(entry[1].c_str(), entry[2].c_str(), nbins, low, high);
            h -> GetXaxis() -> SetTitle(entry[6].c_str());
            h -> GetYaxis() -> SetTitle(entry[7].c_str());
            __format_histo(h);
            return h;
        }

        // build 2d histogram
        // format in : "TH2F, name, title, bins, min, max, bins, min, max, title, title"
        TH2F* __build_th2f(const std::vector<std::string> &entry)
        {
            int xbins=100, ybins=100;
            float xlow=0, ylow=0, xhigh=0, yhigh=0;
            try{
                xbins = stoi(entry[3]), ybins = stoi(entry[6]);
                xlow = stod(entry[4]), ylow = stod(entry[7]);
                xhigh = stod(entry[5]), yhigh = stod(entry[8]);
            }catch(...){
                std::cout<<"ERROR: failed to convert string to int/float"<<std::endl;
            }

            TH2F *h = new TH2F(entry[1].c_str(), entry[2].c_str(), xbins, xlow, xhigh, ybins, ylow, yhigh);
            h -> GetXaxis() -> SetTitle(entry[9].c_str());
            h -> GetYaxis() -> SetTitle(entry[10].c_str());
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
            //g -> SetDirectory(0);
            double label_size = 0.045;                                                       
            double title_size = 0.055;
            //g->GetXaxis()->SetTitle(x_title.c_str());                                        
            g->GetXaxis()->SetLabelSize(label_size);                                         
            g->GetXaxis()->SetTitleSize(title_size);                                         
            g->GetXaxis()->SetLabelFont(62);                                                 
            g->GetXaxis()->SetTitleFont(62);
            g->GetXaxis()->SetTitleOffset(0.8);                                              
            g->GetXaxis()->CenterTitle();                                                    

            //g->GetYaxis()->SetTitle(y_title.c_str());                                        
            g->GetYaxis()->SetLabelSize(label_size);                                         
            g->GetYaxis()->SetTitleSize(title_size);                                         
            g->GetYaxis()->SetLabelFont(62);                                                 
            g->GetYaxis()->SetTitleFont(62);
            g->GetYaxis()->SetTitleOffset(0.8);                                              
            g->GetYaxis()->SetNdivisions(505);                                               
            g->GetYaxis()->CenterTitle();   

            g->SetLineWidth(2);
        }

    private:
        std::string config_path = "config/histo.conf";
        std::unordered_map<std::string, TH1*> __histos;
        TextParser<> text_parser;
    };
};

#endif
