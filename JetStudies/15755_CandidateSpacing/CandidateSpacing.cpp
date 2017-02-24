#include <iostream>
using namespace std;

#include "TFile.h"
#include "TTree.h"
#include "TH1D.h"

#include "ProgressBar.h"

#include "BasicUtilities.h"
#include "Messenger.h"

int main()
{
   TFile File("AA6Dijet220.root");

   TFile OutputFile("OutputFile.root", "RECREATE");

   HiEventTreeMessenger MHiEvent(File);
   PFTreeMessenger MPF(File, "pfcandAnalyzer/pfTree");

   if(MHiEvent.Tree == NULL)
      return -1;

   TH1D HSpacing1("HSpacing1", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing2("HSpacing2", ";DR;", 10000, 0, 0.5);
   TH1D HSpacing11("HSpacing11", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing14("HSpacing14", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing15("HSpacing15", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing44("HSpacing44", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing45("HSpacing45", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing55("HSpacing55", ";DR;", 10000, 0, 0.01);
   TH1D HSpacing3("HSpacing3", ";DR;", 10000, 0, 0.5);
   TH1D HWideSpacing11("HWideSpacing11", ";DR;", 100, 0, 0.4);
   TH1D HWideSpacing14("HWideSpacing14", ";DR;", 100, 0, 0.4);
   TH1D HWideSpacing15("HWideSpacing15", ";DR;", 100, 0, 0.4);
   TH1D HWideSpacing44("HWideSpacing44", ";DR;", 100, 0, 0.4);
   TH1D HWideSpacing45("HWideSpacing45", ";DR;", 100, 0, 0.4);
   TH1D HWideSpacing55("HWideSpacing55", ";DR;", 100, 0, 0.4);

   int EntryCount = MHiEvent.Tree->GetEntries() * 1.0;
   ProgressBar Bar(cout, EntryCount);
   Bar.SetStyle(-1);

   for(int iE = 0; iE < EntryCount; iE++)
   {
      Bar.Update(iE);
      Bar.Print();

      MHiEvent.GetEntry(iE);
      MPF.GetEntry(iE);

      int Size = MPF.ID->size();
      for(int iPF1 = 0; iPF1 < Size; iPF1++)
      {
         if((*MPF.Eta)[iPF1] < -1.5 || (*MPF.Eta)[iPF1] > 1.5)
            continue;
         if((*MPF.PT)[iPF1] < 1)
            continue;

         for(int iPF2 = iPF1 + 1; iPF2 < Size; iPF2++)
         {
            if((*MPF.Eta)[iPF2] < -1.5 || (*MPF.Eta)[iPF2] > 1.5)
               continue;
            if((*MPF.PT)[iPF2] < 1)
               continue;

            double DR = GetDR((*MPF.Eta)[iPF1], (*MPF.Phi)[iPF1], (*MPF.Eta)[iPF2], (*MPF.Phi)[iPF2]);

            HSpacing1.Fill(DR);
            HSpacing2.Fill(DR);
            
            int IDMin = min((*MPF.ID)[iPF1], (*MPF.ID)[iPF2]);
            int IDMax = max((*MPF.ID)[iPF1], (*MPF.ID)[iPF2]);

            if(IDMin == 1 && IDMax == 1)
            {
               HSpacing11.Fill(DR);
               HWideSpacing11.Fill(DR);
            }
            if(IDMin == 1 && IDMax == 4)
            {
               HSpacing14.Fill(DR);
               HWideSpacing14.Fill(DR);
            }
            if(IDMin == 1 && IDMax == 5)
            {
               HSpacing15.Fill(DR);
               HWideSpacing15.Fill(DR);
            }
            if(IDMin == 4 && IDMax == 4)
            {
               HSpacing44.Fill(DR);
               HWideSpacing44.Fill(DR);
            }
            if(IDMin == 4 && IDMax == 5)
            {
               HSpacing45.Fill(DR);
               HWideSpacing45.Fill(DR);
            }
            if(IDMin == 5 && IDMax == 5)
            {
               HSpacing55.Fill(DR);
               HWideSpacing55.Fill(DR);
               HSpacing3.Fill(DR);
            }
         }
      }
   }

   Bar.Update(EntryCount);
   Bar.Print();
   Bar.PrintLine();

   HSpacing1.Write();
   HSpacing2.Write();
   HSpacing11.Write();
   HSpacing14.Write();
   HSpacing15.Write();
   HSpacing44.Write();
   HSpacing45.Write();
   HSpacing55.Write();
   HSpacing3.Write();
   HWideSpacing11.Write();
   HWideSpacing14.Write();
   HWideSpacing15.Write();
   HWideSpacing44.Write();
   HWideSpacing45.Write();
   HWideSpacing55.Write();

   OutputFile.Close();

   File.Close();

   return 0;
}



