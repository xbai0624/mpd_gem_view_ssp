#include <iostream> 
#include <TH1.h>
#include <TFile.h> 
#include <TKey.h> 
#include <TClass.h> 
#include <TROOT.h> 
#include <TF1.h> 

//A helper function to generate the gaus fit equation needed 
TF1* gaus_fit(TH1F* hist){
    int nBins = hist->GetNbinsX(); 
    int maxBin = hist->GetMaximumBin(); 
    double maxCount = hist->GetBinContent(maxBin);
    double percentage = 0.1;  
    double limitCount = maxCount / (1/percentage);
    double low = -1, high = -1; 
    
     for (int i = 1; i <= maxBin; ++i) {
        if (hist->GetBinContent(i) >= limitCount) {
            double diff_A = std::abs(hist->GetBinContent(i) - limitCount);
            double diff_B = std::abs(hist->GetBinContent(i-1) - limitCount);

            if (diff_A <= diff_B) {
                low = hist->GetBinLowEdge(i); 
                break;
            }

            if (diff_B < diff_A){ 
                low = hist->GetBinLowEdge(i - 1);  
                break; 
                }
        }
    }

    for (int i = maxBin; i <= nBins; ++i) {
        if (hist->GetBinContent(i) <= limitCount) {
            double diff_A = std::abs(hist->GetBinContent(i) - limitCount);
            double diff_B = std::abs(hist->GetBinContent(i-1) - limitCount);

            if (diff_A <= diff_B) {
                high = hist->GetBinLowEdge(i) + hist->GetBinWidth(i); 
                break;
            }

            if (diff_B < diff_A){ 
                high = hist->GetBinLowEdge(i - 1) + hist->GetBinWidth(i - 1);  
                break; 
                }
        }
    }

    if (low == -1 || high == -1) {
        std::cerr << "Failed to determine fitting range: low = " << low << ", high = " << high << std::endl;
        return nullptr;
    }

    TF1* fitFunc = new TF1("fitFunc", "gaus", low, high);
    //std::cout << "Fitting from: " << low << " to " << high << std::endl;
    return fitFunc;
}

void setHistogramFitStats(TH1F* hist) {
    hist->SetStats(1);
    hist->Draw(); 
    gPad->Update(); 

    TPaveStats* stats = (TPaveStats*)hist->GetListOfFunctions()->FindObject("stats");
    if (stats) {
        stats->SetOptStat(11); // Hide the default statistics
        stats->SetOptFit(1111);
    }
    gPad->Modified();  // Ensure the pad is updated with the new statistics box
    gPad->Update();    // Final update to the pad
}

