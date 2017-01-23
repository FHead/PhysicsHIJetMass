#include <algorithm>
#include <cmath>
#include <iostream>

#include "include/CATree.h"

Node::Node()
   : P(0, 0, 0, 0), Child1(NULL), Child2(NULL), Parent(NULL), N(1)
{
}
   
Node::Node(FourVector &p)
   : P(p), Child1(NULL), Child2(NULL), Parent(NULL), N(1)
{
}

Node::Node(Node *n1, Node *n2)
   : P(0, 0, 0, 0), Child1(NULL), Child2(NULL), Parent(NULL), N(1)
{
   if(n1 == NULL || n2 == NULL)
      return;

   P = n1->P + n2->P;
   Child1 = n1;
   Child2 = n2;
   Child1->Parent = this;
   Child2->Parent = this;
   N = n1->N + n2->N + 1;

   if(Child1->P.GetPT() < Child2->P.GetPT())
      std::swap(Child1, Child2);
}

Node::~Node()
{
   if(Child1 != NULL)   delete Child1;
   if(Child2 != NULL)   delete Child2;

   Child1 = NULL;
   Child2 = NULL;
}

void BuildCATree(std::vector<Node *> &Nodes)
{
   while(Nodes.size() > 1)
   {
      int IndexL = 0, IndexR = 1;
      double BestDistance = -1;

      for(int i = 0; i < (int)Nodes.size() - 1; i++)
      {
         for(int j = i + 1; j < (int)Nodes.size(); j++)
         {
            double Distance = GetDR(Nodes[i]->P, Nodes[j]->P);

            if(Distance < BestDistance || BestDistance < 0)
            {
               IndexL = i;
               IndexR = j;
               BestDistance = Distance;
            }
         }
      }

      if(IndexR < IndexL)   // just in case
         std::swap(IndexL, IndexR);

      // Now make a new node
      Node *NewNode = new Node(Nodes[IndexL], Nodes[IndexR]);
      Nodes[IndexL] = NewNode;
      Nodes.erase(Nodes.begin() + IndexR);
   }
}

void BuildCATree2(std::vector<Node *> &Nodes)
{
   std::vector<std::pair<double, int>> NodeEta(Nodes.size());
   for(int i = 0; i < (int)Nodes.size(); i++)
      NodeEta[i] = std::pair<double, int>(Nodes[i]->P.GetEta(), i);
   std::sort(NodeEta.begin(), NodeEta.end());

   while(Nodes.size() > 1)
   {
      NodePair Closest = FindClosestPair(Nodes, NodeEta);

      int ID1 = Closest.ID1;
      int ID2 = Closest.ID2;
      if(ID1 > ID2)
         std::swap(ID1, ID2);

      // Now make a new node
      Node *NewNode = new Node(Nodes[ID1], Nodes[ID2]);
      Nodes[ID1] = NewNode;
      Nodes.erase(Nodes.begin() + ID2);

      NodeEta.pop_back();
      for(int i = 0; i < (int)Nodes.size(); i++)
         NodeEta[i] = std::pair<double, int>(Nodes[i]->P.GetEta(), i);
      std::sort(NodeEta.begin(), NodeEta.end());
   }
}

