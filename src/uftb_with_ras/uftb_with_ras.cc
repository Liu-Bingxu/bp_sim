#include "stdio.h"
#include "cJSON.h"
#include "stdlib.h"
#include "string.h"
#include <sys/mman.h>
#include "ifu.h"
#include "exu.h"
#include "uftb_with_ras/uftb_with_ras.h"
#include "assert.h"

static uint32_t ftq_entry_num;
static uint32_t ftb_entry_num;
static uint32_t ftb_entry_num_bit;
static uint32_t predict_size;
static uint32_t predict_bit_size;
static uint32_t tag_bit_size;
static uint32_t tag_start_bit;
static uint32_t ras_max_size;
static uint32_t sq_max_size;

static uint64_t inst_cnt  = 0;
static uint64_t pred_miss = 0;

// code list
// 0: not a branch inst and not a jump list
// 1: a branch inst
// 2: a jump inst
// 3: call
// 4: jalr(noly a indirect jump)
// 5: ret

// 0x80:rvc
// 0x40:jump

uftb_with_ras_class::uftb_with_ras_class(uint32_t ftb_entry_num_i, uint32_t ftb_entry_num_bit_i, uint32_t ras_max_size_i, uint32_t sq_max_size_i, uint32_t pc_i){
    pc = pc_i;
    uftb_cnt = ftb_entry_num_i;
    uftb_entrys = (ftb_entry *)calloc(sizeof(ftb_entry) * ftb_entry_num_i, 1);
    uftb_ras = new ras_class(ras_max_size_i, sq_max_size_i);
    uftb_plru = new plru_class(ftb_entry_num_bit_i);
}

uftb_with_ras_class::~uftb_with_ras_class(){
    free(uftb_entrys);
    delete uftb_ras;
    delete uftb_plru;
}

uint32_t uftb_with_ras_class::get_way(){
    return uftb_plru->plru_select_one();
}

