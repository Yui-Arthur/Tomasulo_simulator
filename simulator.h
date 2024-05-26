#ifndef _SIMULATER_H_
#define _SIMULATER_H_

#include "Tomasulo.h"
#include "decoed.h"

/*
 * level 0 => no show
 * level 1 => only show at something change
 * level 2 => show every cycles
 */
#define SHOW_LEVEL 0
#define MAX(i, j) (((i) > (j)) ? (i) : (j))

int start_simulation(char *filename);
bool compare_answer(Tomasulo_arch *T, instruction *ins, int ins_cnt, char *filename);
void goal_register(instruction *ins, int ins_cnt, double *double_register, double *double_memory);
void show_the_cycle_status(Tomasulo_arch *T, instruction *ins, int ins_cnt);
void get_unit_type(int unit, char *name);
void get_operation_type(int op, char *name);
void show_rsv_and_ins(Tomasulo_arch *T, instruction *ins, int ins_cnt);
void show_one_rsv(reservation_station rs, int unit_id);
void show_one_ins(instruction ins);
void show_double_registers(Tomasulo_arch *T);
void show_double_memeory(Tomasulo_arch *T);

int start_simulation(char *filename){

    Tomasulo_arch T;
    instruction *ins;
    init(&T);
    int idx = 0, ins_cnt = read_instruction_file(filename, &ins);

    do{
        bool f;
        f = next_cycle(&T, ins);
        if(idx < ins_cnt && issue_instruction(&T, ins) != -1)
            f = true, idx++;

        if(SHOW_LEVEL == 2 || (SHOW_LEVEL == 1 && f)){
            printf("\n\n==================================== Cycle %4d ====================================\n\n", T.cycles);
            show_the_cycle_status(&T, ins, ins_cnt);
            printf("\n\n===================================================================================\n\n");
        }

    }while(!empty(&T));

    printf("%15s : %d\n", filename, compare_answer(&T, ins, ins_cnt, filename));

    return 0;
}

bool compare_answer(Tomasulo_arch *T, instruction *ins, int ins_cnt, char *filename){
    double goal_double_register[double_register_cnt];
    double goal_double_memory[double_memory_cnt];
    bool correct = true;
    goal_register(ins, ins_cnt, goal_double_register, goal_double_memory);
    
    /* check register value */
    for(int i=0; i < double_register_cnt; i++){
        if(T->double_register[i] != goal_double_register[i]){
            printf("Error at register %d, expected %f, got %f\n", 2*i, goal_double_register[i], T->double_register[i]);
            correct = false;
        }
    }

    /* check memory value */
    for(int i=0; i < double_memory_cnt; i++){
        if(T->double_memory[i] != goal_double_memory[i]){
            printf("Error at memory %d, expected %f, got %f\n", 2*i, goal_double_memory[i], T->double_memory[i]);
            correct = false;
        }
    }

    if(correct)
        printf("success pass register and memory check !\n");

    /* check instruction value */
    char ans_filename[30];
    instruction *ans_ins;
    
    get_answer_filename(filename, ans_filename);
    if(read_answer_file(ans_filename, &ans_ins) == -1){
        printf("fail to read answer file with path %s !\n", ans_filename);
        return correct;
    }

    for(int i=0; i<ins_cnt; i++){
        if(ans_ins[i].issue_cycle != ins[i].issue_cycle){
            printf("Error at instruction %d issue cycle, expected %d, got %d\n", 2*i, ans_ins[i].issue_cycle, ins[i].issue_cycle);
            correct = false;
        }
        if(ans_ins[i].end_execution_cycle != ins[i].end_execution_cycle){
            printf("Error at instruction %d end execution cycle, expected %d, got %d\n", 2*i, ans_ins[i].end_execution_cycle, ins[i].end_execution_cycle);
            correct = false;
        }
        if(ans_ins[i].write_back_cycle != ins[i].write_back_cycle){
            printf("Error at instruction %d end write back cycle, expected %d, got %d\n", 2*i, ans_ins[i].write_back_cycle, ins[i].write_back_cycle);
            correct = false;
        }
    }

    if(correct)
        printf("success pass instruction check !\n");

    return correct;
}

void goal_register(instruction *ins, int ins_cnt, double *double_register, double *double_memory){
    int int_register[int_register_cnt] = {0};
    int_register[1] = 16;

    for(int i=0; i<double_register_cnt; i++) double_register[i] = 1.0;
    for(int i=0; i<double_memory_cnt; i++) double_memory[i] = 1.0;

    for(int i=0; i<ins_cnt; i++){
        switch (ins[i].operation)
        {
            case ADDD:
                double_register[ins[i].rd] = double_register[ins[i].rs] + double_register[ins[i].rt];
                break;
            case SUBD:
                double_register[ins[i].rd] = double_register[ins[i].rs] - double_register[ins[i].rt];
                break;
            case MULD:
                double_register[ins[i].rd] = double_register[ins[i].rs] * double_register[ins[i].rt];
                break;
            case DIVD:
                double_register[ins[i].rd] = double_register[ins[i].rs] / double_register[ins[i].rt]; 
                break;
            case LOAD:
                double_register[ins[i].rt] = double_memory[ins[i].offset + int_register[ins[i].rs] / 8];
                break;
            case STORE:
                double_memory[ins[i].offset + int_register[ins[i].rs] / 8] = double_register[ins[i].rt];
                break;
        }
    }
}

