/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file defines the function prototypes
 *	    for the functions in redir.c
 *
 * Date: 04/15/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/

#ifndef REDIR_H
#define REDIR_H

/* Function prototypes
 *
 * individual functions are better described
 * inside the function code in redir.c
 */
void check_redir(void);
int check_pipe(void);
void pipe_execute(char*,int);
#endif