void uftb_with_ras_class::uftb_predict(ftq_class &ftq){
    uint64_t start_pc = pc;
    while(ftq.full() == false){
        ftq_entry entry;
        uint64_t tag = ((start_pc >> tag_start_bit) & (((uint64_t)0x1 << tag_bit_size) - (uint32_t)1));
        entry.hit = false;
        entry.token = false;
        entry.is_tail = false;
        entry.start_pc = start_pc;
        entry.end_pc = start_pc + predict_size;
        entry.next_pc = entry.end_pc;
        entry.first_pred_flag = false;
        entry.old_entry.valid = false;
        entry.old_entry.carry = false;
        entry.old_entry.br_slot.valid  = false;
        entry.old_entry.br_slot.carry[0]  = false;
        entry.old_entry.br_slot.carry[1]  = false;
        entry.old_entry.br_slot.is_rvc  = false;
        entry.old_entry.tail_slot.valid  = false;
        entry.old_entry.tail_slot.carry[0]  = false;
        entry.old_entry.tail_slot.carry[1]  = false;
        entry.old_entry.tail_slot.is_rvc  = false;
        entry.old_entry.is_branch = false;
        entry.old_entry.is_call   = false;
        entry.old_entry.is_jalr   = false;
        entry.old_entry.is_ret    = false;
        // entry.issue_inst = NULL;
        for(uint32_t i = 0; i < uftb_cnt; i++){
            if(uftb_entrys[i].valid & (uftb_entrys[i].tag == tag)){
                //? hit
                entry.hit = true;
                entry.hit_sel = i;
                entry.old_entry = uftb_entrys[i];
                if(uftb_entrys[i].always_tacken[0] | (uftb_entrys[i].br_slot.valid & (uftb_entrys[i].br_slot.bit2_cnt > 1))){
                    //? br_slot jump
                    uint64_t end_pc = (start_pc + uftb_entrys[i].br_slot.offset);
                    if(uftb_entrys[i].br_slot.is_rvc)
                        end_pc = end_pc + 2;
                    else 
                        end_pc = end_pc + 4;
                    uint64_t next_pc = (start_pc >> 13);
                    if(uftb_entrys[i].br_slot.carry[0])
                        next_pc++;
                    else if(uftb_entrys[i].br_slot.carry[1])
                        next_pc--;
                    next_pc = (((next_pc << 12) + uftb_entrys[i].br_slot.next_low) << 1);
                    entry.token = true;
                    entry.end_pc = end_pc;
                    entry.is_tail = false;
                    start_pc = next_pc;
                }else if((uftb_entrys[i].tail_slot.valid & (uftb_entrys[i].is_branch == false))){
                    //? tail_slot jump
                    uint64_t end_pc = (start_pc + uftb_entrys[i].tail_slot.offset);
                    if(uftb_entrys[i].tail_slot.is_rvc)
                        end_pc = end_pc + 2;
                    else 
                        end_pc = end_pc + 4;
                    uint64_t next_pc = (start_pc >> 21);
                    if(uftb_entrys[i].tail_slot.carry[0])
                        next_pc++;
                    else if(uftb_entrys[i].tail_slot.carry[1])
                        next_pc--;
                    next_pc = (((next_pc << 20) + uftb_entrys[i].tail_slot.next_low) << 1);
                    if(uftb_entrys[i].is_call)
                        uftb_ras->pred_push(end_pc);
                    else if(uftb_entrys[i].is_ret)
                        next_pc = uftb_ras->pred_pop(next_pc);
                    entry.token = true;
                    entry.end_pc = end_pc;
                    entry.is_tail = true;
                    start_pc = next_pc;
                }else if(uftb_entrys[i].always_tacken[1] | (uftb_entrys[i].tail_slot.valid & (uftb_entrys[i].tail_slot.bit2_cnt > 1))){
                    //? tail_slot jump
                    uint64_t end_pc = (start_pc + uftb_entrys[i].tail_slot.offset);
                    if(uftb_entrys[i].tail_slot.is_rvc)
                        end_pc = end_pc + 2;
                    else 
                        end_pc = end_pc + 4;
                    uint64_t next_pc = (start_pc >> 21);
                    if(uftb_entrys[i].tail_slot.carry[0])
                        next_pc++;
                    else if(uftb_entrys[i].tail_slot.carry[1])
                        next_pc--;
                    next_pc = (((next_pc << 20) + uftb_entrys[i].tail_slot.next_low) << 1);
                    entry.token = true;
                    entry.end_pc = end_pc;
                    entry.is_tail = true;
                    start_pc = next_pc;
                }else{
                    //? no jump
                    uint64_t end_pc = (start_pc >> predict_bit_size);
                    if(uftb_entrys[i].carry)
                        end_pc++;
                    end_pc = ((end_pc << predict_bit_size) + uftb_entrys[i].next_low);
                    entry.end_pc = end_pc;
                    start_pc = end_pc;
                }
                uftb_plru->plru_update(i);
                entry.next_pc = start_pc;
            }
        }
        if(entry.hit == false){
            start_pc = start_pc + predict_size;
        }
        ftq.enqueue(&entry);
    }
    pc = start_pc;
}

void uftb_with_ras_class::update(ftq_entry *result){
    if(result->hit){
        uftb_entrys[result->hit_sel] = result->old_entry;
    }else if(result->old_entry.valid){
        uint32_t i = 0;
        uint64_t tag = ((result->start_pc >> tag_start_bit) & (((uint64_t)0x1 << tag_bit_size) - (uint32_t)1));
        for(i = 0; i < uftb_cnt; i++){
            if(uftb_entrys[i].tag == tag)
                break;
        }
        if(i != uftb_cnt){
            uftb_entrys[i] = result->old_entry;
        }else{
            uint32_t way_sel = get_way();
            uftb_entrys[way_sel] = result->old_entry;
            uftb_plru->plru_update(way_sel);
        }
    }
}

