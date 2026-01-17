#include "ifu.h"

bool comp_ftb_entry(ftb_entry *old_entry, ftb_entry *new_entry){
    if(old_entry->br_slot.valid != new_entry->br_slot.valid){
        return true;
    }
    if(old_entry->br_slot.valid){
        bool br_slot_offset     = old_entry->br_slot.offset != new_entry->br_slot.offset;
        bool br_slot_carry_0    = old_entry->br_slot.carry[0] != new_entry->br_slot.carry[0];
        bool br_slot_carry_1    = old_entry->br_slot.carry[1] != new_entry->br_slot.carry[1];
        bool br_slot_next_low   = old_entry->br_slot.next_low != new_entry->br_slot.next_low;
        bool br_slot_rvc        = old_entry->br_slot.is_rvc != new_entry->br_slot.is_rvc;
        if(br_slot_carry_0 | br_slot_carry_1 | br_slot_next_low | br_slot_offset | br_slot_rvc)
            return true;
    }
    if(old_entry->tail_slot.valid != new_entry->tail_slot.valid){
        return true;
    }
    if(old_entry->tail_slot.valid){
        bool tail_slot_offset     = old_entry->tail_slot.offset != new_entry->tail_slot.offset;
        // bool tail_slot_carry_0    = old_entry->tail_slot.carry[0] != new_entry->tail_slot.carry[0];
        // bool tail_slot_carry_1    = old_entry->tail_slot.carry[1] != new_entry->tail_slot.carry[1];
        // bool tail_slot_next_low   = old_entry->tail_slot.next_low != new_entry->tail_slot.next_low;
        bool tail_slot_rvc        = old_entry->tail_slot.is_rvc != new_entry->tail_slot.is_rvc;
        bool branch               = old_entry->is_branch != new_entry->is_branch;
        bool call                 = old_entry->is_call != new_entry->is_call;
        bool jalr                 = old_entry->is_jalr != new_entry->is_jalr;
        bool ret                  = old_entry->is_ret != new_entry->is_ret;
        // if(tail_slot_carry_0 | tail_slot_carry_1 | tail_slot_next_low | tail_slot_offset | tail_slot_rvc | branch | call | jalr | ret)
        if(tail_slot_offset | tail_slot_rvc | branch | call | jalr | ret)
            return true;
    }
    bool valid = old_entry->valid != new_entry->valid;
    bool carry = old_entry->carry != new_entry->carry;
    bool next_low = old_entry->next_low != new_entry->next_low;
    if(valid | carry | next_low)
        return true;
    return false;
}

