#ifndef _TOMASULO_H_
#define _TOMASULO_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"


#define RST_INIT {.busy = 0, .Vj = 0, .Vk = 0, .Qj = -1, .Qk = -1, .operation = -1, .remain_execution_time = -1}
#define FREEZE -1


typedef struct {
    int operation;
    int rs;
    int rt;
    int rd;
    int offset;
    int execution_unit;
    int issue_cycle;
    int start_execution_cycle;
    int end_execution_cycle;
    int write_back_cycle;
} instruction;

typedef struct {
    
    int busy;      /* -1 = freeze, 0 = unused, 1 = used */
    int operation;
    double Vj;      
    double Vk;
    int Qj;
    int Qk;
    int address;
    int remain_execution_time;
}  reservation_station;


typedef struct {
    int int_register[int_register_cnt];
    int register_status[double_register_cnt];
    double double_register[double_register_cnt];
    double double_memory[double_memory_cnt];
    reservation_station rsv[adder_cnt + mul_cnt + load_cnt + store_cnt];
    int issued_instruction;
    int cycles;
} Tomasulo_arch;

void init(Tomasulo_arch *T);
bool next_cycle(Tomasulo_arch *T, instruction *ins);
bool write_back(Tomasulo_arch *T, instruction *ins);
double calculate_results(Tomasulo_arch *T, int unit_id);
void find_require_write_back_unit(Tomasulo_arch *T, int unit_id, double value, instruction *ins);
void count_down_executon_cycle(Tomasulo_arch *T);
void check_execution_ready(Tomasulo_arch *T, instruction *ins);
int issue_instruction(Tomasulo_arch *T, instruction *ins);
int get_idle_unit(Tomasulo_arch *T, int oepration_type);
void load_store_instruction_issue(Tomasulo_arch *T, instruction ins, int reservation_id);
void calculate_instruction_issue(Tomasulo_arch *T, instruction ins, int reservation_id);

void init(Tomasulo_arch *T){
    for(int i=0; i < int_register_cnt; i++)
        T->int_register[i] = 0;
    for(int i=0; i < double_register_cnt; i++)
        T->double_register[i] = 1.0, T->register_status[i] = -1;
    for(int i=0; i < double_memory_cnt; i++)
        T->double_memory[i] = 1.0;
    for(int i=0; i < adder_cnt + mul_cnt + load_cnt + store_cnt; i++)
        T->rsv[i] = (reservation_station) RST_INIT;

    T->int_register[1] = 16;

    T->cycles = 0;
    T->issued_instruction = -1;
}

bool next_cycle(Tomasulo_arch *T, instruction *ins){
    /*
     * step 1. find the finished unit and write back the result
     * step 2. check any one unit can start execution
     * step 3. count down the execution cycle
     */
    T->cycles += 1;
    bool r;
    r = write_back(T, ins);
    check_execution_ready(T, ins);
    count_down_executon_cycle(T);
    return r;
}

bool write_back(Tomasulo_arch *T, instruction *ins){
    bool has_unit_done = false;
    for(int i = 0; i < total_unit; i++){
        if(T->rsv[i].busy <= 0)
            continue;

        // the unit execution is end
        if(T->rsv[i].remain_execution_time == 0){
            // store will not write back value to reg
            if(T->rsv[i].operation != STORE){
                double result = calculate_results(T, i);
                find_require_write_back_unit(T, i, result, ins);
                // init the rsv
                T->rsv[i] = (reservation_station) RST_INIT;
                // the rsv need wait next cycle until use again
                T->rsv[i].busy = FREEZE;
                has_unit_done = true;
            }
            // store mut wait Vk get value until finish 
            else if(T->rsv[i].Qk == -1){
                calculate_results(T, i);
                find_require_write_back_unit(T, i, 0, ins);
                // init the rsv
                T->rsv[i] = (reservation_station) RST_INIT;
                // the rsv need wait next cycle until use again
                T->rsv[i].busy = FREEZE;
                has_unit_done = true;
            }
            
        }
    }
    return has_unit_done;
}

