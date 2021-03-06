#include <iostream>
#include <iomanip>
using namespace std;

#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TProfile.h"
#include "TProfile2D.h"

#include "Code/DrawRandom.h"
#include "Code/TauHelperFunctions2.h"

#include "BasicUtilities.h"
#include "Messenger.h"
#include "SDJetHelper.h"
#include "CATree.h"

int main(int argc, char *argv[]);
double CalculateDR(double eta1, double phi1, double eta2, double phi2);
void PrintSDCATree(Node *N, FourVector J1, FourVector J2);

int main(int argc, char *argv[])
{
   if(argc < 6)
   {
      cerr << "Usage: " << argv[0] << " Input Output Tag PTHatMin PTHatMax" << endl;
      return -1;
   }

   string Input = argv[1];
   string Output = argv[2];
   string Tag = argv[3];
   double PTHatMin = atof(argv[4]);
   double PTHatMax = atof(argv[5]);

   bool IsData = IsDataFromTag(Tag);
   bool IsPP = IsPPFromTag(Tag);

   if(IsData == true)
   {
      cerr << "I'd rather not run on data for this study" << endl;
      return -1;
   }

   TFile InputFile(Input.c_str());

   string JetTreeName = "akCs4PFJetAnalyzer/t";
   string SoftDropJetTreeName = "akCsSoftDrop4PFJetAnalyzer/t";
   string PFTreeName = "pfcandAnalyzerCS/pfTree";
   if(IsPP == true)
   {
      JetTreeName = "ak4PFJetAnalyzer/t";
      SoftDropJetTreeName = "akSoftDrop4PFJetAnalyzer/t";
      PFTreeName = "pfcandAnalyzer/pfTree";
   }

   HiEventTreeMessenger MHiEvent(InputFile);
   JetTreeMessenger MJet(InputFile, JetTreeName);
   JetTreeMessenger MSDJet(InputFile, SoftDropJetTreeName);
   GenParticleTreeMessenger MGen(InputFile);
   PFTreeMessenger MPF(InputFile);

   if(MHiEvent.Tree == NULL)
      return -1;

   TFile OutputFile(Output.c_str(), "RECREATE");

   string CBin[5] = {"CBin0", "CBin1", "CBin2", "CBin3", "CBin4"};
   double CBinMin[5+1] = {0.0, 0.1, 0.3, 0.5, 0.8, 1.0};
   string PTBin[7] = {"PTBin0", "PTBin1", "PTBin2", "PTBin3", "PTBin4", "PTBin5", "PTBin6"};
   double PTBinMin[7+1] = {80, 100, 120, 140, 160, 200, 300, 500};

   TH1D HN("HN", "Raw event count", 1, 0, 1);
   TH1D HPTHatAll("HPTHatAll", "PTHat", 100, 0, 500);
   TH1D HPTHatSelected("HPTHatSelected", "PTHat", 100, 0, 500);

   // TH2D *HSubJet1Location[5][7] = {NULL};
   // TH2D *HSubJet2Location[5][7] = {NULL};
   
   for(int i = 0; i < 5; i++)
   {
      for(int j = 0; j < 7; j++)
      {
         char Title[1024] = "";
         sprintf(Title, "C %.0f%%-%.0f%%, PT %.0f-%.0f", CBinMin[i] * 100, CBinMin[i+1] * 100,
            PTBinMin[j], PTBinMin[j+1]);

         // HSubJet1Location[i][j] = new TH2D(Form("HSubJet1Location%d%d", i, j),
         //    Form("%s;x;y", Title), 100, -5, 5, 100, -5, 5);
         // HSubJet2Location[i][j] = new TH2D(Form("HSubJet2Location%d%d", i, j),
         //    Form("%s;x;y", Title), 100, -5, 5, 100, -5, 5);
      }
   }

   int EntryCount = MHiEvent.Tree->GetEntries();
   for(int iEntry = 0; iEntry < EntryCount; iEntry++)
   {
      HN.Fill(0);

      MHiEvent.GetEntry(iEntry);
      MJet.GetEntry(iEntry);
      MSDJet.GetEntry(iEntry);
      MGen.GetEntry(iEntry);
      MPF.GetEntry(iEntry);
      
      SDJetHelper HSDJet(MSDJet);

      HPTHatAll.Fill(MSDJet.PTHat);
      if(MSDJet.PTHat <= PTHatMin || MSDJet.PTHat > PTHatMax)
         continue;

      HPTHatSelected.Fill(MSDJet.PTHat);
      
      double Centrality = GetCentrality(MHiEvent.hiBin);

      for(int i = 0; i < MSDJet.JetCount; i++)
      {
         if(MSDJet.JetPT[i] < 80 || MSDJet.JetPT[i] > 500)
            continue;
         if(MSDJet.JetEta[i] < -1.3 || MSDJet.JetEta[i] > 1.3)
            continue;

         if(HSDJet.GoodRecoSubJet[i] == false)
            continue;
         if(HSDJet.GoodGenSubJet[i] == false)
            continue;

         if(HSDJet.Region[i] != 2)   // we want region II for this study
            continue;
         if(HSDJet.Inside[i] == true)   // we want outside jets
            continue;

         int CIndex = GetIndex(Centrality, CBinMin, CBinMin + 6);
         int PIndex = GetIndex(MSDJet.JetPT[i], PTBinMin, PTBinMin + 8);

         vector<Node *> GenNodes;
         vector<Node *> PFNodes;

         for(int j = 0; j < (int)(*MGen.PT).size(); j++)
         {
            int ID = (*MGen.ID)[j];
            if(ID < 0)
               ID = -ID;
            if(ID == 12 || ID == 14 || ID == 16)
               continue;
            
            double DR = GetDR(MSDJet.JetEta[i], MSDJet.JetPhi[i], (*MGen.Eta)[j], (*MGen.Phi)[j]);
            if(DR < 0.4)
            {
               FourVector P;
               P.SetPtEtaPhi((*MGen.PT)[j], (*MGen.Eta)[j], (*MGen.Phi)[j]);
               GenNodes.push_back(new Node(P));
            }
         }
         for(int j = 0; j < (int)(*MPF.PT).size(); j++)
         {
            double DR = GetDR(MSDJet.JetEta[i], MSDJet.JetPhi[i], (*MPF.Eta)[j], (*MPF.Phi)[j]);
            if(DR < 0.4)
            {
               FourVector P;
               P.SetPtEtaPhi((*MPF.PT)[j], (*MPF.Eta)[j], (*MPF.Phi)[j]);
               P[0] = (*MPF.E)[j];
               PFNodes.push_back(new Node(P));
            }
         }

         BuildCATree(GenNodes);
         BuildCATree(PFNodes);

         cout << "=== Event " << iEntry << ", Jet " << i << " === " << endl;
         cout << endl;

         cout << "* GEN " << endl;

         PrintSDCATree(GenNodes[0], HSDJet.GenSubJet1[i], HSDJet.GenSubJet2[i]);

         cout << "* RECO " << endl;

         PrintSDCATree(PFNodes[0], HSDJet.GenSubJet1[i], HSDJet.GenSubJet2[i]);

         cout << "* GEN Stored " << endl;

         double GenZG = min(HSDJet.GenSubJet1[i].GetPT(), HSDJet.GenSubJet2[i].GetPT())
            / (HSDJet.GenSubJet1[i].GetPT() + HSDJet.GenSubJet2[i].GetPT());
         cout << "(" << HSDJet.GenSubJet1[i].GetPT() << " 1.0000 0.0000)"
            << " + " << "(" << HSDJet.GenSubJet2[i].GetPT() << " -1.0000 0.0000)"
            << " " << GenZG << endl;

         cout << "* RECO Stored " << endl;

         cout << "(" << HSDJet.RecoSubJet1[i].GetPT() << " " << HSDJet.SubJetX1[i] << " " << HSDJet.SubJetY1[i] << ")"
            << " + " << "(" << HSDJet.RecoSubJet2[i].GetPT() << " " << HSDJet.SubJetX2[i] << " " << HSDJet.SubJetY2[i] << ")"
            << " " << MSDJet.JetSym[i] << endl;

         cout << endl;

         delete GenNodes[0];
         delete PFNodes[0];
      }
   }

   HN.Write();
   HPTHatAll.Write();
   HPTHatSelected.Write();

   for(int i = 0; i < 5; i++)
   {
      for(int j = 0; j < 7; j++)
      {
         // if(HSubJet1Location[i][j] != NULL)   HSubJet1Location[i][j]->Write();
         // if(HSubJet2Location[i][j] != NULL)   HSubJet2Location[i][j]->Write();

         // if(HSubJet1Location[i][j] != NULL)   delete HSubJet1Location[i][j];
         // if(HSubJet2Location[i][j] != NULL)   delete HSubJet2Location[i][j];
      }
   }

   OutputFile.Close();

   InputFile.Close();
}

