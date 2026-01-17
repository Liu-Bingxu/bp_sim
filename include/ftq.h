#ifndef __FTQ_H__
#define __FTQ_H__

#include "stdint.h"
#include "ftb.h"
#include "stdlib.h"
#include "plru.h"
#include "decode.h"

typedef struct {
    // start_pc必须全部记下
    uint64_t    start_pc;

    // 以下纯粹为了仿真方便
    inst_decode *issue_inst;
    uint32_t     issue_cnt;
    uint64_t     end_pc;
    bool        rvi_valid;

    bool        first_pred_flag;
    bool        hit;
    bool        token;
    bool        is_tail;
    // 以下是hit有效时才有意义的元素
    uint32_t    hit_sel;
    ftb_entry   old_entry; // precheck后一定有意义
    // plru状态，RTL中需要保存，用于冲刷（重定向）时恢复plru状态
    uint8_t    *plru_status;
}ftq_entry;

class ftq_class{
    uint32_t    max_size;
    ftq_entry  *entry;
    uint32_t    cnt;
    uint32_t    ifu_not_use_cnt;
    uint32_t    bpu_w_ptr;
    uint32_t    ifu_r_ptr;
    uint32_t    commit_ptr;
    plru_class *plru;
public:
    ftq_class(uint32_t count);
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
    void precheck_restore(ftq_entry *entry_i);
    void commit_restore(ftq_entry *entry_i);
};

#endif
