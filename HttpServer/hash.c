#include <iostream>  
#include <malloc.h>
using namespace std; 

typedef enum {EX,EM,DL}State;//储存位置的状态，是已存，空，还是删除过，在这个哈希表中删除过元素的存储空间是不可再用的
typedef int DataType;

struct HTELEM {
  DataType  _data;//存入的关键字（关键字不一定是数值，当然目前我们做的这个表中存的是数值）
  State _state;
};

class HT
{  
  private:  
    HTELEM*  arr;//存放哈希表的数组
    int _capacity;//数组的容量
    int _size;//已用存储位置的大小
    int _IsLineDetective;//标志着选择的哈希函数
  public:
    void   swap (HT*newht)//交换两个哈希表的内容
    {
      long long temp=0;

      temp = (long long)this->arr;
      this->arr = newht->arr;
      newht->arr =(HTELEM*)temp;


      temp=this->_capacity;
      this->_capacity=newht->_capacity;
      newht->_capacity=temp;

      temp=this->_size;
      this->_size=newht->_size;
      newht->_size=temp;

      temp=this->_IsLineDetective;
      this->_IsLineDetective=newht->_IsLineDetective;
      newht->_IsLineDetective=temp;
    }

    void HashTableInit(int capacity,int IsLineDetectiev)//初始化哈希表
    {
      int i=0;
      this->arr=(HTELEM*)malloc(sizeof(HTELEM)*capacity);//给存放哈希表的数组动态开辟内存
      for (i=0;i<capacity;i++)//所有位置的储存状态置为空
      {
        this->arr[i]._state=EM;
      }

      this->_size=0;
      this->_IsLineDetective=IsLineDetectiev;
      this ->_capacity=capacity;
    }


    int HashFunc (DataType  data)
    {
      return    data%this->_capacity;//哈希函数；除留余数法
    }

    int DetectiveLine(int hashaddr)//一次线型探测
    {
      hashaddr++;
      if (hashaddr==this->_capacity)
        hashaddr=0;
      return hashaddr;
    }

    int Detective2(int hashaddr,int i)//二次（平方）探测
    {
      hashaddr=hashaddr+2*i+1;
      if (hashaddr>=this->_capacity)
        hashaddr%=this->_capacity;
      return hashaddr;
    }

    void HashTableInsert (DataType data)//哈希表的插入
    {
      int i=0;
      int hashaddr=-1;


      CheckCapacity();//查看并决定是否需要扩容


      hashaddr=HashFunc(data);//通过哈希函数计算应插位置

      while (EM!=this->arr[hashaddr]._state)
      {
        if (this->arr[hashaddr]._state==EX)
        {
          if (this->arr[hashaddr]._data==data)//已经有这个数据就不用插入，直接返回
            return;
        }
        if (this->_IsLineDetective)//当前的位置被占（被不是要插的数据占），或被删除过，则不可用（哈希冲突），向后探测
        {
          hashaddr=DetectiveLine(hashaddr);
        }
        else {
          ++i;
          hashaddr=Detective2(hashaddr,i);
        }

      }

      this ->arr[hashaddr]._state=EX;//向已经找到的位置，放入数据
      this->arr[hashaddr]._data=data;
      this->_size++;
    }



    int HashTableFind (DataType data)//在哈希表中查找数据，与插入相似
    {
      int hashaddr=-1,startaddr=-1,i=0;

      hashaddr =HashFunc(data);
      startaddr=hashaddr;//记录开始查找位置（既不发生哈希冲突时应存位置）

      while (EM!=this->arr[hashaddr]._state)
      {
        if (this->arr[hashaddr]._state==EX)//找到返回
          if (this->arr[hashaddr]._data==data)
            return    hashaddr;

        if (this->_IsLineDetective)//向后查找
        {
          hashaddr=DetectiveLine(hashaddr);
          if (hashaddr==startaddr)
            return -1;
        }
        else {
          ++i;
          hashaddr=Detective2(hashaddr,i);
          if (hashaddr==startaddr)
            return -1;

        }
      }
      return -1;
    }



    void HashTableDelete(DataType   data)//哈希表的删除
    {

      int ret=-1;


      ret =HashTableFind(data);

      if (ret!=-1)
      {
        this->arr[ret]._state=DL;
        this->_size--;
      }
    }

    int HashTableSize()//哈希表的大小
    {

      return this->_size;
    }


    int HashTableEmpty()//哈希表是否为空
    {
      if (  0==this->_size)
        return 1;
    }


    void HashTableDestory()//哈希表的销毁

    {
      free(this->arr);
      this->arr=NULL;
      this->_size=0;
      this->_IsLineDetective=0;
      this->_capacity=0;
    }

    int CheckCapacity()//哈希表的扩容
    {
      int i=0;
      int capacity;
      if (this->_size*10/this->_capacity>7)//当已用空间占到总容量的70%时扩容
      {
        HT newht;
        capacity=this->_capacity*2;
        newht.HashTableInit(capacity,this->_IsLineDetective);//初始化一个新表，（新表的容量是老表的2倍）

        for (i=0;i<this->_capacity;i++)//把老表中的元素插入新表
        {
          if (EX==this->arr[i]._state)
            newht.HashTableInsert (this->arr[i]._data);
        }
        swap(&newht);//把新表的内容交给老表（新表只是一个函数体内的变量，除了函数体会被销毁）
        newht.HashTableDestory();//销毁新表
      }
      return 1;
    }  

}; 



void TestHashTable()
{
  HT ht;
  ht.HashTableInit(6,1);
  ht.HashTableInsert (23);
  ht.HashTableInsert (14);
  ht.HashTableInsert (74);
  ht.HashTableInsert (33);
  ht.HashTableInsert (19);
  ht.HashTableInsert (29);
  ht.HashTableInsert (29);

  cout<<ht.HashTableSize()<<endl;
  if (-1!=ht.HashTableFind(33))
  {
    cout<<"have"<<endl;
  }
  else
    cout<<"no have"<<endl;


  if (-1!=ht.HashTableFind(3))
  {
    cout<<"have"<<endl;
  }
  else
    cout<<"no have"<<endl;


  ht.HashTableDelete(23);

  cout<<ht.HashTableSize()<<endl;
  if (-1!=ht.HashTableFind(23))
  {
    cout<<"have"<<endl;
  }
  else
    cout<<"no have"<<endl;

}


int main ()
{
  TestHashTable();
  return 0;
}
