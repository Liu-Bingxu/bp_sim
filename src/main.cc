#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "cJSON.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

extern int bp_sim_no_bp(FILE *bin_fp, FILE *db_fp, cJSON *conf_json);
extern int bp_sim_uftb(FILE *bin_fp, FILE *db_fp, cJSON *conf_json);
typedef int (*test_fn)(FILE *bin_fp, FILE *db_fp, cJSON *conf_json);

test_fn test_funcs[] = {bp_sim_no_bp, bp_sim_uftb};

int main(int argc, char const *argv[]){
    if(argc < 2){
        printf("usage %s <conf_json_name>\n", argv[0]);
        return 0;
    }

    size_t ret_f = 0;

    FILE *conf_fp = NULL;
    conf_fp = fopen(argv[1], "rb");
    if(conf_fp == NULL){
        perror("can't open conf_json file");
        return 1;
    }
    fseek(conf_fp, 0, SEEK_END);
    uint32_t json_size = ftell(conf_fp);
    fseek(conf_fp, 0, SEEK_SET);
    char *str = (char *)malloc(sizeof(char) * (json_size + 1));
    ret_f =fread(str, json_size, 1, conf_fp);
    if(ret_f != 1){
        perror("read config json file error");
        fclose(conf_fp);
        return 1;
    }
    str[json_size] = '\0';
    cJSON *conf_json = cJSON_Parse(str);
    free(str);
    str = NULL;
    fclose(conf_fp);
    conf_fp = NULL;

    cJSON *bin_name = cJSON_GetObjectItem(conf_json, "bin file name");
    str = cJSON_GetStringValue(bin_name);
    FILE *bin_fp = NULL;
    bin_fp = fopen(str, "rb");
    if(bin_fp == NULL){
        perror("can't open bin file");
        return 1;
    }

    cJSON *db_name = cJSON_GetObjectItem(conf_json, "itrace file name");
    str = cJSON_GetStringValue(db_name);
    FILE *db_fp = NULL;
    db_fp = fopen(str, "rb");
    if(db_fp == NULL){
        perror("can't open itrace file");
        return 1;
    }

    cJSON *test_sel_json = cJSON_GetObjectItem(conf_json, "test_fn_index");

    cJSON *func_name = cJSON_GetObjectItem(conf_json, "bp_sim func");
    if(cJSON_IsArray(func_name)){
        cJSON *func_name_array = NULL;
        cJSON_ArrayForEach(func_name_array, func_name){
            fseek(bin_fp, 0, SEEK_SET);
            fseek(db_fp, 0, SEEK_SET);
            str = cJSON_GetStringValue(func_name_array);
            cJSON *test_sel_num = cJSON_GetObjectItem(test_sel_json, str);
            int sel_num = cJSON_GetNumberValue(test_sel_num);
            cJSON *test_conf = cJSON_GetObjectItem(conf_json, str);
            test_funcs[sel_num](bin_fp, db_fp, test_conf);
        }
    }else{
        str = cJSON_GetStringValue(func_name);
        cJSON *test_sel_num = cJSON_GetObjectItem(test_sel_json, str);
        int sel_num = cJSON_GetNumberValue(test_sel_num);
        cJSON *test_conf = cJSON_GetObjectItem(conf_json, str);
        test_funcs[sel_num](bin_fp, db_fp, test_conf);
    }

    fclose(db_fp);
    fclose(bin_fp);
    cJSON_Delete(conf_json);

    return 0;
}