void show_the_cycle_status(Tomasulo_arch *T, instruction *ins, int ins_cnt){
    
    show_rsv_and_ins(T, ins, ins_cnt);
    show_double_registers(T);
    show_double_memeory(T);
    return;
}

void get_unit_type(int unit, char *name){
    const char unit_name[][10] = {"Adder ", "Mul ", "Load ", "Store ", "X"};
    int name_end_idx[] = {6, 4, 5, 6};

    int idx = 0, value = 0;

    if(adder_st <= unit && unit <= adder_ed)
        strcpy(name, unit_name[0]), idx = 0, value = adder_st;
    else if(mul_st <= unit && unit <= mul_ed)
        strcpy(name, unit_name[1]), idx = 1, value = mul_st;
    else if(load_st <= unit && unit <= load_ed)
        strcpy(name, unit_name[2]), idx = 2, value = load_st;
    else 
        strcpy(name, unit_name[3]), idx = 3, value = store_st;

    name[name_end_idx[idx]] = '0' + unit - value;
    name[name_end_idx[idx] + 1] = '\0';

    if(unit == -1)
        strcpy(name, unit_name[4]);

}

void get_operation_type(int op, char *name){
    const char operation_name[][10] = {"ADD.D", "SUB.D", "MUL.D", "DIV.D", "L.D", "S.D", "X"};
    if(op == -1)
        op = 6;
    strcpy(name, operation_name[op]); 
}

void show_rsv_and_ins(Tomasulo_arch *T, instruction *ins, int ins_cnt){
    printf("                Instruction                     ");
    printf("                             Reservation Stations\n");
    printf("--------------------------------------------    ");
    printf("---------------------------------------------------------------------------\n");
    printf("| Operate | Issue | Execution | Write Back |    ");
    printf("|   Unit   | Cycle | Operate |   Vj  |   Vk  |    Qj    |    Qk    | Addr |\n");

    for(int i=0; i < MAX(total_unit, ins_cnt) + 1; i++){

        if(i < ins_cnt)
            show_one_ins(ins[i]);
        else if(i == ins_cnt)
            printf("--------------------------------------------    ");
        else
            printf("                                                ");
        
        if(i < total_unit)
            show_one_rsv(T->rsv[i], i);
        else if(i == total_unit){
            printf("---------------------------------------------------------------------------\n");
        }

    }
}

void show_one_rsv(reservation_station rs, int unit_id){
    char unit_name[10], operation_name[10], qj_name[10], qk_name[10];
    get_unit_type(unit_id, unit_name);
    get_unit_type(rs.Qj, qj_name);
    get_unit_type(rs.Qk, qk_name);
    get_operation_type(rs.operation, operation_name);
    printf("| %8s | %5d | %7s | %3.3f | %3.3f | %8s | %8s | %4d |\n", unit_name, rs.remain_execution_time, operation_name, rs.Vj, rs.Vk, qj_name, qk_name, rs.address);
}

void show_one_ins(instruction ins){
    char operation_name[10];
    get_operation_type(ins.operation, operation_name);
    printf("| %7s | %5d | %3d - %3d | %10d |    ", operation_name, ins.issue_cycle, ins.start_execution_cycle, ins.end_execution_cycle, ins.write_back_cycle);
}

void show_double_registers(Tomasulo_arch *T){
    /* register status */
    for(int i=0; i<double_register_cnt; i++)
        printf("     ");
    printf(" Register Status \n");

    for(int i=0; i<double_register_cnt; i++)
        printf("----------");
    printf("\n");
    for(int i=0; i<double_register_cnt; i++){
        printf("| %4d    ", 2*i);
    }
    printf("|\n");
    for(int i=0; i<double_register_cnt; i++){
        char unit_name[10];
        get_unit_type(T->register_status[i], unit_name);
        printf("| %7s ", unit_name);
    }
    printf("|\n");
    for(int i=0; i<double_register_cnt; i++)
        printf("----------");
    printf("\n");

    /* register value */
    for(int i=0; i<double_register_cnt; i++)
        printf("     ");
    printf(" Register Value \n");

    for(int i=0; i<double_register_cnt; i++)
        printf("----------");
    printf("\n");
    for(int i=0; i<double_register_cnt; i++){
        printf("| %4d    ", 2*i);
    }
    printf("|\n");
    for(int i=0; i<double_register_cnt; i++){
        printf("| % 3.4f ", T->double_register[i]);
    }
    printf("|\n");
    for(int i=0; i<double_register_cnt; i++)
        printf("----------");
    printf("\n\n");
}

void show_double_memeory(Tomasulo_arch *T){
    /* register value */
    for(int i=0; i<double_register_cnt; i++)
        printf("     ");
    printf(" Memory Value \n");

    for(int i=0; i<double_register_cnt; i++)
        printf("----------");
    printf("\n");
    for(int i=0; i<double_register_cnt; i++){
        printf("| %4d    ", i);
    }
    printf("|\n");
    for(int i=0; i<double_memory_cnt; i++){
        printf("| % 3.4f ", T->double_memory[i]);
    }
    printf("|\n");
    for(int i=0; i<double_memory_cnt; i++)
        printf("----------");
    printf("\n");
}

#endif