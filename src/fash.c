/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This program attempts to create
 *		a very basic shell similar to the UNIX/Linux
 *		BASH. 
 *
 * Date: 03/18/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "uthash.h"
#include "history.h"
#include "alias.h"
#include "tabcomplete.h"
#include "cinterp.h"
#include "redir.h"

/* declared externs */
extern int errno; //error number from errno.h
extern int historyMax; //this is the default maximum # of history items
extern int navHistoryMax; //default max for nav history
extern char* tabCompHold[100];
extern void clear_tab_hold(void);
extern char* tab_complete(char*);

//global variables
char* line; //the line being built in main
char *args[100]; //command arguments

//local global variables
static char *PROMPT = "$ "; //shell prompt
static char *path; //current working path (truncated to top 2 directs)
static char *fullPath;
static struct termios old_tty_attr; //a copy of the old termios struct
static int tabPressNo = 0; //# of times a user has pressed tab
static int curPos; //current position of the "cursor"  (used for backspace issues)
static int handleSig; //used to determine if interupt handler was called during an execute

/********
 * begin function prototypes
 ********/
static void tty_raw_mode(void); 
typedef void (*sighandler_t)(int); //prototype from linux man page "signal"
static void quit_raw_mode(void);
static void handle_sig(int);
static void get_path(void);
static void clear_line(int);
static void backspace(void);
static void read_config(void);
static int check_separ(char*);
static void process_multiple(void);

/*these are referenced outside this code, so non-static */
void parse_args(char*);
void execute(char*);
void clear_args(void);
int check_builtin(char*);

/********
 * end function prototypes
 ********/


void read_config(void)
{
	/* Arguments: none
	
		Purpose: looks for a file called ".fashrc"
			 in the current directory and reads it
					  
			 in the file can be aliases and environment
			 variables that can be used to set specific
			 variables inside this bash
					  
		Environment variables this will currently recognize:
			COM_HIST = sets the historyMax
			PROMPT = sets the PROMP variable
			

	*/
	FILE *fp;
	char *tok;
	char *tok2;
	char *eq = "=";
	char *line = (char*)malloc(sizeof(char)*128);
	memset(line,0,128);
	char *lineCpy = (char*)malloc(sizeof(char)*128);
	memset(lineCpy,0,128);
	fp = fopen(".fashrc","r");
	if(fp != NULL) {
		while(fgets(line, 128, fp) != NULL) {
			strcpy(lineCpy, line); //copy the line, because strtok fucks it up
			tok = strtok(line,eq);
			tok2 = strstr(lineCpy,"alias");
			if(tok2 != NULL) {tok2[5] = '\0'; }
			if(tok != NULL && tok2 == NULL) {
				//there was an equal sign in this line
				//tok SHOULD be an environ var or alias
				if(!strcmp(tok,"COM_HIST")) {
					//trying to set com_hist					
					tok = strtok(NULL,eq); //tok now equals the number
					tok[strlen(tok)-1] = '\0'; //replace newline
					if(tok != NULL) {
						int x;
						x = atoi(tok); //convert to int
						if(x > 0) { historyMax = x; } //set var
					}
				}
				else if(!strcmp(tok,"NAV_HIST")) {
					//trying to set com_hist					
					tok = strtok(NULL,eq); //tok now equals the number
					tok[strlen(tok)-1] = '\0'; //replace newline
					if(tok != NULL) {
						int x;
						x = atoi(tok); //convert to int
						if(x > 0) { navHistoryMax = x; } //set var
					}
				}
				else if(!strcmp(tok,"PROMPT")) {
					//setting prompt
					tok = strtok(NULL,eq);
					tok[strlen(tok)-1] = '\0';
					if(tok != NULL) {
						memset(PROMPT,0,100); //clear old prompt
						strncpy(PROMPT, tok, strlen(tok)); //set new prompt
					}
				}
			}
			if(tok2 != NULL) {
				//setting an alias
				tok2 = tok2+6;
				add_alias(tok2);
			}
		}
	fclose(fp); //dont forget to close file
	}
	 //cleanup
	free(line);
	free(lineCpy);
}

