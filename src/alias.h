/*******************************************
 * Author: David Fritz
 * 
 * Purpose: This file defines the function prototypes
 *	    for the functions in alias.c
 *
 * Date: 04/12/10
 *
 * Statement: All of this is my own work. 
 *
*********************************************/

#ifndef ALIAS_H
#define ALIAS_H

/* Function prototypes
 *
 * individual functions are better described
 * inside the function code in alias.c
 */
void add_alias(char*);
void check_alias(char*);
void remove_alias(char*);
void print_aliases(void);
extern void clear_args(void);
extern void parse_args(char*);

#endif
