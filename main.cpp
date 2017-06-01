#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstring>
#include <cmath>

#define BLOCKSIZE  4096
#define BLOCKCOUNT 12000
#define BLOCK_MAX  127
#pragma pack(1)

using namespace std;

int str2int (const string &str) {
  stringstream ss(str);
  int num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}
unsigned str2unsign (const string &str) {
  stringstream ss(str);
  unsigned num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}
float str2float (const string &str) {
  stringstream ss(str);
  float num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}

fstream Dat_File;

typedef struct {
   unsigned blockNumber;
   unsigned studentId;
} SaveInfo;

vector<SaveInfo> infoVector;

void insertBlockNumber(unsigned blockNumber, unsigned studentId){

   SaveInfo s;
   s.blockNumber = blockNumber;
   s.studentId = studentId;
   infoVector.push_back(s);
}

bool checkBlockNumber(unsigned blockNumber, unsigned studentId){
   bool isThere = false;
   for(int i = 0; i < infoVector.size(); i++){
      if(infoVector[i].studentId == studentId) {
         if(infoVector[i].blockNumber == blockNumber){
            isThere = true;
         }
      }
   }

   return isThere;
}

class Student{
public :
   char Name[20];//20
   unsigned ID;//4
   float Score;//4
   unsigned advisorID;
   Student();
};

class Block{
public :
   Student Record[127];
   Block();
   int  Record_Count;
   int  Bit_Number;
   char Block_Garbage[22];
};

Student::Student()
{
   ID     = 0;
   Score  = 0;
   memset(Name, '\0', sizeof(Name));
    advisorID = 0;
}

Block::Block()
{
   Record_Count      = 0;
   Bit_Number         = 0;
}
Block Dat_block;

class Hash_Table{
public:
   Hash_Table();
   long Table_Block_Offset[BLOCKSIZE/sizeof(long)];
};

class Dynamic_Hash{

private:
   unsigned HASH(string str);
   void Expand_Table(unsigned Primary_Index, fstream& Dat_File);
   int Table_Bit_Number;
   Hash_Table Hash_table;
   fstream Hash_File;

public:
   Dynamic_Hash(char* filename);
   ~Dynamic_Hash();
   unsigned Get_Hash_Offset(string stu_id);
   void Block_Full(string stu_id, int Block_Bit_Number, fstream& Dat_File);
   void Make_txt();
   void Print_Hash();
};

Hash_Table::Hash_Table()
{
   for(int i=0; i<(BLOCKSIZE/sizeof(long)); i++)
      Table_Block_Offset[i] = -1;
}

