#ifndef __PLRU_H__
#define __PLRU_H__

#include "stdint.h"

class plru_class{
private:
    uint32_t entry_bit;
    uint32_t entry_num;
    uint8_t *entry_sel;
public:
    plru_class(uint32_t entry_bit_i);
    ~plru_class();
    uint32_t plru_select_one();
    uint32_t get_plru_size();
    void *get_plru_status();
    void plru_update(uint32_t update);
    void plru_restore(uint8_t *restore_data);
};

#endif
