#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
using namespace std;

#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TProfile.h"
#include "TProfile2D.h"

#include "Code/DrawRandom.h"
#include "Code/TauHelperFunctions2.h"
#include "ProgressBar.h"

#include "BasicUtilities.h"
#include "Messenger.h"
#include "SDJetHelper.h"
#include "CATree.h"

int main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
   if(argc < 7)
   {
      cerr << "Usage: " << argv[0] << " Input Output Tag PTHatMin PTHatMax Smear" << endl;
      return -1;
   }

   string Input = argv[1];
   string Output = argv[2];
   string Tag = argv[3];
   double PTHatMin = atof(argv[4]);
   double PTHatMax = atof(argv[5]);

   double Smear = atof(argv[6]);

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

   TH2D HJetPTComparison("HJetPTComparison", ";Jet PT;New Jet PT", 100, 0, 400, 100, 0, 400);
   TH2D HSDMassComparison("HSDMassComparison", ";Jet SD Mass;New Jet SD Mass", 100, 0, 100, 100, 0, 100);
   TH2D HRecoDRComparison("HRecoDRComparison", ";Jet Reco DR;New Jet Reco DR", 100, 0, 0.4, 100, 0, 0.4);

   TH2D HJetPTVsPrediction("HJetPTVsPrediction", ";New Jet PT;Predicted Jet PT", 100, 0, 400, 100, 0, 400);

   TH2D HSDMassJetPT("HSDMassJetPT", ";Jet PT;SD Mass", 100, 80, 500, 100, 0, 100);
   TH2D HNewSDMassNewJetPT("HNewSDMassNewJetPT", ";New Jet PT;New SD Mass", 100, 0, 600, 100, 0, 100);

   TH1D HSDMass("HSDMass", "SD Mass, DR > 0.1;SD Mass", 100, 0, 100);
   TH1D HNewSDMass("HNewSDMass", "New SD Mass, New DR > 0.1; New SD Mass", 100, 0, 100);

   double Rho = 300;

   int EntryCount = MHiEvent.Tree->GetEntries();
   ProgressBar Bar(cout, EntryCount);
   Bar.SetStyle(1);

   for(int iEntry = 0; iEntry < EntryCount; iEntry++)
   {
      if(EntryCount <= 250 || iEntry % (EntryCount / 300) == 0)
      {
         Bar.Update(iEntry);
         Bar.Print();
      }

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

      for(int iJ = 0; iJ < MSDJet.JetCount; iJ++)
      {
         // if(MSDJet.JetPT[iJ] < 80 || MSDJet.JetPT[iJ] > 500)
         //    continue;
         if(MSDJet.JetEta[iJ] < -1.3 || MSDJet.JetEta[iJ] > 1.3)
            continue;

         FourVector JetP;
         JetP.SetPtEtaPhi(MSDJet.JetPT[iJ], MSDJet.JetEta[iJ], MSDJet.JetPhi[iJ]);

         // Step 1 - get all PF candidates within range
         vector<FourVector> Candidates;
         for(int i = 0; i < MPF.ID->size(); i++)
         {
            FourVector P;
            P.SetPtEtaPhi((*MPF.PT)[i], (*MPF.Eta)[i], (*MPF.Phi)[i]);
            if(GetDR(P, JetP) < 0.4)
               Candidates.push_back(P);
         }

         // Step 2 - sprinkle underlying event contribution
         double TotalPT = Rho * 0.4 * 0.4 * PI * DrawGaussian(1, Smear);
         while(TotalPT > 0)
         {
            double PT = DrawRandom(0.1, 3);
            if(TotalPT > PT)
               TotalPT = TotalPT - PT;
            else
            {
               PT = TotalPT;
               TotalPT = 0;
            }

            double DEta, DPhi;
            do
            {
               DEta = DrawRandom(-0.4, 0.4);
               DPhi = DrawRandom(-0.4, 0.4);
            } while(DEta * DEta + DPhi * DPhi > 0.4 * 0.4);

            FourVector P;
            P.SetPtEtaPhi(PT, MSDJet.JetEta[iJ] + DEta, MSDJet.JetPhi[iJ] + DPhi);
            Candidates.push_back(P);
         }
         
         FourVector TotalJetPBeforeSubtraction;
         for(int i = 0; i < (int)Candidates.size(); i++)
            TotalJetPBeforeSubtraction = TotalJetPBeforeSubtraction + Candidates[i];

         double PredictedJetPT = TotalJetPBeforeSubtraction.GetPT() - Rho * 0.4 * 0.4 * PI;
         if(PredictedJetPT < 80 && MSDJet.JetPT[iJ] < 80)   // Small => small, no need to run
            continue;

         // Step 3 - do pileup subtraction algorithm
         vector<FourVector> Ghosts;
         double Delta = 0.04;
         for(double Eta = -0.4; Eta <= 0.4; Eta = Eta + Delta)
         {
            for(double Phi = -0.4; Phi <= 0.4; Phi = Phi + Delta)
            {
               if(Eta * Eta + Phi * Phi > 0.4 * 0.4)
                  continue;

               FourVector P;
               P.SetPtEtaPhi(1, MSDJet.JetEta[iJ] + Eta, MSDJet.JetPhi[iJ] + Phi);
               Ghosts.push_back(P);
            }
         }
         double GhostPT = Rho * 0.4 * 0.4 * PI / Ghosts.size();
         for(int i = 0; i < (int)Ghosts.size(); i++)
            Ghosts[i].SetPtEtaPhi(GhostPT, Ghosts[i].GetEta(), Ghosts[i].GetPhi());
         
         vector<bool> CandidatesAlive(Candidates.size());
         vector<bool> GhostsAlive(Ghosts.size());

         for(int i = 0; i < (int)Candidates.size(); i++)
            CandidatesAlive[i] = true;
         for(int i = 0; i < (int)Ghosts.size(); i++)
            GhostsAlive[i] = true;

         multimap<double, pair<int, int> > Distances;
         for(int i = 0; i < (int)Candidates.size(); i++)
            for(int j = 0; j < (int)Ghosts.size(); j++)
               Distances.insert(pair<double, pair<int, int> >(GetDR(Candidates[i], Ghosts[j]), pair<int, int>(i, j)));

         for(multimap<double, pair<int, int> >::iterator iter = Distances.begin(); iter != Distances.end(); iter++)
         {
            if(CandidatesAlive[iter->second.first] == false)
               continue;
            if(GhostsAlive[iter->second.second] == false)
               continue;

            FourVector CP = Candidates[iter->second.first];
            FourVector GP = Ghosts[iter->second.second];

            if(CP.GetPT() > GP.GetPT())
            {
               Candidates[iter->second.first].SetPtEtaPhi(CP.GetPT() - GP.GetPT(), CP.GetEta(), CP.GetPhi());
               GhostsAlive[iter->second.second] = false;
            }
            else
            {
               CandidatesAlive[iter->second.first] = false;
               Ghosts[iter->second.second].SetPtEtaPhi(GP.GetPT() - CP.GetPT(), GP.GetEta(), GP.GetPhi());
            }
         }

         vector<FourVector> GoodCandidates;
         for(int i = 0; i < (int)Candidates.size(); i++)
            if(CandidatesAlive[i] == true)
               GoodCandidates.push_back(Candidates[i]);

         if(GoodCandidates.size() == 0)
         {
            // cout << "Nothing left!" << endl;
            continue;
         }

         FourVector NewJetP;
         for(int i = 0; i < (int)GoodCandidates.size(); i++)
            NewJetP = NewJetP + GoodCandidates[i];

         HJetPTComparison.Fill(MSDJet.JetPT[iJ], NewJetP.GetPT());

         HJetPTVsPrediction.Fill(NewJetP.GetPT(), PredictedJetPT);
         
         // Step 4 - Run SD algorithm on the subtracted candidates
         vector<Node *> Nodes;
         for(int i = 0; i < (int)GoodCandidates.size(); i++)
            Nodes.push_back(new Node(GoodCandidates[i]));

         BuildCATree(Nodes);
         Node *Groomed = FindSDNode(Nodes[0]);

         if(Groomed->N > 1)
         {
            HSDMassComparison.Fill(MSDJet.JetM[iJ], Groomed->P.GetMass());
            HRecoDRComparison.Fill(HSDJet.RecoSubJetDR[iJ], GetDR(Groomed->Child1->P, Groomed->Child2->P));

            HSDMassJetPT.Fill(MSDJet.JetPT[iJ], MSDJet.JetM[iJ]);
            HNewSDMassNewJetPT.Fill(NewJetP.GetPT(), Groomed->P.GetMass());
         }
            
         if(HSDJet.RecoSubJetDR[iJ] > 0.1 && MSDJet.JetPT[iJ] > 80)
            HSDMass.Fill(MSDJet.JetM[iJ]);
         if(Groomed->N > 1 && GetDR(Groomed->Child1->P, Groomed->Child2->P) > 0.1 && Groomed->P.GetPT() > 80)
            HNewSDMass.Fill(Groomed->P.GetMass());

         // Cleanup
         if(Nodes.size() > 0)
            delete Nodes[0];
      }
   }

   Bar.Update(EntryCount);
   Bar.Print();
   Bar.PrintLine();

   HN.Write();
   HPTHatAll.Write();
   HPTHatSelected.Write();

   HJetPTComparison.Write();
   HSDMassComparison.Write();
   HRecoDRComparison.Write();

   HJetPTVsPrediction.Write();

   HSDMassJetPT.Write();
   HNewSDMassNewJetPT.Write();

   HSDMass.Write();
   HNewSDMass.Write();

   // for(int i = 0; i < 5; i++)
   // {
   //    for(int j = 0; j < 7; j++)
   //    {
   //       if(HGenMatchingQuality[i][j] != NULL)   HGenMatchingQuality[i][j]->Write();
   //       if(HGenMatchingQuality[i][j] != NULL)   delete HGenMatchingQuality[i][j];
   //    }
   // }

   OutputFile.Close();

   InputFile.Close();
}

