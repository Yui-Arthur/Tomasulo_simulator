#include <stdio.h>
#include <stdbool.h>
#include "Tomasulo.h"
#include "decoed.h"
#include "simulator.h"

int main(){
    // Tomasulo_arch T;
    // init(&T);

    // show_the_cycle_status(&T);1
    start_simulation("input/input1.txt");
    start_simulation("input/input2.txt");
    start_simulation("input/input3.txt");
    start_simulation("input/input4.txt");
    start_simulation("input/input5.txt");
    start_simulation("input/sample_input1.txt");
    start_simulation("input/sample_input2.txt");
    start_simulation("input/sample_input3.txt");
    start_simulation("input/sample_input4.txt");
    start_simulation("input/sample_input5.txt");
    start_simulation("input/sample_input6.txt");
    start_simulation("input/sample_input7.txt");
    // char tmp[15];
    // get_answer_filename("input/sample_input3.txt", tmp);
    // instruction *ins;
    printf("end\n");

    return 1;
}