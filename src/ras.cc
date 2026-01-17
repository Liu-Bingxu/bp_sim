#include "ras.h"
#include "stdlib.h"
#include "assert.h"

return_addr_stack::return_addr_stack(uint32_t max_size_ras_i, uint32_t max_size_sq_i){
    max_size_ras = max_size_ras_i;
    max_size_sq  = max_size_sq_i;
    ras.bos      = 0;
    ras.nsp      = 0;
    ras.ssp      = 0;
    ras.psp      = 0;
    sq.bos       = 0;
    sq.ptosr     = 0;
    sq.ptosw     = 0;
    sq.tosr      = 0;
    sq.tosw      = 0;
    ras.entry    = (stack_entry *)malloc(sizeof(stack_entry) * max_size_ras);
    sq.entry     = (queue_entry *)malloc(sizeof(queue_entry) * max_size_sq);
    for(uint32_t i = 0; i < max_size_ras; i++){
        ras.entry[i].cnt = 0;
    }
    for(uint32_t i = 0; i < max_size_sq; i++){
        sq.entry[i].pred_cnt = 0;
    }
}

return_addr_stack::~return_addr_stack(){
    free(ras.entry);
    free( sq.entry);
}

bool return_addr_stack::sq_empty(){
    return (sq.entry[sq.tosr].pred_cnt == 0);
}

bool return_addr_stack::sq_full(){
    return (((sq.tosw + 1) % max_size_sq) == sq.bos);
}

bool return_addr_stack::sq_precheck_empty(){
    return (sq.entry[sq.ptosr].precheck_cnt == 0);
}

bool return_addr_stack::sq_precheck_full(){
    return (((sq.ptosw + 1) % max_size_sq) == sq.bos);
}

bool return_addr_stack::stack_empty(){
    return (ras.entry[ras.bos].cnt == 0);
}

bool return_addr_stack::stack_full(){
    return (((ras.nsp + 1) % max_size_ras) == ras.bos);
}

bool return_addr_stack::stack_precheck_empty(){
    return (ras.entry[ras.bos].precheck_cnt == 0);
}

bool return_addr_stack::stack_precheck_full(){
    return (((ras.psp + 1) % max_size_ras) == ras.bos);
}

bool return_addr_stack::stack_pred_empty(){
    return (ras.entry[ras.bos].pred_cnt == 0);
}

bool return_addr_stack::stack_pred_full(){
    return (((ras.ssp + 1) % max_size_ras) == ras.bos);
}

void return_addr_stack::pred_push(uint64_t push_addr){
    if((sq_empty() == false) & (sq.entry[sq.tosr].addr == push_addr) & (sq.entry[sq.tosr].pred_cnt < 255)){
        sq.entry[sq.tosr].pred_cnt++;
    }else if(sq_full()){
        assert(0);
        sq.entry[sq.bos].pred_cnt = 0;
        sq.entry[sq.tosw].addr = push_addr;
        sq.entry[sq.tosw].pred_cnt = 1;
        sq.entry[sq.tosw].nos = sq.tosr;
        sq.tosr = sq.tosw;
        sq.tosw = sq.bos;
        sq.bos  = ((sq.bos  + 1) % max_size_sq);
    }else{
        sq.entry[sq.tosw].addr = push_addr;
        sq.entry[sq.tosw].pred_cnt = 1;
        sq.entry[sq.tosw].nos = sq.tosr;
        sq.tosr = sq.tosw;
        sq.tosw = ((sq.tosw + 1) % max_size_sq);
    }
}

uint64_t return_addr_stack::pred_pop(){
    if(sq_empty() == false){
        uint64_t ret_pc = sq.entry[sq.tosr].addr;
        sq.entry[sq.tosr].pred_cnt--;
        if(sq.entry[sq.tosr].pred_cnt == 0){
            sq.tosr = sq.entry[sq.tosr].nos;
        }
        return ret_pc;
    }
    else if(stack_pred_empty() == false){
        uint64_t ret_pc = ras.entry[ras.ssp].addr;
        ras.entry[ras.ssp].pred_cnt--;
        if(ras.entry[ras.ssp].pred_cnt == 0){
            ras.ssp = ((ras.ssp + max_size_ras - 1) % max_size_ras);
        }
        return ret_pc;
    }
    return 0x80000000;
}

void return_addr_stack::precheck_push(uint64_t push_addr){
    if((sq_precheck_empty() == false) & (sq.entry[sq.ptosr].addr == push_addr) & (sq.entry[sq.ptosr].precheck_cnt < 255)){
        sq.entry[sq.ptosr].precheck_cnt++;
    }else if(sq_precheck_full()){
        assert(0);
        sq.entry[sq.bos].precheck_cnt = 0;
        sq.entry[sq.ptosw].addr = push_addr;
        sq.entry[sq.ptosw].precheck_cnt = 1;
        sq.entry[sq.ptosw].nos = sq.ptosr;
        sq.ptosr = sq.ptosw;
        sq.ptosw = sq.bos;
        sq.bos  = ((sq.bos  + 1) % max_size_sq);
    }else{
        sq.entry[sq.ptosw].addr = push_addr;
        sq.entry[sq.ptosw].precheck_cnt = 1;
        sq.entry[sq.ptosw].nos = sq.ptosr;
        sq.ptosr = sq.ptosw;
        sq.ptosw = ((sq.ptosw + 1) % max_size_sq);
    }
}

