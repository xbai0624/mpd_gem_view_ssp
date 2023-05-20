// #include "GEM_cosmic_tracks.C"
#include "TChain.h"
#include "TTree.h"
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TMatrixD.h"
#include "TVectorD.h"
#include "TVector3.h"
#include "TRotation.h"
#include "TEventList.h"
#include "TCut.h"
#include <iostream>
#include <fstream>
#include "TMinuit.h"
#include "TMath.h"
#include "TObjArray.h"
#include "TObjString.h"

double PI = TMath::Pi();

int nlayers = 4;
int nmodules = 4;

// Make these global for chi^2 function for numerical minimization:

map<int, int> mod_layer;
map<int, double> mod_x0; // key = module index, value = x center position
map<int, double> mod_y0; // key = module index, value = y center position
map<int, double> mod_z0; // key = module index, value = z position
map<int, double> mod_ax; // key = module index, value = X axis rotation (yaw)
map<int, double> mod_ay; // key = module index, value = Y axis rotation (pitch)
map<int, double> mod_az; // key = module index, value = Z axis rotation (roll)
map<int, double> mod_dx0;
map<int, double> mod_dy0;
map<int, double> mod_dz0;
map<int, double> mod_dax;
map<int, double> mod_day;
map<int, double> mod_daz;
// Need "U" and "V" strip angle definitions to generalize to the case of arbitrary strip orientation:
map<int, double> mod_uangle; // key = module index, value = angle between module x axis and "U" direction
map<int, double> mod_vangle; // key = module index, value = angle between modyle x axis and "V" direction
map<int, double> mod_Pxu;    // cos(uangle);
map<int, double> mod_Pyu;    // sin(uangle);
map<int, double> mod_Pxv;    // cos(vangle);
map<int, double> mod_Pyv;    // cos(vangle);

map<int, bool> fixmod; // allowing fixing the position and orientation of arbitrary combinations of modules:

long NMAX;

// Let's see if we can improve things by doing one iteration of linearized, then use TMinuit:
int NTRACKS;
vector<double> XTRACK, YTRACK, XPTRACK, YPTRACK;
vector<int> TRACKNHITS;
vector<vector<int>> HITMOD;
vector<vector<double>> HITX, HITY;

void CHI2_FCN(int &npar, double *gin, double &f, double *par, int flag)
{
    double chi2 = 0.0;

    // for( int pass=0; pass<2; pass++ ){ //First pass, re-fit tracks, Second pass: fit parameters:
    for (int itr = 0; itr < NTRACKS; itr++)
    {
        // First: re-fit track using latest parameters:

        for (int ihit = 0; ihit < TRACKNHITS[itr]; ihit++)
        {
            double ulocal = HITX[itr][ihit];
            double vlocal = HITY[itr][ihit];

            int module = HITMOD[itr][ihit];

            int ipar_x0 = 6 * module;
            int ipar_y0 = 6 * module + 1;
            int ipar_z0 = 6 * module + 2;
            int ipar_ax = 6 * module + 3;
            int ipar_ay = 6 * module + 4;
            int ipar_az = 6 * module + 5;

            double det = mod_Pxu[module] * mod_Pyv[module] - mod_Pyu[module] * mod_Pxv[module];

            double xlocal = (mod_Pyv[module] * ulocal - mod_Pyu[module] * vlocal) / det; //(sin(alphav)*U - sin(alphau)*V)/det = U = X for alphau = 0, alphav = 90
            double ylocal = (mod_Pxu[module] * vlocal - mod_Pxv[module] * ulocal) / det; //(cos(alphau)*V - cos(alphav)*U)/det = V = Y for alphau = 0, alphav = 90

            TVector3 hitpos_local(xlocal, ylocal, 0);
            TRotation R;
            R.RotateX(mod_ax[module] + par[ipar_ax]);
            R.RotateY(mod_ay[module] + par[ipar_ay]);
            R.RotateZ(mod_az[module] + par[ipar_az]);

            TVector3 modcenter_global(mod_x0[module] + par[ipar_x0],
                                      mod_y0[module] + par[ipar_y0],
                                      mod_z0[module] + par[ipar_z0]);

            TVector3 hitpos_global = modcenter_global + R * hitpos_local;

            TVector3 trackpos_global(XTRACK[itr] + XPTRACK[itr] * hitpos_global.Z(),
                                     YTRACK[itr] + YPTRACK[itr] * hitpos_global.Z(),
                                     hitpos_global.Z());

            TVector3 diff = hitpos_global - trackpos_global;

            chi2 += pow(diff.X(), 2) + pow(diff.Y(), 2); //

            // update tracks? this is the tricky part:
        }

        // if( pass == 0 ){ //update tracks:
        // 	TVectorD Track = Atrack.Invert() * btrack;

        // 	XTRACK[itr] = Track(0);
        // 	XPTRACK[itr] = Track(1);
        // 	YTRACK[itr] = Track(2);
        // 	YPTRACK[itr] = Track(3);
        // } //loop over hits
    } // loop over tracks
    // loop over two passes

    f = chi2;
}

