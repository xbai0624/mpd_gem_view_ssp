

//function to order clust_x_i and clust_y_i indices by largest to smallest in ADC value
void cluster_matching(vector<float> &clust_x, vector<float> &clust_y, vector<int> &clust_x_i, vector<int> &clust_y_i){


  for(int i = 0; i < clust_x.size(); i++){
    double largest = clust_x[i];
    int largest_i = i;

    for(int j = i+1; j < clust_x.size(); j++){
      if(clust_x[j] > largest){
        largest = clust_x[j];
        largest_i = j;
      }
    }

    clust_x[largest_i] = clust_x[i];
    clust_x[i] = largest;

    int temp_i = clust_x_i[largest_i];
    clust_x_i[largest_i] = clust_x_i[i];
    clust_x_i[i] = temp_i;
  }

  for(int i = 0; i < clust_y.size(); i++){
    double largest = clust_y[i];
    int largest_i = i;

    for(int j = i+1; j < clust_y.size(); j++){
      if(clust_y[j] > largest){
        largest = clust_y[j];
        largest_i = j;
      }
    }

    clust_y[largest_i] = clust_y[i];
    clust_y[i] = largest;

    int temp_i = clust_y_i[largest_i];
    clust_y_i[largest_i] = clust_y_i[i];
    clust_y_i[i] = temp_i;
  }


}

//void fill_plots(int min_i_





