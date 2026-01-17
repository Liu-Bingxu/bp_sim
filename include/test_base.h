#ifndef __TEST_BASE_H__
#define __TEST_BASE_H__

#include "stdint.h"
#include "ftq.h"

class test_base_class{
public:
    virtual void update(ftq_entry *result)=0;
    virtual void update_pc(uint64_t pc, uint64_t *pop_pc, bool is_call, bool is_ret, bool is_precheck)=0;
    virtual void precheck_update_ras(uint64_t push_pc, bool is_call)=0;
    virtual void commit_update_ras(uint64_t push_pc, bool is_call)=0;
};

#endif