void tty_raw_mode(void)
{
	/* Arguments: none
	
		Purpose: sets the terminal into raw mode
			credit to http://www.cs.purdue.edu for code
			
			check "man termios" for a lot of the prototypes
			and descriptions of structures in termios
			
			see also:
			http://unixwiz.net/techtips/termios-vmin-vtime.html
			
			c_lflag = local modes
			c_cc = control chacaters
			
			ICANON = canonical input
			ECHO = enables echo (prints input when typed)
			
			VMIN and VTIME specify when the read() call returns
			in our case VMIN = 1 and VTIME = 0
			which means that read() will only return
			when 1 character has been put in the caller's
			buffer and will return immediately (no timing involved) 

	*/
	struct termios tty_attr;
     
	tcgetattr(0,&tty_attr);
	tcgetattr(0,&old_tty_attr); //save old one
	/* Set raw mode. */
	tty_attr.c_lflag &= (~(ICANON|ECHO)); //turns off canonical mode and echo
	tty_attr.c_cc[VTIME] = 0;
	tty_attr.c_cc[VMIN] = 1; //wait for 1 character of input
     
	tcsetattr(0,TCSANOW,&tty_attr); //set these new attributes
}

void quit_raw_mode(void)
{
	/* Arguments: none
	
		Purpose: sets the terminal into canonical mode
			 similar to tty_raw_mode() but changes
			 the flags to quit raw mode instead of 
			 entering it
	*/	
	tcsetattr(0,TCSANOW,&old_tty_attr); //set attributes back to old struct
}

void handle_sig(int sigNo)
{
	/* Arguments:
			*sigNo --- signal number of the interrupt signal
		
		Purpose:
			handles the SIGINT signal specifically
	*/
	printf("\nTermination interrupted!\n");
	if(!handleSig){printf("%s:%s",path,PROMPT);} //if we were in the middle of an execute
	fflush(stdout); //flush standard ouput buffer
	curPos = strlen(path)+strlen(PROMPT)+1; //just reset this in case
}

void parse_args(char *line)
{
	/* Arguments:
			*line --- string of the command w/ args
		
		Purpose:
			this function takes a string and parses it into 
			an array of arguments stored in global args
	*/
	char *linecpy = line;
	char indArg[100]; //individual argument
	memset(indArg,0,100); //needed to clear out the crap
	int i = 0;
	int insideQuote = 0; //boolean to treat stuff in quotes as 1 arg
	
	while(*linecpy != '\0') { //loop until end of string
		if(*linecpy == '\"') { insideQuote = 1-insideQuote;}
		if(*linecpy == ' ' && !insideQuote) { //a space in line = new arg
			if(strlen(indArg) != 0) {
				args[i] = (char *)malloc(sizeof(char) * strlen(indArg) + 1);
				memset(args[i],0,strlen(indArg)+1);
				strncpy(args[i], indArg, strlen(indArg)); //copy 
				strncat(args[i], "\0", 1); //add the null terminator
				memset(indArg, 0, 100); //reset the indArg array
				++i;
			}
		}
		else {
			strncat(indArg, linecpy, 1); //build up indArg with next char
		}
		++linecpy;
	}
	if(strcmp(indArg,"")) {
		//dont add nothing onto the end
		args[i] = (char *)malloc(sizeof(char) * strlen(indArg) +1); //last arg malloc
		memset(args[i],0,strlen(indArg)+1);
		strncpy(args[i], indArg, strlen(indArg)); //add the last arg
		strncat(args[i], "\0", 1); //add null terminator
	}
}

void execute(char *cmd)
{
	/* Arguments:
			*cmd --- the program to run
		
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
		//now in child process
		check_redir(); //check for redirections
		i = execvp(args[0], args); //using execvp to avoid PATH crap		
		if(i < 0) {
			printf("-fash: %s: command not found\n", cmd); //error, we'll just say not found
			exit(1);
		}
	}
	else { handleSig = 1; wait(NULL); }
	handleSig = 0;
}

void clear_args(void)
{	
	/* Arguments:
			none
		
		Purpose:
			this function clears the global args array
			of any arguments
	*/
	int i;
	for(i=0;args[i]!=NULL;++i) {
		memset(args[i],0,strlen(args[i])+1); //clear arg string
		args[i] = NULL; //nullify pointer
		free(args[i]); //free array block
	}
}

