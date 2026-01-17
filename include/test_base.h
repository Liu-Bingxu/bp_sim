#ifndef __TEST_BASE_H__
#define __TEST_BASE_H__

#include "stdint.h"
#include "ftq.h"

class test_base_class{
public:
    virtual void update(ftq_entry *result)=0;
    virtual void update_pc(uint64_t pc)=0;
};

#endif
