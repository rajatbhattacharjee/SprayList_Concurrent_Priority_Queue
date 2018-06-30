#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <pthread.h>
#include <atomic>
#include <cstdint>
#include <thread>
#include <math.h>
#include <random>

#include <string.h>
#include <signal.h>
#include <stdlib.h>
using namespace std;

#define NUM_THREADS 5

// Class to implement node
struct Node
{
	public:
		std::atomic<int> key;
		std::atomic<int> NodeLevel;
		std::atomic<int> value;
		// Array to hold pointers to node of different level 
		std::atomic<Node**> next;
		std::atomic<bool> mark;
		Node(int key);
		Node(int key, int level);
};
 
Node::Node(int key, int level)
{
	this->key = key;
	this->value = rand() % 50 + 1;
	this->NodeLevel = level;
	this->mark = false;
	// Allocate memory to next 
	next = new Node*[level+1];
	
	// Fill next array with 0(NULL)
	memset(next, 0, sizeof(Node*)*(level+1));
}

//Sentinal Node
Node::Node(int key)
{
	this->key = key;
	this->value = -1;
	this->mark = false;
	this->NodeLevel = 5;
	// Allocate memory to next 
	next = new Node*[(this->NodeLevel)+1];
 
	// Fill next array with 0(NULL)
	memset(next, 0, sizeof(Node*)*((this->NodeLevel) + 1));
}
 
// Class for Skip list
class SkipList
{
	// Maximum level for this skip list
	int maxLevel;
 
	// current level of skip list
	int level;
 
	Node *head;
	Node *tail;
	Node *endTail;
	public:
		SkipList(int);
		Node* createNode(int, int);
		bool find(int, Node **, Node **);
		bool add(int);
		bool remove(int);
		Node* DeleteMin();
		void displayList();
};

 
SkipList::SkipList(int maxLevel)
{
	this->maxLevel = maxLevel;
	level = 0;
 
	// create header node and initialize key to -1
	head = new Node(-1);
	tail = new Node(999);
	endTail = new Node(999);
	
	
	for (int i = 0; i <= head->NodeLevel; i++)
	{
		head->next[i] = tail;
	}
	for (int i = 0; i <= tail->NodeLevel; i++)
	{
		tail->next[i] = endTail;
	}
};
 
// create new node
Node* SkipList::createNode(int key, int level)
{
	Node *n = new Node(key, level);
	return n;
};
 
// Display skip list level wise
void SkipList::displayList()
{
	cout<<"\n*****Skip List*****"<<"\n";
	for (int i = 0; i <= level; i++)
	{
		Node *node = head;
		cout << "Level " << i << ": ";
		while (node != NULL)
		{
			cout << node->key <<" ";
			node = node->next[i];
		}
		cout << "\n";
	}
};

bool SkipList::find(int x, Node *preds[], Node *succs[])
{
	int bottomLevel = 0;
	int key = x;
	bool mark1;
	bool snip;
	Node *pred = NULL;
	Node *curr = NULL;
	Node *succ = NULL;
	bool flag = false;
	while (true)
	{
		int levelsToDescend = 1;		// In the paper, for the implementation D = 1
		pred = head;
		for (int level = maxLevel; level >= bottomLevel; level = level - levelsToDescend)
		{
			curr = pred->next[level];
			while (true)
			{
				succ = curr->next[level];
				mark1 = (curr->next[level])->mark;
				if(level <= curr->NodeLevel && level >= succ->NodeLevel)
					mark1 = true;
				
				while (mark1)
				{
					if((pred->next[level])->mark == false)
					{
						std::atomic<Node*> pnext(pred->next[level]);
						snip = pnext.compare_exchange_weak(curr, succ);
						pred->next[level] = pnext;
					}
					if (!snip)
						continue;
					curr = pred->next[level];
					succ = curr->next[level];
					mark1 = (curr->next[level])->mark;
				}
				if (curr->key < key)
				{
					int jumpLength = 1 * pow (log (NUM_THREADS), 3.0);
					
					/*std::random_device rd;  //Will be used to obtain a seed for the random number engine
					std::mt19937 generator(rd()); //Standard mersenne_twister_engine seeded with rd()
					std::uniform_int_distribution<> distribution(0, jumpLength);
					int jump = distribution(generator);*/
					
					int jump;
					if(level == 0)
						jump = 1;
					else
						jump = rand() % jumpLength + 1;
					
					for(int i = 0; i < jump; i++)
					{
						if(pred->key == 999 || curr->key == 999)
						{
							flag = true;
							break;
						}
						pred = curr; curr = succ; 
						succ = curr->next[level];
					}
				}
				else
					break;
			}
			preds[level] = pred;
			succs[level] = curr;
		}
		return (curr->key == key);
	}
};