Dynamic_Hash::Dynamic_Hash(char* filename)
{
   Hash_File.open(filename, ios::binary | ios::in | ios::out);
   if(!Hash_File)
   {
      Hash_File.clear();
      Hash_File.open(filename, ios::binary | ios::in | ios::out | ios::trunc);
      Table_Bit_Number = 0;
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_table.Table_Block_Offset[0] = 0;
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
   }
   else
   {
      Hash_File.read((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.read((char*)&Hash_table, sizeof(Hash_table));
   }
}

Dynamic_Hash::~Dynamic_Hash()
{
   Hash_File.seekp(0, ios::beg);
   Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
   Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
   Hash_File.close();
}

unsigned Dynamic_Hash::HASH(string str)
{
   unsigned key = 0;
   unsigned i;

   for(i = 0; i < str.length(); i++)
   {
      key = (key * 2014) + str[i];
   }

   key = key%1024;

   unsigned start=0;
   unsigned end=32;
   unsigned hash=key;
   char cf=0;

   while(1){

      cf = hash>>31;
      hash = hash<<1;

      if(cf==1){
         start = start>>1;
         start += unsigned(1<<31);
         end--;
         if(end == 0)
            break;
      }
      else{
         start = unsigned(start>>1);
         end--;
         if(end == 0)
            break;
      }
   }
   key = start;
   return key;
}

unsigned Dynamic_Hash::Get_Hash_Offset(string stu_id)
{
   unsigned Primary_Index = HASH(stu_id);

   if(Table_Bit_Number == 0)
   {
      Primary_Index = 0;
   }
   else
   {
      Primary_Index = Primary_Index >> (32-Table_Bit_Number);
   }
   return Hash_table.Table_Block_Offset[Primary_Index];
}


void Dynamic_Hash::Block_Full(string stu_id, int Block_Bit_Number, fstream& Dat_File)
{
   unsigned Primary_Index = HASH(stu_id);
   if(Table_Bit_Number != 0)
      Primary_Index = Primary_Index >> (32-Table_Bit_Number);
   else
      Primary_Index = 0;

   if(Block_Bit_Number == Table_Bit_Number)
   {

      Expand_Table(Primary_Index, Dat_File);
      return;
   }
   else if(Block_Bit_Number < Table_Bit_Number)
   {
      Block* Buffer = new Block;
      Dat_File.seekp(0, ios::end);
      std::streamoff a = Dat_File.tellg();
      long New_Block_Offset = a;
      Block* New_Block = new Block;
      Block* Old_Block = new Block;

      Dat_File.seekg(Hash_table.Table_Block_Offset[(int)Primary_Index], ios::beg);
      Dat_File.read((char*)Buffer, sizeof(Block));
      Old_Block->Bit_Number = Buffer->Bit_Number + 1;
      New_Block->Bit_Number = Buffer->Bit_Number + 1;

      int start = -1, end = -1;
      int i;
      for(i=0; i<1024; i++)
      {
         if(start == -1 && Hash_table.Table_Block_Offset[i] == Hash_table.Table_Block_Offset[(int)Primary_Index])
            start = i;
         if(start != -1 && Hash_table.Table_Block_Offset[i] != Hash_table.Table_Block_Offset[(int)Primary_Index])
         {
            end = i-1;
            break;
         }
      }

      for(i=start; i<=(start+(end-start)/2); i++)
         Hash_table.Table_Block_Offset[i] = Hash_table.Table_Block_Offset[Primary_Index];

      for(i=(start+(end-start)/2)+1; i<=end; i++)
         Hash_table.Table_Block_Offset[i] = New_Block_Offset;

      unsigned Hash_primary_index;
      char temp2[30]="";
      string id="";

      for(int j=0; j<127; j++)
      {
         unsigned t=Buffer->Record[j].ID;
         sprintf(temp2, "%d",t);
         id = temp2;
         Hash_primary_index = HASH(id);
         Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
         if((int)Hash_primary_index < (start+(end-start)/2)+1)
            Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
         else
         {
            New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];
         }
      }

      Dat_File.seekp(Hash_table.Table_Block_Offset[start], ios::beg);
      Dat_File.write((char*)Old_Block, sizeof(Block));
      Dat_File.seekp(New_Block_Offset, ios::beg);
      Dat_File.write((char*)New_Block, sizeof(Block));

      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));

      delete Buffer;
      delete New_Block;
      delete Old_Block;
   }

}

