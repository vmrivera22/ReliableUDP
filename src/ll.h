#include <cstdlib>
#include <iostream>
#include <vector> 
#include <algorithm>
#include <string.h>
using namespace std;

class Node{
    public:
        int port;
        char *IP;
        int total_pkts = 0;
        int pkt_count = 0;
        char **hold_buff;
        int *r_err_buff;
        int *droped_pkts;
        Node *next;
};

// LinkedList class with a head to indicate the beginning of the list and a boolean flag that is true when the list is
// sorted and false when it is not.
class LinkedList{
  private:
    Node *head = NULL;
    bool sorted;
  public:
    Node* insert(int, char*);
    Node* find(int, char*);
    Node* sort();
    Node* fr_find(int);
    void node_switch(Node*, Node*, Node*);
    string print();
    int length();
    Node* delete_Node(int, char*);
};

Node* LinkedList :: insert(int in_port, char *in_IP){
  sorted = false; // If a new node is added then the linked list is no longer sorted.

  Node *in_node = new Node; //Allocate memory for a new node.
  in_node->port = in_port;
  in_node->IP = in_IP; // If we are inserting a new node then there has been one instance of such word.
  in_node->next = head;
  head = in_node; // Make the new node the head.
  return in_node; // Returns the head of the linked list.
}

Node* LinkedList :: find(int in_port, char *in_IP){
  Node *curr = head;
  //Iterate through the linked list from the head until the end or until the argument string matches the word at the node.
  while(curr != NULL){
    //cout << "in_P: " << in_port << " C_P: " << curr->port << " in_IP: " << in_IP << " Curr_IP: " << curr->IP << "\n\n";
    if(curr->port == in_port && (strcmp(curr->IP, in_IP) == 0)){
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

//Function only used for testing
string LinkedList :: print(){
  char *lis_str = NULL;
  Node *curr = head;
  char *print_temp = new char[10000]{'\0'};
  while(curr != NULL){
    sprintf(print_temp, "%s[%s:%d] ", lis_str, curr->IP, curr->port);
    curr = curr->next;
  }
  cout << print_temp << "\n\n";
  delete[] print_temp;
  return lis_str;
}

int LinkedList :: length(){
  Node *curr = head;
  int count = 0;
  // Increase the count until the end of the linked list.
  while(curr != NULL){
    count++;
    curr = curr->next;
  }
  return count;
}

void LinkedList :: node_switch(Node *curr, Node *nxt, Node *prev){
  curr->next = nxt->next;
  nxt->next = curr;
  if(prev != NULL){
    prev->next = nxt;
  }
  else{
    this->head = nxt;
  }
}

//Sorts the Linked List both by frequency and lexicographically.
Node* LinkedList :: sort(){
  //If the list is already sorted, don't sort again
  if(sorted == true){
    return head;
  }
  Node *prev = NULL;
  Node *curr = head;
  Node *nxt = curr->next;
  int lis_len = length();
  int i = lis_len;
  int j = lis_len;

  while(i != 0){ //Iterate through the linked list
    prev = NULL;
    curr = head;
    nxt = curr->next;
    j = lis_len;
    while(j != 0 && nxt != NULL){ // Iterate through the linked list

      //Move the least frequent (and last lexicographically) to the tail of the list.
      if(curr->IP < nxt->IP){
        this->node_switch(curr, nxt, prev);
        curr = nxt;
        nxt = curr->next;
      }
      else if(curr->IP == nxt->IP && curr->IP > nxt->IP){
        this->node_switch(curr, nxt, prev);
        curr = nxt;
        nxt = nxt->next;
      }
      //Move pointer foward.
      prev = curr;
      curr = nxt;
      nxt = nxt->next;
      j--;
    }
    //The right most nodes are in place so no need to iterate through the whole linked list.
    lis_len--;
    i--;
  }
  sorted = true;
  return head;
}

// Used to search list based on frequency.
Node* LinkedList :: fr_find(int frq){
  Node *curr = head;
  //Iterate through the list until frq == 0 (gives the kth rank item).
  while (curr != NULL){
    if(frq == 0){
      return curr;
    }
    frq--;
    curr = curr->next;
  }
  return NULL;
}

Node* LinkedList :: delete_Node(int in_port, char *in_IP)
{
    Node* prev = NULL;
    Node* curr = head;
    while(curr != NULL)
    {
        if(curr->port == in_port && curr->IP == in_IP){
            break; 
        }
        prev = curr; // move prev by one Node
        curr = curr->next; // move curr by one Node
    }
    if (curr == NULL){
        return NULL; 
    }
    if (prev == NULL){
        head = head->next;
    }
    else{
        prev->next = curr->next;
    }
    curr->next = NULL;
    //delete(curr);///////
    return curr;
}
