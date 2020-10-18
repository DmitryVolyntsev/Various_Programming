#include <stdio.h>
#include <stdlib.h>

struct node {
	int data;
	node* next;
};

void push (node** _s, int data)
{
	node* _n = (node*)malloc(sizeof(node));
	_n->data = data;  
	_n->next = *_s;
	*_s = _n;
}

void print(node** _s)
{
	node* _n = *_s;
	while (_n)
	{
		printf("%d ", _n->data);
		_n = _n->next;
	}
}

void print_r (node* _s)
{
	if (!_s) return;
	//printf("%d ", _s->data);
	print_r(_s->next);
	printf("%d ", _s->data);
}

void empty(node** _s)
{
	node* _n = *_s;
	if (!_n) return;
	*_s = (*_s)->next;
	free(_n);
	empty(_s);	
}

int  main()
{
	node* s = NULL;
	push(&s, 5);
	push(&s, 3);
	push(&s, 7);
	//s.top->next->next->data = 10;
	print_r(s);
	empty(&s);
	return 0;
}
