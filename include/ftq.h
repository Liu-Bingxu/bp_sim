#ifndef __FTQ_H__
#define __FTQ_H__

#include "stdint.h"
#include "ftb.h"
#include "stdlib.h"
#include "plru.h"
#include "decode.h"
#include "test_base.h"

#define PLRU_CNT 128

class ftq_class{
    uint32_t    max_size;
    ftq_entry  *entry;
    uint32_t    bpu_w_ptr;
    uint32_t    ifu_r_ptr;
    uint32_t    commit_ptr;
    plru_class *plru;
    test_base_class &test_top;
public:
    ftq_class(uint32_t count, test_base_class &test_top_i);
    ~ftq_class();
    bool ifu_empty();
    bool empty();
    bool full();
    void enqueue(ftq_entry *enqueue_entry);
    void dequeue();
    void ifu_use_mark();
    ftq_entry *ifu_queue_get_top();
    ftq_entry *commit_queue_get_top();
    void set_plru_ptr(plru_class *plru_i);
    void precheck_restore(ftq_entry *entry_i, uint64_t pc, uint64_t push_pc, uint64_t *pop_pc, bool is_call, bool is_ret);
    void commit_restore(ftq_entry *entry_i, uint64_t pc, uint64_t push_pc, uint64_t *pop_pc, bool is_call, bool is_ret);
    void precheck_update_ras(uint64_t push_pc, bool is_call);
    void commit_update_ras(uint64_t push_pc, bool is_call);
};

#endif
