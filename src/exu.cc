#include "exu.h"
#include "assert.h"

exu_class::exu_class(ftq_class &ftq_i, test_base_class &test_i, FILE *fp, uint32_t predict_bit_size_i, ifu_class &ifu_i)
    :ftq(ftq_i), test(test_i), db_fp(fp), predict_bit_size(predict_bit_size_i), ifu(ifu_i){}

exu_class::~exu_class(){}

bool exu_class::execute(uint64_t &inst_cnt, uint64_t &pred_miss){
    while(ftq.empty() == false){
        // 以下开始只能使用result，因为他是ftq里面的项，后面是ROB或EXU发出指令正常向后流的信号
        ftq_entry *result = ftq.commit_queue_get_top();
        uint64_t pc = result->start_pc;
        size_t ret_f = 0;
        char type = 0;

        for(uint32_t inst_index = 0; inst_index < result->issue_cnt; inst_index++){
            assert(result->issue_inst[inst_index].inst_pc < result->end_pc);
            uint32_t token_offset = (result->is_tail) ? result->old_entry.tail_slot.offset : result->old_entry.br_slot.offset;
            ret_f = fread(&pc, sizeof(uint64_t), 1, db_fp);
            assert(pc == result->issue_inst[inst_index].inst_pc);
            if(ret_f != 1){
                perror("from db read pc error");
                goto out;
            }
            ret_f = fread(&type, sizeof(char), 1, db_fp);
            if(ret_f != 1){
                perror("from db read type error");
                goto out;
            }
            bool rvc  = ((type & 0x80) == 0x80);
            bool jump = ((type & 0x40) == 0x40);
            bool halt = ((type & 0x20) == 0x20);
            if(halt)goto out;
            inst_cnt++;
            if(result->old_entry.valid & result->old_entry.br_slot.valid & ((result->issue_inst[inst_index].inst_pc - result->start_pc) == result->old_entry.br_slot.offset)){
                    //* 以下更新 不跳转时会顺便更新result.token_offset和result.token
                    if(result->first_pred_flag & jump){
                        result->old_entry.br_slot.bit2_cnt = 3;
                        result->old_entry.always_tacken[0] = true;
                        result->old_entry.always_tacken[1] = false;
                        // 冲刷并以new_entry.br_slot.next_pc为next_pc接下去预测
                    }else if(result->first_pred_flag & (jump == false)){
                        result->old_entry.br_slot.bit2_cnt = 0;
                        result->old_entry.always_tacken[0] = false;
                    }else if((token_offset == (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & jump){
                        if((result->old_entry.br_slot.bit2_cnt < 3) & (result->old_entry.always_tacken[0] == false))
                            result->old_entry.br_slot.bit2_cnt++;
                    }else if((token_offset == (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & (jump == false)){
                        if(result->old_entry.br_slot.bit2_cnt > 0)
                            result->old_entry.br_slot.bit2_cnt--;
                        result->old_entry.always_tacken[0] = false;
                        /*
                        *
                        * if(result.old_entry.tail_slot.valid & result.old_entry.is_branch & ((result.old_entry.tail_slot.bit2_cnt > 1) | result.old_entry.always_tacken[1])){
                        *   冲刷并以new_entry.tail_slot.next_pc为next_pc接下去预测
                        * }else if(result.old_entry.tail_slot.valid & (result.old_entry.is_branch == false)){
                        *   冲刷并以new_entry.tail_slot.next_pc为next_pc接下去预测
                        * }else{
                        *   冲刷并以new_entry指示的块结束地址为next_pc接下去预测
                        * }
                        * 
                        */
                    }else if((token_offset != (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & jump){
                        if(result->old_entry.br_slot.bit2_cnt < 3)
                            result->old_entry.br_slot.bit2_cnt++;
                        // 冲刷并以new_entry.br_slot.next_pc为next_pc接下去预测
                    }else if((result->token == false) & jump){
                        if(result->old_entry.br_slot.bit2_cnt < 3)
                            result->old_entry.br_slot.bit2_cnt++;
                        // 冲刷并以new_entry.br_slot.next_pc为next_pc接下去预测
                    }
            }else if(result->old_entry.valid & result->old_entry.tail_slot.valid & ((result->issue_inst[inst_index].inst_pc - result->start_pc) == result->old_entry.tail_slot.offset)){
                    if(result->first_pred_flag & jump & result->old_entry.is_branch){
                        result->old_entry.tail_slot.bit2_cnt = 3;
                        result->old_entry.always_tacken[1] = true;
                        // 冲刷并以new_entry.tail_slot.next_pc为next_pc接下去预测
                    }else if(result->first_pred_flag & jump & (result->old_entry.is_branch == false)){
                        result->old_entry.tail_slot.bit2_cnt = 0;
                        result->old_entry.always_tacken[1] = false;
                    }else if(result->first_pred_flag & (jump == false)){
                        result->old_entry.tail_slot.bit2_cnt = 0;
                        result->old_entry.always_tacken[1] = false;
                    }else if((token_offset == (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & jump){
                        if((result->old_entry.tail_slot.bit2_cnt < 3) & (result->old_entry.always_tacken[1] == false) & result->old_entry.is_branch)
                            result->old_entry.tail_slot.bit2_cnt++;
                    }else if((token_offset == (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & (jump == false)){
                        if(result->old_entry.tail_slot.bit2_cnt > 0)
                            result->old_entry.tail_slot.bit2_cnt--;
                        result->old_entry.always_tacken[1] = false;
                        //* 冲刷并以next_block_addr为next_pc接下去预测
                    }else if((result->token == false) & jump){
                        if(result->old_entry.tail_slot.bit2_cnt < 3)
                            result->old_entry.tail_slot.bit2_cnt++;
                        // 冲刷并以new_entry.tail_slot.next_pc为next_pc接下去预测
                    }
                }
            if(jump){
                ret_f = fread(&pc, sizeof(uint64_t), 1, db_fp);
                if(ret_f != 1){
                    perror("from db read jump pc error");
                    goto out;
                }
                if(result->token == false){
                    ftq.commit_restore(result);
                    ifu.set_rvi_status(false);
                    test.update_pc(pc);
                    pred_miss++;
                }else if(result->next_pc != pc){
                    ftq.commit_restore(result);
                    ifu.set_rvi_status(false);
                    test.update_pc(pc);
                    pred_miss++;
                }
                break;
            }else if((token_offset == (result->issue_inst[inst_index].inst_pc - result->start_pc)) & result->token & (jump == false)){
                ftq.commit_restore(result);
                if(rvc)
                    test.update_pc(result->issue_inst[inst_index].inst_pc + 2);
                else 
                    test.update_pc(result->issue_inst[inst_index].inst_pc + 4);
                ifu.set_rvi_status(false);
                pred_miss++;
            }
        }
        test.update(result);
        // free(result->issue_inst);
        // result->issue_inst = NULL;
        ftq.dequeue();
    }
    return false;
out:
    return true;
}

