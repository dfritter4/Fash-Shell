/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file contains all the functions
 *				 necessary for the tab completeion
 *				 functionality in FASH
 *
 * Date: 04/15/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "tabcomplete.h"

extern int errno;
char *tabCompHold[100];

char* tab_complete(char* line)
{
	/* Arguments:
			line = line of what has been typed so far
		
		Purpose:
			takes the most recent argument in
			line (from end to most recent space)
			and looks through the current directory
			to find any files that match the first few
			letters
			
		Returns:
			returns a pointer to a string that contains
			the result of a single tab completion, or NULL
			if there were none or more than one
	*/
	if(!strcmp(line,"")) { return NULL; }
	DIR *dirp;
	struct dirent *dp;	
	
	char *lineArg = (char*)malloc(sizeof(char) * 100);
	memset(lineArg,0,100);
	strcpy(lineArg,line);	
	int possInd = 0;	
	int i = strlen(lineArg)-1;
	char c = lineArg[i];
	char *line2;
	if(strstr(line," ")) { //a space does exist
		//first get most recent argument		
		line2 = (char*)malloc(sizeof(char) * 100);
		memset(line2,0,100);
		while(c != ' ') { //this finds the first space
			--i;
			c = lineArg[i];
		}
		++i;
		while(c != '\0') { //copies from space till end
			c = lineArg[i];
			strncat(line2,&c,1);
			++i;
		}
		strncat(line2,"\0",1);
	} else { line2 = lineArg; } //otherwise just do the first arg
	
	if((dirp = opendir(".")) == NULL) {
	 //error, but i dont care
	}
	
	//search through everything in the directory for
	//things that start with what they typed
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if(strncmp(dp->d_name, line2,strlen(line2)) != 0) {
				continue;
			}
			//we found something they matches
			tabCompHold[possInd] = (char*)malloc(sizeof(char)*100);
			strcpy(tabCompHold[possInd],dp->d_name);
			++possInd;
		}
	} while( dp != NULL);
	
	char *tmp = (char*)calloc(0,sizeof(char) * 100);
	char *tmp2 = (char*)calloc(0,sizeof(char) * 100);
	char *tmpp = tmp;
	if(possInd == 1) {
		//just return that one thing and clear the hold
		strcpy(tmp,tabCompHold[0]);
		tmp+=strlen(line2); //only the completion part
		strcpy(tmp2,tmp); //copy this completion part
		free(tmpp); //free old crap
		clear_tab_hold(); //clear anything in the hold
		return tmp2; //return this new completion
	} else {
		// multiple matches, continue to build complete line
		// tabCompHold will have the matches
		//
		// for example, lets say the line is 
		//		$ grep al
		//
		// if there are three files that start with "al"
		//		alias.c alias.h alias.o
		// 
		// then we can match up to the "."
		//
		// line should then print out
		//		$ grep alias.
		tmp[0] = *(tabCompHold[0]);
		char nextChar = *(tabCompHold[0]+1);
		int holds = 0;
		int x=0,y=1;
		//we can only match up to length of the shortest
		//string, so we'll go until the end of the first
		while(nextChar != '\0'){
			for(x = 1;x<possInd; ++x){
				if(nextChar == *(tabCompHold[x]+y)){
					holds = 1;
				} else {
					holds = 0;
					break;
				}
			}
			if(!holds){
				break;
			}
			else{
				tmp[y] = *(tabCompHold[0]+y);
			}
			++y;
			nextChar = *(tabCompHold[0]+y);
			holds = 0;
		}
		tmp+=strlen(line2); //only the completion part
		strcpy(tmp2,tmp); //copy this completion part
		free(tmpp); //free old crap
		return tmp2; //return this new completion
	}
	
	//cleanup
	free(lineArg);
	if(!line2) { free(line2);} //only free if we malloced
	
	return NULL;
}

void clear_tab_hold(void)
{
	/* Arguments:
			none
		
		Purpose:
			this function clears the tabCompHold array
	*/
	int i;
	for(i=0;tabCompHold[i]!=NULL;++i) {
		memset(tabCompHold[i],0,strlen(tabCompHold[i])+1); //clear arg string
		tabCompHold[i] = NULL; //nullify pointer
		free(tabCompHold[i]); //free array block
	}
}