void module_plots(){


  TString run = "337";
  TString HitDir = "../Rootfiles/";

  TString output = "./cluster_" + run + ".pdf";

  TChain *t = new TChain("GEMCluster");

  t->Add(HitDir + "cluster_bbgem_" + run + "*.root");

  int evtID;
  int nCluster;
  int planeID[500];   //Events would never have more
  int prodID[500];    //more than 50 clusters
  int moduleID[500];
  int axis[500];
  int size[500];
  float adc[500];
  float pos[500];

  TBranch *b_evtID;
  TBranch *b_nCluster;
  TBranch *b_planeID;
  TBranch *b_prodID;
  TBranch *b_moduleID;
  TBranch *b_axis;
  TBranch *b_size;
  TBranch *b_adc;
  TBranch *b_pos;



  t->SetBranchAddress("evtID",&evtID,&b_evtID);
  t->SetBranchAddress("nCluster",&nCluster,&b_nCluster);
  t->SetBranchAddress("size",&size,&b_size);
  t->SetBranchAddress("axis",axis,&b_axis);
  t->SetBranchAddress("moduleID",moduleID,&b_moduleID);
  t->SetBranchAddress("adc",adc,&b_adc);
  t->SetBranchAddress("pos",pos,&b_pos);

  //int nmodules = t->GetMaximum("moduleID") + 1;
  int nmodules = 4;

  TCanvas *c = new TCanvas("c","",1600,1200);
  c->Divide(nmodules,3);

  TCanvas *c2 = new TCanvas("c2","",1600,1200);
  c2->Divide(nmodules,3);

  TCanvas *c3 = new TCanvas("c3","",1600,1200);
  c3->Divide(nmodules,3);


  TH2F *cluster2D[nmodules];
  TH1F *hit_x[nmodules];
  TH1F *hit_y[nmodules];


  TH2F *ADC2D[nmodules];
  TH1F *ADC_x[nmodules];
  TH1F *ADC_y[nmodules];

  TH1F *clust_size_x[nmodules];
  TH1F *clust_size_y[nmodules];

  double length_y = 700;

  for(int imod = 0; imod < nmodules; imod++){
    hit_x[imod] = new TH1F(Form("hit_x_%i",imod),Form("X Cluster Pos Module %i;x (mm);",imod),200,-350,350);
    hit_y[imod] = new TH1F(Form("hit_y_%i",imod),Form("Y Cluster Pos Module %i;y (mm);",imod),200,-720 -length_y/2 + 480*imod,-720 + length_y/2 + 480*imod);
    cluster2D[imod] = new TH2F(Form("cluster2D_%i",imod),Form("Cluster Distribution Module %i;x (mm);y (mm)",imod),200,-350,350,200,-720 -length_y/2 + 480*imod,-720 + length_y/2 + 480*imod);

    ADC_x[imod] = new TH1F(Form("ADC_x_%i",imod),Form("X Cluster Charge Module %i;ADC;",imod),200,0,2000);
    ADC_y[imod] = new TH1F(Form("ADC_y_%i",imod),Form("Y Cluster Charge Module %i;ADC;",imod),200,0,2000);
    ADC2D[imod] = new TH2F(Form("ADC2D_%i",imod),Form("Cluster Charge Sharing Module %i;ADC x;ADC y",imod),200,0,2000,200,0,2000);

    clust_size_x[imod] = new TH1F(Form("clust_size_x_%i",imod),Form("Cluster Size X Module %i;Size;",imod),10,0,10);
    clust_size_y[imod] = new TH1F(Form("clust_size_y_%i",imod),Form("Cluster Size Y Module %i;Size;",imod),10,0,10);

  }

  vector<float> clust_x;
  vector<float> clust_y;
  vector<int> clust_x_i, clust_y_i;


  int ntot = t->GetEntries();
  int ievent = 0;

  while(t->GetEntry(ievent++)){

    if(ievent%100 == 0) cout<<std::setprecision(2)<<ievent*1.0/ntot*100<<"% \r"<<std::flush;
    //cout<<ievent<<" "<<nCluster<<endl;

    clust_x.clear();
    clust_y.clear();
    clust_x_i.clear();
    clust_y_i.clear();

    int currentmod = -1;

    if(nCluster > 50) continue;

    for(int iclust = 0; iclust < nCluster; iclust++){

      //Entered new module, do cluster matching
      if(moduleID[iclust] < currentmod) {

        //Skip if there is no 2D cluster possible
        if(clust_x.size() == 0 || clust_y.size() == 0) continue;

        cluster_matching(clust_x,clust_y,clust_x_i,clust_y_i);

        //If odd number of x and y clusters, then leave out the last cluster
        int min_i_dim = clust_x.size();
        if(clust_y_i.size() < min_i_dim) min_i_dim = clust_y_i.size();


        //Now we fill our histograms
        for(int i=0; i < min_i_dim; i++){


          hit_x[moduleID[clust_x_i[i]]]->Fill(pos[clust_x_i[i]]);
          hit_y[moduleID[clust_y_i[i]]]->Fill(pos[clust_y_i[i]]);
          cluster2D[moduleID[clust_x_i[i]]]->Fill(pos[clust_x_i[i]],pos[clust_y_i[i]]);

          ADC_x[moduleID[clust_x_i[i]]]->Fill(adc[clust_x_i[i]]);
          ADC_y[moduleID[clust_y_i[i]]]->Fill(adc[clust_y_i[i]]);
          ADC2D[moduleID[clust_x_i[i]]]->Fill(adc[clust_x_i[i]],adc[clust_y_i[i]]);

          clust_size_x[moduleID[clust_x_i[i]]]->Fill(size[clust_x_i[i]]);
          clust_size_y[moduleID[clust_y_i[i]]]->Fill(size[clust_y_i[i]]);


        }

        clust_x.clear();
        clust_y.clear();
        clust_x_i.clear();
        clust_y_i.clear();


      }


      currentmod = moduleID[iclust];

      //Read in the cluster adc values and id's
      //Data always counts from highest number mod to lowest

      if(axis[iclust] == 0){
        clust_x.push_back(adc[iclust]);
        clust_x_i.push_back(iclust);
      }
      if(axis[iclust] == 1){
        clust_y.push_back(adc[iclust]);
        clust_y_i.push_back(iclust);
      }




      //Last module read in, do cluster matching
      if(iclust == nCluster - 1){

        //Skip there is no 2D cluster possible
        if(clust_x.size() == 0 || clust_y.size() == 0) continue;

        cluster_matching(clust_x,clust_y,clust_x_i,clust_y_i);

        //If odd number of x and y clusters, then leave out the last cluster
        int min_i_dim = clust_x.size();
        if(clust_y_i.size() < min_i_dim) min_i_dim = clust_y_i.size();


        //Now we fill our histograms
        for(int i=0; i < min_i_dim; i++){

          hit_x[moduleID[clust_x_i[i]]]->Fill(pos[clust_x_i[i]]);
          hit_y[moduleID[clust_y_i[i]]]->Fill(pos[clust_y_i[i]]);
          cluster2D[moduleID[clust_x_i[i]]]->Fill(pos[clust_x_i[i]],pos[clust_y_i[i]]);

          ADC_x[moduleID[clust_x_i[i]]]->Fill(adc[clust_x_i[i]]);
          ADC_y[moduleID[clust_y_i[i]]]->Fill(adc[clust_y_i[i]]);
          ADC2D[moduleID[clust_x_i[i]]]->Fill(adc[clust_x_i[i]],adc[clust_y_i[i]]);

          clust_size_x[moduleID[clust_x_i[i]]]->Fill(size[clust_x_i[i]]);
          clust_size_y[moduleID[clust_y_i[i]]]->Fill(size[clust_y_i[i]]);
        }

        clust_x.clear();
        clust_y.clear();
        clust_x_i.clear();
        clust_y_i.clear();

      }

    }

  }

  for(int imod = 0; imod < nmodules; imod++){

    c->cd(imod + 1);

    cluster2D[imod]->Draw("colz");

    c->cd(imod + 5);

    hit_x[imod]->Draw();

    c->cd(imod + 9);

    hit_y[imod]->Draw();

    c2->cd(imod + 1);

    ADC2D[imod]->Draw("colz");

    c2->cd(imod + 5);

    ADC_x[imod]->Draw();

    c2->cd(imod + 9);

    ADC_y[imod]->Draw();


    c3->cd(imod + 1);

    clust_size_x[imod]->Draw();

    c3->cd(imod + 5);

    clust_size_y[imod]->Draw();


  }

  c->Print(output + "(");
  c2->Print(output);
  c3->Print(output + ")");

}

