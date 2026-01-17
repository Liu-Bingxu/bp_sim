#ifndef __UFTB_H__
#define __UFTB_H__

#include "stdint.h"
#include "ftb.h"
#include "ras.h"
#include "plru.h"
#include "test_base.h"

class uftb_class: public test_base_class{
private:
    uint32_t    uftb_cnt;
    ftb_entry  *uftb_entrys;
    plru_class *uftb_plru;
    uint32_t get_way();
    uint32_t    pc;
public:
    uftb_class(uint32_t ftb_entry_num_i, uint32_t ftb_entry_num_bit_i, uint32_t pc_i);
    ~uftb_class();
    void uftb_predict(ftq_class &ftq);
    inline plru_class *get_uftb_plru(){
        return uftb_plru;
    };
    virtual void update(ftq_entry *result);
    virtual void update_pc(uint64_t pc_i);
};


#endif