double CalculateDR(double eta1, double phi1, double eta2, double phi2)
{
   double DEta = eta1 - eta2;
   double DPhi = phi1 - phi2;
   if(DPhi < -PI)   DPhi = DPhi + 2 * PI;
   if(DPhi > +PI)   DPhi = DPhi - 2 * PI;
   return sqrt(DEta * DEta + DPhi * DPhi);
}

void PrintSDCATree(Node *N, FourVector J1, FourVector J2)
{
   if(N == NULL)
      return;

   double DeltaEta = J1.GetEta() - J2.GetEta();
   double MeanEta = (J1.GetEta() + J2.GetEta()) / 2;
   double DeltaPhi = (J1.GetPhi() - J2.GetPhi());
   if(DeltaPhi < -PI)   DeltaPhi = DeltaPhi + 2 * PI;
   if(DeltaPhi > +PI)   DeltaPhi = DeltaPhi - 2 * PI;
   double MeanPhi = J2.GetPhi() + DeltaPhi / 2;
   if(MeanPhi < -PI)   MeanPhi = MeanPhi + 2 * PI;
   if(MeanPhi > +PI)   MeanPhi = MeanPhi - 2 * PI;

   double M11 = DeltaEta / 2;
   double M12 = DeltaPhi / 2;
   double M21 = -DeltaPhi / 2;
   double M22 = DeltaEta / 2;

   double DetM = M11 * M22 - M12 * M21;

   double IM11 = M22 / DetM;
   double IM12 = -M21 / DetM;
   double IM21 = -M12 / DetM;
   double IM22 = M11 / DetM;

   while(N->Child1 != NULL && N->Child2 != NULL)
   {
      double SubJetEta1 = N->Child1->P.GetEta() - MeanEta;
      double SubJetPhi1 = N->Child1->P.GetPhi() - MeanPhi;
      double SubJetEta2 = N->Child2->P.GetEta() - MeanEta;
      double SubJetPhi2 = N->Child2->P.GetPhi() - MeanPhi;
      if(SubJetPhi1 > +PI)   SubJetPhi1 = SubJetPhi1 - 2 * PI;
      if(SubJetPhi1 < -PI)   SubJetPhi1 = SubJetPhi1 + 2 * PI;
      if(SubJetPhi2 > +PI)   SubJetPhi2 = SubJetPhi2 - 2 * PI;
      if(SubJetPhi2 < -PI)   SubJetPhi2 = SubJetPhi2 + 2 * PI;

      double X1 = IM11 * SubJetEta1 + IM12 * SubJetPhi1;
      double Y1 = IM21 * SubJetEta1 + IM22 * SubJetPhi1;
      double X2 = IM11 * SubJetEta2 + IM12 * SubJetPhi2;
      double Y2 = IM21 * SubJetEta2 + IM22 * SubJetPhi2;

      double ZG = N->Child2->P.GetPT() / (N->Child1->P.GetPT() + N->Child2->P.GetPT());
      
      // cout << "(" << N->P.GetPT() << " " << N->P.GetEta() << " " << N->P.GetPhi() << ")";
      // cout << " = (" << N->Child1->P.GetPT() << " " << N->Child1->P.GetEta() << " " << N->Child1->P.GetPhi() << ")";
      // cout << " + (" << N->Child2->P.GetPT() << " " << N->Child2->P.GetEta() << " " << N->Child2->P.GetPhi() << ")";
      // cout << "; ZG = " << ZG << endl;
      
      cout << setprecision(5);
      cout << "(" << N->Child1->P.GetPT() << " " << X1 << " " << Y1 << ")"
         << " + " << "(" << N->Child2->P.GetPT() << " " << X2 << " " << Y2 << ")"
         << " " << ZG;
      if(ZG > 0.1)
         cout << " *";
      cout << endl;
      
      N = N->Child1;
   }
}




