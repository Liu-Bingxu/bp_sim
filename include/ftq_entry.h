#ifndef __FTQ_ENTRY_H__
#define __FTQ_ENTRY_H__

#include "ftb.h"
#include "decode.h"

typedef struct {
    // start_pc必须全部记下
    uint64_t    start_pc;
    uint64_t    next_pc;

    // 以下纯粹为了仿真方便
    inst_decode  issue_inst[DECODE_CNT];
    uint32_t     issue_cnt;
    uint64_t     end_pc;

    bool        first_pred_flag;
    bool        hit;
    bool        token;
    bool        is_tail;
    // 以下是hit有效时才有意义的元素
    uint32_t    hit_sel;
    ftb_entry   old_entry; // precheck后一定有意义
    // plru状态，RTL中需要保存，用于冲刷（重定向）时恢复plru状态
    uint8_t     plru_status[128];
}ftq_entry;

#endif