void Dynamic_Hash::Expand_Table(unsigned Primary_Index, fstream& Dat_File)
{
   Block* Buffer = new Block;
    int i = 0;
   Dat_File.seekp(0, ios::end);
   std::streamoff b = Dat_File.tellg();
   long New_Block_Offset = b;
   Block* New_Block = new Block;
   Block* Old_Block = new Block;
   unsigned Table_Block_Number = 2;

   if(Table_Bit_Number != 0)
      Table_Block_Number = Table_Block_Number << (Table_Bit_Number-1);
   else
      Table_Block_Number = 1;


   for(int i=Table_Block_Number-1; i>=0; i--)
   {
      if(i == (int)Primary_Index)
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = New_Block_Offset;


         Dat_File.seekg(Hash_table.Table_Block_Offset[i], ios::beg);
         Dat_File.read((char*)Buffer, sizeof(Block));

         Table_Bit_Number++;
         Old_Block->Bit_Number = Table_Bit_Number;
         New_Block->Bit_Number = Table_Bit_Number;

         unsigned Hash_primary_index;
         char temp3[30]="";
         string id="";
         unsigned j;
         for(j=0; j<127; j++)
         {

            unsigned t= Buffer->Record[j].ID;
            sprintf(temp3, "%d",t);
            id = temp3;
            Hash_primary_index = HASH(id);
            Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
            if(Hash_primary_index == 2*i)
               Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
            else
            {
               New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];

            }
         }
         Dat_File.seekp(Hash_table.Table_Block_Offset[2*i], ios::beg);
         Dat_File.write((char*)Old_Block, sizeof(Block));
         Dat_File.seekp(New_Block_Offset, ios::beg);
         Dat_File.write((char*)New_Block, sizeof(Block));
      }
      else
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = Hash_table.Table_Block_Offset[i];
      }


   }
      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
      delete Buffer;
      delete New_Block;
      delete Old_Block;
}
// new func
void Dynamic_Hash::Make_txt(){
    ofstream outfile("DB.txt");
    int total = 0;
    int DB_size = 0;

    Dat_File.seekg(0,ios::end);
    DB_size = Dat_File.tellg();
    Dat_File.seekg(0,ios::beg);

     for(int i = 0; i< DB_size/BLOCKSIZE ; i++){
        Dat_File.read((char*)&Dat_block,sizeof(Dat_block));
        for(int j = 0; j< Dat_block.Record_Count; j++){
            total++;
            outfile << Dat_block.Record[j].Name << " " << Dat_block.Record[j].ID << " " << Dat_block.Record[j].Score << " "  << Dat_block.Record[j].advisorID <<" " << Dat_block.Record_Count << " total : " << total <<endl;
        }
        outfile << endl ;
    }
    cout << endl;
    cout << "====================================================="<< endl << endl;
    cout << " ■ Total Entered Record Num : "<< total << endl;
    cout << " ■ Total BLOCK Num IN Students.DB : "<< DB_size/4096 << endl;
    cout << endl;
    outfile.close();

}
void Dynamic_Hash::Print_Hash(){
    int i  = 0;
    cout<< "================<HASH_TABLE>=========================" << endl;
    cout <<" ■ Table_Bit : "<< Table_Bit_Number<< endl;
    cout<< "=====================================================" << endl;
    while(1){
        if(Hash_table.Table_Block_Offset[i]==-1){
            break;
        }
        else{
            cout<< " ■ Primary Index " << i << " of Block Offset : "<< Hash_table.Table_Block_Offset[i] << endl;
        }
        i++;
    }
    cout<< "================<HASH_TABLE>=========================" << endl;
}
Dynamic_Hash* Hash;



class Score_Element{
public :
   long  Offset;
   float Key_Score;
   Score_Element();
};

Score_Element::Score_Element()
{
   Offset = -1;
   Key_Score  = -1;
}


class Node{
public :
   Score_Element Node_Element[BLOCKSIZE/sizeof(Score_Element)-2];
   long    Last_Offset;
   long    Next_Node_Offset;
   int     Score_Element_Count;
   int     Node_part;                                               // 1: Root Node 2: Internal Node 3: External Node. 4 : Root Node & External Node.
   Node();
};

Node::Node()
{
   Last_Offset = -1;
   Next_Node_Offset = -1;
   Score_Element_Count = 0;
   Node_part = 0;
}

class B_Plus_Tree{
private :
   int      Node_Count;
   long     Root_Node_Offset;

   long     Current_Node_Offset;
   bool     checkok;
   int      Element_In_Node;
   long     Left_element;
   long     Right_element;

   Node* Current_Node;


