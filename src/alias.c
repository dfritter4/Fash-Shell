/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file contains all the functions
 *	    necessary for the alias functionality
 *	    in FASH
 *
 * Date: 04/12/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "uthash.h"
#include "alias.h"

extern char *args[100]; //command arguments
struct Alias_Table {
	char aliasName[100]; //the name of the alias
	char aliasValue[100]; //the value of the alias
	UT_hash_handle hh; //necessary for hash table
};
struct Alias_Table *alias_hash_table; //the hash_table of aliases

void add_alias(char* aName)
{
	/* Arguments:
			aName = name/value of the alias to add
		
		Purpose:
			takes in an a string in the format of
				command=alias for command
			and creates an alias for it
			
			if format of command is:
				alias ls
			will list the alias for "ls" if exists
			or specify if not
	*/
	struct Alias_Table *s;
	struct Alias_Table *f;
	
	char *tok;
	char *eq = "=";
	char *nameCopy = (char*)malloc(sizeof(char) *100);
	char *name = (char*)malloc(sizeof(char) * 100);
	char *value = (char*)malloc(sizeof(char) * 100);
	char *pNameCopy;
	memset(nameCopy,0,100);
	memset(name,0,100);
	memset(value,0,100);
	strcpy(nameCopy, aName);
	tok = strstr(aName,eq);
	if(tok != NULL) {
		tok = strtok(aName,eq);
		//there was an = sign, so they had something
		strcpy(name,tok); //name is now the name of the alias
		f = (struct Alias_Table*)malloc(sizeof(struct Alias_Table));
		HASH_FIND_STR(alias_hash_table,name,f);
		if(f != NULL) {
			//name was already in the hash table, so we need
			//to delete it
			HASH_DEL(alias_hash_table, f); //deletes old alias
		}
		pNameCopy = nameCopy; //keep track of the this so we can free it later
		nameCopy += strlen(tok)+1; //nameCopy now equals the value, yay pointers
		
		//we need to remove the quotation mark characters (-2 because of newline)
		//this assumes (probably unsafely) that if it starts with a quotation mark, it
		//ends with one
		if(nameCopy[0] == '"') { nameCopy += 1; nameCopy[strlen(nameCopy)-2] = '\0'; }
		
		if(nameCopy != NULL) {			
			//we actually have a value
			strcpy(value,nameCopy); //value is now alias' value
			s = (struct Alias_Table*)malloc(sizeof(struct Alias_Table));
			strcpy(s->aliasName, name);
			strcpy(s->aliasValue, value);
			HASH_ADD_STR(alias_hash_table,aliasName,s); //add to hash table
			free(pNameCopy);
			free(name);
			free(value);
			return;
		}
		free(f);
	}
	//there was no '=' sign, so they wanted to print
	//the alias if it exists, or issue a message if it doesnt
	HASH_FIND_STR(alias_hash_table, aName, s);
	char *tmp = (char*)malloc(sizeof(char) *100);
	memset(tmp,0,100);
	if(s == NULL) {
		//they wanted to print an alias that doesnt exist		
		sprintf(tmp,"fritzbash: alias: %s: not found\n",aName);
		write(1,tmp,strlen(tmp));
	} else {
		//print out its value
		sprintf(tmp, "alias %s='%s'\n",s->aliasName,s->aliasValue);
		write(1,tmp,strlen(tmp));
	}
	//cleanup
	free(tmp);
	free(nameCopy);
	free(name);
	free(value);
	free(s);
}

void check_alias(char* cmd)
{
	/* Arguments:
			*cmd --- the command to check for aliases
		
		Purpose:
			looks for aliases to cmd and if it finds one,
			replaces cmd (arg[0]) and adds any arguments
			to the argument list
	*/
	struct Alias_Table *s;
	HASH_FIND_STR(alias_hash_table, cmd, s);
	if(s!= NULL) {
		//an alias exists for that command
		char *line = (char*)malloc(sizeof(char)*100);
		memset(line,0,100);
		strcat(line,s->aliasValue); 
		char *arg = args[1];
		int i = 2;
		//we are going to build up a new string
		//to parse with the replaced alias
		while(arg != NULL) {
			strcat(line, " ");
			strcat(line,arg);
			arg = args[i];
			++i;
		}
		clear_args(); //clear old args
		parse_args(line); //reparse this line
		free(line); //cleanup
	}	
}

void remove_alias(char* aName)
{
	/* Arguments:
			aName = name/value of the alias to remove
		
		Purpose:
			searches for alias called "aName" and
			removes it if it exists, or specifies that there
			wasn't one
			
			if aName == "-a", remove all aliases
	*/
	if(aName	 == NULL) {
		//they didnt provide a name to unalias
		//so issue a usage statement
		char *tmp = (char*)malloc(sizeof(char)*100);
		memset(tmp,0,100);
		sprintf(tmp,"unalias: usage: unalias [-a] name [name ...]\n");
		write(1,tmp,strlen(tmp));
		free(tmp);
		return;
	} else if(!strcmp(aName,"-a")) {
		//asking to remove all
		struct Alias_Table *s;
		while(alias_hash_table) {
			s = alias_hash_table;
			HASH_DEL(alias_hash_table,s);
			free(s);
		} return;
	} else {
		//they provided a name (or multiple) so
		//lets see if we can find them and remove them
		//if they exist, or print something if we didnt find them
		char *arg = args[1];
		int i=2;
		struct Alias_Table *s;
		char *tmp = (char*)malloc(sizeof(char)*100);
		memset(tmp,0,100);
		while(arg != NULL) {
			HASH_FIND_STR(alias_hash_table, arg, s);
			if(s == NULL) {
				//they requested something that wasnt in the alias table
				sprintf(tmp, "fritzbash: unalias: %s: not found\n",arg);
				write(1,tmp,strlen(tmp));
				memset(tmp,0,100);
			} else {
				//we did find something, so lets delete it
				HASH_DEL(alias_hash_table, s);
				free(s);
			}
			arg = args[i];
			++i;
		}
		free(tmp);
	}
}

void print_aliases(void)
{
	/* Arguments:
			none
		
		Purpose:
			prints all aliases
	*/
	struct Alias_Table *s;
	s = alias_hash_table;
	if(s == NULL) {return;}
	char *tmp = (char*)malloc(sizeof(char) * 100);
	memset(tmp,0,100);
	for(;s!=NULL;s=s->hh.next) {
		sprintf(tmp, "alias %s='%s'\n",s->aliasName,s->aliasValue);
		write(1,tmp,strlen(tmp));
	}
	free(tmp);
}
