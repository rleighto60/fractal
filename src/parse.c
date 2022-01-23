/*

program     -   parse.c
author      -   Russell Leighton
date        -   4 Mar 1994
revision    -   9 Jan 1995 (completely rewritten)

input       -   input - input string to parse

output      -   args - array of string pointers to hold tokens

description -   Routine to parse input command lines

*/

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int parse(char *input, char *args[], int narg)
{
    int i,j=0,flag=0,n;
    char *arg;

    arg = input;

    for(i=0;i<narg;i++) {
        while(*arg == ' ') ++arg;
        if(flag != 2) {
            switch(*arg) {
                case '\"':
                    args[i] = ++arg;
                    arg += strcspn(arg,(char *)"\"");
                    if(*arg != 0) *arg++ = 0;
                    break;
                case '\'':
                    args[i] = ++arg;
                    arg += strcspn(arg,(char *)"\'");
                    if(*arg != 0) *arg++ = 0;
                    break;
                default: 
                    args[i] = arg; 
                    break;
            }
        }
        else args[i] = arg;
        switch(flag) {
            case 0: arg += strcspn(arg,(char *)" =(");
                    break;
            case 1: arg += strlen(arg);
                    flag = 0;
                    break;
            case 2: arg += strcspn(arg,(char *)",");
                    if(*arg == 0) {
                        while(*(--arg) == ' ');
                        if(*arg == ')') {
                            *arg = 0;
                            flag = 0;
                        }
                    }
                    else *arg++ = 0;
                    break;
        }
        if(*arg == 0) break;
        while(*arg == ' ') *arg++ = 0;
        if(!flag) {
            switch(*arg) {
                case '=': flag = 1; *arg++ = 0; break;
                case '(': flag = 2; *arg++ = 0; break;
            }
        }
    }
    if(flag) return(0);

    if(i < narg) i++;
    arg = args[0];
    while(*arg != 0) {
        *arg = (char)tolower((int)(*arg));
        arg++;
    }
    for(n=0;n<i;n++) if((args[n] != NULL) && (args[n][0] != 0)) j = n;
    return(++j);
}