   void Open_Tree(char* Filename);
   void GetNode(long s_Offset);
   int  FindInNode(Node* score_current_node, float find_score, bool& score_is_find);
   void Insert_Element(float score, long ID_offset, Node* score_current_node, int Insert_Index);
   void NoSplitInsert(float score,  long ID_offset, Node* score_current_node, int Insert_Index);
   void SplitInsert(float score, long ID_offset, Node* score_current_node, int Insert_Index);
   int  Get_Parent_Node(float score, long score_current_node_Offset);

public :
    fstream  Tree_File;
   B_Plus_Tree(char* Filename);
   ~B_Plus_Tree();
   long Find(float find_score, int& S_Index, bool& score_is_find);
   bool Insert(float score, long ID_offset);
   void Score_search(float lower, float uppper, int* Result);
   void get_leaf_Node(int num);
};

B_Plus_Tree::B_Plus_Tree(char* Filename)
{
   Current_Node = new Node;
   Current_Node_Offset = -1;
   checkok = false;
   Element_In_Node = BLOCKSIZE/sizeof(Score_Element)-2;
   Left_element = -1;
   Right_element   = -1;
   Open_Tree(Filename);

}

B_Plus_Tree::~B_Plus_Tree()
{
   if(checkok && Current_Node != NULL)
   {
      Tree_File.seekp(Current_Node_Offset, ios::beg);
      Tree_File.write((char*)Current_Node, sizeof(Node));
   }

   if(Current_Node != NULL)
      delete Current_Node;

   Tree_File.seekp(0, ios::beg);
   Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
   Tree_File.close();
}



void B_Plus_Tree::Open_Tree(char* Filename)
{

   Tree_File.open(Filename, ios::in | ios::out | ios::binary);
   if(!Tree_File)
   {
      Tree_File.clear();
      Tree_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);


      Node* Root = new Node;
      Root->Node_part = 4;
      Tree_File.seekp(sizeof(long), ios::beg);
      std::streamoff c = Tree_File.tellg();
      Root_Node_Offset = c;
      Tree_File.write((char*)Root, sizeof(Node));
      Tree_File.seekp(0, ios::beg);
      Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
      delete Root;
   }

   else
   {
      Tree_File.seekg(0, ios::beg);
      Tree_File.read((char*)&Root_Node_Offset, sizeof(long));
   }

   Tree_File.seekg(0, ios::end);
   std::streamoff tree = Tree_File.tellg();
   Node_Count = (tree-(long)4)/(sizeof(Node));
   Tree_File.seekg(0);

}


long B_Plus_Tree::Find(float find_score, int& S_Index, bool& score_is_find)
{
   score_is_find = false;
   S_Index = 0;
   long s_Node_Offset = Root_Node_Offset;
   int i=0;

   while(1)
   {
      GetNode(s_Node_Offset);


      if(Current_Node->Node_part == 3 || Current_Node->Node_part == 4)
         break;
      else
      {
         for(i=0; i<Current_Node->Score_Element_Count; i++)
         {
            if(find_score <= Current_Node->Node_Element[i].Key_Score)
            {
               s_Node_Offset = Current_Node->Node_Element[i].Offset;
               break;
            }
         }
         if(i == Current_Node->Score_Element_Count)
         {
            if(Current_Node->Score_Element_Count == Element_In_Node)
               s_Node_Offset = Current_Node->Last_Offset;
            else
               s_Node_Offset = Current_Node->Node_Element[i].Offset;
         }
      }
   }


   S_Index = FindInNode(Current_Node, find_score, score_is_find);

   if(score_is_find == true)
      return Current_Node->Node_Element[S_Index].Offset;
   else
      return -1;
}



void B_Plus_Tree::GetNode(long s_Offset)
{
    // Current Node 가 맞을경우 바로 Return
   if(Current_Node_Offset == s_Offset)
   {
      checkok = true;
      return;
   }
   if(checkok)
   {
      Tree_File.seekp(Current_Node_Offset, ios::beg);
      Tree_File.write((char*)Current_Node, sizeof(Node));
      checkok = false;
   }
    // Current Node 로 바꿔줌
   Current_Node_Offset = s_Offset;
   Tree_File.seekg(Current_Node_Offset, ios::beg);
   Tree_File.read((char*)Current_Node, sizeof(Node));
}

