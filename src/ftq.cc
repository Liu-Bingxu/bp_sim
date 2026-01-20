#include "ftq.h"

ftq_class::ftq_class(uint32_t count, test_base_class &test_top_i):test_top(test_top_i){
    max_size = count;
    entry = (ftq_entry *)calloc(sizeof(ftq_entry) * count, 1);
    bpu_w_ptr = 0;
    ifu_r_ptr = 0;
    commit_ptr = 0;
}

ftq_class::~ftq_class(){
    free(entry);
}

bool ftq_class::ifu_empty(){
    return ifu_r_ptr == bpu_w_ptr;
}

bool ftq_class::empty(){
    return commit_ptr == bpu_w_ptr;
}

bool ftq_class::full(){
    return ((commit_ptr != bpu_w_ptr) & ((commit_ptr % max_size) == (bpu_w_ptr % max_size)));
}

void ftq_class::enqueue(ftq_entry *enqueue_entry){
    entry[(bpu_w_ptr % max_size)] = *enqueue_entry;
    bpu_w_ptr = ((bpu_w_ptr + 1) % (max_size * 2));
}

void ftq_class::dequeue(){
    commit_ptr = ((commit_ptr + 1) % (max_size * 2));
}

void ftq_class::ifu_use_mark(){
    ifu_r_ptr = ((ifu_r_ptr + 1) % (max_size * 2));
}

ftq_entry *ftq_class::ifu_queue_get_top(){
    return &entry[(ifu_r_ptr % max_size)];
}

ftq_entry *ftq_class::commit_queue_get_top(){
    return &entry[(commit_ptr % max_size)];
}

void ftq_class::precheck_restore(uint64_t pc, uint64_t push_pc, uint64_t *pop_pc, bool is_call, bool is_ret){
    bpu_w_ptr = ((ifu_r_ptr + 1) % (max_size * 2));
    test_top.update_pc(pc, push_pc, pop_pc, is_call, is_ret, true);
}

void ftq_class::commit_restore(uint64_t pc, uint64_t push_pc, uint64_t *pop_pc, bool is_call, bool is_ret){
    bpu_w_ptr = ((commit_ptr + 1) % (max_size * 2));
    ifu_r_ptr = bpu_w_ptr;
    test_top.update_pc(pc, push_pc, pop_pc, is_call, is_ret, false);
}

void ftq_class::precheck_update_ras(uint64_t push_pc, bool is_call){
    test_top.precheck_update_ras(push_pc, is_call);
}

void ftq_class::commit_update_ras(uint64_t push_pc, bool is_call){
    test_top.commit_update_ras(push_pc, is_call);
}

