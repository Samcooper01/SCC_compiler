#ifndef SCC_H
#define SCC_H

#define MAX_LINE_SIZE_CHAR      1024
#define MAX_OPERATION_ARGS      5
#define MAX_VARIABLES           1024

enum COMPILER_STATES
{
    INIT,
    PRECODE,
    CODE,
    CLEANUP
};

char * scc16_filename; //default scc input filename
char * samco_filename;//"main.samco"; //default samco output filename

int PROGRAM_MEMORY_START;
int PROGRAM_MEMORY_END;

int DATA_MEMORY_START; //By default variables start at the middle of data mem
int DATA_MEMORY_END;

int VAR_MEMORY_START;
int VAR_MEMORY_INDEX;

#endif /* SCC_H */