int B_Plus_Tree::FindInNode(Node* score_current_node, float find_score, bool& score_is_find)
{
   for(int i=0; i<score_current_node->Score_Element_Count; i++)
   {
      if(score_current_node->Node_Element[i].Key_Score == find_score)
      {
         score_is_find = true;
         return i;
      }
      if(find_score < score_current_node->Node_Element[i].Key_Score)
      {
         score_is_find = false;
         return i;
      }
   }
   score_is_find = false;
   return score_current_node->Score_Element_Count;
}

bool B_Plus_Tree::Insert(float score, long ID_offset)
{
   bool score_is_find = false;
   int Insert_Index = -1;

   Find(score, Insert_Index, score_is_find);

   Insert_Element(score, ID_offset, Current_Node, Insert_Index);
   return true;
}


void B_Plus_Tree::Insert_Element(float score, long ID_offset, Node* score_current_node, int Insert_Index)
{
   if(score_current_node->Score_Element_Count != Element_In_Node)
      NoSplitInsert(score, ID_offset, score_current_node, Insert_Index);
   else
      SplitInsert(score, ID_offset, score_current_node, Insert_Index);
}

void B_Plus_Tree::NoSplitInsert(float score, long ID_offset, Node* score_current_node, int Insert_Index)
{
   if(score_current_node->Score_Element_Count <= 106)
      score_current_node->Node_Element[score_current_node->Score_Element_Count+1].Offset = score_current_node->Node_Element[score_current_node->Score_Element_Count].Offset;
   else
      score_current_node->Last_Offset = score_current_node->Node_Element[score_current_node->Score_Element_Count].Offset;

   for(int j=score_current_node->Score_Element_Count; j>Insert_Index; j--)
   {
      score_current_node->Node_Element[j].Key_Score    = score_current_node->Node_Element[j-1].Key_Score;
      score_current_node->Node_Element[j].Offset = score_current_node->Node_Element[j-1].Offset;
   }

   score_current_node->Node_Element[Insert_Index].Key_Score = score;
   score_current_node->Node_Element[Insert_Index].Offset = ID_offset;
   score_current_node->Score_Element_Count++;

   checkok = true;
   if(Right_element != -1 || Left_element != -1)
   {
      score_current_node->Node_Element[Insert_Index].Offset = Left_element;
      score_current_node->Node_Element[Insert_Index+1].Offset = Right_element;
      Right_element = -1;
      Left_element = -1;
   }
}