void uftb_with_ras_class::update_pc(uint64_t pc_i, uint64_t push_pc, uint64_t *pop_pc, bool is_call, bool is_ret, bool is_precheck){
    pc = pc_i;
    uint64_t ret_pc = pc;

    if(is_precheck & is_ret){
        ret_pc = uftb_ras->precheck_pop(pc);
        pc = ret_pc;
        uftb_ras->precheck_restore();
    }else if(is_precheck & is_call){
        uftb_ras->precheck_push(push_pc);
        uftb_ras->precheck_restore();
    }else if(is_precheck){
        uftb_ras->precheck_restore();
    }else if(is_ret){
        uftb_ras->commit_pop();
        uftb_ras->commit_restore();
    }else if(is_call){
        uftb_ras->commit_push(push_pc);
        uftb_ras->commit_restore();
    }else{
        uftb_ras->commit_restore();
    }

    if(pop_pc != NULL){
        if(is_ret)
            *pop_pc = ret_pc;
        else
            *pop_pc = pc;
    }
}

void uftb_with_ras_class::precheck_update_ras(uint64_t push_pc, bool is_call){
    if(is_call)
        uftb_ras->precheck_push(push_pc);
    else
        uftb_ras->_precheck_pop();
}

void uftb_with_ras_class::commit_update_ras(uint64_t push_pc, bool is_call){
    if(is_call)
        uftb_ras->commit_push(push_pc);
    else
        uftb_ras->commit_pop();
}

// 全相连uftb性能仿真测试
int bp_sim_uftb_with_ras(FILE *bin_fp, FILE *db_fp, cJSON *conf_json){
    cJSON *temp = NULL;
    temp = cJSON_GetObjectItem(conf_json, "ftq_entry_num");
    ftq_entry_num = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "ftb_entry_num");
    ftb_entry_num = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "ftb_entry_num_bit");
    ftb_entry_num_bit = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "predict_size");
    predict_size = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "predict_bit_size");
    predict_bit_size = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "tag_bit_size");
    tag_bit_size = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "tag_start_bit");
    tag_start_bit = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "ras_max_size");
    ras_max_size = cJSON_GetNumberValue(temp);
    temp = cJSON_GetObjectItem(conf_json, "sq_max_size");
    sq_max_size = cJSON_GetNumberValue(temp);

    temp = cJSON_GetObjectItem(conf_json, "start_pc");
    char *start_pc_str = cJSON_GetStringValue(temp);
    uint64_t start_pc = strtoull(start_pc_str, NULL, 16);

    uint8_t *mmap_ptr = NULL;
    fseek(bin_fp, 0, SEEK_END);
    long bin_size = ftell(bin_fp);
    if(bin_size < 0){
        printf("bin_size error:%ld\n", bin_size);
        return 1;
    }
    fseek(bin_fp, 0, SEEK_SET);

    mmap_ptr = (uint8_t *)mmap(NULL, (size_t)bin_size, PROT_READ, MAP_SHARED, fileno(bin_fp), 0);

    uftb_with_ras_class uftb(ftb_entry_num, ftb_entry_num_bit, ras_max_size, sq_max_size, start_pc);
    ftq_class  uftb_ftq(ftq_entry_num, uftb);
    ifu_class  uftb_ifu(false, uftb_ftq, mmap_ptr, start_pc);
    exu_class  uftb_exu(uftb_ftq, uftb, db_fp, predict_bit_size, uftb_ifu);

    while(true){
        uftb.uftb_predict(uftb_ftq);
        uftb_ifu.fetch_code_check(tag_start_bit, tag_bit_size, predict_bit_size, predict_size);
        if(uftb_exu.execute(inst_cnt, pred_miss)){
            goto out;
        }
    }

out:
    printf("uftb_with_ras-test:\n");
    printf("inst_cnt is %ld-0x%lx; pred_miss is %ld-0x%lx\n", inst_cnt, inst_cnt, pred_miss, pred_miss);
    printf("miss rate is %f\n", (double)pred_miss / (double)inst_cnt);
    munmap(mmap_ptr, (size_t)bin_size);
    return 0;
}