void get_path(void)
{
	/* Arguments:
			none
		
		Purpose:
			this function is probably a HELL of a lot more
			work than necessary, but i couldn't find any
			Linux functions to do this
			
			essentially, this will get the current working path
			and set our global path variable to ONLY the 
			top two directories (current directory and one up)
			so that the path doesnt ever get too long
	*/
	long size;
	char *pathbuf;
	size = pathconf(".", _PC_PATH_MAX);
	pathbuf = (char *)malloc((size_t)size); //buffer for working path
	strcpy(path,getcwd(pathbuf,(size_t)size)); //getcwd gets current working path
	strcpy(fullPath,path); //save full path
	char *tmpPath = (char*)malloc(sizeof(char)*100);
	memset(tmpPath,0,100);
	strcpy(tmpPath,path); //going to loop through path
	char *pTmpPath = tmpPath; //keep track of beginning
	while(*tmpPath != '\0') { ++tmpPath; } //gets to end of string
	--tmpPath; //step back 1
	int i=0;
	
	//this while loop gets the top 2 directories
	int slashNo = 0;
	while(*tmpPath != '/' && *tmpPath != '\0') {
		--tmpPath;
		++i;
		if(*tmpPath == '/' && ++slashNo == 1) {
			++i;
			--tmpPath;
		}
	}

	memset(path,0,100);
	strcpy(path,tmpPath); //set path to new 2 directory long path
	free(pathbuf);
	free(pTmpPath);
}

int check_builtin(char *cmd)
{
	/* Arguments:
			cmd = program to run
			
		Purpose:
			this function checks to see if the 
			cmd specified is a built in command
			(in this case only "cd" is built-in)
		
		Returns:
			0  -- if the cmd is built in
			-1 -- if the cmd is NOT built in
	*/

	if(!strcmp(cmd,"cd")) {
		//command was "cd" which is a builtin, so
		//lets handle it
		add_nav_history(fullPath);
		chdir(args[1]); //change directory to first argument
		get_path();
	}
	else if(cmd[0] == '!') {
		//theyre requesting some sort of history
		exec_history(cmd); //execute the argument number 
	}
	else if(cmd[0] == '@') {
		//requesting nav history
		exec_nav_history(cmd);
		get_path();
	}
	else if(!strcmp(cmd,"history")) {
		print_history();
	}
	else if(!strcmp(cmd, "navhist")) {
		print_nav_history();
	}
	else if(!strcmp(cmd,"alias")) {
		//trying to set an alias
		if(args[1] == NULL) {
			//no argument, so just print all
			print_aliases();
		} else {
			add_alias(args[1]);
		}
	}
	else if(!strcmp(cmd,"unalias")) {
		//trying to remove an alias (args[1] is what is being removed)
		remove_alias(args[1]);
	}
	else if(!strcmp(cmd,"cinterp")) {
		//want to use the C interpreter
		//quit raw mode first (will make the cinterp easier)
		quit_raw_mode();
		cinterp();
		tty_raw_mode(); //reenter raw mode
	}
	else { return -1; }
	return 0;
}

int check_separ(char *line)
{
	/* Arguments:
			none
			
		Purpose:
			this function checks to see if there are
			semicolons which indicates a sequence of
			commands. will call the process_multiple()
			function if it finds a semicolon
		
		Returns:
			0  -- no semicolons
			1 -- found semicolon, dealt with sequence
	*/	
	int x;
	for(x=0;x<strlen(line);++x) {
		if(line[x] == ';') {
			process_multiple();
			return 1;
		}
	}
	return 0;
}

void process_multiple()
{
	/* Arguments:
			none
			
		Purpose:
			this function will split a sequence of commands
			by semicolon and execute them in order
			
	*/	
	char* list_cmds[20]; //we assume no more than 20 commands
	char *tmp_cmd = (char *)malloc(sizeof(char)*100);
	memset(tmp_cmd,0,100);
	int cmd_num = 0;
	
	int x,y=0;
	for(x=0;x<strlen(line);++x){
		if(line[x] != ';'){
			tmp_cmd[y] = line[x];
			++y;
		}
		else {
			tmp_cmd[y] = '\0';
			list_cmds[cmd_num] = (char *)calloc(100,sizeof(char)*strlen(tmp_cmd));
			//memset(list_cmds[cmd_num],0,strlen(tmp_cmd)+1);
			strcpy(list_cmds[cmd_num],tmp_cmd);
			memset(tmp_cmd, 0, 100);
			++cmd_num;
			y = 0;
		}
	}
	tmp_cmd[y+1] = '\0';
	list_cmds[cmd_num] = (char *)malloc(sizeof(char)*strlen(tmp_cmd));
	strcpy(list_cmds[cmd_num],tmp_cmd);
	memset(tmp_cmd,0,100);
	for(x=0;x<=cmd_num;++x){
		if(!check_pipe(list_cmds[x])){
			parse_args(list_cmds[x]);
			strcpy(tmp_cmd,args[0]);
			execute(tmp_cmd);
			clear_args();
			free(list_cmds[x]);
		}
	}	
	free(tmp_cmd);
}

