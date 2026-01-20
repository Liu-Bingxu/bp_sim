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
    void plru_update(uint32_t update);
};

#endif
