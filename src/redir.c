/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file contains all the functions
 *				 necessary for the redirection
 *				 functionality in FASH
 *
 * Date: 04/15/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "redir.h"

extern char *line; //the line command (from fash.c)
extern char *args[100]; //array of arguments

void check_redir(void)
{
	/* Arguments:
			none
		
		Purpose:
			analyzes args for possible redirection and handles
			any if found.
			
			this will be called after a fork()	but before an execvp() 
			so that fash's file descriptors are unchanged
	*/
	if(strstr(line,"<") || strstr(line,">")) { //do work if there exists a < or >
		int i =0;
		int fd;
		char *arg = (char*)malloc(sizeof(char)*100);
		char *fName = (char*)malloc(sizeof(char) * 100);
		memset(fName,0,100);
		memset(arg,0,100);
		//char *pArg;
		arg = args[i];
		//pArg = arg;
		while(arg != NULL) {
			//lets check  all the cases
			if(arg[0] == '>') {
				//want to redirect stdout
				if(arg[1] == '\0') {
					// next argument is the file name
					strcpy(args[i], ""); //we dont want this in the arg list
					strcpy(fName, args[++i]);
					strcpy(args[i], ""); //user will have to play nice and not do 2 things w/o space		
					fd = open(fName, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
				} else if((arg[1] >= 65 && arg[1] <= 90) || (arg[1] >= 97 && arg[1] <= 122)) {
					// next part of this argument is the file name
					strcpy(fName, arg+1);
					strcpy(args[i], ""); //remove from args list		
					fd = open(fName, O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
				} else if(arg[1] == '>') {
					//want to redirect and append to stdout
					if(arg[2] == '\0') {
						//next argument is file name
						strcpy(args[i], ""); //we dont want this in the arg list
						strcpy(fName, args[++i]);
						strcpy(args[i], ""); //user will have to play nice and not do 2 things w/o space		
						fd = open(fName, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
					} else if((arg[2] >= 65 && arg[2] <= 90) || (arg[2] >= 97 && arg[2]<= 122)) {
						//rest of this argument is the filename
						strcpy(fName, arg+2);
						strcpy(args[i], ""); //remove from args list		
						fd = open(fName, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);
					}
				}				
				//now do redirection if open was successful
				if(fd < 0) {
					//error opening file
					char *tmp = (char*)malloc(sizeof(char)*100);
					memset(tmp,0,100);
					sprintf(tmp,"fash: %s: No such file or directory\n", fName);
					write(1,tmp,strlen(tmp));
					free(tmp);
					return;
				}
				dup2(fd, 1);
				close(fd);
			} else if(arg[0] == '<') {
				//want to redirect stdin
				if(arg[1] == '\0') {
					//next argument is file name
					strcpy(args[i], ""); //we dont want this in the arg list
					strcpy(fName, args[++i]);
					strcpy(args[i], ""); //user will have to play nice and not do 2 things w/o space		
					fd = open(fName, O_RDONLY);
				} else if((arg[1] >= 65 && arg[1] <= 90) || (arg[1] >= 97 && arg[1] <= 122)) {
					//rest of this argument is file name
					strcpy(fName, arg+1);
					strcpy(args[i], ""); //remove from args list		
					fd = open(fName, O_RDONLY);
				}
				if(fd < 0) {
					//error opening file
					char *tmp = (char*)malloc(sizeof(char)*100);
					memset(tmp,0,100);
					sprintf(tmp,"fash: %s: No such file or directory\n", fName);
					write(1,tmp,strlen(tmp));
					free(tmp);
					return;
				}
				//now do redirection
				close(0);
				dup(fd);
				close(fd);
			}		
			++i;
			arg = args[i];
		}
		
		//since we've removed some "arguments" we need
		//to rebuild the new command line and reparse
		char *s;
		s = args[0];
		i = 1;
		memset(line,0,100);
		while(s!=NULL) {
			strcat(line,s);
			strcat(line, " ");
			s = args[i];
			++i;
		}
		clear_args();
		parse_args(line);
		
		//cleanup
		free(arg);
		free(fName);
	}
}

int check_pipe(char *line)
{
	/* Arguments:
			none
		
		Purpose:
			first looks for '|' character in the command line
			
			if it doesnt find one, then the function returns,
			if it does find a pipe, it will split the string by
			pipes and run each command in order, doing the
			correct piping to/from different commands
			
		Returns:
			0 - if the line contains a | character
			1 - if this function has handled the command
	*/
	if(!strchr(line,(int)'|')){ return 0;}
	
	char *pipes[100]; //= (char*)malloc(sizeof(char)*100);
	int numPipes = 0;
	
	//split by pipes
	int i;
	int x = strlen(line);
	char *tmp = (char*)malloc(sizeof(char)*100);
	memset(tmp,0,100);
	int pos = 0;
	for(i = 0; i<x; ++i) {
		if(line[i] == '|') {
			//add the subline to our pipes array
			strncat(tmp,"\0",1);
			pipes[numPipes] = (char *)malloc(sizeof(char)*100);
			memset(pipes[numPipes],0,100);
			strcpy(pipes[numPipes],tmp);
			memset(tmp,0,100);
			++numPipes;
			pos = 0;
		} else {
			//build the line
			tmp[pos] = line[i];
			++pos;
		}
	}
	//add the last substring
	strncat(tmp,"\0",1);
	pipes[numPipes] = (char *)malloc(sizeof(char)*100);
	memset(pipes[numPipes],0,100);
	strcpy(pipes[numPipes],tmp);

	//loop through each command
	//quit_raw_mode();
	for(i = 0; i<=numPipes;++i) {
		parse_args(pipes[i]); //parse the subcom
		if(i == 0){
			//first pipe command
			pipe_execute(args[0], 1);
		}
		else if(i > 0 && i != numPipes) {
			//in a middle pipe command
			pipe_execute(args[0], 2);
		}
		else if(i == numPipes) {
			//end pipe command
			pipe_execute(args[0], 0);
		}
		clear_args(); //clear subcom args
	}
		system("rm -f pipe"); //destroy the pipe
		system("rm -f pipe_middle");
	//tty_raw_mode();
	return 1;
}

void pipe_execute(char *cmd, int readWrite)
{
		/* Arguments:
			*cmd --- the program to run
			*fds   --- array of two file descriptors for pipe
			readWrite --- 0 = read, 1 = write, 2 = both
		
		Purpose:
			this function takes a string and forks off
			a new process with the global args array
			
			this will first check to see if aliases exist for 
			the command. check_alias will deal with 
			changing arg[0] if an alias exists, so always
			call execvp with arg[0] (instead of cmd arg) in
			case check_alias changes the cmd
			
			it will then check to see if the cmd is a built in
			command. if it is, then this doesnt fork any process
	*/
	int i;
	check_alias(cmd);
	i = check_builtin(args[0]);
	if(!i) { return;} //cmd was a built in (like "cd")
	if(fork() == 0) {
		//in child
		check_redir();
		int newfd, readfd, writefd;
		if(readWrite == 2){ //this is a middle command, no closing, dup both stdin and stdout			
			readfd = open("pipe", O_RDONLY);	
			writefd = open("pipe_middle", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);	
			close(0);
			dup(readfd);
			close(1);
			dup(writefd);		
		}else if (readWrite == 1){ //first command, purely writing	
			newfd = open("pipe", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH);	
			dup2(newfd,1);
			close(newfd);
		}else{ //last command, purely reading
			newfd = open("pipe", O_RDONLY);			
			close(0);		
			dup(newfd);
			close(newfd);
		}
		i = execvp(args[0], args);	
			
		if(i < 0) {
			printf("-fash: %s: command not found\n", cmd); //error, we'll just say not found
			exit(1);
		}
		if(readWrite == 0){ close(0);close(newfd);}
		else if(readWrite ==1){close(1);close(newfd);}
		else{ close(0);close(1);close(readfd);close(writefd);system("mv pipe_middle pipe");}
	} else {wait(NULL);}	//wait for everyone
}
