/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file contains all the functions
 *				 necessary for the history and
 * 				 navhistory functionality in FASH
 *
 * Date: 04/12/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "history.h"

extern char *args[100]; //command arguments
struct HistoryList {
	char *data; //string of the command
	struct HistoryList *next; //pointer to next node in HistoryList
	struct HistoryList *prev; //pointer to previous node
};
struct HistoryList *historyHead; //head (first node) for history
struct HistoryList *navHistoryHead; //head for navhistory
int historyMax = 500; //this is the default maximum # of history items
int navHistoryMax = 500; //same as historymax but for nav
static int navHistoryInd; 
static int historyInd; //history index (total)
struct HistoryList *curHistory; //current history node
struct HistoryList *curNavHistory; //current navhistory node

void add_history(char *line)
{
	/* Arguments:
			line = line to add to bash history
		
		Purpose:
			keeps a history of bash commands
			for the user to feel more like standard bash
			
			this implements a circularly linked list for
			the history items
	*/
	struct HistoryList *temp = (struct HistoryList*)malloc(sizeof(struct HistoryList));
	//curHistory will be a new special node that has no data, has no
	//next, but prev points to the previous node (in this case the head)
	curHistory = (struct HistoryList*)malloc(sizeof(struct HistoryList)); 
	curHistory->data = NULL;
	curHistory->next = NULL;
	
	if(historyHead == NULL) { //special case if the list is empty
		temp->data = (char*)malloc(sizeof(char) * 100); //allocate space
		memset(temp->data,0,100);
		strncpy(temp->data,line,strlen(line)); //put the command into the data member
		temp->next = NULL; //there isnt a next yet
		temp->prev = temp; //previous points to itself? wow...
		historyHead = temp; //this 1st node is the head
		historyHead->next = curHistory;
		curHistory->prev = historyHead;
	}
	else {
		temp = historyHead;
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp = temp->prev;
		struct HistoryList *temp2 = (struct HistoryList*)malloc(sizeof(struct HistoryList));
		temp2->data = (char *)malloc(sizeof(char) * 100);
		memset(temp2->data,0,100);
		strncpy(temp2->data,line,strlen(line));
		temp2->next = curHistory;
		temp2->prev = temp;
		temp->next = temp2;
		historyHead->prev = temp2;
		curHistory->prev = temp2;
	}	
	++historyInd;
	if (historyInd > historyMax) {
		//this command is past the historyMax
		//so we need to add this new command
		//and cut off the hold one
		struct HistoryList *tmp = historyHead->prev;
		historyHead = historyHead->next;
		historyHead->prev = tmp;
	}
}

void add_nav_history(char *line)
{
	/* Arguments:
			line = line to add to nav history
		
		Purpose:
			keeps a history of current working directories
			
			this implements a circularly linked list for
			the history items
	*/
	struct HistoryList *temp = (struct HistoryList*)malloc(sizeof(struct HistoryList));
	//curNavHistory will be a new special node that has no data, has no
	//next, but prev points to the previous node (in this case the head)
	curNavHistory = (struct HistoryList*)malloc(sizeof(struct HistoryList)); 
	curNavHistory->data = NULL;
	curNavHistory->next = NULL;
	
	if(navHistoryHead == NULL) { //special case if the list is empty
		temp->data = (char*)malloc(sizeof(char) * 100); //allocate space
		memset(temp->data,0,100);
		strncpy(temp->data,line,strlen(line)); //put the command into the data member
		temp->next = NULL; //there isnt a next yet
		temp->prev = temp; //previous points to itself? wow...
		navHistoryHead = temp; //this 1st node is the head
		navHistoryHead->next = curNavHistory;
		curNavHistory->prev = navHistoryHead;
	}
	else {
		temp = navHistoryHead;
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp = temp->prev;
		struct HistoryList *temp2 = (struct HistoryList*)malloc(sizeof(struct HistoryList));
		temp2->data = (char *)malloc(sizeof(char) * 100);
		memset(temp2->data,0,100);
		strncpy(temp2->data,line,strlen(line));
		temp2->next = curNavHistory;
		temp2->prev = temp;
		temp->next = temp2;
		navHistoryHead->prev = temp2;
		curNavHistory->prev = temp2;
	}	
	++navHistoryInd;
	if (navHistoryInd > navHistoryMax) {
		//this command is past the historyMax
		//so we need to add this new command
		//and cut off the hold one
		struct HistoryList *tmp = navHistoryHead->prev;
		navHistoryHead = navHistoryHead->next;
		navHistoryHead->prev = tmp;
	}
}

char* prev_history(void)
{
	/* Arguments:
			none
		
		Purpose:
			prints to the line the last command in
			history or returns 0 if none
		
		Returns:
			char* string of last command (if exists) or 0
	*/
	if(curHistory == NULL) {return 0;} //get trivial case	
	if(curHistory != historyHead) {
		curHistory = curHistory->prev;
	}
	char *tmp = (char*)malloc(sizeof(char)*100);
	memset(tmp,0,100);
	strncpy(tmp, curHistory->data, strlen(curHistory->data));
	return tmp;
}

char* next_history(void)
{
	/* Arguments:
			none
		
		Purpose:
			prints to the line the next command in
			history (if we've gone back a few)
		
		Returns:
			char* string of next command (if exists) or 0 if no next history
	*/
	if(curHistory == NULL) {return 0;} //trivial
	if(curHistory->next == NULL){return 0;} //still trivial
	if(curHistory->next->data != NULL) {
		curHistory = curHistory->next;
	} else {curHistory = curHistory->next; return 0;}
	char *tmp = (char*)malloc(sizeof(char)*100);
	memset(tmp,0,100);
	strncpy(tmp, curHistory->data, strlen(curHistory->data));
	return tmp;
}