void plot_posRes_fit(const char* filename = "default",  int runNum = -1)
{   
    gErrorIgnoreLevel = kError;

    bool runNumFlag = false; 
    if (runNum > 0) runNumFlag = true; 

    TFile* f1 = TFile::Open(filename, "READ"); //Establish variable for path to desired rootfile
    if (!f1 || f1->IsZombie()) { //Error check in case problem with file 
	std::cerr << "Error opening file: " << filename << std::endl; 
	return; 
    }

    /*
    std::string defaultPath = "/home/daq/data_viewer/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.root"; 
    std::string outputPath = "/home/daq/data_viewer/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.root";
    if (runNumFlag) outputPath = Form("/home/daq/data_viewer/mpd_gem_view_ssp/Rootfiles/fitted_positionResolution_%d.root", runNum); 
    */

    std::string defaultPath = "/media/minh-dao/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.root";
    std::string outputPath = "/media/minh-dao/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.root";
    if (runNumFlag) outputPath = Form("/media/minh-dao/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/mpd_gem_view_ssp/Rootfiles/fitted_positionResolution_%d.root", runNum);

    TFile* outputFile; 
    TFile* defaultFile; 

    if (outputPath == defaultPath && !runNumFlag) {

    	outputFile = TFile::Open(outputPath.c_str(), "RECREATE"); 

    } else { 
	
	outputFile = TFile::Open(outputPath.c_str(), "RECREATE"); 
	defaultFile = TFile::Open(defaultPath.c_str(), "RECREATE"); 
	
    }

    if (!outputFile || outputFile->IsZombie()) {
                std::cerr << "Error creating output file (input/default faulty): path/to/mpd_gem_view_ssp/scripts/tmp_plots/fitted_histograms.root" << std::endl;
                f1->Close();
                delete f1;
		return;
    }

    gROOT->SetBatch(kTRUE); 

    // Get the list of keys in the file 
    TIter keyList(f1->GetListOfKeys()); 
    TKey* key;
    std::vector<TH1F*> fittedHistVector;
    std::vector<TH1F*> zoomedHistVector; 
    std::vector<TH1F*> zoomed10mmHistVector; 
  
    TH1F* zoomedHist;
    TH1F* zoomed10mmHist;
    TH1F* fittedHist; 
    TF1* fitFunc; 

    // Iterate through the keys 
    while ((key = (TKey*)keyList())) {
	// Get the class of the object
	TClass* cl = gROOT->GetClass(key->GetClassName()); 
	if (!cl->InheritsFrom("TH1F")) continue; // Skip non-histogram objects 

	// Get the histogram object
	TH1F* hist = (TH1F*)key->ReadObj(); 
	std::string histName = hist->GetName();
	
	if (!hist) {
	    std::cerr << "Error retrieving histogram: " << histName << std::endl; 
	    f1->Close();
	    delete f1; 
	    return; 
	}

	// Check if the histogram name contains the specified labels
	if ((histName.find("h_xresidue_gem") != std::string::npos || histName.find("h_yresidue_gem") != std::string::npos) && histName.find("_fitted") == std::string::npos) {
	    std::cout  << "Found histogram: " << histName << "\n" << endl;  
		
	    if (hist->GetEntries() == 0) {
                std::cerr << "Histogram " << histName << " is empty." << std::endl;
                continue;
            }

	    // Fit the histogram 
	    std::string fittedHistName = histName + "_fitted";
	    fittedHist = dynamic_cast<TH1F*>(hist->Clone(fittedHistName.c_str()));

	    std::string zoomed10mmHistName = fittedHistName + "_zoomed_10mm";
            zoomed10mmHist = dynamic_cast<TH1F*>(hist->Clone(zoomed10mmHistName.c_str()));
	   
	    std::string zoomedHistName = fittedHistName + "_zoomed_2mm";
            zoomedHist = dynamic_cast<TH1F*>(hist->Clone(zoomedHistName.c_str()));

	    if (!fittedHist) {
                std::cerr << "Error cloning histogram: " << histName << std::endl;
                continue;
   	    }

	    fitFunc = gaus_fit(fittedHist); //use helper function to create gaus fit parameters
	    if (fitFunc) {

                fittedHist->Fit(fitFunc, "RQ"); //fit the copied histogram 
		std::string newTitle = (std::string)fittedHist->GetTitle() + " (Full Range)";
       		fittedHist->SetTitle(newTitle.c_str()); 
		setHistogramFitStats(fittedHist); 

		zoomed10mmHist->SetAxisRange(-10,10,"X"); 
		fitFunc = gaus_fit(zoomed10mmHist); 
                zoomed10mmHist->Fit(fitFunc, "RQ"); 
		newTitle = (std::string)zoomed10mmHist->GetTitle() + " (-10mm to 10mm)";
		zoomed10mmHist->SetTitle(newTitle.c_str()); 
		setHistogramFitStats(zoomed10mmHist); 

		zoomedHist->SetAxisRange(-2,2,"X"); 
		fitFunc = gaus_fit(zoomedHist); 
		zoomedHist->Fit(fitFunc, "RQ"); 
		newTitle = (std::string)zoomedHist->GetTitle() + " (-2mm to 2mm)"; 
		zoomedHist->SetTitle(newTitle.c_str()); 
		setHistogramFitStats(zoomedHist);

		outputFile->cd(); 
                fittedHist->Write(); // Write the fitted histogram onto the rootfile 
		zoomed10mmHist->Write(); 
		zoomedHist->Write();

		if (runNumFlag) {
		    defaultFile->cd(); 
		    fittedHist->Write(); 
		    zoomed10mmHist->Write();
		    zoomedHist->Write(); 
		
		}
		fittedHistVector.push_back(fittedHist); // Store the fitted histogram for drawing
		zoomedHistVector.push_back(zoomedHist); // Store the zoomed histogram for drawing 
		zoomed10mmHistVector.push_back(zoomed10mmHist); //Store the 10mm zoomed histogram for drawing

            } else {
                std::cerr << "Failed to create fit function for: " << histName << std::endl;
            }
	}
    }

    if (fittedHistVector.empty()) {
	std::cerr << "\nNo fitted histograms to draw." << std::endl;
	f1->Close(); 
	outputFile->Close();
	delete f1;
	delete outputFile;
	gErrorIgnoreLevel = kInfo;
	return; 
    }

    gErrorIgnoreLevel = kInfo; 
    
    int nHists = fittedHistVector.size(); 
    int nZoomedHists = zoomedHistVector.size(); 
    int nZoomed10mmHists = zoomed10mmHistVector.size(); 
    int nCols = TMath::CeilNint(TMath::Sqrt(nHists)); 
    int nRows = TMath::CeilNint(nHists / static_cast<double>(nCols)); 
    TCanvas* canvas = new TCanvas("Fitted_Histograms_Canvas", "Fitted Histograms", 800, 600);
    canvas->Divide(nCols, nRows); 

    for (int i = 0; i < nHists; ++i) {
	canvas->cd(i+1);
	gStyle->SetOptFit(111); 
	fittedHistVector[i]->Draw();
	gPad->Update(); 
    }

    // std::string defaultPdfPath = "/home/daq/data_viewer/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.pdf";
    std::string defaultPdfPath = "/media/minh-dao/Dual_OS_Drive/Liyanage_MPGD_Files/DAQ_Analysis_Software/mpd_gem_view_ssp/scripts/tmp_plots/fitted_positionResolution.pdf";

    canvas->Update();
    canvas->Print((defaultPdfPath + "(").c_str());
    if (runNumFlag) canvas->Print((outputPath + ".pdf(").c_str());
   
    TCanvas* multiPageCanvas;
    int histIndex = 0;
    while (histIndex < nHists) {
        multiPageCanvas = new TCanvas(Form("Canvas_%d", histIndex / 4 + 1), "Fitted Histograms", 800, 600);
        multiPageCanvas->Divide(2, 2);  // 2x2 grid for up to 4 histograms per page

        for (int i = 0; i < 4 && histIndex < nHists; ++i, ++histIndex) {
            multiPageCanvas->cd(i + 1);
	    gStyle->SetOptFit(111); 
            fittedHistVector[histIndex]->Draw();
            gPad->Update();
        }

        multiPageCanvas->Update();
        multiPageCanvas->Print(defaultPdfPath.c_str());
	if (runNumFlag) multiPageCanvas->Print((outputPath + ".pdf").c_str()); 
        delete multiPageCanvas;
    }

    int zoomed10mmHistIndex = 0; 
    while (zoomed10mmHistIndex < nZoomed10mmHists) {
	multiPageCanvas = new TCanvas(Form("Canvas_Zoomed10mm_%d", zoomed10mmHistIndex / 4 + 1), "Zoomed 10mm in Fitted Histograms", 800, 600); 
	multiPageCanvas->Divide(2,2); 

	for (int i = 0; i < 4 && zoomed10mmHistIndex < nZoomed10mmHists; ++i, ++zoomed10mmHistIndex) {
	    multiPageCanvas->cd(i+1);
	    zoomed10mmHistVector[zoomed10mmHistIndex]->Draw(); 
	    gPad->Update();
	}

	multiPageCanvas->Update();
	multiPageCanvas->Print(defaultPdfPath.c_str()); 
	if (runNumFlag) multiPageCanvas->Print((outputPath + ".pdf").c_str()); 
	delete multiPageCanvas; 
    }

    int zoomedHistIndex = 0; 
    while (zoomedHistIndex < nZoomedHists) {
	multiPageCanvas = new TCanvas(Form("Canvas_Zoomed_%d", zoomedHistIndex / 4 + 1), "Zoomed 2mm in Fitted Histograms", 800, 600); 
	multiPageCanvas->Divide(2,2); 

	for (int i = 0; i < 4 && zoomedHistIndex < nZoomedHists; ++i, ++zoomedHistIndex) {
	    multiPageCanvas->cd(i+1);
	    zoomedHistVector[zoomedHistIndex]->Draw(); 
	}

	multiPageCanvas->Update();
	
	if (zoomedHistIndex == nZoomedHists) {
	    multiPageCanvas->Print((defaultPdfPath + ")").c_str());
	    if (runNumFlag) multiPageCanvas->Print((outputPath + ".pdf)").c_str()); 
	} else {
	    multiPageCanvas->Print(defaultPdfPath.c_str());
	    if (runNumFlag) multiPageCanvas->Print((outputPath + ".pdf").c_str()); 
	}

	delete multiPageCanvas; 
    }

    delete canvas;
    const char* outputFilename = outputFile->GetName();
    std::cout << "Output root file is: " << outputFilename << endl;
    if (runNumFlag) {
	std::cout << "Default output root file is: " << defaultFile->GetName() << endl; 
    }

    f1->Close(); 
    outputFile->Close();
    delete outputFile;
    delete f1; 

    if (runNumFlag) {
	defaultFile->Close();
	delete defaultFile; 
    }

    TCanvas* defaultCanvas = (TCanvas*)gROOT->FindObject("c1");
    if (defaultCanvas) {
        std::cout << "Deleting unwanted default canvas 'c1'." << std::endl;
        delete defaultCanvas;
    }

    gROOT->SetBatch(kFALSE); 
    
    std::cout << "Completed fitting residue histograms" << endl;  
}


