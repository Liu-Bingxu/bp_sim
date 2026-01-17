#include "stdio.h"
#include "cJSON.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"

static uint64_t inst_cnt  = 0;
static uint64_t pred_miss = 0;

// 0x80:rvc
// 0x40:jump

// 无BP性能仿真测试
int bp_sim_no_bp(FILE *bin_fp, FILE *db_fp, cJSON *conf_json){
    size_t ret_f = 0;
    uint64_t pc = 0;
    char type = 0;

    while(true){
        ret_f = fread(&pc, sizeof(uint64_t), 1, db_fp);
        if(ret_f != 1){
            perror("from db read pc error");
            goto out;
        }
        ret_f = fread(&type, sizeof(char), 1, db_fp);
        if(ret_f != 1){
            perror("from db read type error");
            goto out;
        }
        bool jump = ((type & 0x40) == 0x40);
        bool halt = ((type & 0x20) == 0x20);
        if(halt)goto out;
        inst_cnt++;
        if(jump){
            pred_miss++;
            ret_f = fread(&pc, sizeof(uint64_t), 1, db_fp);
            if(ret_f != 1){
                perror("from db read pc error");
                goto out;
            }
        }
    }

out:
    printf("no_bp-test:\n");
    printf("inst_cnt is %ld-0x%lx; pred_miss is %ld-0x%lx\n", inst_cnt, inst_cnt, pred_miss, pred_miss);
    printf("miss rate is %f\n", (double)pred_miss / (double)inst_cnt);
    return 0;
}
