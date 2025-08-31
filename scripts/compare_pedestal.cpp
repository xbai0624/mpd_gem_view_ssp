struct APVAddress
{
    int crate_id, mpd_id, adc_ch;

    APVAddress():crate_id(-1), mpd_id(-1), adc_ch(-1)
    {}

    APVAddress(const APVAddress &addr):crate_id(addr.crate_id),
    mpd_id(addr.mpd_id), adc_ch(addr.adc_ch)
    {}

    APVAddress & operator=(const APVAddress &addr) {
        crate_id = addr.crate_id, mpd_id = addr.mpd_id, adc_ch = addr.adc_ch;
        return *this;
    };

    bool operator==(const APVAddress &addr) const
    {
        return (crate_id == addr.crate_id) && (mpd_id == addr.mpd_id) && (adc_ch == addr.adc_ch);
    }
};

namespace std {
    template<> struct hash<APVAddress>
    {
        std::size_t operator()(const APVAddress &k) const
        {
            return ( (k.adc_ch & 0xf)
                    | ((k.mpd_id & 0x7f) << 4)
                    | ((k.crate_id & 0xff)<<11)
                   );
        }
    };
}

unordered_map<APVAddress, TH1F*> GetPedestalDistribution(const char* file)
{
    unordered_map<APVAddress, TH1F*> res;

    fstream f(file, iostream::in);
    if(!f.is_open()) cout<<"Error: cannot open file: "<<file<<endl;

    return res;
}

void compare_pedestal(const char *file1="", const char* file2="")
{
    GetPedestalDistribution("../database/gem_ped_532.dat");
}
