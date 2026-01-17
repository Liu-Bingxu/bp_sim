#include "decode.h"
#include "stdlib.h"
#include "assert.h"

#define BITMASK(bits) ((1ull << (bits)) - 1)
#define BITS(x, hi, lo) (((x) >> (lo)) & BITMASK((hi) - (lo) + 1)) // similar to x[hi:lo] in verilog
#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (uint64_t)__x.n; })

#define c_j_jal_imm (SEXT(((BITS(c_inst, 12, 12) << 11) | (BITS(c_inst, 11, 11) << 4) | (BITS(c_inst, 10, 9) << 8) | (BITS(c_inst, 8, 8) << 10) | (BITS(c_inst, 7, 7) << 6) | (BITS(c_inst, 6, 6) << 7) | (BITS(c_inst, 5, 3) << 1) | (BITS(c_inst, 2, 2) << 5)), 12))
#define c_b_imm (SEXT(((BITS(c_inst, 12, 12) << 8) | (BITS(c_inst, 11, 10) << 3) | (BITS(c_inst, 6, 5) << 6) | (BITS(c_inst, 4, 3) << 1) | (BITS(c_inst, 2, 2) << 5)), 9))

#define immJ (SEXT((BITS(inst, 31, 31) << 20) | (BITS(inst, 30, 21) << 1) | (BITS(inst, 20, 20) << 11) | (BITS(inst, 19, 12) << 12), 21))
#define immB (SEXT((BITS(inst, 31, 31) << 12) | (BITS(inst, 30, 25) << 5) | (BITS(inst, 11, 8) << 1) | (BITS(inst, 7, 7) << 11),13))

void predecode_one(uint32_t inst, uint64_t inst_pc, inst_decode *decode_one){
    decode_one->is_branch = false;
    decode_one->is_call   = false;
    decode_one->is_jal    = false;
    decode_one->is_jalr   = false;
    decode_one->is_ret    = false;
    decode_one->is_rvc    = false;
    decode_one->inst_pc   = inst_pc;
    decode_one->inst      = inst;
    if((inst & 0x3) != 0x3){
        //! rvc instruction
        uint16_t c_inst = inst & 0xffff;
        decode_one->is_rvc = true;
        if(((c_inst & 0x3) == 0x1) & ((c_inst & 0xe000) == 0xa000)){ // c.j
            decode_one->is_jal = true;
            decode_one->branch_addr = inst_pc + c_j_jal_imm;
        }else if(((c_inst & 0x3) == 0x1) & ((c_inst & 0xe000) == 0xc000)){ // c.beqz
            decode_one->is_branch = true;
            decode_one->branch_addr = inst_pc + c_b_imm;
        }else if(((c_inst & 0x3) == 0x1) & ((c_inst & 0xe000) == 0xe000)){ // c.bnez
            decode_one->is_branch = true;
            decode_one->branch_addr = inst_pc + c_b_imm;
        }else if(((c_inst & 0x7f) == 0x2) & ((c_inst & 0xf80) != 0x0) & ((c_inst & 0xf000) == 0x8000)){ // c.jr
            if(c_inst == 0x8082)
                decode_one->is_ret = true;
            else
                decode_one->is_jalr = true;
            decode_one->branch_addr = inst_pc;
        }else if(((c_inst & 0x7f) == 0x2) & ((c_inst & 0xf80) != 0x0)  & ((c_inst & 0xf000) == 0x9000)){ // c.jalr
            decode_one->is_call = true;
            decode_one->branch_addr = inst_pc;
        }
        return;
    }
    if((inst & 0x7f) == 0x6f){//jal
        if((inst & 0xf80) == 0x80)decode_one->is_call = true;
        else                      decode_one->is_jal  = true;
        decode_one->branch_addr = inst_pc + immJ;
    }else if(((inst & 0x7f) == 0x67) & ((inst & 0x7000) == 0)){// jalr
        if((inst & 0xf80) == 0x80)decode_one->is_call = true;
        else if(inst == 0x8067)decode_one->is_ret = true;
        else                      decode_one->is_jalr  = true;
        decode_one->branch_addr = inst_pc;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0)){// beq
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0x1000)){// bne
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0x4000)){// blt
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0x6000)){// bltu
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0x5000)){// bge
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }else if(((inst & 0x7f) == 0x63) & ((inst & 0x7000) == 0x7000)){// bgeu
        decode_one->is_branch = true;
        decode_one->branch_addr = inst_pc + immB;
    }
}

