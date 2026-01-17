#ifndef __FTB_H__
#define __FTB_H__

#include "stdint.h"

typedef struct {
    bool        valid;
    uint32_t    offset;
    bool        is_rvc;
    bool        carry[2];
    uint32_t    next_low;
    char        bit2_cnt;
}slot;

typedef struct {
    bool        valid;
    uint64_t    tag;
    slot        br_slot;
    slot        tail_slot;
    bool        carry;
    uint32_t    next_low;
    bool        is_branch;
    bool        is_call;
    bool        is_ret;
    bool        is_jalr;
    bool        always_tacken[2];
}ftb_entry;

#endif

