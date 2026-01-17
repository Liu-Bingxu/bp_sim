#ifndef __RAS_H__
#define __RAS_H__

#include "stdint.h"
typedef struct {
    uint64_t addr;
    uint8_t  cnt;
    uint8_t  pred_cnt;
    uint8_t  precheck_cnt;
}stack_entry;
typedef struct {
    uint64_t addr;
    uint8_t  pred_cnt;
    uint8_t  precheck_cnt;
    uint32_t nos;
}queue_entry;

typedef struct {
    stack_entry *entry;
    uint32_t  nsp; // 提交栈指针
    uint32_t  ssp; // 预测栈指针
    uint32_t  psp; // 预译码栈指针
    uint32_t  bos; // 栈底
}stack;
typedef struct {
    queue_entry *entry;
    uint32_t  tosr;  // 预测队读指针
    uint32_t  tosw;  // 预测队写指针
    uint32_t  ptosr; // 预译码队读指针
    uint32_t  ptosw; // 预译码队写指针
    uint32_t  bos;   // 队尾指针
}queue;
class return_addr_stack{
private:
    stack    ras;
    queue    sq;
    uint32_t max_size_ras;
    uint32_t max_size_sq;
public:
    return_addr_stack(uint32_t max_size_ras_i, uint32_t max_size_sq_i);
    ~return_addr_stack();
    bool sq_empty();
    bool sq_full();
    bool sq_precheck_empty();
    bool sq_precheck_full();
    bool stack_empty();
    bool stack_full();
    bool stack_precheck_empty();
    bool stack_precheck_full();
    bool stack_pred_empty();
    bool stack_pred_full();
    void pred_push(uint64_t push_addr);
    uint64_t pred_pop();
    void precheck_push(uint64_t push_addr);
    uint64_t precheck_pop();
    void _precheck_pop();
    void commit_push(uint64_t push_addr);
    uint64_t commit_pop();
    void _commit_pop();
    void precheck_restore();
    void commit_restore();
};

//! 溢出问题！！！！！
// tosw == tosr == bos 也许是

typedef return_addr_stack ras_class;

#endif