void precheck(predecode_result *decode_result, ftq_entry *result, precheck_result *check_result, uint32_t tag_start_bit, uint32_t tag_bit_size, uint32_t predict_bit_size, uint32_t predict_size){
    check_result->new_entry.valid = true;
    check_result->new_entry.tag   = ((result->start_pc >> tag_start_bit) & (((uint64_t)0x1 << tag_bit_size) - (uint32_t)1));
    check_result->new_entry.carry = false;
    check_result->new_entry.br_slot.valid  = false;
    check_result->new_entry.br_slot.carry[0]  = false;
    check_result->new_entry.br_slot.carry[1]  = false;
    check_result->new_entry.br_slot.is_rvc  = false;
    check_result->new_entry.tail_slot.valid  = false;
    check_result->new_entry.tail_slot.carry[0]  = false;
    check_result->new_entry.tail_slot.carry[1]  = false;
    check_result->new_entry.tail_slot.is_rvc  = false;
    check_result->new_entry.is_branch = false;
    check_result->new_entry.is_call   = false;
    check_result->new_entry.is_jalr   = false;
    check_result->new_entry.is_ret    = false;
    uint64_t end_pc = 0;
    if(decode_result->has_three_branch){ // two branch + branch/jump
        check_result->new_entry.br_slot.valid  = true;
        check_result->new_entry.br_slot.is_rvc = decode_result->decode[decode_result->one_branch_index].is_rvc;
        check_result->new_entry.br_slot.offset = decode_result->decode[decode_result->one_branch_index].offset;
        check_result->new_entry.br_slot.carry[0] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) - 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.carry[1] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) + 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.next_low = ((decode_result->decode[decode_result->one_branch_index].branch_addr >> 1) & 0xfff);
        check_result->new_entry.tail_slot.valid  = true;
        check_result->new_entry.tail_slot.is_rvc = decode_result->decode[decode_result->two_branch_index].is_rvc;
        check_result->new_entry.tail_slot.offset = decode_result->decode[decode_result->two_branch_index].offset;
        check_result->new_entry.tail_slot.carry[0] = (((decode_result->decode[decode_result->two_branch_index].branch_addr >> 21) - 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.carry[1] = (((decode_result->decode[decode_result->two_branch_index].branch_addr >> 21) + 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.next_low = ((decode_result->decode[decode_result->two_branch_index].branch_addr >> 1) & 0xfffff);
        check_result->new_entry.is_branch = true;
        end_pc = result->start_pc + decode_result->decode[decode_result->three_branch_index].offset;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }else if(decode_result->has_two_branch){ // two branch
        check_result->new_entry.br_slot.valid  = true;
        check_result->new_entry.br_slot.is_rvc = decode_result->decode[decode_result->one_branch_index].is_rvc;
        check_result->new_entry.br_slot.offset = decode_result->decode[decode_result->one_branch_index].offset;
        check_result->new_entry.br_slot.carry[0] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) - 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.carry[1] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) + 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.next_low = ((decode_result->decode[decode_result->one_branch_index].branch_addr >> 1) & 0xfff);
        check_result->new_entry.tail_slot.valid  = true;
        check_result->new_entry.tail_slot.is_rvc = decode_result->decode[decode_result->two_branch_index].is_rvc;
        check_result->new_entry.tail_slot.offset = decode_result->decode[decode_result->two_branch_index].offset;
        check_result->new_entry.tail_slot.carry[0] = (((decode_result->decode[decode_result->two_branch_index].branch_addr >> 21) - 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.carry[1] = (((decode_result->decode[decode_result->two_branch_index].branch_addr >> 21) + 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.next_low = ((decode_result->decode[decode_result->two_branch_index].branch_addr >> 1) & 0xfffff);
        check_result->new_entry.is_branch = true;
        end_pc = result->start_pc + predict_size;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }else if((decode_result->has_jump) & (decode_result->has_one_branch == true)){ // branch + jump
        check_result->new_entry.br_slot.valid  = true;
        check_result->new_entry.br_slot.is_rvc = decode_result->decode[decode_result->one_branch_index].is_rvc;
        check_result->new_entry.br_slot.offset = decode_result->decode[decode_result->one_branch_index].offset;
        check_result->new_entry.br_slot.carry[0] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) - 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.carry[1] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) + 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.next_low = ((decode_result->decode[decode_result->one_branch_index].branch_addr >> 1) & 0xfff);
        check_result->new_entry.tail_slot.valid  = true;
        check_result->new_entry.tail_slot.is_rvc = decode_result->decode[decode_result->jump_index].is_rvc;
        check_result->new_entry.tail_slot.offset = decode_result->decode[decode_result->jump_index].offset;
        check_result->new_entry.tail_slot.carry[0] = (((decode_result->decode[decode_result->jump_index].branch_addr >> 21) - 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.carry[1] = (((decode_result->decode[decode_result->jump_index].branch_addr >> 21) + 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.next_low = ((decode_result->decode[decode_result->jump_index].branch_addr >> 1) & 0xfffff);
        check_result->new_entry.is_branch = false;
        check_result->new_entry.is_call   = decode_result->decode[decode_result->jump_index].is_call;
        check_result->new_entry.is_jalr   = decode_result->decode[decode_result->jump_index].is_jalr;
        check_result->new_entry.is_ret    = decode_result->decode[decode_result->jump_index].is_ret;
        end_pc = result->start_pc + decode_result->decode[decode_result->jump_index].offset;
        if(decode_result->decode[decode_result->jump_index].is_rvc)
            end_pc += 2;
        else
            end_pc += 4;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }else if((decode_result->has_jump) & (decode_result->has_one_branch == false)){ // has not condition jump
        check_result->new_entry.br_slot.valid    = false;
        check_result->new_entry.br_slot.carry[0] = false;
        check_result->new_entry.br_slot.carry[1] = false;
        check_result->new_entry.br_slot.is_rvc   = false;
        check_result->new_entry.tail_slot.valid  = true;
        check_result->new_entry.tail_slot.is_rvc = decode_result->decode[decode_result->jump_index].is_rvc;
        check_result->new_entry.tail_slot.offset = decode_result->decode[decode_result->jump_index].offset;
        check_result->new_entry.tail_slot.carry[0] = (((decode_result->decode[decode_result->jump_index].branch_addr >> 21) - 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.carry[1] = (((decode_result->decode[decode_result->jump_index].branch_addr >> 21) + 1) == (result->start_pc >> 21));
        check_result->new_entry.tail_slot.next_low = ((decode_result->decode[decode_result->jump_index].branch_addr >> 1) & 0xfffff);
        check_result->new_entry.is_branch = false;
        check_result->new_entry.is_call   = decode_result->decode[decode_result->jump_index].is_call;
        check_result->new_entry.is_jalr   = decode_result->decode[decode_result->jump_index].is_jalr;
        check_result->new_entry.is_ret    = decode_result->decode[decode_result->jump_index].is_ret;
        end_pc = result->start_pc + decode_result->decode[decode_result->jump_index].offset;
        if(decode_result->decode[decode_result->jump_index].is_rvc)
            end_pc += 2;
        else
            end_pc += 4;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }else if(decode_result->has_one_branch){// one branch
        check_result->new_entry.br_slot.valid  = true;
        check_result->new_entry.br_slot.is_rvc = decode_result->decode[decode_result->one_branch_index].is_rvc;
        check_result->new_entry.br_slot.offset = decode_result->decode[decode_result->one_branch_index].offset;
        check_result->new_entry.br_slot.carry[0] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) - 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.carry[1] = (((decode_result->decode[decode_result->one_branch_index].branch_addr >> 13) + 1) == (result->start_pc >> 13));
        check_result->new_entry.br_slot.next_low = ((decode_result->decode[decode_result->one_branch_index].branch_addr >> 1) & 0xfff);
        check_result->new_entry.tail_slot.valid  = false;
        check_result->new_entry.tail_slot.carry[0] = false;
        check_result->new_entry.tail_slot.carry[1] = false;
        check_result->new_entry.tail_slot.is_rvc   = false;
        end_pc = result->start_pc + predict_size;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }else{// don't have branch and jump
        check_result->new_entry.valid = false;
        end_pc = result->start_pc + predict_size;
        check_result->new_entry.carry = ((end_pc >> predict_bit_size) != (result->start_pc >> predict_bit_size));
        check_result->new_entry.next_low = (uint32_t)(end_pc & (((uint32_t)0x1 << predict_bit_size) - (uint32_t)1));
    }
    check_result->update = true;
    if(result->hit){
        check_result->update = comp_ftb_entry(&result->old_entry, &check_result->new_entry);
    }
}

ifu_class::ifu_class(bool rvi_valid, ftq_class &ftq_i, uint8_t *mmap_ptr_i, uint64_t pc_bias_i, test_base_class &test_i)
    :ftq(ftq_i), test(test_i){
    decode_result.has_jump = false;
    decode_result.has_one_branch = false;
    decode_result.has_two_branch = false;
    decode_result.has_three_branch = false;
    decode_result.rvi_valid = rvi_valid;
    decode_result.decode = NULL;
    mmap_ptr = mmap_ptr_i;
    pc_bias = pc_bias_i;
}

ifu_class::~ifu_class(){
    if(decode_result.decode != NULL)
        free(decode_result.decode);
}

void ifu_class::set_rvi_status(bool rvi_valid){
    decode_result.rvi_valid = rvi_valid;
}

bool ifu_class::get_rvi_status(){
    return decode_result.rvi_valid;
}

void ifu_class::fetch_code_check(uint32_t tag_start_bit, uint32_t tag_bit_size, uint32_t predict_bit_size, uint32_t predict_size){
    while(ftq.ifu_empty() == false){
        ftq_entry *result = ftq.ifu_queue_get_top();
        result->rvi_valid = decode_result.rvi_valid;
        predecode(mmap_ptr + result->start_pc - pc_bias, predict_size, result->start_pc, &decode_result);
        precheck(&decode_result, result, &check_result, tag_start_bit, tag_bit_size, predict_bit_size, predict_size);
        /*
        * 冲刷：重定向next_pc的同时恢复ras和plru，同时第二种情况更新plru，并且更新end_pc，即miss update了ftb的整个状态
        *if(check_result.update){
        *    if((check_result.new_entry.valid) & (result.hit == false)){
        *        if(decode_result.has_jump & check_result.new_entry.is_ret)
        *            冲刷并以ras.pop()为next_pc接下去预测
        *        else if(decode_result.has_jump & (check_result.new_entry.is_ret == false))
        *            冲刷并以new_entry.next_pc为next_pc接下去预测
        *    }else if((check_result.new_entry.valid) & (result.hit == true)){
        *        if(decode_result.has_jump & check_result.new_entry.is_ret)
        *            冲刷并以ras.pop()为next_pc接下去预测
        *        else if(decode_result.has_jump & (check_result.new_entry.is_ret == false))
        *            冲刷并以new_entry.next_pc为next_pc接下去预测
        *        else
        *            冲刷并以start_pc + predict_size为next_pc接下去预测
        *    }else if(result.hit){
        *        冲刷并以start_pc + predict_size为next_pc接下去预测
        *    }
        *}
        */
        //! TODO
        //这里有个问题，如果此时处理的ftq_entry非最早的，而是位于中间，那么如何让ras恢复？checkpoint吗？

        if(check_result.update){
            uint64_t end_pc = (result->start_pc + check_result.new_entry.tail_slot.offset);
            if(check_result.new_entry.tail_slot.is_rvc)
                end_pc = end_pc + 2;
            else 
                end_pc = end_pc + 4;
            uint64_t block_pc = (result->start_pc >> predict_bit_size);
            if(check_result.new_entry.carry)
                block_pc += 1;
            block_pc = (block_pc << predict_bit_size) + check_result.new_entry.next_low;
            if((check_result.new_entry.valid) & (result->hit == false)){
                //? 第一次遇到，无别名问题
                if(decode_result.has_jump & check_result.new_entry.is_ret){
                    result->old_entry = check_result.new_entry;
                    result->token = true;
                    result->is_tail = true;
                    result->end_pc = end_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(decode_result.decode[decode_result.jump_index].branch_addr);
                }
                else if(decode_result.has_jump & (check_result.new_entry.is_ret == false)){
                    result->old_entry = check_result.new_entry;
                    result->token = true;
                    result->is_tail = true;
                    result->end_pc = end_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(decode_result.decode[decode_result.jump_index].branch_addr);
                }else{
                    result->old_entry = check_result.new_entry;
                    result->token = false;
                    result->end_pc = block_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(block_pc);
                    if(decode_result.has_three_branch){
                        decode_result.rvi_valid = false;
                    }
                }
            }else if((check_result.new_entry.valid) & (result->hit == true)){
                //? 匹配错误，别名问题
                if(decode_result.has_jump & check_result.new_entry.is_ret){
                    result->old_entry = check_result.new_entry;
                    result->token = true;
                    result->is_tail = true;
                    result->end_pc = end_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(decode_result.decode[decode_result.jump_index].branch_addr);
                }
                else if(decode_result.has_jump & (check_result.new_entry.is_ret == false)){
                    result->old_entry    = check_result.new_entry;
                    result->token = true;
                    result->is_tail = true;
                    result->end_pc = end_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(decode_result.decode[decode_result.jump_index].branch_addr);
                }else{
                    result->old_entry = check_result.new_entry;
                    result->token = false;
                    result->end_pc = block_pc;
                    ftq.precheck_restore(result);
                    test.update_pc(block_pc);
                    if(decode_result.has_three_branch){
                        decode_result.rvi_valid = false;
                    }
                }
            }else if(result->hit){
                //? 别名
                result->old_entry = check_result.new_entry;
                result->token = false;
                result->end_pc = result->start_pc + predict_size;
                ftq.precheck_restore(result);
                test.update_pc(result->start_pc + predict_size);
            }
            result->old_entry.always_tacken[0] = false;
            result->old_entry.always_tacken[1] = false;
            result->old_entry.br_slot.bit2_cnt = 0;
            result->old_entry.tail_slot.bit2_cnt = 0;
        }
        result->first_pred_flag = check_result.update;
        if(result->issue_inst != NULL){
            free(result->issue_inst);
        }
        result->issue_inst = decode_result.decode;
        uint32_t inst_index = 0;
        uint32_t inst_count = 0;
        while(inst_index < decode_result.cnt){
            if(result->issue_inst[inst_index].inst_pc < result->end_pc)
                inst_count++;
            inst_index++;
        }
        result->issue_cnt = inst_count;
        decode_result.decode = NULL;
        if(result->token | decode_result.has_three_branch){
            decode_result.rvi_valid = false;
        }
        ftq.ifu_use_mark();
    }
}
