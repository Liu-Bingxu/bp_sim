#include "plru.h"
#include "stdlib.h"
#include "string.h"

plru_class::plru_class(uint32_t entry_bit_i){
    entry_bit = entry_bit_i;
    entry_num = 1;
    for(uint32_t i = 0;i < entry_bit;i++){
        entry_num *= 2;
    }
    entry_sel = (uint8_t *)calloc(sizeof(uint8_t) * (entry_num - 1), 1);
}

plru_class::~plru_class(){
    free(entry_sel);
}

uint32_t plru_class::plru_select_one(){
    uint32_t pos = 1;
    uint32_t add = entry_num / 2;
    uint32_t ret = 0;
    for(uint32_t i = 0;i < entry_bit;i++){
        if(entry_sel[pos - 1] == 1){
            ret += add;
        }
        entry_sel[pos - 1] = (entry_sel[pos - 1] == 1) ? 0 : 1;
        pos = pos * 2 + entry_sel[pos - 1];
        add /= 2;
    }
    return ret;
}

uint32_t plru_class::get_plru_size(){
    return (entry_num - 1);
}

void *plru_class::get_plru_status(){
    return (void *)entry_sel;
}

void plru_class::plru_update(uint32_t update){
    uint32_t update_res = 0;
    uint32_t update_pos = 1;
    for(uint32_t i = 0;i < entry_bit;i++){
        uint32_t bit1 = update % 2;
        update_res = update_res * 2 + bit1;
        update /= 2;
    }
    for(uint32_t i = 0;i < entry_bit;i++){
        uint32_t bit1 = update_res % 2;
        entry_sel[update_pos - 1] = (bit1 == 0) ? 1 : 0;
        update_pos = update_pos * 2 + bit1;
        update_res /= 2;
    }
}

void plru_class::plru_restore(uint8_t *restore_data){
    memcpy(entry_sel, restore_data, (entry_num - 1));
}