void print_history(void)
{
	/* Arguments:
			none
		
		Purpose:
			prints to screen an indexed listing
			of the history of comands just like
			in bash
	*/
	int x = 0;
	struct HistoryList *temp;
	temp = historyHead;
	if(temp == NULL) { printf("No history.\n"); }
	else {
		while(temp != NULL && temp->data != NULL) {
			printf("  %i    %s\n",x+1,temp->data);
			++x;
			temp = temp->next;
		}
	}
}

void print_nav_history(void)
{
	/* Arguments:
			none
		
		Purpose:
			prints to screen an indexed listing
			of the navigation history
	*/
	int x = 0;
	struct HistoryList *temp;
	temp = navHistoryHead;
	if(temp == NULL) { printf("No nav history.\n"); }
	else {
		while(temp != NULL && temp->data != NULL) {
			printf("  %i    %s\n",x+1,temp->data);
			++x;
			temp = temp->next;
		}
	}
}

void exec_history(char* cmd)
{
	/* Arguments:
			cmd = ![number]
		
		Purpose:
			executes the command in history
			specified by the # in history 
			
			Example:
				!13 -- executes history #13 if
					it exists
				
				! -- executes the last command
	*/
	++cmd; //removes ! char
	int hisNum = atoi(cmd); //converts string number to int
	if(strlen(cmd) == 0) { hisNum = historyInd; } //if they provide no number, run most recent
	if(hisNum == 0) { 
		//some error
		char *tmp = (char*)malloc(sizeof(char) * 40);
		memset(tmp,0,40);
		sprintf(tmp, "-fritzbash: !%s: not a valid event\n", cmd);
		write(1,tmp,strlen(tmp));
		free(tmp);
		return;
	}
	char *tmp = (char*)malloc(sizeof(char) * 100);
	memset(tmp,0,100);
	if(hisNum > historyInd || hisNum < 0) { 
		//if they request a negative command or one that
		// "hasn't" happened yet
		char *tmp = (char*)malloc(sizeof(char) * 40);
		memset(tmp,0,40);
		sprintf(tmp, "-fritzbash: !%i: event not found\n", hisNum);
		write(1,tmp,strlen(tmp));
		free(tmp);
	} else {			
		if(hisNum <= (int)(historyInd/2)) {
			//go forward through linked list
			int x = 0;
			struct HistoryList *cur = historyHead; 
			for(;x<hisNum-1;++x) {
				cur = cur->next;	//next node			
			}
			strncpy(tmp,cur->data,strlen(cur->data)); //copy command
		} else {
			//go backward through linked list
			int x = 0;
			int y = historyInd-hisNum;
			struct HistoryList *cur = historyHead->prev; //start at end
			for(;x<y;++x) {
				cur = cur->prev; //go back
			}
			strncpy(tmp,cur->data,strlen(cur->data)); //copy command		
		}
		//now prepare to send to execute
		char *tmp2 = (char*)malloc(sizeof(char) * 100);
		memset(tmp2,0,100);
		sprintf(tmp2,"%s\n",tmp); //print the command they chose
		write(1,tmp2,strlen(tmp2));
		clear_args(); //clear all old arguments
		parse_args(tmp); //reparse this old command
		add_history(tmp); //this is what goes into history
		execute(args[0]); //arg[0] is the command itself
		free(tmp);
	}
	
}

void exec_nav_history(char* cmd)
{
	/* Arguments:
			cmd = @[number]
		
		Purpose:
			executes the command in nav history
			specified by the # in nav history 
			
			Example:
				@13 -- changes to nav history #13 if
					it exists
				
				@ -- executes the last command
	*/
	++cmd; //removes @ char
	int hisNum = atoi(cmd); //converts string number to int
	if(strlen(cmd) == 0) { hisNum = historyInd; } //if they provide no number, run most recent
	if(hisNum == 0) { 
		//some error
		char *tmp = (char*)malloc(sizeof(char) * 40);
		memset(tmp,0,40);
		sprintf(tmp, "-fritzbash: @%s: not a valid event\n", cmd);
		write(1,tmp,strlen(tmp));
		free(tmp);
		return;
	}
	char *tmp = (char*)malloc(sizeof(char) * 100);
	memset(tmp,0,100);
	if(hisNum > navHistoryInd || hisNum < 0) { 
		//if they request a negative command or one that
		// "hasn't" happened yet
		char *tmp = (char*)malloc(sizeof(char) * 40);
		memset(tmp,0,40);
		sprintf(tmp, "-fritzbash: @%i: event not found\n", hisNum);
		write(1,tmp,strlen(tmp));
		free(tmp);
	} else {			
		if(hisNum <= (int)(navHistoryInd/2)) {
			//go forward through linked list
			int x = 0;
			struct HistoryList *cur = navHistoryHead; 
			for(;x<hisNum-1;++x) {
				cur = cur->next;	//next node			
			}
			strncpy(tmp,cur->data,strlen(cur->data)); //copy command
		} else {
			//go backward through linked list
			int x = 0;
			int y = navHistoryInd-hisNum;
			struct HistoryList *cur = navHistoryHead->prev; //start at end
			for(;x<y;++x) {
				cur = cur->prev; //go back
			}
			strncpy(tmp,cur->data,strlen(cur->data)); //copy command		
		}
		//now change directory
		chdir(tmp);
		free(tmp);
	}
}