void clear_line(int len)
{
	/* Arguments:
			len = # of characters to backspace
		
		Purpose:
			deletes from stdin number of positions
			back as per len argument
	*/
	char c;
	int x = len;
	while(x){
		c = 8;
		write(1,&c,1);
		--x;
	}
	x = len;
	while(x) {
		c = ' ';
		write(1,&c,1);
		--x;
	}
	x = len;
	while(x) {
		c = 8;
		write(1,&c,1);
		--x;
	}
}

void backspace(void)
{
	/* Arguments:
			none
		
		Purpose:
			deletes one character back
			(essentially clear_line(1))
			but (barely) faster
	*/
	char c;
	c = 8;
	write(1,&c,1);
	c = ' ';
	write(1,&c,1);
	c = 8;
	write(1,&c,1);
}

int main(int argc, char *argv[], char *envp[])
{
	read_config(); //read the config file
	char c; //reading one letter at time here
	
	//building a command line which will eventually get parsed
	line = (char *)malloc(sizeof(char) * 100); 
	memset(line,0,100);
	char *cmd = (char *)malloc(sizeof(char) * 100); //the program (command w/o args)
	char *printBuff = (char *)malloc(sizeof(char)*100); //a printing buffer (for use with raw tty)
	
	//holder for history if stuff is typed, then history is used to go back to typed stuff
	char *historyHold = (char *)malloc(sizeof(char)*100); 
	path = (char*)malloc(sizeof(char)*100);
	fullPath = (char*)malloc(sizeof(char)*100);
	memset(printBuff,0,100);
	memset(historyHold,0,100);
	memset(cmd,0,100);
	signal(SIGINT, handle_sig); //register interrupt signal (for CTRL+C)
	int promptLen; //making sure we dont backspace the prompt
	int fromHistory = 0; //a type of check to see if our line is from history (not user typed)
	
	if(fork() == 0) {
		execve("/usr/bin/clear", argv, envp); //simply clear the screen
		exit(1);
	}
	else {
		wait(NULL); //wait to clear screen
	}
	get_path(); //gets the 2dir path for prompt
	tty_raw_mode();//set terminal into raw mode	
	sprintf(printBuff,"%s:%s",path,PROMPT); //build print buff
	promptLen = strlen(printBuff);
	curPos = promptLen; //leave a space
	write(1,printBuff,promptLen); //print initial prompt
	memset(printBuff,0,100); //clear printBuff
	clear_args(); //just get any initial crap out
	
	/* MAIN LOOP */
	while(1) {
		read(0,&c,1); //read 1 character from stdin
		if(((c >= 32) && c!=127) || c == 10) { 
			//here, we only want to process characters that are
			//"readable" (or enter). special characters will be
			//handled differently
			tabPressNo = 0; //they didnt press tab
			write(1,&c,1); //write char (echo is off for raw mode)
			++curPos;
			switch(c) {
				case '\n': //end of the line (enter was pressed after input)
					if(line[0] == '\0') { 
						//they didnt type anything
						sprintf(printBuff,"%s:%s",path,PROMPT);
						write(1,printBuff,promptLen); 
					} 
					else if(strcmp(line,"exit")==0) {
						printf("\n"); //for niceness
						quit_raw_mode(); //play nice and restore term state
						return 0; //quit if they type "exit"
					}
					else { //prepare to actually process						
						strncat(line,"\0",1);
						if(line[0] != '!') {
							add_history(line); //add command to history
						}	
						int pipe = 0;
						int separ = check_separ(line);
						if(!separ){
							pipe = check_pipe(line);					
						}
						if(!separ && !pipe){ //try to execute the command if there werent pipes or semicolons
							parse_args(line); //build array of arguments
							strcpy(cmd, args[0]); //cmd = program to run
							execute(cmd);
							clear_args(); //resets all arg array strings
						}
						c = '\0';
						memset(cmd, 0, 100); //clear the cmd array
						//reprint prompt
						sprintf(printBuff,"%s:%s",path,PROMPT);
						promptLen = strlen(printBuff);
						curPos = promptLen;
						write(1,printBuff,promptLen);						
					}
					memset(line,0,100); //clear line array
					memset(historyHold,0,100);//clear history hold
					break;
				default: strncat(line, &c, 1);//build the line
					break;
			}
		}
		else if(c == 8) {
			//backspace pressed
			if(curPos > promptLen) {
				backspace(); //backspace until we reach prompt
				line[strlen(line)-1] = 0; //thank god this finally works
				--curPos;
			}
		}
		else if(c == 27) {
			//the user pressed some sort of
			//escape sequence
			char c1;
			char c2;
			read(0,&c1,1);
			read(0,&c2,1);
			
			//ok, we have the two parts of the 
			//escape sequence in c1 and c2
			if(c1 == 91 && c2 == 65) {
				//this is the escape for the up arrow
				//which we want to use for history 
				//browsing
				char *tmpLine;
				tmpLine = prev_history();
				if(tmpLine != 0) {
					if(line[0] != '\0' && fromHistory==0) {
						//store what user currently has typed (if anything)
						memset(historyHold,0,100);
						strncpy(historyHold,line,strlen(line)); 
					}
					clear_line(strlen(line)); //clears whatever is at the prompt
					memset(line,0,100);
					strncpy(line,tmpLine,strlen(tmpLine)); //copy this command
					free(tmpLine); //play nice
					write(1,line,strlen(line)); //write old command
					fromHistory = 1; //current line has been replaced by history
					curPos = strlen(line) + promptLen; //so we know where are
				}
			}
			else if(c1 == 91 && c2 == 66) {
				//this is the escape for the down arrow
				//which should make us go "forward"
				//in history (if we are back in it)
				char *tmpLine;
				tmpLine = next_history(); //get the next history
				if(tmpLine != 0) {
					//next_history gave us a line
					clear_line(strlen(line)); //clear old line from screen
					memset(line,0,100); //clear old line in mem
					strncpy(line,tmpLine,strlen(tmpLine)); //copy new line to old line
					write(1,line,strlen(line)); //write new line to screen
					curPos = strlen(line) + promptLen; //update pos
					free(tmpLine);
				}
				else if(historyHold[0] != '\0') {
					//if we dont have a next_line, lets see if
					//we had some buffer before browsing history
					clear_line(strlen(line));
					memset(line,0,100);					
					strncpy(line,historyHold,strlen(historyHold));
					write(1,line,strlen(line));
					curPos = strlen(line) +promptLen;
					fromHistory = 0; //back to user typed
				}
				else {
					//it was blank before history was browsed
					clear_line(strlen(line));
					memset(line,0,100);
					curPos = promptLen;
				}
			}
		}
		else if(c == '\t') {
			//tab press. should i dare try to do tab
			//completion? i guess...
			
			//if this is the 2nd time in a row pressing tab
			//they want a listing of everything that can be
			//completed
			if(tabPressNo) {
				//print everything in tabHold
				tabPressNo = 0;
				if(tabCompHold[0] != NULL) {
					int i = 1;
					char *x = tabCompHold[0];
					char *tmp = (char*)malloc(sizeof(char)*100);
					memset(tmp,0,100);
					write(1,"\n",1);
					while(x != NULL) {
						sprintf(tmp,"%s\t",x);
						write(1,tmp,strlen(tmp));
						memset(tmp,0,100);
						x = tabCompHold[i];
						++i;
					}
					write(1,"\n",1);
					//reprint prompt
					sprintf(printBuff,"%s:%s",path,PROMPT);
					promptLen = strlen(printBuff);
					curPos = promptLen + strlen(line);
					write(1,printBuff,promptLen);
					//write the line again
					write(1,line,strlen(line));
					clear_tab_hold();
				}
			} else {
				//otherwise just let tab_complete
				//print a single completion
				char *tabcomp;
				tabcomp = tab_complete(line);
				if(tabcomp != NULL) {
					//tab comp found a single thing, so
					//lets just complete it
					int i = 1;
					char c = tabcomp[0];
					while(c!='\0') {
						write(1,&c,1);
						strncat(line,&c,1);
						c = tabcomp[i];
						++i;
					}
					curPos += strlen(tabcomp); //set our new position
					free(tabcomp);
				}
				++tabPressNo;
			}
		}
		else if(c == '\177') {
			//other form of backspace
			if(curPos > promptLen) {
			backspace(); //backspace until we reach prompt
			line[strlen(line)-1] = 0; //thank god this finally works
			--curPos;
			}			
		}
		memset(printBuff,0,100); //clear printing buffer
	}
	printf("\n"); //for niceness
	quit_raw_mode(); //so we dont get stuck in it
	return 0; //goodbye
}