void GEM_align(const char *configfilename, const char *outputfilename = "newGEMalignment.txt")
{

    ifstream configfile(configfilename);

    int niter = 1; // number of alignment iterations:

    // default to 100000
    NMAX = 100000; // limit number of events so we can maximize number of alignment iterations.

    int refmod = 4;

    TCut globalcut = "";

    int offsetsonlyflag = 0;
    int rotationsonlyflag = 0;

    int fixz = 0; // fix z coordinate of layer if our data don't give us enough sensitivity to determine the z coordinates:
    int fixax = 0, fixay = 0, fixaz = 0;

    double sigma_hitpos = 0.15e-3; // m

    double minchi2change = 2.e-4;

    double minposchange = 5e-6;   // 5 um
    double minanglechange = 5e-5; // 50 urad

    TString prefix = "";

    TChain *C = new TChain("GEMCluster");

    // Copied from GEM_reconstruct: For this routine we are only interested in the number of layers and the number of modules, and the geometrical information:
    if (configfile)
    {
        TString currentline;

        while (currentline.ReadLine(configfile) && !currentline.BeginsWith("endlist"))
        {
            if (!currentline.BeginsWith("#"))
            {
                C->Add(currentline.Data());
            }
        }

        while (currentline.ReadLine(configfile) && !currentline.BeginsWith("endconfig"))
        {
            if (!currentline.BeginsWith("#"))
            {
                TObjArray *tokens = currentline.Tokenize(" ");

                int ntokens = tokens->GetEntries();

                if (ntokens >= 2)
                {
                    TString skey = ((TObjString *)(*tokens)[0])->GetString();

                    if (skey == "prefix")
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        prefix = stemp;
                    }

                    if (skey == "minposchange")
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        minposchange = stemp.Atof();
                    }

                    if (skey == "minanglechange")
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        minanglechange = stemp.Atof();
                    }

                    if (skey == "minchi2change")
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        minchi2change = stemp.Atof();
                    }

                    if (skey == "niter")
                    {
                        TString sniter = ((TObjString *)(*tokens)[1])->GetString();
                        niter = sniter.Atoi();
                    }

                    if (skey == "nlayers")
                    {
                        TString snlayers = ((TObjString *)(*tokens)[1])->GetString();
                        nlayers = snlayers.Atoi();
                    }

                    if (skey == "offsetsonly")
                    {
                        TString sflag = ((TObjString *)(*tokens)[1])->GetString();
                        offsetsonlyflag = sflag.Atoi();
                    }

                    if (skey == "rotationsonly")
                    {
                        TString sflag = ((TObjString *)(*tokens)[1])->GetString();
                        rotationsonlyflag = sflag.Atoi();
                    }

                    if (skey == "nmodules")
                    {
                        TString snmodules = ((TObjString *)(*tokens)[1])->GetString();
                        nmodules = snmodules.Atoi();
                    }

                    if (skey == "refmod")
                    {
                        TString sflag = ((TObjString *)(*tokens)[1])->GetString();
                        refmod = sflag.Atoi();
                    }

                    if (skey == "fixmod" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString stemp = ((TObjString *)(*tokens)[i])->GetString();
                            int flagtemp = stemp.Atoi();
                            fixmod[i - 1] = (flagtemp != 0);
                        }
                    }

                    if (skey == "mod_x0" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smodx = ((TObjString *)(*tokens)[i])->GetString();

                            mod_x0[i - 1] = smodx.Atof();
                        }
                    }

                    if (skey == "mod_y0" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smody = ((TObjString *)(*tokens)[i])->GetString();

                            mod_y0[i - 1] = smody.Atof();
                        }
                    }

                    if (skey == "mod_z0" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smodz = ((TObjString *)(*tokens)[i])->GetString();

                            mod_z0[i - 1] = smodz.Atof();
                        }
                    }

                    if (skey == "mod_ax" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smodax = ((TObjString *)(*tokens)[i])->GetString();

                            mod_ax[i - 1] = smodax.Atof();
                        }
                    }

                    if (skey == "mod_ay" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smoday = ((TObjString *)(*tokens)[i])->GetString();

                            mod_ay[i - 1] = smoday.Atof();
                        }
                    }

                    if (skey == "mod_az" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smodaz = ((TObjString *)(*tokens)[i])->GetString();

                            mod_az[i - 1] = smodaz.Atof();
                        }
                    }

                    if (skey == "mod_layer" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString smodlayer = ((TObjString *)(*tokens)[i])->GetString();

                            mod_layer[i - 1] = smodlayer.Atoi();
                        }
                    }

                    // These angles will be assumed to refer to the strip orientations! The coordinates they measure will be orthogonal to
                    //  their orientations
                    // Angle relative to X axis of "U" strips (assumed to be given in degrees)"
                    if (skey == "mod_uangle" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString stemp = ((TObjString *)(*tokens)[i])->GetString();
                            mod_uangle[i - 1] = stemp.Atof() * PI / 180.0;
                            mod_Pxu[i - 1] = cos(mod_uangle[i - 1]);
                            mod_Pyu[i - 1] = sin(mod_uangle[i - 1]);
                        }
                    }

                    // Angle relative to X axis of "V" strips (assumed to be given in degrees)"
                    if (skey == "mod_vangle" && ntokens >= nmodules + 1)
                    {
                        for (int i = 1; i < ntokens; i++)
                        {
                            TString stemp = ((TObjString *)(*tokens)[i])->GetString();
                            mod_vangle[i - 1] = stemp.Atof() * PI / 180.0;
                            mod_Pxv[i - 1] = cos(mod_vangle[i - 1]);
                            mod_Pyv[i - 1] = sin(mod_vangle[i - 1]);
                        }
                    }

                    if (skey == "fixz" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        fixz = stemp.Atoi();

                        cout << "setting fixz = " << fixz << endl;
                    }

                    if (skey == "fixax" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        fixax = stemp.Atoi();
                    }

                    if (skey == "fixay" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        fixay = stemp.Atoi();
                    }

                    if (skey == "fixaz" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        fixaz = stemp.Atoi();
                    }

                    if (skey == "sigma" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        sigma_hitpos = stemp.Atof();
                    }

                    if (skey == "NMAX" && ntokens >= 2)
                    {
                        TString stemp = ((TObjString *)(*tokens)[1])->GetString();
                        NMAX = stemp.Atoi();
                    }
                }
            }
        }
        while (currentline.ReadLine(configfile) && !currentline.BeginsWith("endcut"))
        {
            if (!currentline.BeginsWith("#"))
            {
                globalcut += currentline;
            }
        }
    }
    else
    {
        return;
    }

    if (offsetsonlyflag != 0 && rotationsonlyflag != 0)
    {
        cout << "nothing to align, quitting..." << endl;
        return;
    }

    TEventList *elist = new TEventList("elist");

    C->Draw(">>elist", globalcut);

    cout << "Number of events passing global cut = " << elist->GetN() << endl;

    // declare variables to hold tree branch addresses:
    int ntracks;
    int besttrack;
    vector<float> *tracknhits = 0;
    vector<float> *trackX = 0, *trackY = 0, *trackXp = 0, *trackYp = 0, *trackChi2NDF = 0;

    // Needed "hit" variables (others can be ignored for now:
    // The data types for all the branches are "double". Hope that doesn't cause problems:
    int ngoodhits;
    vector<int> *hit_trackindex = 0;
    vector<int> *hit_module = 0;
    vector<float> *hit_ulocal = 0;
    vector<float> *hit_vlocal = 0;

    C->SetBranchStatus("*", 0);

    C->SetBranchStatus("fNtracks_found", 1);
    C->SetBranchStatus("besttrack", 1);
    C->SetBranchStatus("fNhitsOnTrack", 1);
    C->SetBranchStatus("fNgoodhits", 1);
    C->SetBranchStatus("fHitTrackIndex", 1);
    C->SetBranchStatus("fXtrack", 1);
    C->SetBranchStatus("fYtrack", 1);
    C->SetBranchStatus("fXptrack", 1);
    C->SetBranchStatus("fYptrack", 1);
    C->SetBranchStatus("fChi2Track", 1);
    C->SetBranchStatus("fHitModule", 1);
    C->SetBranchStatus("fHitXlocal", 1);
    C->SetBranchStatus("fHitYlocal", 1);
    // C->SetBranchStatus(  "%s.track.ntrack", prefix.Data() ), 1 );

    // This SHOULD give us everything we need from the ROOT tree:
    C->SetBranchAddress("fNtracks_found", &ntracks);
    C->SetBranchAddress("besttrack", &besttrack);
    C->SetBranchAddress("fNhitsOnTrack", &tracknhits);
    C->SetBranchAddress("fNgoodhits", &ngoodhits);
    C->SetBranchAddress("fHitTrackIndex", &hit_trackindex);
    C->SetBranchAddress("fXtrack", &trackX);
    C->SetBranchAddress("fYtrack", &trackY);
    C->SetBranchAddress("fXptrack", &trackXp);
    C->SetBranchAddress("fYptrack", &trackYp);
    C->SetBranchAddress("fChi2Track", &trackChi2NDF);
    C->SetBranchAddress("fHitModule", &hit_module);
    C->SetBranchAddress("fHitXlocal", &hit_ulocal);
    C->SetBranchAddress("fHitYlocal", &hit_vlocal);

    // GEM_cosmic_tracks *T = new GEM_cosmic_tracks(C);

    // Declare branch addresses:

    TString outrootfilename;
    TString rootprefix = prefix;
    rootprefix.ReplaceAll(".", "_");

    outrootfilename.Form("GEM_align_results_%s.root", rootprefix.Data());

    TFile *fout = new TFile(outrootfilename.Data(), "RECREATE");

    double Txtrack, Tytrack, Txptrack, Typtrack, Tchi2ndf;
    int Tnhits;
    double Tuhit[nlayers], Tvhit[nlayers], Txhit[nlayers], Tyhit[nlayers], Tzhit[nlayers];
    double Turesid[nlayers], Tvresid[nlayers];
    double Txresid[nlayers], Tyresid[nlayers];
    int Thitlayer[nlayers], Thitmodule[nlayers];

    TTree *Tout = new TTree("Tout", "GEM alignment results");

    Tout->Branch("xtrack", &Txtrack, "xtrack/D");
    Tout->Branch("ytrack", &Tytrack, "ytrack/D");
    Tout->Branch("xptrack", &Txptrack, "xptrack/D");
    Tout->Branch("yptrack", &Typtrack, "yptrack/D");
    Tout->Branch("chi2ndf", &Tchi2ndf, "chi2ndf/D");
    Tout->Branch("nhits", &Tnhits, "nhits/I");
    Tout->Branch("uhit", Tuhit, "uhit[nhits]/D");
    Tout->Branch("vhit", Tvhit, "vhit[nhits]/D");
    Tout->Branch("xhit", Txhit, "xhit[nhits]/D");
    Tout->Branch("yhit", Tyhit, "yhit[nhits]/D");
    Tout->Branch("zhit", Tzhit, "zhit[nhits]/D");
    Tout->Branch("uresid", Turesid, "uresid[nhits]/D");
    Tout->Branch("vresid", Tvresid, "vresid[nhits]/D");
    Tout->Branch("xresid", Txresid, "xresid[nhits]/D");
    Tout->Branch("yresid", Tyresid, "yresid[nhits]/D");
    Tout->Branch("hitlayer", Thitlayer, "hitlayer[nhits]/I");
    Tout->Branch("hitmodule", Thitmodule, "hitmodule[nhits]/I");

    long nevent = 0;

    cout << "Alignment starting solution: " << endl;
    for (map<int, double>::iterator imod = mod_x0.begin(); imod != mod_x0.end(); ++imod)
    {
        int module = imod->first;
        cout << "Module " << module << ": (x0,y0,z0,ax,ay,az)=("
             << mod_x0[module] << ", " << mod_y0[module] << ", " << mod_z0[module] << ", "
             << mod_ax[module] << ", " << mod_ay[module] << ", " << mod_az[module] << ")" << endl;
    }

    // niter = 1;
    double trackchi2_cut = 10000.0;
    double oldchi2cut = trackchi2_cut;
    // double resid_cut = 100.0; //mm
    // double resid2_sum = 0.0;

    double maxresid = 0.5e-3; // mm

    double ndf_max = 2.0 * nlayers - 4;

    double minchi2cut = pow(maxresid / sigma_hitpos, 2); // smallest cut on chi2/dof that we are allowed to use:

    double meanchi2 = 10000.0;
    double oldmeanchi2 = meanchi2;

    double maxposchange = 1e9, maxanglechange = 1e9;
    double oldmaxposchange = maxposchange;
    double oldmaxanglechange = maxanglechange;

    for (int iter = 0; iter <= niter; iter++)
    {

        cout << "starting iteration " << iter << ", maxpos change = " << maxposchange
             << ", max angle change = " << maxanglechange << ", meanchi2/old mean chi2 = "
             << meanchi2 / oldmeanchi2 << endl;
        // at beginning of each iteration check:
        // if this is not the first iteration, cut short if chi2 stops improving:
        if (iter > 0 && fabs(1. - meanchi2 / oldmeanchi2) <= minchi2change)
            niter = iter;
        if (fabs(maxposchange) < minposchange && fabs(maxanglechange) < minanglechange)
            niter = iter;
        // if( fabs(maxposchange) > oldmaxposchange && fabs(maxanglechange) > oldmaxanglechange ) niter = iter;

        if (meanchi2 / oldmeanchi2 > 1.0)
            niter = iter;

        nevent = 0;

        // For each alignment iteration, let's define our chi^2 as a function of the global geometrical alignment parameters
        // and then try to linearize the problem:
        //     int nparam = nmodules*6; //order of parameters is x0,y0,z0,ax,ay,az
        int nparam = 0;
        // To simplify the special cases for alignment only or rotation only, we define separate matrices for alignment only or position only fits:
        int nparam_rot = 0;
        int nparam_pos = 0;

        int nfreemodules = 0;
        vector<int> freemodlist;
        map<int, int> freemodindex;

        for (int imod = 0; imod < nmodules; imod++)
        {
            if (!fixmod[imod])
            {
                nparam += 6;

                nparam_rot += 3;
                nparam_pos += 3;

                freemodlist.push_back(imod);
                freemodindex[imod] = nfreemodules;
                nfreemodules++;
            }
        }

        // if( refmod >= 0 && refmod < nmodules ){
        //   nparam = (nmodules-1)*6;
        // }

        if (nparam == 0)
        {
            cout << "all modules fixed, nothing to align... quitting" << endl;
            break;
        }

        cout << "nparam = " << nparam << endl;

        TMatrixD M(nparam, nparam);
        TVectorD b(nparam);

        for (int ipar = 0; ipar < nparam; ipar++)
        {
            for (int jpar = 0; jpar < nparam; jpar++)
            {
                M(ipar, jpar) = 0.0;
            }
            b(ipar) = 0.0;
        }

        TMatrixD Mrot(nparam_rot, nparam_rot);
        TVectorD brot(nparam_rot);

        TMatrixD Mpos(nparam_pos, nparam_pos);
        TVectorD bpos(nparam_pos);

        for (int ipar = 0; ipar < nparam_rot; ipar++)
        {
            for (int jpar = 0; jpar < nparam_rot; jpar++)
            {
                Mrot(ipar, jpar) = 0.0;
                Mpos(ipar, jpar) = 0.0;
            }
            brot(ipar) = 0.0;
            bpos(ipar) = 0.0;
        }

        // We wish to minimize the sum of squared residuals between all hits and tracks by varying the x,y,z position offsets and ax,ay,az
        // rotation angles of all nmodules modules:
        // For example:
        // chi^2 = sum_{i=1}^Nevent sum_{j=1}^{Nhit} (xhit_ij - xtrack_ij)^2/sigxij^2 + (yhit_ij - ytrack_ij)^2/sigyij^2
        // poslocal = (xlocal,ylocal,0);
        // posglobal = R*poslocal + modcenter
        // R = Rz*Ry*Rx
        // Rx mixes y and z components:
        // Rx = | 1       0         0        |  x' = x
        //      | 0       cos(ax)   -sin(ax) |  y' = cos(ax)*y -sin(ax)*z ~= y
        //      | 0       sin(ax)   cos(ax)  |  z' = sin(ax)*y +cos(ax)*z ~= +ax*y
        // Similarly for Ry, we have:
        // x' = cos(ay)*x + sin(ay)*z ~= x
        // y' = y
        // z' = -sin(ay)*x + cos(ay)*z ~= -ay*x
        // Similarly for Rz, we have:
        // x' = cos(az)*x - sin(az)*y ~= x - az*y
        // y' = sin(az)*x + cos(az)*y ~= az*x + y
        // Linearized global transformation:
        // xglobal = x - modaz*y + modx0
        // yglobal = y + modaz*x + mody0
        // zglobal = modax*y - moday*x + modz0
        // chi^2 = sum_i,j (xlocal - modaz*ylocal + modx0 - (xtrack + xptrack*(modz0 + modax*ylocal - moday*xlocal)))^2/sigx^2 +
        //                 (ylocal + modaz*xlocal + mody0 - (ytrack + yptrack*(modz0 + modax*ylocal - moday*xlocal)))^2/sigy^2
        // dchi2/dx0 = 2*(xlocal - modaz*ylocal + modx0 - (xtrack + xptrack*(modz0 + modax*ylocal - moday*xlocal))/sigx^2*1
        // dchi2/dy0 = 2*(ylocal + modaz*xlocal + mody0 - (ytrack + yptrack*(modz0 + modax*ylocal - moday*xlocal))/sigy^2*1
        // dchi2/dz0 = 2*(xlocal - modaz*ylocal + modx0 - (xtrack + xptrack*(modz0 + modax*ylocal - moday*xlocal))/sigx^2*-xptrack +
        //             2*(ylocal + modaz*xlocal + mody0 - (ytrack + yptrack*(modz0 + modax*ylocal - moday*xlocal))/sigy^2*-yptrack
        // dchi2/dax =

        // if( iter < niter ){
        //   offsetsonlyflag = true;
        // } else {
        //   offsetsonlyflag = false;
        //   rotationsonlyflag = false;
        // }

        cout << "Starting linearized alignment procedure, iteration = " << iter << ", chi2/dof cut = " << trackchi2_cut
             << ", mean chi2 = " << meanchi2 << ", old mean chi2 = " << oldmeanchi2 << endl;

        if (iter == niter)
        { // only fill these arrays on the last iteration:
            NTRACKS = 0;
        }

        double trackchi2_sum = 0.0;
        double ntracks_passed = 0.0;
        // double resid_sum = 0.0;
        // double nhit_sum = 0.0;

        while (C->GetEntry(elist->GetEntry(nevent++)))
        {

            if (nevent % 10000 == 0)
            {
                cout << "Linearized alignment, nevent = " << nevent << endl;
            }

            double trackchi2 = 0.0;

            int itrack = int(besttrack);

            // cout << itrack << endl;

            int NHITS = int(ngoodhits);
            int nhitsonbesttrack = int((*tracknhits)[itrack]);

            // cout << "N hits total = " << NHITS << ", hits on best track = " << nhitsonbesttrack << endl;
            // cout << "best track x, y, x', y' = " << (*trackX)[itrack] << ", " << (*trackY)[itrack]
            // 	   << ", " << (*trackXp)[itrack] << ", " << (*trackYp)[itrack] << endl;
            // cout << "track chi2/ndf = " << (*trackChi2NDF)[itrack] << endl;

            double xptrack, yptrack, xtrack, ytrack;
            if (iter < 0)
            { // on first iteration use track from ROOT tree:
                // xptrack = T->TrackXp;
                // yptrack = T->TrackYp;
                // xtrack = T->TrackX;
                // ytrack = T->TrackY;

                xtrack = (*trackX)[itrack];
                ytrack = (*trackY)[itrack];
                xptrack = (*trackXp)[itrack];
                yptrack = (*trackYp)[itrack];
                trackchi2 = (*trackChi2NDF)[itrack];
            }
            else
            { // on ALL iterations, we re-fit the track using updated alignment parameters (or the initial ones from the config file)
                double sumX = 0.0, sumY = 0.0, sumZ = 0.0, sumXZ = 0.0, sumYZ = 0.0, sumZ2 = 0.0;

                for (int ihit = 0; ihit < NHITS; ihit++)
                {
                    int tridx = int((*hit_trackindex)[ihit]);
                    if (tridx == itrack)
                    {

                        int module = int((*hit_module)[ihit]);

                        double ulocal = (*hit_ulocal)[ihit]; //"U" local: generalized "X"
                        double vlocal = (*hit_vlocal)[ihit]; //"V" local: generalized "Y"

                        double det = mod_Pxu[module] * mod_Pyv[module] - mod_Pyu[module] * mod_Pxv[module]; // cos( alphau) * sin(alphav) - sin(alphau)*cos(alphav) = 1 for alphau = 0, alphav = 90

                        double xlocal = (mod_Pyv[module] * ulocal - mod_Pyu[module] * vlocal) / det; //(sin(alphav)*U - sin(alphau)*V)/det = U = X for alphau = 0, alphav = 90
                        double ylocal = (mod_Pxu[module] * vlocal - mod_Pxv[module] * ulocal) / det; //(cos(alphau)*V - cos(alphav)*U)/det = V = Y for alphau = 0, alphav = 90

                        // cout << "module, (uhit,vhit) = " << module << ", (" << ulocal << ", " << vlocal << "), (xhit, yhit) = ("
                        // 	 << xlocal << ", " << ylocal << ")" << endl;

                        TVector3 hitpos_local(xlocal, ylocal, 0);
                        TRotation R;
                        R.RotateX(mod_ax[module]);
                        R.RotateY(mod_ay[module]);
                        R.RotateZ(mod_az[module]);

                        TVector3 modcenter_global(mod_x0[module], mod_y0[module], mod_z0[module]);
                        TVector3 hitpos_global = modcenter_global + R * hitpos_local;

                        // cout << "Global hit position = ";
                        // hitpos_global.Print();

                        double sigma = 0.1e-3;
                        double weight = pow(sigma_hitpos, -2);

                        weight = 1.0;

                        sumX += hitpos_global.X() * weight;
                        sumY += hitpos_global.Y() * weight;
                        sumZ += hitpos_global.Z() * weight;
                        sumXZ += hitpos_global.X() * hitpos_global.Z() * weight;
                        sumYZ += hitpos_global.Y() * hitpos_global.Z() * weight;
                        sumZ2 += pow(hitpos_global.Z(), 2) * weight;

                        // nhitsonbesttrack++;
                    }
                }

                double nhits = nhitsonbesttrack;

                double denom = (sumZ2 * nhits - pow(sumZ, 2));
                xptrack = (nhits * sumXZ - sumX * sumZ) / denom;
                yptrack = (nhits * sumYZ - sumY * sumZ) / denom;
                xtrack = (sumZ2 * sumX - sumZ * sumXZ) / denom;
                ytrack = (sumZ2 * sumY - sumZ * sumYZ) / denom;
            }

            Txtrack = xtrack;
            Tytrack = ytrack;
            Txptrack = xptrack;
            Typtrack = yptrack;

            Tnhits = nhitsonbesttrack;

            for (int ihit = 0; ihit < NHITS; ihit++)
            {
                int tridx = int((*hit_trackindex)[ihit]);

                if (tridx == itrack)
                {
                    int module = int((*hit_module)[ihit]);

                    double ulocal = (*hit_ulocal)[ihit]; //"U" local: generalized "X"
                    double vlocal = (*hit_vlocal)[ihit]; //"V" local: generalized "Y"

                    double det = mod_Pxu[module] * mod_Pyv[module] - mod_Pyu[module] * mod_Pxv[module]; // cos( alphau) * sin(alphav) - sin(alphau)*cos(alphav) = 1 for alphau = 0, alphav = 90

                    double xlocal = (mod_Pyv[module] * ulocal - mod_Pyu[module] * vlocal) / det; //(sin(alphav)*U - sin(alphau)*V)/det = U = X for alphau = 0, alphav = 90
                    double ylocal = (mod_Pxu[module] * vlocal - mod_Pxv[module] * ulocal) / det; //(cos(alphau)*V - cos(alphav)*U)/det = V = Y for alphau = 0, alphav = 90

                    TVector3 hitpos_local(xlocal, ylocal, 0);
                    TRotation R;
                    R.RotateX(mod_ax[module]);
                    R.RotateY(mod_ay[module]);
                    R.RotateZ(mod_az[module]);

                    TRotation Rinv = R;
                    Rinv.Invert();

                    TVector3 modcenter_global(mod_x0[module], mod_y0[module], mod_z0[module]);
                    TVector3 hitpos_global = modcenter_global + R * hitpos_local;

                    trackchi2 += (pow(hitpos_global.X() - (xtrack + xptrack * hitpos_global.Z()), 2) +
                                  pow(hitpos_global.Y() - (ytrack + yptrack * hitpos_global.Z()), 2)) *
                                 pow(sigma_hitpos, -2);

                    // for consistency with how we calculate residuals, should we change the chi2 calculation to be in terms of the u and v residuals instead of X and Y?

                    TVector3 trackpos_global(xtrack + xptrack * hitpos_global.Z(), ytrack + yptrack * hitpos_global.Z(), hitpos_global.Z());

                    TVector3 trackpos_local = Rinv * (trackpos_global - modcenter_global);

                    double utrack = trackpos_local.X() * mod_Pxu[module] + trackpos_local.Y() * mod_Pyu[module];
                    double vtrack = trackpos_local.X() * mod_Pxv[module] + trackpos_local.Y() * mod_Pyv[module];

                    //	  trackchi2 += ( pow( ulocal - utrack, 2 ) + pow( vlocal - vtrack, 2 ) ) * pow(sigma_hitpos, -2);

                    int layer_id = module;
                    Tuhit[module] = ulocal;
                    Tvhit[module] = vlocal;
                    Txhit[module] = hitpos_global.X();
                    Tyhit[module] = hitpos_global.Y();
                    Tzhit[module] = hitpos_global.Z();
                    Txresid[module] = hitpos_global.X() - (xtrack + xptrack * hitpos_global.Z());
                    Tyresid[module] = hitpos_global.Y() - (ytrack + yptrack * hitpos_global.Z());
                    Turesid[module] = ulocal - utrack;
                    Tvresid[module] = vlocal - vtrack;
                    Thitlayer[module] = mod_layer[module];
                    Thitmodule[module] = module;
                }
            }
            // cout << "Old track (xp,yp,x,y)=(" << T->TrackXp << ", " << T->TrackYp << ", " << T->TrackX << ", " << T->TrackY
            //      << ")" << endl;
            // cout << "New track (xp,yp,x,y)=(" << xptrack << ", " << yptrack << ", " << xtrack << ", " << ytrack << ")" << endl;
            double dof = double(2 * (*tracknhits)[itrack] - 4);
            trackchi2 /= dof;

            Tchi2ndf = trackchi2;

            if (trackchi2 <= trackchi2_cut)
            {
                if (nevent < NMAX && iter == niter)
                { // fill TRACK arrays
                    NTRACKS++;
                    XTRACK.push_back(xtrack);
                    XPTRACK.push_back(xptrack);
                    YTRACK.push_back(ytrack);
                    YPTRACK.push_back(yptrack);
                    TRACKNHITS.push_back(nhitsonbesttrack);
                }

                if (iter == niter)
                {
                    Tout->Fill();
                }

                // we want to modify this to compute the CHANGE in module parameters required to minimize chi^2;
                //  so the starting parameters are taken as given.
                //  x_0 --> x_0 + dx0
                //  y_0 --> y_0 + dy0
                //  z_0 --> z_0 + dz0
                //  ax --> ax + dax
                //  ay --> ay + day
                //  az --> az + daz
                //  The coefficients of the changes in the parameters should stay the same as those of the parameters themselves, but the RHS needs modified:
                vector<int> HITMODTEMP;
                vector<double> HITXTEMP, HITYTEMP;

                for (int ihit = 0; ihit < NHITS; ihit++)
                {
                    int tridx = int((*hit_trackindex)[ihit]);
                    if (tridx == itrack)
                    {

                        int module = int((*hit_module)[ihit]);

                        double ulocal = (*hit_ulocal)[ihit]; //"U" local: generalized "X"
                        double vlocal = (*hit_vlocal)[ihit]; //"V" local: generalized "Y"

                        double det = mod_Pxu[module] * mod_Pyv[module] - mod_Pyu[module] * mod_Pxv[module]; // cos( alphau) * sin(alphav) - sin(alphau)*cos(alphav) = 1 for alphau = 0, alphav = 90

                        double xlocal = (mod_Pyv[module] * ulocal - mod_Pyu[module] * vlocal) / det; //(sin(alphav)*U - sin(alphau)*V)/det = U = X for alphau = 0, alphav = 90
                        double ylocal = (mod_Pxu[module] * vlocal - mod_Pxv[module] * ulocal) / det; //(cos(alphau)*V - cos(alphav)*U)/det = V = Y for alphau = 0, alphav = 90

                        // On subsequent iterations after the first, we want to fit the changes in the parameters relative to the previous iteration. How can we do this properly?
                        // We need to come up with a new definition for the "local" coordinates that properly accounts for the new coordinate system:
                        // We already re-fit the track; this means that

                        if (nevent < NMAX && iter == niter)
                        {
                            HITMODTEMP.push_back(module);
                            HITXTEMP.push_back(ulocal);
                            HITYTEMP.push_back(vlocal);
                        }
                        TVector3 hitpos_local(xlocal, ylocal, 0);
                        TRotation R;
                        R.RotateX(mod_ax[module]);
                        R.RotateY(mod_ay[module]);
                        R.RotateZ(mod_az[module]);

                        TVector3 modcenter_global(mod_x0[module], mod_y0[module], mod_z0[module]);
                        TVector3 hitpos_global = modcenter_global + R * hitpos_local;

                        double sigma = 0.1e-3;
                        double weight = pow(sigma_hitpos, -2);

                        // int ipar_fix[3] = {3*module,3*module+1,3*module+2};

                        // if( refmod >= 0 && refmod < nmodules ){
                        //   if( module > refmod ){
                        //   // 	ipar_x0 = 6*(module-1);
                        //   // 	ipar_y0 = 6*(module-1)+1;
                        //   // 	ipar_z0 = 6*(module-1)+2;
                        //   // 	ipar_ax = 6*(module-1)+3;
                        //   // 	ipar_ay = 6*(module-1)+4;
                        //   // 	ipar_az = 6*(module-1)+5;

                        //     ipar_fix[0] = 3*(module-1);
                        //     ipar_fix[1] = 3*(module-1)+1;
                        //     ipar_fix[2] = 3*(module-1)+2;
                        //     //   }
                        //     // }
                        //   }
                        // }

                        double xcoeff[6] = {1.0, 0.0, -xptrack, -xptrack * ylocal, xptrack * xlocal, -ylocal};
                        double ycoeff[6] = {0.0, 1.0, -yptrack, -yptrack * ylocal, yptrack * xlocal, xlocal};

                        if (freemodindex.find(module) != freemodindex.end())
                        {
                            int modidx = freemodindex[module];

                            int ipar_x0 = 6 * modidx;
                            int ipar_y0 = 6 * modidx + 1;
                            int ipar_z0 = 6 * modidx + 2;
                            int ipar_ax = 6 * modidx + 3;
                            int ipar_ay = 6 * modidx + 4;
                            int ipar_az = 6 * modidx + 5;

                            int ipar_fix[3] = {3 * modidx, 3 * modidx + 1, 3 * modidx + 2};

                            int ipar[6] = {ipar_x0, ipar_y0, ipar_z0, ipar_ax, ipar_ay, ipar_az};

                            for (int i = 0; i < 6; i++)
                            {
                                for (int j = 0; j < 6; j++)
                                {
                                    M(ipar[i], ipar[j]) += weight * (xcoeff[i] * xcoeff[j] + ycoeff[i] * ycoeff[j]);
                                }
                                b(ipar[i]) += weight * (xcoeff[i] * (xtrack - xlocal) + ycoeff[i] * (ytrack - ylocal));
                                // b(ipar[i]) += xcoeff[i]*xRHS + ycoeff[i]*yRHS;
                            }

                            for (int i = 0; i < 3; i++)
                            {
                                for (int j = 0; j < 3; j++)
                                {
                                    Mpos(ipar_fix[i], ipar_fix[j]) += weight * (xcoeff[i] * xcoeff[j] + ycoeff[i] * ycoeff[j]);
                                    Mrot(ipar_fix[i], ipar_fix[j]) += weight * (xcoeff[i + 3] * xcoeff[j + 3] + ycoeff[i + 3] * ycoeff[j + 3]);
                                }
                                // For the positional offsets, we need to subtract the sum of all alphax, alphay, alphaz dependent terms from the RHS:
                                //  so this is like -xcoeff[i]*(xcoeff[3]*ax + xcoeff[4]*ay + xcoeff[5]*az)-ycoeff[i]*(ycoeff[3]*ax+ycoeff[4]*ay+ycoeff[5]*az)
                                // For the rotational offsets, the opposite is true
                                bpos(ipar_fix[i]) += weight * (xcoeff[i] * (xtrack - xlocal - (xcoeff[3] * mod_ax[module] + xcoeff[4] * mod_ay[module] + xcoeff[5] * mod_az[module])) +
                                                               ycoeff[i] * (ytrack - ylocal - (ycoeff[3] * mod_ax[module] + ycoeff[4] * mod_ay[module] + ycoeff[5] * mod_az[module])));
                                brot(ipar_fix[i]) += weight * (xcoeff[i + 3] * (xtrack - xlocal - (xcoeff[0] * mod_x0[module] + xcoeff[1] * mod_y0[module] + xcoeff[2] * mod_z0[module])) +
                                                               ycoeff[i + 3] * (ytrack - ylocal - (ycoeff[0] * mod_x0[module] + ycoeff[1] * mod_y0[module] + ycoeff[2] * mod_z0[module])));
                            }
                        }
                    }
                }

                if (nevent < NMAX && iter == niter)
                {
                    HITMOD.push_back(HITMODTEMP);
                    HITX.push_back(HITXTEMP);
                    HITY.push_back(HITYTEMP);
                }

                trackchi2_sum += trackchi2;
                ntracks_passed += 1.0;
            }
        }

        oldmeanchi2 = meanchi2;
        meanchi2 = trackchi2_sum / ntracks_passed;

        oldchi2cut = trackchi2_cut;
        trackchi2_cut = std::max(minchi2cut, 5.0 * meanchi2);

        cout << endl
             << "Number of tracks passing chi2 cut = " << ntracks_passed << endl
             << endl;

        // Okay, wish me luck:

        // M.Print();
        // b.Print();

        cout << "Matrix symmetric? = " << M.IsSymmetric() << endl;

        // Here is an idea: to fit the changes of the parameters instead of the parameters themselves, we subtract from the RHS another vector:
        TVectorD PreviousSolution(nparam);
        TVectorD PreviousSolution_posonly(nparam_pos);
        TVectorD PreviousSolution_rotonly(nparam_rot);
        for (int imodule = 0; imodule < nmodules; imodule++)
        {
            if (freemodindex.find(imodule) != freemodindex.end())
            {
                int modidx = freemodindex[imodule];

                int ipar_x0 = modidx * 6;
                int ipar_y0 = modidx * 6 + 1;
                int ipar_z0 = modidx * 6 + 2;
                int ipar_ax = modidx * 6 + 3;
                int ipar_ay = modidx * 6 + 4;
                int ipar_az = modidx * 6 + 5;

                PreviousSolution(ipar_x0) = mod_x0[imodule];
                PreviousSolution(ipar_y0) = mod_y0[imodule];
                PreviousSolution(ipar_z0) = mod_z0[imodule];
                PreviousSolution(ipar_ax) = mod_ax[imodule];
                PreviousSolution(ipar_ay) = mod_ay[imodule];
                PreviousSolution(ipar_az) = mod_az[imodule];

                int iparx = 3 * modidx;
                int ipary = 3 * modidx + 1;
                int iparz = 3 * modidx + 2;

                // if( imodule != refmod ){

                PreviousSolution_posonly(iparx) = mod_x0[imodule];
                PreviousSolution_posonly(ipary) = mod_y0[imodule];
                PreviousSolution_posonly(iparz) = mod_z0[imodule];

                PreviousSolution_rotonly(iparx) = mod_ax[imodule];
                PreviousSolution_rotonly(ipary) = mod_ay[imodule];
                PreviousSolution_rotonly(iparz) = mod_az[imodule];
            }
        }

        TVectorD bshift = b - M * PreviousSolution;
        TVectorD bshiftpos = bpos - Mpos * PreviousSolution_posonly;
        TVectorD bshiftrot = brot - Mrot * PreviousSolution_rotonly;

        TVectorD Solution(nparam);
        TVectorD Solution_posonly(nparam_pos);
        TVectorD Solution_rotonly(nparam_rot);

        M.Invert();
        Mpos.Invert();
        Mrot.Invert();

        if (iter >= 0)
        {
            Solution = M * bshift;
            Solution_posonly = Mpos * bshiftpos;
            Solution_rotonly = Mrot * bshiftrot;
        }
        else
        {
            Solution = M * b;
            Solution_posonly = Mpos * bpos;
            Solution_rotonly = Mrot * brot;
        }

        // M.Print();

        // Solution.Print();

        map<int, double> prev_x0 = mod_x0;
        map<int, double> prev_y0 = mod_y0;
        map<int, double> prev_z0 = mod_z0;
        map<int, double> prev_ax = mod_ax;
        map<int, double> prev_ay = mod_ay;
        map<int, double> prev_az = mod_az;

        double startpar[6 * nmodules];

        for (int imodule = 0; imodule < nmodules; imodule++)
        {
            if (freemodindex.find(imodule) != freemodindex.end())
            {
                int modidx = freemodindex[imodule];
                int ipar_x0 = modidx * 6;
                int ipar_y0 = modidx * 6 + 1;
                int ipar_z0 = modidx * 6 + 2;
                int ipar_ax = modidx * 6 + 3;
                int ipar_ay = modidx * 6 + 4;
                int ipar_az = modidx * 6 + 5;

                mod_x0[imodule] = Solution(ipar_x0);
                mod_y0[imodule] = Solution(ipar_y0);
                mod_z0[imodule] = Solution(ipar_z0);
                mod_ax[imodule] = Solution(ipar_ax);
                mod_ay[imodule] = Solution(ipar_ay);
                mod_az[imodule] = Solution(ipar_az);

                if (iter >= 0)
                {
                    mod_x0[imodule] += prev_x0[imodule];
                    mod_y0[imodule] += prev_y0[imodule];
                    mod_z0[imodule] += prev_z0[imodule];
                    mod_ax[imodule] += prev_ax[imodule];
                    mod_ay[imodule] += prev_ay[imodule];
                    mod_az[imodule] += prev_az[imodule];
                }

                // Strictly speaking, I don't think this is necessary:
                PreviousSolution(ipar_x0) = mod_x0[imodule];
                PreviousSolution(ipar_y0) = mod_y0[imodule];
                PreviousSolution(ipar_z0) = mod_z0[imodule];
                PreviousSolution(ipar_ax) = mod_ax[imodule];
                PreviousSolution(ipar_ay) = mod_ay[imodule];
                PreviousSolution(ipar_az) = mod_az[imodule];

                //	}
                mod_dx0[imodule] = sqrt(fabs(M(ipar_x0, ipar_x0)));
                mod_dy0[imodule] = sqrt(fabs(M(ipar_y0, ipar_y0)));
                mod_dz0[imodule] = sqrt(fabs(M(ipar_z0, ipar_z0)));
                mod_dax[imodule] = sqrt(fabs(M(ipar_ax, ipar_ax)));
                mod_day[imodule] = sqrt(fabs(M(ipar_ay, ipar_ay)));
                mod_daz[imodule] = sqrt(fabs(M(ipar_az, ipar_az)));

                int iparx = 3 * modidx;
                int ipary = 3 * modidx + 1;
                int iparz = 3 * modidx + 2;

                if (offsetsonlyflag != 0)
                {
                    mod_x0[imodule] = Solution_posonly(iparx) + prev_x0[imodule];
                    mod_y0[imodule] = Solution_posonly(ipary) + prev_y0[imodule];
                    mod_z0[imodule] = Solution_posonly(iparz) + prev_z0[imodule];

                    mod_ax[imodule] = prev_ax[imodule];
                    mod_ay[imodule] = prev_ay[imodule];
                    mod_az[imodule] = prev_az[imodule];
                }

                if (rotationsonlyflag != 0)
                {
                    mod_x0[imodule] = prev_x0[imodule];
                    mod_y0[imodule] = prev_y0[imodule];
                    mod_z0[imodule] = prev_z0[imodule];

                    mod_ax[imodule] = Solution_rotonly(iparx) + prev_ax[imodule];
                    mod_ay[imodule] = Solution_rotonly(ipary) + prev_ay[imodule];
                    mod_az[imodule] = Solution_rotonly(iparz) + prev_az[imodule];
                }
            }

            for (int ipar = 0; ipar < 6; ipar++)
            {
                startpar[imodule * 6 + ipar] = 0.0;
            }
        }

        cout << "ending solution: " << endl;
        for (map<int, double>::iterator imod = mod_x0.begin(); imod != mod_x0.end(); ++imod)
        {
            int module = imod->first;
            cout << "Module " << module << ": (x0,y0,z0,ax,ay,az)=("
                 << mod_x0[module] << ", " << mod_y0[module] << ", " << mod_z0[module] << ", "
                 << mod_ax[module] << ", " << mod_ay[module] << ", " << mod_az[module] << ")" << endl;
        }

        oldmaxposchange = fabs(maxposchange);
        oldmaxanglechange = fabs(maxanglechange);

        maxposchange = 0.0;
        maxanglechange = 0.0;

        for (map<int, double>::iterator imod = mod_x0.begin(); imod != mod_x0.end(); ++imod)
        {
            int module = imod->first;
            cout << "(Change from previous)/sigma: (dx0,dy0,dz0,dax,day,daz)=("
                 << (mod_x0[module] - prev_x0[module]) / mod_dx0[module] << ", "
                 << (mod_y0[module] - prev_y0[module]) / mod_dy0[module] << ", "
                 << (mod_z0[module] - prev_z0[module]) / mod_dz0[module] << ", "
                 << (mod_ax[module] - prev_ax[module]) / mod_dax[module] << ", "
                 << (mod_ay[module] - prev_ay[module]) / mod_day[module] << ", "
                 << (mod_az[module] - prev_az[module]) / mod_daz[module] << ")" << endl;

            TVector3 poschange(mod_x0[module] - prev_x0[module],
                               mod_y0[module] - prev_y0[module],
                               mod_z0[module] - prev_z0[module]);

            TVector3 anglechange(mod_ax[module] - prev_ax[module],
                                 mod_ay[module] - prev_ay[module],
                                 mod_az[module] - prev_az[module]);

            maxposchange = fabs(poschange.X()) > maxposchange ? fabs(poschange.X()) : maxposchange;
            maxposchange = fabs(poschange.Y()) > maxposchange ? fabs(poschange.Y()) : maxposchange;
            maxposchange = fabs(poschange.Z()) > maxposchange ? fabs(poschange.Z()) : maxposchange;

            maxanglechange = fabs(anglechange.X()) > maxanglechange ? fabs(anglechange.X()) : maxanglechange;
            maxanglechange = fabs(anglechange.Y()) > maxanglechange ? fabs(anglechange.Y()) : maxanglechange;
            maxanglechange = fabs(anglechange.Z()) > maxanglechange ? fabs(anglechange.Z()) : maxanglechange;
        }

        cout << "iteration " << iter << ", max position change = " << maxposchange << " mm, max angle change = " << maxanglechange << " rad" << endl;
    }

    // cout << "NTRACKS = " << NTRACKS << ", XTRACK.size() = " << XTRACK.size()
    //      << ", YTRACK.size() = " << YTRACK.size() << ", XPTRACK.size() = " << XPTRACK.size()
    //      << ", YPTRACK.size() = " << YPTRACK.size() << ", TRACKNHITS.size() = " << TRACKNHITS.size()
    //      << ", HITMOD.size() = " << HITMOD.size() << ", HITX.size() = " << HITX.size()
    //      << ", HITY.size() = " << HITY.size() << endl;

    // if( (offsetsonlyflag == 0 && rotationsonlyflag == 0) ){

    ofstream outfile(outputfilename);

    // Also write the alignment stuff in the format that the SBS-offline database wants:
    TString dbfilename;
    dbfilename.Form("db_align_%s.dat", prefix.Data());

    ofstream outfile_DB(dbfilename.Data());

    // outfile << "mod_x0 ";
    TString x0line = "mod_x0 ", y0line = "mod_y0 ", z0line = "mod_z0 ";
    TString axline = "mod_ax ", ayline = "mod_ay ", azline = "mod_az ";
    for (map<int, double>::iterator imod = mod_x0.begin(); imod != mod_x0.end(); ++imod)
    {
        int module = imod->first;
        TString stemp;
        stemp.Form(" %15.7g", mod_x0[module]);
        x0line += stemp;
        stemp.Form(" %15.7g", mod_y0[module]);
        y0line += stemp;
        stemp.Form(" %15.7g", mod_z0[module]);
        z0line += stemp;
        stemp.Form(" %15.7g", mod_ax[module]);
        axline += stemp;
        stemp.Form(" %15.7g", mod_ay[module]);
        ayline += stemp;
        stemp.Form(" %15.7g", mod_az[module]);
        azline += stemp;

        stemp.Form("%s.m%d.position = %15.7g %15.7g %15.7g", prefix.Data(), module,
                   mod_x0[module], mod_y0[module], mod_z0[module]);

        outfile_DB << stemp << endl;

        stemp.Form("%s.m%d.angle = %15.7g %15.7g %15.7g", prefix.Data(), module,
                   mod_ax[module] * 180.0 / PI, mod_ay[module] * 180.0 / PI, mod_az[module] * 180.0 / PI);

        outfile_DB << stemp << endl;
    }
    outfile << x0line << endl;
    outfile << y0line << endl;
    outfile << z0line << endl;
    outfile << axline << endl;
    outfile << ayline << endl;
    outfile << azline << endl;

    elist->Delete();

    fout->Write();
    fout->Close();
}
