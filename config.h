#ifndef _CONFIG_H_
#define _CONFIG_H_

#define adder_cnt   3
#define mul_cnt     2
#define load_cnt    2
#define store_cnt   2
#define total_unit adder_cnt + mul_cnt + load_cnt + store_cnt

#define int_register_cnt    32
#define double_register_cnt 16
#define double_memory_cnt   16

/*
 * mapping operation to type
 */
enum operation_type {
    ADDD = 0, SUBD = 1, MULD = 2, DIVD = 3, LOAD = 4, STORE = 5
};

/*
 * mapping operation to rsv index
 */
enum unit_range {
    adder_st = 0,
    adder_ed = adder_cnt-1,
    mul_st = adder_cnt,
    mul_ed = adder_cnt + mul_cnt -1,
    load_st = adder_cnt + mul_cnt,
    load_ed = adder_cnt + mul_cnt + load_cnt -1,
    store_st = adder_cnt + mul_cnt + load_cnt,
    store_ed = adder_cnt + mul_cnt + load_cnt + store_cnt -1,
} unit_range_array[][2]= {{adder_st, adder_ed}, {adder_st, adder_ed}, 
                          {mul_st, mul_ed}, {mul_st, mul_ed}, 
                          {load_st, load_ed}, {load_st, load_ed}, 
                          {store_st, store_ed}, {store_st, store_ed}};


/*
 * mapping operation to execution cycles
 */
enum instructions_execution_cycle{
    ADDD_C = 2, SUBD_C = 2, MULD_C = 10, DIVD_C = 40, LD_C = 2, SD_C = 1
} instructions_execution_cycle_array[] = {ADDD_C, SUBD_C, MULD_C, DIVD_C, LD_C, SD_C};



#endif