#ifndef SCRIPT_H__
#define SCRIPT_H__

#define SCRIPT_MAX_TEXT_BUFFER             64

#include <stdio.h>
#include "utilities.h"

enum instruction_command_t
{
    INSTRUCTION_NAME,
    INSTRUCTION_CREATE,
    INSTRUCTION_LOAD,
    INSTRUCTION_TRANSLATE,
    INSTRUCTION_ROTATE,
    INSTRUCTION_ASSIGN,
    INSTRUCTION_TAG,
    INSTRUCTION_ADD,
    INSTRUCTION_EQUALS,
    INSTRUCTION_GOTO,
    INSTRUCTION_RENDER,
    INSTRUCTION_EXIT
};

struct variable_t
{
    char *name;
    float value;  
};

struct instruction_t
{
    char type[SCRIPT_MAX_TEXT_BUFFER];
    char name[SCRIPT_MAX_TEXT_BUFFER];

    int line;

    enum instruction_command_t command;

    float x, y, z, t;
};

void script_run_file(const char *file_name);

#endif