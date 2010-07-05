/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file contains all the functions
 *	    necessary for the C program interpreter
 *	    functionality in FASH
 *
 * Date: 04/22/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "cinterp.h"

void cinterp(void)
{
	/* Arguments:
			none
		
		Purpose:
			allows the user to type in C code, terminated by
			"done" on a single line

			this will then compile and run that C code
	*/
	
	int fd;
	if((fd = open("tmp.c", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		//error opening file for writing
		printf("fash: Can not run cinterp. Bad file creation.\n");
		return;
	}
	
	//print out some messages so the user knows what to do
	printf("\n\nWRITE A C PROGRAM.\n");
	printf("Write \"done\" and press enter when you are finished\n");
	printf("or write \"quit\" if you want to exit the C interpreter\n\n");
	
	char *tmp = (char*)malloc(sizeof(char)*100);
	memset(tmp,0,100);
	
	//were going to write a few includes to the file first
	// #include <stdio.h>
	// #include <stdlib.h>
	// #inlcude <string.h>
	strcpy(tmp,"#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n");
	write(fd,tmp,strlen(tmp));
	
	//prepare to accept user input
	char line[128];
	//get a line and make sure its not "done"
	int quit=0;
	while((fgets(line, 128, stdin) != NULL) && strcmp(line,"done\n")) {
		if(!strcmp(line,"quit\n")) {quit=1;break;} //quit if user wants to
		strcpy(tmp,line);
		write(fd,tmp,strlen(tmp)); //write their line to file
	}
	printf("\n");

	//cleanup
	free(tmp);
	close(fd);

	if(!quit){
		//try to compile
		system("gcc -o tmpc tmp.c");
		FILE *fp;
		//if compile was successful, tmpc will exist, so
		//check to see if its there
		if((fp = fopen("tmpc","r"))!=NULL) {
			//ok gcc worked
			fclose(fp);
			system("./tmpc"); //execute the code
			system("rm tmpc"); //remove the executable
		}
	}
	system("rm tmp.c"); //remove temporary c file
}