NodePair FindClosestPair(std::vector<Node *> &Nodes, std::vector<std::pair<double, int>> &NodeEta)
{
   if(NodeEta.size() < 2)
   {
      NodePair Nonsense;
      Nonsense.ID1 = -1;
      Nonsense.ID2 = -1;
      Nonsense.N1 = NULL;
      Nonsense.N2 = NULL;
      Nonsense.DR2 = -1;
      Nonsense.DEta = 0;
      Nonsense.DPhi = 0;
      return Nonsense;
   }

   if(NodeEta.size() <= 6)
   {
      // brute-force

      int IndexL = 0, IndexR = 1;
      double BestDR2 = -1;

      for(int i = 0; i < (int)NodeEta.size(); i++)
      {
         for(int j = i + 1; j < (int)NodeEta.size(); j++)
         {
            double DR = GetDR(Nodes[NodeEta[i].second]->P, Nodes[NodeEta[j].second]->P);
            double DR2 = DR * DR;

            if(DR2 < BestDR2 || BestDR2 < 0)
            {
               IndexL = NodeEta[i].second;
               IndexR = NodeEta[j].second;
               BestDR2 = DR2;
            }
         }
      }

      NodePair Result;
      Result.ID1 = IndexL;
      Result.ID2 = IndexR;
      Result.N1 = Nodes[IndexL];
      Result.N2 = Nodes[IndexR];
      Result.DR2 = BestDR2;
      // Result.DEta = GetDEta(Nodes[N1]->P, Nodes[N2]->P);
      // Result.DPhi = GetDPhi(Nodes[N1]->P, Nodes[N2]->P);
      return Result;
   }

   // recursion
   int Size = NodeEta.size();
   std::vector<std::pair<double, int>> NodeEtaLeft(NodeEta.begin(), NodeEta.begin() + Size / 2);
   std::vector<std::pair<double, int>> NodeEtaRight(NodeEta.begin() + Size / 2, NodeEta.end());
   
   NodePair LeftPair = FindClosestPair(Nodes, NodeEtaLeft);
   NodePair RightPair = FindClosestPair(Nodes, NodeEtaRight);

   double BestDR = std::sqrt(std::min(LeftPair.DR2, RightPair.DR2));

   int Begin = -1, End = -1;
   for(int i = 0; i < Size / 2; i++)
   {
      if(NodeEta[Size/2-1].first - NodeEta[i].first <= BestDR)
      {
         Begin = i;
         break;
      }
   }
   for(int i = Size / 2; i < Size; i++)
   {
      if(NodeEta[i].first - NodeEta[Size/2].first > BestDR)
      {
         End = i;
         break;
      }
   }
   if(Begin == -1)
      Begin = 0;
   if(End == -1)
      End = Size - 1;

   int IndexL = Size / 2, IndexR = Size / 2 + 1;
   double BestDistance2 = -1;

   for(int i = Begin; i < Size / 2; i++)
   {
      for(int j = Size / 2; j <= End; j++)
      {
         if(NodeEta[j].first - NodeEta[i].first > BestDR)
            continue;

         double DR = GetDR(Nodes[NodeEta[i].second]->P, Nodes[NodeEta[j].second]->P);
         double DR2 = DR * DR;

         if(DR2 < BestDistance2 || BestDistance2 < 0)
         {
            IndexL = NodeEta[i].second;
            IndexR = NodeEta[j].second;
            BestDistance2 = DR2;
         }
      }
   }

   if(BestDistance2 > BestDR * BestDR)
   {
      if(LeftPair.DR2 < RightPair.DR2)
         return LeftPair;
      else
         return RightPair;
   }

   NodePair Result;
   Result.ID1 = IndexL;
   Result.ID2 = IndexR;
   Result.N1 = Nodes[IndexL];
   Result.N2 = Nodes[IndexR];
   Result.DR2 = BestDistance2;
   // Result.DEta = GetDEta(Nodes[IndexL]->P, Nodes[IndexR]->P);
   // Result.DPhi = GetDPhi(Nodes[IndexL]->P, Nodes[IndexR]->P);
   return Result;
}

Node *FindSDNode(Node *HeadNode, double ZCut, double Beta, double R0)
{
   if(HeadNode == NULL)
      return NULL;

   bool Done = false;
   Node *Current = HeadNode;

   while(Done == false)
   {
      if(Current->N == 1)
         Done = true;
      else if(Current->N == 2)
      {
         // WTF!
         std::cerr << "Error!  N = " << Current->N << "!" << std::endl;
      }
      else if(Current->Child1 == NULL || Current->Child2 == NULL)
      {
         // WTF!
         std::cerr << "Error!  Child NULL while N = " << Current->N << "!" << std::endl;
      }
      else
      {
         double PT1 = Current->Child1->P.GetPT();
         double PT2 = Current->Child2->P.GetPT();
         double PTRatio = std::min(PT1, PT2) / (PT1 + PT2);

         double DR = GetDR(Current->Child1->P, Current->Child2->P);

         double Threshold = ZCut * std::pow(DR / R0, Beta);

         if(PTRatio > Threshold)
            Done = true;
         else
         {
            if(PT1 > PT2)
               Current = Current->Child1;
            else
               Current = Current->Child2;
         }
      }
   }

   return Current;
}