void B_Plus_Tree::SplitInsert(float score, long ID_offset, Node* score_current_node, int Insert_Index)
{
   int s, t,i;
   Node* New_Node = new Node;
   Tree_File.seekg(0, ios::end);
   long  New_Node_Offset = Tree_File.tellg();

   if(score_current_node->Node_part == 1 || score_current_node->Node_part == 2)
      New_Node->Node_part = 2;
   else if(score_current_node->Node_part == 3 || score_current_node->Node_part == 4)
   {
      New_Node->Node_part = 3;
      New_Node->Next_Node_Offset = score_current_node->Next_Node_Offset;
      score_current_node->Next_Node_Offset = New_Node_Offset;
   }
   Tree_File.seekp(0, ios::end);
   New_Node_Offset = Tree_File.tellp();
   Tree_File.write((char*)New_Node, sizeof(Node));
   Node_Count++;


   for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
   {
      New_Node->Node_Element[t].Key_Score    = score_current_node->Node_Element[s].Key_Score;
      New_Node->Node_Element[t].Offset = score_current_node->Node_Element[s].Offset;

      score_current_node->Node_Element[s].Key_Score    = -1;
      score_current_node->Node_Element[s].Offset = -1;
   }

   checkok = true;

   score_current_node->Score_Element_Count = Element_In_Node/2;
   New_Node->Score_Element_Count = Element_In_Node - Element_In_Node/2;


   New_Node->Node_Element[t].Offset = score_current_node->Last_Offset;
   score_current_node->Last_Offset = -1;


   if(Insert_Index < Element_In_Node/2)
   {
      NoSplitInsert(score, ID_offset, score_current_node, Insert_Index);
   }
   else
   {
      NoSplitInsert(score, ID_offset, New_Node, (Insert_Index - Element_In_Node/2));
   }


   Tree_File.seekp(New_Node_Offset, ios::beg);
   Tree_File.write((char*)New_Node, sizeof(Node));
   delete New_Node;

   Left_element = Current_Node_Offset;
   Right_element   = New_Node_Offset;

   float  s_Parent_Insert_Score    = score_current_node->Node_Element[score_current_node->Score_Element_Count-1].Key_Score;

   if(Get_Parent_Node(s_Parent_Insert_Score, Current_Node_Offset) != -1)
   {
      bool score_is_find = false;
      int S_Index = FindInNode(Current_Node, s_Parent_Insert_Score, score_is_find);

      for(i=0; i<Current_Node->Score_Element_Count; i++)
      {
         if(s_Parent_Insert_Score <= Current_Node->Node_Element[i].Key_Score || Current_Node->Node_Element[i].Key_Score == -1)
         {
            break;
         }
      }
      Insert_Element(s_Parent_Insert_Score, Left_element, Current_Node, i);
   }
   else
   {
      Node* New_Root_Node = new Node;
      Tree_File.seekp(0, ios::end);
      Root_Node_Offset = Tree_File.tellp();

      New_Root_Node->Node_Element[0].Key_Score = s_Parent_Insert_Score;
      New_Root_Node->Node_Element[0].Offset = Left_element;
      New_Root_Node->Node_Element[1].Key_Score = s_Parent_Insert_Score;
      New_Root_Node->Node_Element[1].Offset = Right_element;

      Right_element = -1;
      Left_element = -1;
      New_Root_Node->Node_part = 1;
      Current_Node->Node_part = 3;
      New_Root_Node->Score_Element_Count = 1;
      Node_Count++;

      Tree_File.write((char*)New_Root_Node, sizeof(Node));
      Tree_File.seekp(0, ios::beg);
      Tree_File.write((char*)&Root_Node_Offset, sizeof(long));

      delete New_Root_Node;
   }
}

int B_Plus_Tree::Get_Parent_Node(float score, long score_current_node_Offset)
{
   long Temp_P_Node_Offset = -1;

   Tree_File.seekg(0, ios::beg);
   Tree_File.read((char*)&Temp_P_Node_Offset, sizeof(long));

   for(int i=0; i<Node_Count; i++, Temp_P_Node_Offset += sizeof(Node))
   {
      GetNode(Temp_P_Node_Offset);

      if(Current_Node->Node_part == 4)
         return -1;
      if(Current_Node->Node_part == 3)
         continue;

      for(int j=0; j<=Current_Node->Score_Element_Count; j++)
      {
         if(score_current_node_Offset == Current_Node->Node_Element[j].Offset)
            return j;
      }
   }
   return -1;
}

