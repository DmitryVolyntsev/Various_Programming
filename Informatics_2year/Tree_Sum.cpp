#include <iostream>
#include <vector>
#include <thread>

using namespace std;

class Tree
{
	public:
	int val;
	Tree *parent;
	vector<Tree *> leaves;
//	vector<int> sm;
};

void Add(Tree * p)
{
	Tree *q;
	q = new Tree;
	cout << "Введите значение узла:" << endl;
	cin >> q->val;
	q->parent = p;
	p->leaves.push_back(q);
//	cout << "Пушбак прошел успешно" << endl;
	cout << "Введите число потомков:" << endl;
	int m;
	cin >> m;
	for(int i=0; i<m; i++)
	{
		Add(q);
	}
}

Tree * Create()
{
	Tree *q;
	q = new Tree;
	q->parent = NULL;
	cout << "Введите значение корня:" << endl;
	cin >> q->val;
	cout << "Введиет число потомков:" << endl;
	int m;
	cin >> m; 
	for(int i=0; i<m; i++)
		Add(q);
	return q;
}
	
void DelTree(Tree *p)
{
	int k;
//	cout << "Im inside deltree" << endl;
	k = p->leaves.size();
	for(int i=0; i<k; i++)
		{
//			cout << "Inside cycle" << endl;
			DelTree(p->leaves[i]);
		}
	cout << p << " deleted!" << endl;
	delete p;
}

int EasySum(Tree *p)
{
	int m;
	m = p->leaves.size();
	int s = p->val;
	for(int i=0; i<m; i++)
		s += EasySum(p->leaves[i]);
	return s; 
}

void Sum(int *s, Tree *p, int k)
{
	int m;
	m = p->leaves.size();
	if(m == 0)
		*s = p->val;
	else
	{	
		if(k == 0)
		{
			*s = EasySum(p);	
		} else
		{
			int quotient, remainder;
			quotient = k / m;
			remainder = k % m;
			int *values;
			thread *mas;
			mas = new thread[m];
			values = new int[m];
			for(int i=0; i<m; i++)
				*(values+i) = 0;	
			if(quotient > 0)
			{
				for(int i=0; i<remainder; i++)
					mas[i] = thread(Sum, values+i, p->leaves[i], quotient);
				for(int i=remainder; i<m; i++)
					mas[i] = thread(Sum, values+i, p->leaves[i], quotient-1);
	cout << "Before join" << endl;			
				for(int i=0; i<m; i++)
					mas[i].join();
	cout << "After join" << endl;
			}
			else
			{
				for(int i=0; i<remainder; i++)
					mas[i] = thread(Sum, values+i, p->leaves[i], 0);
				for(int i=remainder; i<m; i++)
					*(values+i) = EasySum(p->leaves[i]);
	cout << "Bef join" << endl;
				for(int i=0; i<remainder; i++)
					mas[i].join();	
	cout << "AFt join" << endl;
			}
			for(int i=0; i<m; i++)
				*s += *(values+i);
			*s += p->val;
//			delete mas;
//			delete values; 	
		}
	}
}

int main()
{
	Tree *p;
	p = Create();
	
	cout << "I'm here!" << endl;
	
//	cout << p << endl;
//	cout << p->val << endl;	

	cout << "Введите число вычислителей:" << endl;
	int k;
	cin >> k;
		
	int *Res;
	Res = new int;
	*Res = 0;	

	Sum(Res, p, k);
	
	cout << "Сумма узлов дерева равна " << *Res << endl;	
	
	delete Res;
	DelTree(p);
	return 0;
}
