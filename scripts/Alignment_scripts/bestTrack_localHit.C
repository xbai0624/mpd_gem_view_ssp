#include <TFile.h> 
#include <TTree.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <TSystem.h>
#include <iomanip>
#include <cstdio>

//Helper to clean final text file
void cleanEmptyLines(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream inputFile(inputPath);
    std::ofstream outputFile(outputPath);

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open input file." << std::endl;
        return;
    }

    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
        inputFile.close();
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
      if (line.find("Layer") != std::string::npos) {
            outputFile << line << std::endl;
        }
    }

    inputFile.close();
    outputFile.close();
}

void bestTrack_localHit(const char *clusterRootName = "default") 
{
  //gSystem->Load("libRIO");
  //gSystem->Load("libtree");
  //gSystem->Load("libPhysics");
  
    if (strcmp(clusterRootName, "default") == 0) { //Check if input root file is typed in
	std::cout << "Input GEMCluster root file with tracking on IS NEEDED" << endl;
	return; 
    }
   	
    TFile* inputFile = TFile::Open(clusterRootName, "READ"); //Access input cluster root file
    if (!inputFile || inputFile->IsZombie()) { //Error check in case problem with file
	std::cerr << "Error opening file: " << clusterRootName << std::endl; 
	return; 
    }

    TTree *inputTree = (TTree*)inputFile->Get("GEMCluster;1"); //Directly get TTree from file
    if (!inputTree) {
	std::cerr << "Error: TTree could not be retrieved from file: " << inputFile->GetName() << endl; 
    }

    std::string outputPath = "temp_all_localHits_bestTrack.txt"; //Setting output path for text file
    std::ofstream outputFile(outputPath, std::ofstream::out | std::ofstream::trunc); 
    if (!outputFile.is_open()) {
	std::cerr << "Failed to open file for writing" << std::endl; 
	return; 
    }

    std::string clean_outputPath = "all_localHits_bestTrack.txt"; //String for final text file cleaned up

    //Initializing variables to hold data from branches
    int trackCandidates; 
    std::vector<int> *hit_trackIndex = nullptr, *besttrack_hitLayer = nullptr; 
    std::vector<double> *hit_xLocal = nullptr, *hit_yLocal = nullptr, *hit_zLocal = nullptr; 

    // Disable all branches
    inputTree->SetBranchStatus("*",0);

    //Enable specific branches
    //inputTree->SetBranchStatus("besttrack", 1); //Identify which track is best
    inputTree->SetBranchStatus("fHitTrackIndex", 1); //Find which hits belong to the best track
    inputTree->SetBranchStatus("fBestTrackHitLayer", 1); //Determine the layer index for each hit
    inputTree->SetBranchStatus("fHitXlocal", 1); //Extract X coordinates from branch
    inputTree->SetBranchStatus("fHitYlocal", 1); //Extract Y coordinates from branch
    inputTree->SetBranchStatus("fHitZlocal", 1); //Extract Z coordinates from branch
    inputTree->SetBranchStatus("fNAllGoodTrackCandidates", 1);
    
    //Assign variable pointers for each tree
    //inputTree->SetBranchAddress("besttrack", &besttrack); 
    inputTree->SetBranchAddress("fHitTrackIndex", &hit_trackIndex); 
    inputTree->SetBranchAddress("fBestTrackHitLayer", &besttrack_hitLayer); 
    inputTree->SetBranchAddress("fHitXlocal", &hit_xLocal);
    inputTree->SetBranchAddress("fHitYlocal", &hit_yLocal); 
    inputTree->SetBranchAddress("fHitZlocal", &hit_zLocal);
    inputTree->SetBranchAddress("fNAllGoodTrackCandidates", &trackCandidates); 

    /*
    //Initialize vectors to save layer + coordinates
    std::vector<double> bestTrackHitX; 
    std::vector<double> bestTrackHitY; 
    std::vector<double> bestTrackHitZ; 
    std::vector<int> bestTrackHitLayer; 
    */

    //Loop over all entries 
    for (int i = 0; i < inputTree->GetEntries(); ++i) {
	inputTree->GetEntry(i); //Select entry
        
	// Get index of best track
	// int bestTrackIndex = besttrack;
	int totalTracks = trackCandidates;
	
	outputFile << "Evt:" << std::setw(6) << std::left <<i;
	
	// Loop over hits to find those corresponding to the best track, which should be track 1
	if (hit_trackIndex != nullptr && besttrack_hitLayer != nullptr && hit_xLocal != nullptr && hit_yLocal != nullptr && hit_zLocal != nullptr && totalTracks > 0) {
		for (size_t j = 0; j < hit_trackIndex->size(); ++j) {
		  if (hit_trackIndex->at(j) == 0){
		    if (j >= hit_xLocal->size() || j >= hit_yLocal->size() || j >= hit_zLocal->size() || j >= besttrack_hitLayer->size()) {
                        std::cerr << "Index out of range: " << j << std::endl;
                        continue;
                    }
		    
		    // Get the x, y, and z coordinates of the hit
		    double x = hit_xLocal->at(j); 
		    double y = hit_yLocal->at(j); 
		    double z = hit_zLocal->at(j); 

		    // Get the layer index of said hit 
		    int layer = besttrack_hitLayer->at(j);

		    outputFile << "   Layer:" << layer
			       << " (" << std::setw(10) << x
			       << "," << std::setw(10) << y
			       << "," << z << ")";
		  }
		}
	}

	outputFile << endl; 
    }
 
    outputFile.close();
    inputFile->Close();

    cleanEmptyLines(outputPath, clean_outputPath);

    if (remove(outputPath.c_str()) != 0) {
        std::cerr << "Error deleting file: " << outputPath << std::endl;
    } else {
        std::cout << "File successfully deleted: " << outputPath << std::endl;
    }

    return; 
}