void B_Plus_Tree::Score_search(float lower, float uppper, int* Result)
{
   int S_Index, Result_Count = 0;
   bool score_is_find;
   Find(lower, S_Index, score_is_find);
   int i = S_Index;

   while(1)
   {

      if(i == (Current_Node->Score_Element_Count-1) && Current_Node->Node_Element[i].Key_Score <=  uppper)
      {
         Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
         if(Current_Node->Next_Node_Offset != -1)
         {

            GetNode(Current_Node->Next_Node_Offset);
            i = 0;
         }
         else
            return;
      }

      if(lower <= Current_Node->Node_Element[i].Key_Score && Current_Node->Node_Element[i].Key_Score <= uppper)
         Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
      if(uppper < Current_Node->Node_Element[i].Key_Score)
         return;
      i++;
   }
   return;
}
// New Func
void B_Plus_Tree::get_leaf_Node(int num){

    int Result[20000];
    char temp[80] = "";
    unsigned Dat_File_Offset;
    string id   = "";
    for(int i=0; i<20000; i++)
      Result[i] = -1;

    GetNode(Root_Node_Offset);

    while(1){
        GetNode(Current_Node->Node_Element[0].Offset);
        if(Left_element == -1){  // 1st Leaf Node
            for(int check = 1; check < num; check++){
                GetNode(Current_Node->Next_Node_Offset);
            }
            for(int i = 0;i<Current_Node->Score_Element_Count; i++){
                Result[i] = Current_Node->Node_Element[i].Offset;
            }
            break;
        }

    }
    for(int i = 0 ; i < 20000; i++){
        if(Result[i]!=-1){
             sprintf(temp, "%d", Result[i]);
             id = temp;
             Dat_File_Offset = Hash->Get_Hash_Offset(id);
             Dat_File.seekg(Dat_File_Offset, ios::beg);
             Dat_File.read((char*)&Dat_block, sizeof(Block));

             for(int j=0; j<Dat_block.Record_Count; j++)
                if(Result[i] == Dat_block.Record[j].ID){
                    cout << Dat_block.Record[j].ID << "  " << Dat_block.Record[j].Name << " "<< Dat_block.Record[j].Score << " " << Dat_block.Record[j].advisorID << endl;
                    break;
                }
        }
        else
            break;

    }

}

B_Plus_Tree* Tree;

bool openDB(char* filename){

   char Filename[80] = "";
   sprintf(Filename, "%s.DB", filename);
   remove(Filename);
   Dat_File.open(Filename, ios::in | ios::out | ios::binary);

   if(!Dat_File)
   {
      Dat_File.clear();
      Dat_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
      Dat_File.seekp(0, ios::beg);
      Dat_File.write((char*)&Dat_block, sizeof(Block));
   }
   strcpy(Filename,"");
   sprintf(Filename, "%s.hash", filename);
   remove(Filename);
   Hash = new Dynamic_Hash(Filename);

   strcpy(Filename,"");
   sprintf(Filename, "Students_score.idx");
   remove(Filename);
   Tree = new B_Plus_Tree("Students_score.idx");

   return true;
}

