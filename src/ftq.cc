#include "ftq.h"

ftq_class::ftq_class(uint32_t count){
    max_size = count;
    entry = (ftq_entry *)calloc(sizeof(ftq_entry) * count, 1);
    bpu_w_ptr = 0;
    ifu_r_ptr = 0;
    commit_ptr = 0;
    cnt   = 0;
    ifu_not_use_cnt = 0;
    plru = NULL;
    for(uint32_t i = 0; i < max_size; i++){
        entry[i].issue_inst = NULL;
    }
}

ftq_class::~ftq_class(){
    free(entry);
    for(uint32_t i = 0; i < max_size; i++){
        if(entry[i].issue_inst != NULL)
            free(entry[i].issue_inst);
    }
}

bool ftq_class::ifu_empty(){
    return ifu_not_use_cnt == 0;
}

bool ftq_class::empty(){
    return cnt == 0;
}

bool ftq_class::full(){
    return cnt == max_size;
}

void ftq_class::enqueue(ftq_entry *enqueue_entry){
    entry[bpu_w_ptr] = *enqueue_entry;
    bpu_w_ptr = ((bpu_w_ptr + 1) % max_size);
    cnt++;
    ifu_not_use_cnt++;
}

void ftq_class::dequeue(){
    commit_ptr = ((commit_ptr + 1) % max_size);
    cnt--;
}

void ftq_class::ifu_use_mark(){
    ifu_r_ptr = ((ifu_r_ptr + 1) % max_size);
    ifu_not_use_cnt--;
}

ftq_entry *ftq_class::ifu_queue_get_top(){
    return &entry[ifu_r_ptr];
}

ftq_entry *ftq_class::commit_queue_get_top(){
    return &entry[commit_ptr];
}

void ftq_class::set_plru_ptr(plru_class *plru_i){
    plru = plru_i;
}

void ftq_class::precheck_restore(ftq_entry *entry_i){
    bpu_w_ptr = ((ifu_r_ptr + 1) % max_size);
    cnt = ((bpu_w_ptr + max_size - commit_ptr) % max_size);
    ifu_not_use_cnt = 1;
    plru->plru_restore(entry_i->plru_status);
}

void ftq_class::commit_restore(ftq_entry *entry_i){
    bpu_w_ptr = ((commit_ptr + 1) % max_size);
    ifu_r_ptr = bpu_w_ptr;
    cnt = 1;
    ifu_not_use_cnt = 0;
    plru->plru_restore(entry_i->plru_status);
}