double calculate_results(Tomasulo_arch *T, int unit_id){
    switch (T->rsv[unit_id].operation)
    {
        case ADDD:
            // reg[rs](vj) + reg[rt](vk)
            return T->rsv[unit_id].Vj + T->rsv[unit_id].Vk;
        case SUBD:
            // reg[rs](vj) - reg[rt](vk)
            return T->rsv[unit_id].Vj - T->rsv[unit_id].Vk;
        case MULD:
            // reg[rs](vj) * reg[rt](vk)
            return T->rsv[unit_id].Vj * T->rsv[unit_id].Vk;
        case DIVD:
            // reg[rs](vj) / reg[rt](vk)
            return T->rsv[unit_id].Vj / T->rsv[unit_id].Vk;            
        case LOAD:
            // double reg[rt] = double memory[int reg[rs] + offest]
            return T->double_memory[(int)T->rsv[unit_id].Vj / 8 + T->rsv[unit_id].address];
        case STORE:
            // double memory[int reg[rs] + offest] = double reg[rt]
            T->double_memory[(int)T->rsv[unit_id].Vj / 8 + T->rsv[unit_id].address] = T->rsv[unit_id].Vk;
            return 0;
    }

    return 0;
}

void find_require_write_back_unit(Tomasulo_arch *T, int unit_id, double value, instruction *ins){
    // search all rsv, find which unit need the write back value
    for(int i = 0; i < total_unit; i++){
        if(T->rsv[i].Qj == unit_id)
            T->rsv[i].Qj = -1, T->rsv[i].Vj = value;
        if(T->rsv[i].Qk == unit_id)
            T->rsv[i].Qk = -1, T->rsv[i].Vk = value;
    }

    // search all resgister, find which one need the write back value
    for(int i=0; i < double_register_cnt; i++){
        if(T->register_status[i] == unit_id)
            T->double_register[i] = value, T->register_status[i] = -1;
    }

    // search all ins, find which one need can write back
    for(int i=0; i <= T->issued_instruction; i++){
        if(ins[i].execution_unit == unit_id){
            ins[i].write_back_cycle = T->cycles;
            ins[i].execution_unit = -1;
            // store ins is set end exec time at issue
            if(ins[i].operation != STORE)
                ins[i].end_execution_cycle = T->cycles - 1;
        }
    }

    return;
}

void count_down_executon_cycle(Tomasulo_arch *T){
    for(int i = 0; i < total_unit; i++){
        if(T->rsv[i].busy == 1 && T->rsv[i].remain_execution_time != -1){
            // store can finish execute but not write back
            if(T->rsv[i].operation == STORE && T->rsv[i].Qk != -1) 
                continue;
            T->rsv[i].remain_execution_time--;
        }
    }
}

void check_execution_ready(Tomasulo_arch *T, instruction *ins){
    for(int i=0; i<total_unit; i++){
        if(T->rsv[i].busy <= 0)
            continue;
        // if rsv is not waiting for value (Qj/Qk == -1), start execution
        if(T->rsv[i].remain_execution_time == -1 && T->rsv[i].Qj == -1 && T->rsv[i].Qk == -1){
            T->rsv[i].remain_execution_time = instructions_execution_cycle_array[T->rsv[i].operation] + 1;
            // search all ins, find which one start execution
            for(int j=0; j <= T->issued_instruction; j++){
                if(ins[j].execution_unit == i)
                    ins[j].start_execution_cycle = T->cycles + 1;
            }
        }
    }
}