unsigned insertRecord(char* name, unsigned ID, float score, unsigned advisorID)
{
   char* temp = new char[10];
   char* temp1 = new char[10];
   string id="";
   bool Is_Insert = false;

   unsigned blockNumber = 0;

   sprintf(temp, "%d",ID);
   id=temp;

   int Dat_File_Offset = Hash->Get_Hash_Offset(id);

   Dat_File.seekg(Dat_File_Offset, ios::beg);
   Dat_File.read((char*)&Dat_block, sizeof(Block));

   if(Dat_block.Record_Count < BLOCK_MAX)
      {
         strcpy(Dat_block.Record[Dat_block.Record_Count].Name, name);
         Dat_block.Record[Dat_block.Record_Count].ID = ID;
         Dat_block.Record[Dat_block.Record_Count].Score = score;
         Dat_block.Record[Dat_block.Record_Count].advisorID = advisorID;

         Dat_block.Record_Count++;

         Dat_File.seekp(Dat_File_Offset, ios::beg);
         Dat_File.write((char*)&Dat_block, sizeof(Block));
         Is_Insert = true;

      }
      else
      {
         Hash->Block_Full(id, Dat_block.Bit_Number, Dat_File);

         for(int i=0; i<Dat_block.Record_Count; i++)
         {
            unsigned studentid = Dat_block.Record[i].ID;
            sprintf(temp1, "%d",studentid);
            string student_id=temp1;
            Dat_File_Offset = Hash->Get_Hash_Offset(student_id);
            blockNumber=Dat_File_Offset;
            insertBlockNumber(blockNumber, studentid);
         }
         insertRecord(name,ID,score,advisorID);
      }

      if(Is_Insert == true)
      {
         Tree->Insert(score,(long)ID);
      }

   blockNumber=Dat_File_Offset;
   return blockNumber;
}
unsigned searchID(unsigned ID){

   char* temp = new char[10];
   string id="";

   unsigned blockNumber = 0;

   sprintf(temp, "%d",ID);
   id=temp;

   int Dat_File_Offset = Hash->Get_Hash_Offset(id);

   Dat_File.seekg(Dat_File_Offset, ios::beg);
   Dat_File.read((char*)&Dat_block, sizeof(Block));

   blockNumber=Dat_File_Offset;

   return blockNumber;
}
unsigned searchScore(float lower, float upper){

   unsigned numOfScore = 0;
   int Result[20000];
   unsigned Dat_File_Offset;
   int i;
   for(i=0; i<20000; i++)
      Result[i] = -1;

   Tree->Score_search(lower, upper, Result);

   int Total_Count = 0;

   char temp[80] = "";
   string id   = "";
   for(i=0; i<20000; i++)
   {
      if(Result[i] != -1)
      {
         sprintf(temp, "%d", Result[i]);
         id = temp;
         Dat_File_Offset = Hash->Get_Hash_Offset(id);
         Dat_File.seekg(Dat_File_Offset, ios::beg);
         Dat_File.read((char*)&Dat_block, sizeof(Block));

         for(int j=0; j<Dat_block.Record_Count; j++)
            if(Result[i] == Dat_block.Record[j].ID)
            {
                cout << Dat_block.Record[j].ID << "  " << Dat_block.Record[j].Name << " "<< Dat_block.Record[j].Score << " " << Dat_block.Record[j].advisorID << endl;
               Total_Count++;
               break;
            }
      }
      else
         break;
   }
   numOfScore=Total_Count;
   return numOfScore;
}
// main
int main()
{

   openDB("Students");

   ifstream fin("sampleData.csv", ios::in);

   // for ESPA
   ofstream fout("sampleData.out", ios::out);


    string line;
    char com;
    int total_num;
    char name[21];
    string first_name = "";
    string last_name = "";
    string total_name = "";
    unsigned studentID = 0;
    float score = 0;
    unsigned advisorID = 0;

    unsigned last_cell_id = 0 ;


    int input_num = 0;
    int sign = 0;
    while(getline(fin,line))
    {
        stringstream  lineStream(line);
        string        cell;

        if(sign == 0){
            getline(lineStream,cell,',');
            total_num = str2int(cell);
            for(int i =0 ; i<3; i++){
                getline(lineStream,cell,',');
            }
            sign++;
        }
        else if(sign > 0){
            getline(lineStream,cell,',');
            strcpy(name,cell.c_str());
             if(cell.length() > 19){
                cout << cell <<  " " << cell.length() << name <<endl;
                cell = cell.substr(0,19);
            }
            getline(lineStream,cell,',');
            studentID = str2unsign(cell);
            getline(lineStream,cell,',');
            score = str2float(cell);
            getline(lineStream,cell,',');
            advisorID = str2unsign(cell);


            // insertRecord
            unsigned blockNumber = insertRecord(name, studentID, score, advisorID);
             // for ESPA
            insertBlockNumber(blockNumber, studentID);
        }
    }
    Hash->Make_txt();
    Hash->Print_Hash();
    int num;
    cout << " Please Enter leaf Num : ";
    cin >> num;
    cout << "ID " << "NAME" << "" << "" <<endl;
    Tree->get_leaf_Node(num);

   fout.close();
   fin.close();
   return 0;
}
