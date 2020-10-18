#include <stdio.h>
#include <stdlib.h>

struct node {
	int data;
	node* next;
};

struct stack {
	node* top;
};

void push (stack* _s, int data)
{
	node* _n = (node*)malloc(sizeof(node));
	_n->data = data;  
	_n->next = _s->top;
	_s->top = _n;
}

void print(stack* _s)
{
	node* _n = _s->top;
	while (_n)
  	{
		printf("%d ", _n->data);
		_n = _n->next;
	}
}

void empty(stack* s)
{
	node* _n = s->top;
	if (!_n) return;
	s->top = s->top->next;
	free(_n);
	empty(s);	
}

int  main()
{
	stack s;
	s.top = NULL;
	push(&s, 5);
	push(&s, 3);
	push(&s, 7);
	//s.top->next->next->data = 10;
	print(&s);
	empty(&s);
	return 0;
}