int issue_instruction(Tomasulo_arch *T, instruction *ins){

    instruction current_ins = ins[T->issued_instruction + 1];
    int reservation_id = get_idle_unit(T, current_ins.operation);
    
    // operation unit is full
    if(reservation_id == -1)
        return -1;
    
    // success issue ins
    T->issued_instruction += 1;
    // setting rsv
    T->rsv[reservation_id].busy = 1;
    T->rsv[reservation_id].operation = current_ins.operation;
    // set issue cycle / execution unit
    ins[T->issued_instruction].issue_cycle = T->cycles;
    ins[T->issued_instruction].execution_unit = reservation_id;
    
    // the store address will calculate at next cycle

    if(current_ins.operation >= LOAD)
        load_store_instruction_issue(T, current_ins, reservation_id);
    else 
        calculate_instruction_issue(T, current_ins, reservation_id);

    // store or load can execute when Qk == -1
    if(current_ins.operation == STORE){
        T->rsv[reservation_id].remain_execution_time = instructions_execution_cycle_array[current_ins.operation];
        ins[T->issued_instruction].start_execution_cycle = T->cycles;
        ins[T->issued_instruction].end_execution_cycle = T->cycles + 1;
        
    }
    // other ins need wait all value is prepared
    else if(T->rsv[reservation_id].Qk == -1 && T->rsv[reservation_id].Qj == -1){
        T->rsv[reservation_id].remain_execution_time = instructions_execution_cycle_array[current_ins.operation];
        ins[T->issued_instruction].start_execution_cycle = T->cycles;
    }

    return reservation_id;
}

int get_idle_unit(Tomasulo_arch *T, int oepration_type){
    
    int idle_unit = -1;

    for(int i= (int)unit_range_array[oepration_type][0]; i <= (int)unit_range_array[oepration_type][1]; i++){
        if(T->rsv[i].busy == 0){
            idle_unit = i;
            break;
        }
    }

    // unfreeze the rsv finish in this cycle
    for(int i=0; i < total_unit; i++){
        if(T->rsv[i].busy == FREEZE)
            T->rsv[i].busy = 0;
    }
    return idle_unit;
}

void load_store_instruction_issue(Tomasulo_arch *T, instruction ins, int reservation_id){
    /*
     * store : vj = rs(int reg), vk = rt(double reg)
     * load  : vj = rs(int reg), vk = X  , double reg[rt]
     */ 

    /* 
     * because load / store must get addres from int reg
     * and this project will not contain int calculate like ADD / SUB
     * so we will not wating for int reg result 
     */
    T->rsv[reservation_id].Vj = T->int_register[ins.rs], T->rsv[reservation_id].Vk = 0;

    // store opeation and no waiting result
    if(ins.operation == STORE && T->register_status[ins.rt] == -1)
        T->rsv[reservation_id].Vk = T->double_register[ins.rt];
    // store opeation and waiting for unit result
    else if(ins.operation == STORE)
        T->rsv[reservation_id].Qk = T->register_status[ins.rt], T->rsv[reservation_id].Vk = 0;

    T->rsv[reservation_id].address = ins.offset;

    // store will not write back value
    if(ins.operation != STORE)
        T->register_status[ins.rt] = reservation_id;
    


    return;
}

void calculate_instruction_issue(Tomasulo_arch *T, instruction ins, int reservation_id){
    /*
     * calculate : 
     *      vj = rs, vk = rt, dest reg[rd]
     */ 

    // no waiting result
    if(T->register_status[ins.rs] == -1)
        T->rsv[reservation_id].Vj = T->double_register[ins.rs];
    // waiting for unit result
    else
        T->rsv[reservation_id].Qj = T->register_status[ins.rs];

    // no waiting result
    if(T->register_status[ins.rt] == -1)
        T->rsv[reservation_id].Vk = T->double_register[ins.rt];
    // waiting for unit result
    else
        T->rsv[reservation_id].Qk = T->register_status[ins.rt];

    T->register_status[ins.rd] = reservation_id;
    return;
}

bool empty(Tomasulo_arch *T){
    for(int i = 0; i < total_unit; i++){
        if(T->rsv[i].busy == 1)
            return 0;
    }

    return 1;
}

#endif