uint64_t return_addr_stack::precheck_pop(){
    if(sq_precheck_empty() == false){
        uint64_t ret_pc = sq.entry[sq.ptosr].addr;
        sq.entry[sq.ptosr].precheck_cnt--;
        if(sq.entry[sq.ptosr].precheck_cnt == 0){
            sq.ptosr = sq.entry[sq.ptosr].nos;
        }
        return ret_pc;
    }
    else if(stack_precheck_empty() == false){
        uint64_t ret_pc = ras.entry[ras.psp].addr;
        ras.entry[ras.psp].precheck_cnt--;
        if(ras.entry[ras.psp].precheck_cnt == 0){
            ras.psp = ((ras.psp + max_size_ras - 1) % max_size_ras);
        }
        return ret_pc;
    }
    return 0x80000000;
}

void return_addr_stack::_precheck_pop(){
    if(sq_precheck_empty() == false){
        sq.entry[sq.ptosr].precheck_cnt--;
        if(sq.entry[sq.ptosr].precheck_cnt == 0){
            sq.ptosr = sq.entry[sq.ptosr].nos;
        }
    }
    else if(stack_precheck_empty() == false){
        ras.entry[ras.psp].precheck_cnt--;
        if(ras.entry[ras.psp].precheck_cnt == 0){
            ras.psp = ((ras.psp + max_size_ras - 1) % max_size_ras);
        }
    }
}

void return_addr_stack::commit_push(uint64_t push_addr){
    if(sq.bos != sq.ptosw){
        sq.bos = ((sq.bos + 1) % max_size_sq);
    }
    uint32_t last_commit_ptr = ((ras.nsp + max_size_ras - 1) % max_size_ras);
    if((stack_empty() == false) & (ras.entry[last_commit_ptr].addr == push_addr) & (ras.entry[last_commit_ptr].cnt < 255)){
        ras.entry[last_commit_ptr].cnt++;
    }else if(stack_full()){
        assert(0);
        ras.entry[ras.bos].cnt = 0;
        ras.entry[ras.nsp].addr = push_addr;
        ras.entry[ras.nsp].cnt  = 1;
        ras.nsp = ras.bos;
        ras.bos = ((ras.bos + 1) % max_size_ras);
    }else{
        ras.entry[ras.nsp].addr = push_addr;
        ras.entry[ras.nsp].cnt  = 1;
        ras.nsp = ((ras.nsp + 1) % max_size_ras);
    }
}

uint64_t return_addr_stack::commit_pop(){
    uint32_t last_commit_ptr = ((ras.nsp + max_size_ras - 1) % max_size_ras);
    if(stack_empty() == false){
        uint64_t ret_pc = ras.entry[last_commit_ptr].addr;
        ras.entry[last_commit_ptr].cnt--;
        if(ras.entry[last_commit_ptr].cnt == 0){
            ras.nsp = last_commit_ptr;
        }
        return ret_pc;
    }
    return 0x80000000;
}

void return_addr_stack::_commit_pop(){
    uint32_t last_commit_ptr = ((ras.nsp + max_size_ras - 1) % max_size_ras);
    if(stack_empty() == false){
        ras.entry[last_commit_ptr].cnt--;
        if(ras.entry[last_commit_ptr].cnt == 0){
            ras.nsp = last_commit_ptr;
        }
    }
}

void return_addr_stack::precheck_restore(){
    ras.ssp = ras.psp;
    for(uint32_t i = 0; i < max_size_ras; i++){
        ras.entry[i].pred_cnt = ras.entry[i].precheck_cnt;
    }
    sq.tosr = sq.ptosr;
    sq.tosw = sq.ptosw;
    for(uint32_t i = 0; i < max_size_sq; i++){
        sq.entry[i].pred_cnt = sq.entry[i].precheck_cnt;
    }
}

void return_addr_stack::commit_restore(){
    ras.psp = ras.nsp;
    ras.ssp = ras.nsp;
    for(uint32_t i = 0; i < max_size_ras; i++){
        ras.entry[i].pred_cnt = ras.entry[i].cnt;
        ras.entry[i].precheck_cnt = ras.entry[i].cnt;
    }
    sq.ptosr = sq.bos;
    sq.ptosw = sq.bos;
    sq.tosr = sq.bos;
    sq.tosw = sq.bos;
    for(uint32_t i = 0; i < max_size_sq; i++){
        sq.entry[i].pred_cnt = 0;
        sq.entry[i].precheck_cnt = 0;
    }
}