bool SkipList::add(int x)
{
	static int key = 0;
	int topLevel = rand() % 5;
	int bottomLevel = 0;
	Node *preds[maxLevel + 1];
	Node *succs[maxLevel + 1];
	
	while (true)
	{
		bool found = find(x, preds, succs);
		if (found)
		{
			return false;
		}
		else
		{
			Node *newNode = createNode(x, topLevel);
			key++;
			for (int level = bottomLevel; level <= topLevel; level++)
			{
				Node *succ;
				succ = succs[level];
				newNode->next[level] = succ;
			}
			
			Node *pred;
			pred = preds[bottomLevel];
			Node *succ;
			succ = succs[bottomLevel];
			newNode->next[bottomLevel] = succ;
			
			std::atomic<Node*> pnewnext(pred->next[bottomLevel]);
			if (!(pnewnext.compare_exchange_weak(succ, newNode)))
				continue;
			pred->next[bottomLevel] = pnewnext;
			if(topLevel > level) {
				level = topLevel;
			}
			
			for (int level = bottomLevel + 1; level <= topLevel; level++) {
				while (true)
				{
					pred = preds[level];
					succ = succs[level];
					std::atomic<Node*> pnewnext2(pred->next[level]);
					if (pnewnext2.compare_exchange_weak(succ, newNode))
					{
						pred->next[level] = pnewnext2;
						break;
					}
					find(x, preds, succs);
				}
			}
			
			return true;
		}
	}
}

Node* SkipList::DeleteMin()
{
	Node *curr = NULL;
	Node *succ = NULL;
	Node *preds[maxLevel + 1];
	Node *succs[maxLevel + 1];
	curr = head->next[0];
	while (curr != tail)
	{
		if (!((curr->next[0])->mark))
		{
			std::atomic_flag curr_flag_test((curr->next[0])->mark);
			if(!(std::atomic_flag_test_and_set(&curr_flag_test)))
			{
				(curr->next[0])->mark = true;
				find(curr->key, preds, succs);
				return curr;
			}
			else
				curr = curr->next[0];
		}
	}
	
	return NULL; // no unmarked nodes
}

struct arg_struct {
    SkipList *arg1;
    int arg2;
};

// Driver to test above code
int main()
{ 
	// create SkipList object with maxLevel and P 
	
	SkipList s1(4);
	
	auto timeA = std::chrono::high_resolution_clock::now();
	
	s1.add(1);
	s1.add(2);
	s1.add(3);
	s1.add(4);
	s1.add(5);
	/*
	s1.displayList();
	cout << "End of 1st display()" << endl;
	s1.DeleteMin();
	s1.displayList();
	*/
	
	pthread_t threads[NUM_THREADS];
	void *status;
	void *add_wrapper(void*);
	//SkipList s1(4);
	int rc1;
	int i;
    struct arg_struct args;
    args.arg1 = &s1;

	for( i = 0; i < NUM_THREADS; i++ )
	{
		cout <<"main() : creating thread, " << i << endl;
		args.arg2 = i + 1;
		
		rc1 = pthread_create(&threads[i], NULL, add_wrapper, (void *)&args);
		if (rc1)
		{
			cout << "Error:unable to create thread," << rc1 << endl;
			exit(-1);
		}
	}
	
	for( i = 0; i < NUM_THREADS; i++ )
	{
		rc1 = pthread_join(threads[i], &status);
		if (rc1)
		{
			cout << "Error:unable to join," << rc1 << endl;
			exit(-1);
		} 
		cout << "Main: completed thread id :" << i ;
		cout << "  exiting with status :" << status << endl;
	}
	s1.displayList();
	cout << "End of 1st display()" << endl;
	s1.DeleteMin();
	s1.displayList();
	
    auto timeB = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = timeB - timeA;
		  
	std::cout << "Execution Time = " << duration.count() << std::endl;
	
	pthread_exit(NULL);
	return 0;
}

static int x = 6;
void *add_wrapper(void *arguments)
{
	struct arg_struct *args = (arg_struct*)arguments;
	SkipList *s1 = args->arg1;
	bool retval = s1->add(x);
	x++;
	bool *ret = (bool*)malloc(sizeof(bool));
	*ret = retval;
	return ret;
}