void predecode(uint8_t *ptr, uint32_t predict_size, uint64_t start_pc, predecode_result *decode_result){
    uint32_t use_size = 0;
    uint32_t inst_cnt = 0;
    int branch_cnt = 0;
    bool next_rvi_valid = false;

    decode_result->has_jump = false;
    decode_result->has_one_branch = false;
    decode_result->has_two_branch = false;
    decode_result->has_three_branch = false;

    if(decode_result->rvi_valid){
        use_size += 2;
    }
    while(use_size < predict_size){
        if((ptr[use_size] & 0x3) == 0x3){
            use_size += 4;
        }else{
            use_size += 2;
        }
        inst_cnt++;
    }
    if(use_size > predict_size){
        next_rvi_valid = true;
    }
    // decode_result->decode = (inst_decode *)calloc(sizeof(inst_decode) * inst_cnt, 1);
    assert(inst_cnt <= DECODE_CNT);
    decode_result->cnt    = inst_cnt;
    use_size = 0;
    inst_cnt = 0;
    if(decode_result->rvi_valid){
        use_size += 2;
    }
    if(next_rvi_valid)
        decode_result->rvi_valid = true;
    else 
        decode_result->rvi_valid = false;
    while(use_size < predict_size){
        uint32_t inst = 0;
        inst = ((uint32_t)ptr[use_size]) | (((uint32_t)ptr[use_size + 1]) << 8) | (((uint32_t)ptr[use_size + 2]) << 16) | (((uint32_t)ptr[use_size + 3]) << 24);
        decode_result->decode[inst_cnt].offset = use_size;
        predecode_one(inst, start_pc + use_size, &decode_result->decode[inst_cnt]);
        if((ptr[use_size] & 0x3) == 0x3){
            use_size += 4;
        }else{
            use_size += 2;
        }
        if(decode_result->decode[inst_cnt].is_branch){
            branch_cnt++;
            if((branch_cnt == 1) & (decode_result->has_jump == false)){
                decode_result->has_one_branch = true;
                decode_result->one_branch_index = inst_cnt;
            }
            if((branch_cnt == 2) & (decode_result->has_jump == false)){
                decode_result->has_two_branch = true;
                decode_result->two_branch_index = inst_cnt;
            }
            if((branch_cnt == 3) & (decode_result->has_jump == false) & (decode_result->has_three_branch == false)){
                decode_result->has_three_branch = true;
                decode_result->three_branch_index = inst_cnt;
            }
        }
        if((decode_result->has_two_branch == false) & (decode_result->has_jump == false)){
            if((decode_result->decode[inst_cnt].is_jal) & (decode_result->has_jump == false)){
                decode_result->has_jump = true;
                decode_result->jump_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_jalr & (decode_result->has_jump == false)){
                decode_result->has_jump = true;
                decode_result->jump_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_call & (decode_result->has_jump == false)){
                decode_result->has_jump = true;
                decode_result->jump_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_ret & (decode_result->has_jump == false)){
                decode_result->has_jump = true;
                decode_result->jump_index = inst_cnt;
            }
        }else if((decode_result->has_two_branch == true) & (decode_result->has_three_branch == false)){
            if((decode_result->decode[inst_cnt].is_jal)){
                decode_result->has_three_branch = true;
                decode_result->three_branch_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_jalr){
                decode_result->has_three_branch = true;
                decode_result->three_branch_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_call){
                decode_result->has_three_branch = true;
                decode_result->three_branch_index = inst_cnt;
            }else if(decode_result->decode[inst_cnt].is_ret){
                decode_result->has_three_branch = true;
                decode_result->three_branch_index = inst_cnt;
            }
        }
        inst_cnt++;
    }
}
