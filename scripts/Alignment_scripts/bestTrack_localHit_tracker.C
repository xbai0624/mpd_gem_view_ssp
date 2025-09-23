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
	    std::cout << "Input GEMCluster root file with tracking on IS NEEDED" << std::endl;
	    return; 
    }
   	
    TFile* inputFile = TFile::Open(clusterRootName, "READ"); //Access input cluster root file
    if (!inputFile || inputFile->IsZombie()) { //Error check in case problem with file
	    std::cerr << "Error opening file: " << clusterRootName << std::endl; 
	    return; 
    }

    TTree *inputTree = (TTree*)inputFile->Get("GEMCluster"); //Directly get TTree from file
    if (!inputTree) {
	    std::cerr << "Error: TTree could not be retrieved from file: " << inputFile->GetName() << std::endl; 
    }
    //inputTree->Print();

    std::string outputPath = "temp_all_localHits_bestTrack.txt"; //Setting output path for text file
    std::ofstream outputFile(outputPath, std::ofstream::out | std::ofstream::trunc); 
    if (!outputFile.is_open()) {
	    std::cerr << "Failed to open file for writing" << std::endl; 
	    return; 
    }

    std::string clean_outputPath = "all_localHits_bestTrack.txt"; //String for final text file cleaned up

    std::string healthPath = "health.txt";
    std::ofstream healthFile(healthPath, std::ofstream::out | std::ofstream::trunc);
    if (!healthFile.is_open()) {
	    std::cerr << "Failed to open health file for writing" << std::endl;
	    return;
    }

    std::string filter1Path = "filter1.txt"; 
    std::ofstream filter1File(filter1Path, std::ofstream::out | std::ofstream::trunc);
    if (!filter1File.is_open()) {
	    std::cerr << "Failed to open filter1 file for writing" << std::endl;
	    return;
    }

    std::string filter2Path = "filter2.txt";
    std::ofstream filter2File(filter2Path, std::ofstream::out | std::ofstream::trunc);
    if (!filter2File.is_open()) {
        std::cerr << "Failed to open filter2 file for writing" << std::endl;
        return;
    }

    //Initializing variables to hold data from branches
    int trackCandidates, NTracks_found, nCluster; 
    std::vector<int> *hit_trackIndex = nullptr, *besttrack_hitLayer = nullptr, *hit_module = nullptr, *planeID = nullptr, *axis = nullptr; 
    std::vector<double> *hit_xLocal = nullptr, *hit_yLocal = nullptr, *hit_zLocal = nullptr;

    // Disable all branches
    inputTree->SetBranchStatus("*",0);

    //Enable specific branches
    //inputTree->SetBranchStatus("besttrack", 1); //Identify which track is best
    inputTree->SetBranchStatus("axis", 1);
    inputTree->SetBranchStatus("planeID", 1);
    inputTree->SetBranchStatus("nCluster", 1); 
    inputTree->SetBranchStatus("fHitModule", 1); // Make sure each layer only get hit once
    inputTree->SetBranchStatus("fNtracks_found", 1); // Select events where there should only be 1 track
    inputTree->SetBranchStatus("fHitTrackIndex", 1); //Find which hits belong to the best track
    inputTree->SetBranchStatus("fBestTrackHitLayer", 1); //Determine the layer index for each hit
    inputTree->SetBranchStatus("fHitXlocal", 1); //Extract X coordinates from branch
    inputTree->SetBranchStatus("fHitYlocal", 1); //Extract Y coordinates from branch
    inputTree->SetBranchStatus("fHitZlocal", 1); //Extract Z coordinates from branch
    inputTree->SetBranchStatus("fNAllGoodTrackCandidates", 1);
    
    //Assign variable pointers for each tree
    //inputTree->SetBranchAddress("besttrack", &besttrack);
    inputTree->SetBranchAddress("axis", &axis); 
    inputTree->SetBranchAddress("planeID", &planeID); 
    inputTree->SetBranchAddress("nCluster", &nCluster); 
    inputTree->SetBranchAddress("fHitModule", &hit_module); 
    inputTree->SetBranchAddress("fNtracks_found", &NTracks_found);
    inputTree->SetBranchAddress("fHitTrackIndex", &hit_trackIndex); 
    inputTree->SetBranchAddress("fBestTrackHitLayer", &besttrack_hitLayer); 
    inputTree->SetBranchAddress("fHitXlocal", &hit_xLocal);
    inputTree->SetBranchAddress("fHitYlocal", &hit_yLocal); 
    inputTree->SetBranchAddress("fHitZlocal", &hit_zLocal);
    inputTree->SetBranchAddress("fNAllGoodTrackCandidates", &trackCandidates); 

    if (!inputTree->GetBranch("planeID")) {
        std::cerr << "Branch 'planeID' does not exist!" << std::endl;
        return;
    }

    //Loop over all entries
    
    std::cout << "All setup is done, moving on to event selection" << std::endl;

    for (int i = 0; i < inputTree->GetEntries(); ++i) {
	    inputTree->GetEntry(i); //Select entry

	    int tracker0 = 0;
	    int tracker0_x = 0; 
	    int tracker0_y = 0;

	    int tracker1 = 0;
	    int tracker1_x = 0;
        int tracker1_y = 0;

	    int tracker4 = 0; 
	    int tracker4_x = 0;
        int tracker4_y = 0;

	    int tracker5 = 0;
	    int tracker5_x = 0;
        int tracker5_y = 0;
	
	    //healthFile << "Event: " << i << endl;   

	    for(int j = 0; j < nCluster; ++j) {
	        //healthFile << "planeID num : " << planeID->at(j) << endl;  
	        if (planeID->at(j) == 0) {
	            tracker0++;
	            if (axis->at(j) == 0) tracker0_x++; 
	            if (axis->at(j) == 1) tracker0_y++;
	        }

            if (planeID->at(j) == 1) {
                tracker1++;
                if (axis->at(j) == 0) tracker1_x++;
                if (axis->at(j) == 1) tracker1_y++;
            }

            if (planeID->at(j) == 4) {
                tracker4++;
                if (axis->at(j) == 0) tracker4_x++;
                if (axis->at(j) == 1) tracker4_y++;
            }

            if (planeID->at(j) == 5) {
                tracker5++; 
                if (axis->at(j) == 0) tracker5_x++;
                if (axis->at(j) == 1) tracker5_y++;
            }
	    } 
	
        if (tracker0 != 2 || tracker1 != 2 || tracker4 != 2 || tracker5 != 2) continue;
        if (NTracks_found != 1) continue;

	    healthFile << "Event: " << i << std::endl;
        healthFile << "Tracker0 Cluster Count (X+Y): " << tracker0 << std::endl;
        healthFile << "Tracker0 X Axis Cluster Num: " << tracker0_x << std::endl;
        healthFile << "Tracker0 y Axis Cluster Num: " << tracker0_y << std::endl;
        healthFile << "Tracker1 Cluster Count (X+Y): " << tracker1 << std::endl;
        healthFile << "Tracker1 X Axis Cluster Num: " << tracker1_x << std::endl;
        healthFile << "Tracker1 y Axis Cluster Num: " << tracker1_y << std::endl;
        healthFile << "Tracker4 Cluster Count (X+Y): " << tracker4 << std::endl;
        healthFile << "Tracker4 X Axis Cluster Num: " << tracker4_x << std::endl;
        healthFile << "Tracker4 y Axis Cluster Num: " << tracker4_y << std::endl;
        healthFile << "Tracker5 Cluster Count (X+Y): " << tracker5 << std::endl;
        healthFile << "Tracker5 X Axis Cluster Num: " << tracker5_x << std::endl;
        healthFile << "Tracker5 y Axis Cluster Num: " << tracker5_y << std::endl;

        //if(tracker0 != 2 || tracker1 != 2 || tracker5 != 2 || tracker6 != 2) continue;
        healthFile << "NTracks Found : " << NTracks_found << std::endl;

        int totalTracks = trackCandidates;
	
        outputFile << "Evt:" << std::setw(6) << std::left << i;
        filter1File << "Event Passed Check 1: " << i << std::endl;
	
        // Loop over hits to find those corresponding to the best track, which should be track 1
        if (hit_trackIndex != nullptr && hit_xLocal != nullptr && hit_yLocal != nullptr && hit_zLocal != nullptr && totalTracks == 1) {
            for (size_t j = 0; j < hit_trackIndex->size(); ++j){
            if (hit_trackIndex->at(j) == 0){
                filter2File << "Event Passed Check 2: " << i << std::endl;

                // Get the x, y, and z coordinates of the hit
                double x = hit_xLocal->at(j);
                double y = hit_yLocal->at(j);
                double z = hit_zLocal->at(j);

                // Get the layer index of said hit 
                int layer = hit_module->at(j);

                outputFile << "   Layer:" << layer
                    << " (" << std::setw(10) << x
                    << "," << std::setw(10) << y
                    << "," << z << ")";
            }
            }
        }

        outputFile << std::endl; 
    }
 
    outputFile.close();
    healthFile.close();
    inputFile->Close();

    cleanEmptyLines(outputPath, clean_outputPath);

    if (remove(outputPath.c_str()) != 0) {
        std::cerr << "Error deleting file: " << outputPath << std::endl;
    } else {
        std::cout << "File successfully deleted: " << outputPath << std::endl;
    }

    return; 
}
