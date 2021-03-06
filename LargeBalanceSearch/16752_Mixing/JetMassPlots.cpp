#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
using namespace std;

#include "TTree.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TEllipse.h"
#include "TProfile.h"
#include "TProfile2D.h"

#include "fastjet/ClusterSequence.hh"
#include "fastjet/ClusterSequenceArea.hh"
#include "fastjet/contrib/ConstituentSubtractor.hh"
#include "fastjet/contrib/SoftDrop.hh"
using namespace fastjet;

#include "Code/DrawRandom.h"
#include "Code/TauHelperFunctions3.h"
#include "ProgressBar.h"
#include "PlotHelper3.h"
#include "SetStyle.h"

#include "BasicUtilities.h"
#include "Messenger.h"
#include "SDJetHelper.h"
#include "CATree.h"
#include "DataHelper.h"

#define MAXSIZE 25000000

int main(int argc, char *argv[])
{
   SetThesisStyle();

   if(argc != 10 && argc != 11 && argc != 13)
   {
      cerr << "Usage: " << argv[0] << " Input Output Tag PTHatMin PTHatMax MinBiasForest GhostDistance Range ShiftRho=(N|Y) ReuseRate Mod ModBase" << endl;
      return -1;
   }

   string Input = argv[1];
   string Output = argv[2];
   string Tag = argv[3];
   double PTHatMin = atof(argv[4]);
   double PTHatMax = atof(argv[5]);

   string MBInput = argv[6];

   double GhostDistance = atof(argv[7]);
   double Range = atof(argv[8]);

   bool IsData = IsDataFromTag(Tag);
   bool IsPP = IsPPFromTag(Tag);
   bool IsPPHiReco = IsPPHiRecoFromTag(Tag);

   bool PassThroughMode = false;
   bool UseRhoM = false;   // switch to use rho_m or not
   bool ShiftRho = ((argv[9][0] == 'Y') ? true : false);

   if(IsData == true)
      cerr << "I'd be glad to run on data for this study" << endl;

   int ReuseRate = 1;
   if(argc >= 11)
      ReuseRate = atoi(argv[10]);

   int Mod = -1;   // -1 = don't use
   int ModBase = -1;
   if(argc >= 13)
   {
      Mod = atoi(argv[11]);
      ModBase = atoi(argv[12]);
   }

   TFile *InputFile = TFile::Open(Input.c_str());
   TFile *MBInputFile = TFile::Open(MBInput.c_str());

   string JetTreeName = "akCs4PFJetAnalyzer/t";
   string SoftDropJetTreeName = "akCsSoftDrop4PFJetAnalyzer/t";
   if(IsPP == true)
   {
      JetTreeName = "ak4PFJetAnalyzer/t";
      SoftDropJetTreeName = "akSoftDrop4PFJetAnalyzer/t";
   }
   if(IsPPHiReco == true)
   {
      JetTreeName = "akPu4PFJetAnalyzer/t";
      SoftDropJetTreeName = "akPu4CaloJetAnalyzer/t";
   }

   HiEventTreeMessenger MHiEvent(InputFile);
   JetTreeMessenger MJet(InputFile, JetTreeName);
   JetTreeMessenger MSDJet(InputFile, SoftDropJetTreeName);
   PFTreeMessenger MPF(InputFile, "pfcandAnalyzer/pfTree");
   SkimTreeMessenger MSkim(InputFile);
   TriggerTreeMessenger MHLT(InputFile);
   RhoTreeMessenger MRho(InputFile);
   GenParticleTreeMessenger MGen(InputFile);

   HiEventTreeMessenger MMBHiEvent(MBInputFile);
   PFTreeMessenger MMBPF(MBInputFile, "pfcandAnalyzer/pfTree");
   SkimTreeMessenger MMBSkim(MBInputFile);
   TriggerTreeMessenger MMBHLT(MBInputFile);
   RhoTreeMessenger MMBRho(MBInputFile);

   if(MHiEvent.Tree == NULL)
      return -1;
   if(PassThroughMode == false && MMBHiEvent.Tree == NULL)
      return -1;

   TFile OutputFile(Output.c_str(), "RECREATE");

   TTree OutputTree("OutputTree", "OutputTree");

   bool PassFilter, PassHLT, PassMBHLT;
   OutputTree.Branch("PassFilter", &PassFilter, "PassFilter/O");
   OutputTree.Branch("PassHLT", &PassHLT, "PassHLT/O");
   OutputTree.Branch("PassMBHLT", &PassMBHLT, "PassMBHLT/O");

   double TreeJetPT, TreeJetEta, TreeJetPhi, TreeJetSDMass, TreeJetDR, TreeJetMass;
   double TreeJetShape1, TreeJetShape2, TreeJetShape3, TreeJetShape4, TreeJetShape5;
   double TreeBestJetPT, TreeBestJetEta, TreeBestJetPhi, TreeBestJetSDMass, TreeBestJetDR, TreeBestJetZG, TreeBestJetMass, TreeBestJetSDPT, TreeBestJetWTADR;
   double TreeBestJetSDMass2, TreeBestJetDR2, TreeBestJetZG2, TreeBestJetSDPT2;
   double TreeBestJetSDMass3, TreeBestJetDR3, TreeBestJetZG3, TreeBestJetSDPT3;
   double TreeBestJetShape1, TreeBestJetShape2, TreeBestJetShape3, TreeBestJetShape4, TreeBestJetShape5;
   double TreeBestJetSJ1E, TreeBestJetSJ1PT, TreeBestJetSJ1Eta, TreeBestJetSJ1Phi, TreeBestJetSJ1WTAEta, TreeBestJetSJ1WTAPhi;
   double TreeBestJetSJ2E, TreeBestJetSJ2PT, TreeBestJetSJ2Eta, TreeBestJetSJ2Phi, TreeBestJetSJ2WTAEta, TreeBestJetSJ2WTAPhi;
   double TreeBestJetSJ1E2, TreeBestJetSJ1PT2, TreeBestJetSJ1Eta2, TreeBestJetSJ1Phi2;
   double TreeBestJetSJ2E2, TreeBestJetSJ2PT2, TreeBestJetSJ2Eta2, TreeBestJetSJ2Phi2;
   double TreeTotalPT, TreeRho, TreeTotalPTInJet, TreeTotalStuffInJet;
   double TreePTHat, TreeCentrality;
   double TreePFPT[200], TreePFEta[200], TreePFPhi[200];
   OutputTree.Branch("JetPT",           &TreeJetPT,           "JetPT/D");
   OutputTree.Branch("JetEta",          &TreeJetEta,          "JetEta/D");
   OutputTree.Branch("JetPhi",          &TreeJetPhi,          "JetPhi/D");
   OutputTree.Branch("JetSDMass",       &TreeJetSDMass,       "JetSDMass/D");
   OutputTree.Branch("JetDR",           &TreeJetDR,           "JetDR/D");
   OutputTree.Branch("JetMass",         &TreeJetMass,         "JetMass/D");
   OutputTree.Branch("JetShape1",       &TreeJetShape1,       "JetShape1/D");
   OutputTree.Branch("JetShape2",       &TreeJetShape2,       "JetShape2/D");
   OutputTree.Branch("JetShape3",       &TreeJetShape3,       "JetShape3/D");
   OutputTree.Branch("JetShape4",       &TreeJetShape4,       "JetShape4/D");
   OutputTree.Branch("JetShape5",       &TreeJetShape5,       "JetShape5/D");
   OutputTree.Branch("BestJetPT",       &TreeBestJetPT,       "BestJetPT/D");
   OutputTree.Branch("BestJetEta",      &TreeBestJetEta,      "BestJetEta/D");
   OutputTree.Branch("BestJetPhi",      &TreeBestJetPhi,      "BestJetPhi/D");
   OutputTree.Branch("BestJetSDMass",   &TreeBestJetSDMass,   "BestJetSDMass/D");
   OutputTree.Branch("BestJetDR",       &TreeBestJetDR,       "BestJetDR/D");
   OutputTree.Branch("BestJetWTADR",       &TreeBestJetWTADR,       "BestJetWTADR/D");
   OutputTree.Branch("BestJetZG",       &TreeBestJetZG,       "BestJetZG/D");
   OutputTree.Branch("BestJetMass",     &TreeBestJetMass,     "BestJetMass/D");
   OutputTree.Branch("BestJetSDPT",     &TreeBestJetSDPT,     "BestJetSDPT/D");
   OutputTree.Branch("BestJetSJ1E",     &TreeBestJetSJ1E,    "BestJetSJ1E/D");
   OutputTree.Branch("BestJetSJ1PT",    &TreeBestJetSJ1PT,    "BestJetSJ1PT/D");
   OutputTree.Branch("BestJetSJ1Eta",   &TreeBestJetSJ1Eta,   "BestJetSJ1Eta/D");
   OutputTree.Branch("BestJetSJ1Phi",   &TreeBestJetSJ1Phi,   "BestJetSJ1Phi/D");
   OutputTree.Branch("BestJetSJ1WTAEta",   &TreeBestJetSJ1WTAEta,   "BestJetSJ1WTAEta/D");
   OutputTree.Branch("BestJetSJ1WTAPhi",   &TreeBestJetSJ1WTAPhi,   "BestJetSJ1WTAPhi/D");
   OutputTree.Branch("BestJetSJ2E",     &TreeBestJetSJ2E,    "BestJetSJ2E/D");
   OutputTree.Branch("BestJetSJ2PT",    &TreeBestJetSJ2PT,    "BestJetSJ2PT/D");
   OutputTree.Branch("BestJetSJ2Eta",   &TreeBestJetSJ2Eta,   "BestJetSJ2Eta/D");
   OutputTree.Branch("BestJetSJ2Phi",   &TreeBestJetSJ2Phi,   "BestJetSJ2Phi/D");
   OutputTree.Branch("BestJetSJ2WTAEta",   &TreeBestJetSJ2WTAEta,   "BestJetSJ2WTAEta/D");
   OutputTree.Branch("BestJetSJ2WTAPhi",   &TreeBestJetSJ2WTAPhi,   "BestJetSJ2WTAPhi/D");
   OutputTree.Branch("BestJetSDMass2",  &TreeBestJetSDMass2,  "BestJetSDMass2/D");
   OutputTree.Branch("BestJetDR2",      &TreeBestJetDR2,      "BestJetDR2/D");
   OutputTree.Branch("BestJetZG2",      &TreeBestJetZG2,      "BestJetZG2/D");
   OutputTree.Branch("BestJetSDPT2",    &TreeBestJetSDPT2,    "BestJetSDPT2/D");
   OutputTree.Branch("BestJetSJ1E2",    &TreeBestJetSJ1E2,   "BestJetSJ1E2/D");
   OutputTree.Branch("BestJetSJ1PT2",   &TreeBestJetSJ1PT2,   "BestJetSJ1PT2/D");
   OutputTree.Branch("BestJetSJ1Eta2",  &TreeBestJetSJ1Eta2,  "BestJetSJ1Eta2/D");
   OutputTree.Branch("BestJetSJ1Phi2",  &TreeBestJetSJ1Phi2,  "BestJetSJ1Phi2/D");
   OutputTree.Branch("BestJetSJ2E2",    &TreeBestJetSJ2E2,   "BestJetSJ2E2/D");
   OutputTree.Branch("BestJetSJ2PT2",   &TreeBestJetSJ2PT2,   "BestJetSJ2PT2/D");
   OutputTree.Branch("BestJetSJ2Eta2",  &TreeBestJetSJ2Eta2,  "BestJetSJ2Eta2/D");
   OutputTree.Branch("BestJetSJ2Phi2",  &TreeBestJetSJ2Phi2,  "BestJetSJ2Phi2/D");
   OutputTree.Branch("BestJetSDMass3",  &TreeBestJetSDMass3,  "BestJetSDMass3/D");
   OutputTree.Branch("BestJetDR3",      &TreeBestJetDR3,      "BestJetDR3/D");
   OutputTree.Branch("BestJetZG3",      &TreeBestJetZG3,      "BestJetZG3/D");
   OutputTree.Branch("BestJetSDPT3",    &TreeBestJetSDPT3,    "BestJetSDPT3/D");
   OutputTree.Branch("BestJetShape1",   &TreeBestJetShape1,   "BestJetShape1/D");
   OutputTree.Branch("BestJetShape2",   &TreeBestJetShape2,   "BestJetShape2/D");
   OutputTree.Branch("BestJetShape3",   &TreeBestJetShape3,   "BestJetShape3/D");
   OutputTree.Branch("BestJetShape4",   &TreeBestJetShape4,   "BestJetShape4/D");
   OutputTree.Branch("BestJetShape5",   &TreeBestJetShape5,   "BestJetShape5/D");
   OutputTree.Branch("TotalPT",         &TreeTotalPT,         "TotalPT/D");
   OutputTree.Branch("Rho",             &TreeRho,             "Rho/D");
   OutputTree.Branch("TotalPTInJet",    &TreeTotalPTInJet,    "TotalPTInJet/D");
   OutputTree.Branch("TotalStuffInJet", &TreeTotalStuffInJet, "TotalStuffInJet/D");
   OutputTree.Branch("PTHat",           &TreePTHat,           "PTHat/D");
   OutputTree.Branch("Centrality",      &TreeCentrality,      "Centrality/D");
   OutputTree.Branch("PFPT",            &TreePFPT,            "PFPT[200]/D");
   OutputTree.Branch("PFEta",           &TreePFEta,           "PFEta[200]/D");
   OutputTree.Branch("PFPhi",           &TreePFPhi,           "PFPhi[200]/D");

   double TreeGenPT, TreeGenEta, TreeGenPhi, TreeGenDR;
   OutputTree.Branch("GenPT", &TreeGenPT, "GenPT/D");
   OutputTree.Branch("GenEta", &TreeGenEta, "GenEta/D");
   OutputTree.Branch("GenPhi", &TreeGenPhi, "GenPhi/D");
   OutputTree.Branch("GenDR", &TreeGenDR, "GenDR/D");

   double TreeGenSubJetDR, TreeGenSDMass;
   double TreeGenSubJet1PT, TreeGenSubJet1Eta, TreeGenSubJet1Phi;
   double TreeGenSubJet2PT, TreeGenSubJet2Eta, TreeGenSubJet2Phi;
   OutputTree.Branch("GenSubJetDR", &TreeGenSubJetDR, "GenSubJetDR/D");
   OutputTree.Branch("GenSDMass", &TreeGenSDMass, "GenSDMass/D");
   OutputTree.Branch("GenSubJet1PT", &TreeGenSubJet1PT, "GenSubJet1PT/D");
   OutputTree.Branch("GenSubJet1Eta", &TreeGenSubJet1Eta, "GenSubJet1Eta/D");
   OutputTree.Branch("GenSubJet1Phi", &TreeGenSubJet1Phi, "GenSubJet1Phi/D");
   OutputTree.Branch("GenSubJet2PT", &TreeGenSubJet2PT, "GenSubJet2PT/D");
   OutputTree.Branch("GenSubJet2Eta", &TreeGenSubJet2Eta, "GenSubJet2Eta/D");
   OutputTree.Branch("GenSubJet2Phi", &TreeGenSubJet2Phi, "GenSubJet2Phi/D");

   double TreeSubJetDRs[10], TreeSDMasses[10], TreeSubJetWTADRs[10];
   double TreeSubJet1PTs[10], TreeSubJet1Etas[10], TreeSubJet1Phis[10], TreeSubJet1WTAEtas[10], TreeSubJet1WTAPhis[10];
   double TreeSubJet2PTs[10], TreeSubJet2Etas[10], TreeSubJet2Phis[10], TreeSubJet2WTAEtas[10], TreeSubJet2WTAPhis[10];
   OutputTree.Branch("SubJetDRs", &TreeSubJetDRs, "SubJetDRs[10]/D");
   OutputTree.Branch("SDMasses", &TreeSDMasses, "SDMasses[10]/D");
   OutputTree.Branch("SubJetWTADRs", &TreeSubJetWTADRs, "SubJetWTADRs[10]/D");
   OutputTree.Branch("SubJet1PTs",  &TreeSubJet1PTs,  "SubJet1PTs[10]/D");
   OutputTree.Branch("SubJet1Etas", &TreeSubJet1Etas, "SubJet1Etas[10]/D");
   OutputTree.Branch("SubJet1Phis", &TreeSubJet1Phis, "SubJet1Phis[10]/D");
   OutputTree.Branch("SubJet1WTAEtas", &TreeSubJet1WTAEtas, "SubJet1WTAEtas[10]/D");
   OutputTree.Branch("SubJet1WTAPhis", &TreeSubJet1WTAPhis, "SubJet1WTAPhis[10]/D");
   OutputTree.Branch("SubJet2PTs",  &TreeSubJet2PTs,  "SubJet2PTs[10]/D");
   OutputTree.Branch("SubJet2Etas", &TreeSubJet2Etas, "SubJet2Etas[10]/D");
   OutputTree.Branch("SubJet2Phis", &TreeSubJet2Phis, "SubJet2Phis[10]/D");
   OutputTree.Branch("SubJet2WTAEtas", &TreeSubJet2WTAEtas, "SubJet2WTAEtas[10]/D");
   OutputTree.Branch("SubJet2WTAPhis", &TreeSubJet2WTAPhis, "SubJet2WTAPhis[10]/D");

   TH1D HN("HN", "Raw event count", 1, 0, 1);
   TH1D HPTHatAll("HPTHatAll", "PTHat", 100, 0, 500);
   TH1D HPTHatSelected("HPTHatSelected", "PTHat", 100, 0, 500);

   TH1D HRho("HRho", "Rho distribution", 100, 0, 400);
   TH1D HPTAdded("HPTAdded", "PT of added particles", 100, 0, 20);

   TH2D HJetPTComparison("HJetPTComparison", ";Jet PT;New Jet PT", 100, 0, 400, 100, 0, 400);
   TH2D HSDMassComparison("HSDMassComparison", ";Jet SD Mass;New Jet SD Mass", 100, 0, 100, 100, 0, 100);
   TH2D HRecoDRComparison("HRecoDRComparison", ";Jet Reco DR;New Jet Reco DR", 100, 0, 0.4, 100, 0, 0.4);

   TH2D HSDMassJetPT("HSDMassJetPT", ";Jet PT;SD Mass", 100, 80, 500, 100, 0, 100);
   TH2D HNewSDMassNewJetPT("HNewSDMassNewJetPT", ";New Jet PT;New SD Mass", 100, 0, 600, 100, 0, 100);

   TH1D HSDMass("HSDMass", "SD Mass, DR > 0.1;SD Mass", 100, 0, 100);
   TH1D HNewSDMass("HNewSDMass", "New SD Mass, New DR > 0.1; New SD Mass", 100, 0, 100);

   int EntryCount = MHiEvent.Tree->GetEntries() * 1.00;
   ProgressBar Bar(cout, EntryCount);
   Bar.SetStyle(1);

   PdfFileHelper PdfFile("SanityFiles.pdf");
   PdfFile.AddTextPage("Some plots for individual jets");
   int WriteJetCount = 0;

   int MBEntryCount = MMBHiEvent.Tree->GetEntries();

   for(int iEntry = 0, ReuseCount = 1; iEntry < EntryCount; ReuseCount++)
   {
      if(EntryCount <= 250 || iEntry % (EntryCount / 300) == 0)
      {
         Bar.Update(iEntry);
         Bar.Print();
      }

      if(ReuseRate <= 1 || ReuseCount % ReuseRate == 0)
         iEntry = iEntry + 1;
      if(Mod >= 0 && Mod < ModBase && iEntry % ModBase != Mod)
         continue;

      HN.Fill(0);

      MHiEvent.GetEntry(iEntry);
      MJet.GetEntry(iEntry);
      MSDJet.GetEntry(iEntry);
      MPF.GetEntry(iEntry);
      MHLT.GetEntry(iEntry);
      MSkim.GetEntry(iEntry);
      MGen.GetEntry(iEntry);

      if(PassThroughMode == false)
      {
         MMBHiEvent.GetEntry(iEntry % MBEntryCount);
         MMBPF.GetEntry(iEntry % MBEntryCount);
         MMBSkim.GetEntry(iEntry % MBEntryCount);
         MMBHLT.GetEntry(iEntry % MBEntryCount);
         MMBRho.GetEntry(iEntry % MBEntryCount);
      }
      else
         MRho.GetEntry(iEntry);

      PassFilter = true;
      PassHLT = true;
      PassMBHLT = true;
      if(IsData == true)
      {
         if(MSkim.PassBasicFilterLoose() == false)
            PassFilter = false;
         if(PassThroughMode == false && MMBSkim.PassBasicFilterLoose() == false)
            PassFilter = false;
         if(MHLT.CheckTrigger("HLT_AK4PFJet80_Eta5p1_v1") == false)
            PassHLT = false;
         if(PassThroughMode == false && MMBHLT.CheckTrigger("HLT_HIL1MinimumBiasHF1ANDPixel_SingleTrack_v1") == false)
            PassMBHLT = false;
      }

      // SDJetHelper HSDJet(MSDJet);

      HPTHatAll.Fill(MSDJet.PTHat);
      if(MSDJet.PTHat <= PTHatMin || MSDJet.PTHat > PTHatMax)
         continue;
      TreePTHat = MSDJet.PTHat;

      HPTHatSelected.Fill(MSDJet.PTHat);

      // Preparation - cluster the gen jets if they are available
      vector<PseudoJet> GenJets;
      vector<PseudoJet> GenParticles;
      if(MGen.Tree != NULL)   // gen particle tree exists!
      {
         for(int iGen = 0; iGen < MGen.ID->size(); iGen++)
         {
            if((*MGen.ID)[iGen] == 12 || (*MGen.ID)[iGen] == -12)
               continue;
            if((*MGen.ID)[iGen] == 14 || (*MGen.ID)[iGen] == -14)
               continue;
            if((*MGen.ID)[iGen] == 16 || (*MGen.ID)[iGen] == -16)
               continue;
            if((*MGen.DaughterCount)[iGen] != 0)
               continue;

            FourVector P;
            P.SetPtEtaPhi((*MGen.PT)[iGen], (*MGen.Eta)[iGen], (*MGen.Phi)[iGen]);
            GenParticles.push_back(PseudoJet(P[1], P[2], P[3], P[0]));
         }
      }
      JetDefinition GenDefinition(antikt_algorithm, 0.4);
      ClusterSequence GenSequence(GenParticles, GenDefinition);
      GenJets = GenSequence.inclusive_jets(30);

      // Step 0 - get inputs ready
      double Centrality, Rho, RhoM;
      if(PassThroughMode == false)
      {
         Centrality = GetCentrality(MMBHiEvent.hiBin);
         Rho = GetRho(MMBRho.EtaMax, MMBRho.Rho, 0, true);
         RhoM = GetRho(MMBRho.EtaMax, MMBRho.RhoM, 0, true);
      }
      else
      {
         Centrality = GetCentrality(MHiEvent.hiBin);
         Rho = GetRho(MRho.EtaMax, MRho.Rho, 0, true);
         RhoM = GetRho(MRho.EtaMax, MRho.RhoM, 0, true);
      }
      if(ShiftRho == true)
         Rho = Rho + 3;

      TreeRho = Rho;
      TreeCentrality = Centrality;

      // Step 1 - get all PF candidates within range
      vector<PseudoJet> Candidates;
      for(int i = 0; i < MPF.ID->size(); i++)
      {
         double Mass = 0;
         if(UseRhoM == true && (*MPF.ID)[i] == 1)
            Mass = 0.13957018;   // Pion :D

         FourVector P;
         P.SetPtEtaPhiMass((*MPF.PT)[i], (*MPF.Eta)[i], (*MPF.Phi)[i], Mass);

         Candidates.push_back(PseudoJet(P[1], P[2], P[3], P[0]));
      }

      // Step 2 - sprinkle underlying event contribution from MB
      double TotalPTInJet = 0;
      if(PassThroughMode == false)
      {
         for(int i = 0; i < MMBPF.ID->size(); i++)
         {
            double Mass = 0;
            if(UseRhoM == true && (*MMBPF.ID)[i] == 1)
               Mass = 0.13957018;   // Pion :D

            FourVector P;
            P.SetPtEtaPhiMass((*MMBPF.PT)[i], (*MMBPF.Eta)[i], (*MMBPF.Phi)[i], Mass);

            Candidates.push_back(PseudoJet(P[1], P[2], P[3], P[0]));
         }
      }

      // Step 3 - do pileup subtraction algorithm - via fastjet
      JetDefinition Definition(antikt_algorithm, 0.4);
      double GhostArea = GhostDistance * GhostDistance;
      AreaDefinition AreaDef(active_area_explicit_ghosts, GhostedAreaSpec(6.0, 1, GhostArea));
      ClusterSequenceArea Sequence(Candidates, Definition, AreaDef);
      vector<PseudoJet> JetsWithGhosts = Sequence.inclusive_jets();

      vector<PseudoJet> CSJets(JetsWithGhosts.size());
      for(int i = 0; i < (int)JetsWithGhosts.size(); i++)
      {
         if(UseRhoM == false)
            RhoM = 0;
         contrib::ConstituentSubtractor Subtractor(Rho, RhoM);
         Subtractor.set_alpha(1);  // optional
         // subtractor.set_max_deltaR(2);  // optional
         CSJets[i] = Subtractor(JetsWithGhosts[i]);
      }

      // Step 4 - Clustering and pick the jets
      vector<PseudoJet> CSCandidates;
      for(int i = 0; i < (int)CSJets.size(); i++)
      {
         vector<PseudoJet> Constituents = CSJets[i].constituents();
         CSCandidates.insert(CSCandidates.end(), Constituents.begin(), Constituents.end());
      }
      ClusterSequence NewSequence(CSCandidates, Definition);
      vector<PseudoJet> SDJets = NewSequence.inclusive_jets();

      if(SDJets.size() == 0)   // Huh?
         continue;

      for(int iJ = 0; iJ < (int)SDJets.size(); iJ++)
      {
         FourVector SDJetP;
         SDJetP[0] = SDJets[iJ].e();
         SDJetP[1] = SDJets[iJ].px();
         SDJetP[2] = SDJets[iJ].py();
         SDJetP[3] = SDJets[iJ].pz();

         if(SDJetP.GetAbsEta() > 1.5)
            continue;
         if(SDJetP.GetPT() < 80)
            continue;

         if(GenJets.size() > 0)
         {
            int BestGenJet = 0;
            double BestGenDR = -1;
            for(int i = 0; i < (int)GenJets.size(); i++)
            {
               double DR = GetDR(SDJets[iJ].eta(), SDJets[iJ].phi(), GenJets[i].eta(), GenJets[i].phi());
               if(BestGenDR < 0 || DR < BestGenDR)
               {
                  BestGenJet = i;
                  BestGenDR = DR;
               }
            }

            TreeGenPT = GenJets[BestGenJet].perp();
            TreeGenEta = GenJets[BestGenJet].eta();
            TreeGenPhi = GenJets[BestGenJet].phi();
            TreeGenDR = BestGenDR;

            vector<Node *> GenNodes;
            vector<PseudoJet> GenConstituents = GenJets[BestGenJet].constituents();
            for(int i = 0; i < (int)GenConstituents.size(); i++)
            {
               FourVector P;
               P[0] = GenConstituents[i].e();
               P[1] = GenConstituents[i].px();
               P[2] = GenConstituents[i].py();
               P[3] = GenConstituents[i].pz();
               GenNodes.push_back(new Node(P));
            }

            BuildCATree(GenNodes);

            Node *GroomedGen = FindSDNode(GenNodes[0], 0.1, 0.0, 0.4);

            if(GroomedGen->N <= 1)
               TreeGenSubJetDR = -1;
            else
            {
               FourVector P1 = GroomedGen->Child1->P;
               FourVector P2 = GroomedGen->Child2->P;

               TreeGenSubJet1PT = P1.GetPT();
               TreeGenSubJet1Eta = P1.GetEta();
               TreeGenSubJet1Phi = P1.GetPhi();
               TreeGenSubJet2PT = P2.GetPT();
               TreeGenSubJet2Eta = P2.GetEta();
               TreeGenSubJet2Phi = P2.GetPhi();

               TreeGenSubJetDR = GetDR(P1, P2);
               TreeGenSDMass = GroomedGen->P.GetMass();
            }

            delete GenNodes[0];

         }
         else
         {
            TreeGenPT = -1;
            TreeGenEta = 0;
            TreeGenPhi = 0;
            TreeGenDR = -1;
         }

         int CSIndex = -1;
         double CSDR2 = -1;
         for(int i = 0; i < MJet.JetCount; i++)
         {
            double DR2 = GetDR2(MJet.JetEta[i], MJet.JetPhi[i], SDJetP.GetEta(), SDJetP.GetPhi());
            if(CSDR2 < 0 || CSDR2 > DR2)
            {
               CSIndex = i;
               CSDR2 = DR2;
            }
         }

         FourVector JetP;
         JetP.SetPtEtaPhi(MJet.JetPT[CSIndex], MJet.JetEta[CSIndex], MJet.JetPhi[CSIndex]);

         TreeJetPT = MJet.JetPT[CSIndex];
         TreeJetEta = MJet.JetEta[CSIndex];
         TreeJetPhi = MJet.JetPhi[CSIndex];

         int SDIndex = -1;
         double SDDR2 = -1;
         for(int i = 0; i < MSDJet.JetCount; i++)
         {
            double DR2 = GetDR2(SDJetP.GetEta(), SDJetP.GetPhi(), MSDJet.JetEta[i], MSDJet.JetPhi[i]);
            if(SDDR2 < 0 || SDDR2 > DR2)
            {
               SDIndex = i;
               SDDR2 = DR2;
            }
         }

         // TreeJetDR = HSDJet.RecoSubJetDR[SDIndex];
         TreeJetSDMass = MSDJet.JetM[SDIndex];

         int ClosestCSJet = 0;
         double ClosestCSDR2 = -1;
         for(int i = 0; i < (int)CSJets.size(); i++)
         {
            double DR2 = GetDR2(CSJets[i].eta(), CSJets[i].phi(), SDJetP.GetEta(), SDJetP.GetPhi());
            if(ClosestCSDR2 < 0 || ClosestCSDR2 > DR2)
            {
               ClosestCSJet = i;
               ClosestCSDR2 = DR2;
            }
         }

         int MaxCSJet = 0;
         for(int i = 0; i < (int)CSJets.size(); i++)
            if(CSJets[i].perp() > CSJets[MaxCSJet].perp())
               MaxCSJet = i;

         PseudoJet Jet = CSJets[ClosestCSJet];
         FourVector BestJetP = SDJetP;

         vector<FourVector> GoodCandidatesBest;

         for(int i = 0; i < 200; i++)
         {
            TreePFPT[i] = 0;
            TreePFEta[i] = 0;
            TreePFPhi[i] = 0;
         }
         
         vector<PseudoJet> Constituents = SDJets[iJ].constituents();
         for(int i = 0; i < (int)Constituents.size(); i++)
         {
            FourVector P;
            P[0] = Constituents[i].e();
            P[1] = Constituents[i].px();
            P[2] = Constituents[i].py();
            P[3] = Constituents[i].pz();
            GoodCandidatesBest.push_back(P);

            if(i < 200)
            {
               TreePFPT[i] = P.GetPT();
               TreePFEta[i] = P.GetEta();
               TreePFPhi[i] = P.GetPhi();
            }
            else
               cerr << "Constituent size = " << Constituents.size() << "! increase output size!" << endl;
         }

         if(GoodCandidatesBest.size() == 0)
         {
            TreeBestJetPT = -1;
            OutputTree.Fill();
            continue;
         }

         // Step 5 - Run SD algorithm on the subtracted candidates
         vector<Node *> NodesBest;
         for(int i = 0; i < (int)GoodCandidatesBest.size(); i++)
            NodesBest.push_back(new Node(GoodCandidatesBest[i]));

         BuildCATree(NodesBest);
         Node *GroomedBest = FindSDNode(NodesBest[0]);
         Node *GroomedBest2 = FindSDNode(NodesBest[0], 0.5, 1.5);
         Node *GroomedBest3 = FindSDNode(NodesBest[0], 0.3, 1.5);

         Node *Groomed[10];
         Groomed[0] = FindSDNode(NodesBest[0], 0.20, 0.0, 0.4);
         Groomed[1] = FindSDNode(NodesBest[0], 0.25, 0.0, 0.4);
         Groomed[2] = FindSDNode(NodesBest[0], 0.30, 0.0, 0.4);
         Groomed[3] = FindSDNode(NodesBest[0], 0.35, 0.0, 0.4);
         Groomed[4] = FindSDNode(NodesBest[0], 0.40, 0.0, 0.4);
         Groomed[5] = FindSDNode(NodesBest[0], 0.45, 0.0, 0.4);
         Groomed[6] = FindSDNode(NodesBest[0], 0.20, -1.0, 0.4);
         Groomed[7] = FindSDNode(NodesBest[0], 0.30, -1.0, 0.4);
         Groomed[8] = FindSDNode(NodesBest[0], 0.40, -1.0, 0.4);
         Groomed[9] = FindSDNode(NodesBest[0], 0.20, -2.0, 0.4);

         TreeBestJetPT = BestJetP.GetPT();
         TreeBestJetEta = BestJetP.GetEta();
         TreeBestJetPhi = BestJetP.GetPhi();
         TreeBestJetSDMass = GroomedBest->P.GetMass();
         TreeBestJetSDMass2 = GroomedBest2->P.GetMass();
         TreeBestJetSDMass3 = GroomedBest3->P.GetMass();
         TreeBestJetSDPT = GroomedBest->P.GetPT();
         TreeBestJetSDPT2 = GroomedBest2->P.GetPT();
         TreeBestJetSDPT3 = GroomedBest3->P.GetPT();
         if(GroomedBest->N > 1)
         {
            FourVector P1 = GroomedBest->Child1->P;
            FourVector P2 = GroomedBest->Child2->P;
            TreeBestJetDR = GetDR(P1, P2);
            TreeBestJetZG = min(P1.GetPT(), P2.GetPT()) / (P1.GetPT() + P2.GetPT());

            TreeBestJetSJ1E = P1[0];
            TreeBestJetSJ1PT = P1.GetPT();
            TreeBestJetSJ1Eta = P1.GetEta();
            TreeBestJetSJ1Phi = P1.GetPhi();
            TreeBestJetSJ2E = P2[0];
            TreeBestJetSJ2PT = P2.GetPT();
            TreeBestJetSJ2Eta = P2.GetEta();
            TreeBestJetSJ2Phi = P2.GetPhi();
            
            std::pair<double, double> P1WTA = WinnerTakesAllAxis(GroomedBest->Child1);
            std::pair<double, double> P2WTA = WinnerTakesAllAxis(GroomedBest->Child2);

            TreeBestJetSJ1WTAEta = P1WTA.first;
            TreeBestJetSJ1WTAPhi = P1WTA.second;
            TreeBestJetSJ2WTAEta = P2WTA.first;
            TreeBestJetSJ2WTAPhi = P2WTA.second;
            TreeBestJetWTADR = GetDR(P1WTA.first, P1WTA.second, P2WTA.first, P2WTA.second);
         }
         else
            TreeBestJetDR = -1, TreeBestJetZG = -1, TreeBestJetWTADR = -1;
         if(GroomedBest2->N > 1)
         {
            FourVector P1 = GroomedBest2->Child1->P;
            FourVector P2 = GroomedBest2->Child2->P;
            TreeBestJetDR2 = GetDR(P1, P2);
            TreeBestJetZG2 = min(P1.GetPT(), P2.GetPT()) / (P1.GetPT() + P2.GetPT());

            TreeBestJetSJ1E2 = P1[0];
            TreeBestJetSJ1PT2 = P1.GetPT();
            TreeBestJetSJ1Eta2 = P1.GetEta();
            TreeBestJetSJ1Phi2 = P1.GetPhi();
            TreeBestJetSJ2E2 = P2[0];
            TreeBestJetSJ2PT2 = P2.GetPT();
            TreeBestJetSJ2Eta2 = P2.GetEta();
            TreeBestJetSJ2Phi2 = P2.GetPhi();
         }
         else
            TreeBestJetDR2 = -1, TreeBestJetZG2 = -1;
         if(GroomedBest3->N > 1)
         {
            FourVector P1 = GroomedBest3->Child1->P;
            FourVector P2 = GroomedBest3->Child2->P;
            TreeBestJetDR3 = GetDR(P1, P2);
            TreeBestJetZG3 = min(P1.GetPT(), P2.GetPT()) / (P1.GetPT() + P2.GetPT());
         }
         else
            TreeBestJetDR3 = -1, TreeBestJetZG3 = -1;

         for(int i = 0; i < 10; i++)
         {
            if(Groomed[i]->N > 1)
            {
               TreeSubJetDRs[i] = GetDR(Groomed[i]->Child1->P, Groomed[i]->Child2->P);
               TreeSDMasses[i] = Groomed[i]->P.GetMass();

               TreeSubJet1PTs[i] = Groomed[i]->Child1->P.GetPT();
               TreeSubJet1Etas[i] = Groomed[i]->Child1->P.GetEta();
               TreeSubJet1Phis[i] = Groomed[i]->Child1->P.GetPhi();
               TreeSubJet2PTs[i] = Groomed[i]->Child2->P.GetPT();
               TreeSubJet2Etas[i] = Groomed[i]->Child2->P.GetEta();
               TreeSubJet2Phis[i] = Groomed[i]->Child2->P.GetPhi();

               std::pair<double, double> P1WTA = WinnerTakesAllAxis(Groomed[i]->Child1);
               std::pair<double, double> P2WTA = WinnerTakesAllAxis(Groomed[i]->Child2);

               TreeSubJet1WTAEtas[i] = P1WTA.first;
               TreeSubJet1WTAPhis[i] = P1WTA.second;
               TreeSubJet2WTAEtas[i] = P2WTA.first;
               TreeSubJet2WTAPhis[i] = P2WTA.second;
               TreeSubJetWTADRs[i] = GetDR(P1WTA.first, P1WTA.second, P2WTA.first, P2WTA.second);
            }
            else
               TreeSubJetDRs[i] = -1, TreeSubJetWTADRs[i] = -1;
         }

         // Cleanup
         if(NodesBest.size() > 0)
            delete NodesBest[0];

         if(BestJetP.GetPT() > 80)   // be careful not to blow up storage
            OutputTree.Fill();
      }
   }

   Bar.Update(EntryCount);
   Bar.Print();
   Bar.PrintLine();

   PdfFile.AddTimeStampPage();
   PdfFile.Close();

   OutputTree.Write();

   HN.Write();
   HPTHatAll.Write();
   HPTHatSelected.Write();

   HRho.Write();
   HPTAdded.Write();

   HJetPTComparison.Write();
   HSDMassComparison.Write();
   HRecoDRComparison.Write();

   HSDMassJetPT.Write();
   HNewSDMassNewJetPT.Write();

   HSDMass.Write();
   HNewSDMass.Write();

   OutputFile.Close();

   MBInputFile->Close();
   InputFile->Close();
}




