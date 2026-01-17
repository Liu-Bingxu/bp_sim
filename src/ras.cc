#include "ras.h"
#include "stdlib.h"

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
}

return_addr_stack::~return_addr_stack(){
    free(ras.entry);
    free( sq.entry);
}

bool return_addr_stack::sq_empty(){
    return ((sq.tosr == sq.bos) & (sq.tosw == sq.bos));
}

bool return_addr_stack::sq_full(){
    return (((sq.tosw + 1) % max_size_sq) == sq.bos);
}

void return_addr_stack::pred_push(uint64_t push_addr){
    if(sq_full() & (sq.entry[sq.tosr].cnt > 0)){
        sq.entry[sq.tosr].cnt++;
    }
}
uint64_t return_addr_stack::pred_pop(){
    return 0;
}
void return_addr_stack::precheck_push(uint64_t push_addr){

}
uint64_t return_addr_stack::precheck_pop(){
    return 0;
}
void return_addr_stack::_precheck_pop(){

}
void return_addr_stack::commit_push(uint64_t push_addr){

}
uint64_t return_addr_stack::commit_pop(){
    return 0;
}
void return_addr_stack::_commit_pop(){

}
void return_addr_stack::precheck_restore(){
    ras.ssp = ras.psp;
    sq.tosr = sq.ptosr;
    sq.tosw = sq.ptosw;
}
void return_addr_stack::commit_restore(){
    ras.psp = ras.nsp;
    ras.ssp = ras.nsp;
    sq.ptosr = sq.bos;
    sq.ptosw = sq.bos;
    sq.tosr = sq.bos;
    sq.tosw = sq.bos;
}
