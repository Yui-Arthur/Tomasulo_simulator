#ifndef _DECODE_H_
#define _DECODE_H_

#include <stdio.h>
#include <stdlib.h>
#include "Tomasulo.h"

int read_instruction_file(char *filename, instruction **ins);
instruction decode_calculate_code();
instruction decode_load_store_code();
int instruction_name_to_id(char *name);

int read_instruction_file(char *filename, instruction **ins){
    FILE *fptr = fopen(filename, "r");
    char buf[1024], ins_name[10];
    char l_s[2][10] = {"L.D", "S.D"};
    int idx = 0, lines = 0;

    while(fgets(buf, 1024, fptr))
        lines++;
    rewind(fptr);

    *ins = (instruction *) malloc(lines * sizeof(instruction));

    while(fgets(buf, 1024, fptr)){
        strcpy(ins_name, strtok(buf, " ")) ;
        
        if(strcmp(ins_name, l_s[0]) == 0 || strcmp(ins_name, l_s[1]) == 0)
            (*ins)[idx] = decode_load_store_code();
        else
            (*ins)[idx] = decode_calculate_code();
        
        (*ins)[idx].operation = instruction_name_to_id(ins_name);
        (*ins)[idx].issue_cycle = 0;
        (*ins)[idx].start_execution_cycle = 0;
        (*ins)[idx].end_execution_cycle = 0;
        (*ins)[idx].write_back_cycle = 0;
        (*ins)[idx].execution_unit = -1;
        
        // printf("op %d, rs %d, rt %d, rd %d, offset %d\n", (*ins)[idx].operation, (*ins)[idx].rs, (*ins)[idx].rt, (*ins)[idx].rd, (*ins)[idx].offset);
        idx++;
    }
        
    fclose(fptr);

    return lines;
}

instruction decode_load_store_code(){
    instruction ret;
    char *tmp;
    /* rt */
    tmp = strtok(NULL, ",");
    tmp[0] = ' ';
    ret.rt = atoi(tmp) / 2;

    /* offset */
    tmp = strtok(NULL, "(");
    /* offset is bytes address */
    ret.offset = atoi(tmp) / 8;

    /* rs */
    tmp = strtok(NULL, ")");
    tmp[0] = ' ';
    ret.rs = atoi(tmp);

    return ret;
}

instruction decode_calculate_code(){
    instruction ret;
    char *tmp = NULL; 
    /* rd */
    tmp = strtok(NULL, ",");
    tmp[0] = ' ';
    ret.rd = atoi(tmp) / 2;

    /* rs */
    tmp = strtok(NULL, ",");
    tmp[1] = ' ';
    ret.rs = atoi(tmp) / 2;

    /* rt */
    tmp = strtok(NULL, ",");
    tmp[1] = ' ';
    ret.rt = atoi(tmp) / 2;

    return ret;
}

int instruction_name_to_id(char *name){
    char ins_name[6][10] = {"ADD.D", "SUB.D", "MUL.D", "DIV.D", "L.D", "S.D"};
    if(strcmp(ins_name[0], name) == 0)
        return ADDD;
    else if(strcmp(ins_name[1], name) == 0)
        return SUBD;
    else if(strcmp(ins_name[2], name) == 0)
        return MULD;
    else if(strcmp(ins_name[3], name) == 0)
        return DIVD;
    else if(strcmp(ins_name[4], name) == 0)
        return LOAD;
    else if(strcmp(ins_name[5], name) == 0)
        return STORE;

    return -1;    
}

void get_answer_filename(char *filename, char *ans_filename){
    const char ans_prefix[] = "answer/", ans_suffix[] = "_ans.txt";
    ans_filename = strcpy(ans_filename, ans_prefix);
    char tmp[30];
    strcpy(tmp, filename);
    strtok(tmp ,"/");
    strcat(ans_filename, strtok(NULL ,"."));
    strcat(ans_filename, ans_suffix);

    // printf("%s\n", ans_filename);

}

int read_answer_file(char *filename, instruction **ins){
    FILE *fptr = fopen(filename, "r");
    char buf[1024];
    int idx = 0, lines = 0;

    /* does not have answer file */
    if(fptr == NULL)
        return -1;

    while(fgets(buf, 1024, fptr))
        lines++;
    rewind(fptr);

    *ins = (instruction *) malloc(lines * sizeof(instruction));

    while(fgets(buf, 1024, fptr)){
        
        (*ins)[idx].issue_cycle = atoi(strtok(buf, ","));
        (*ins)[idx].end_execution_cycle = atoi(strtok(NULL, ","));
        (*ins)[idx].write_back_cycle = atoi(strtok(NULL, ","));
        
        // printf("%d , %d , %d\n", (*ins)[idx].issue_cycle, (*ins)[idx].end_execution_cycle, (*ins)[idx].write_back_cycle);
        idx++;
    }
        
    fclose(fptr);

    return lines;
}

#endif