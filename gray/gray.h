/*
 * =====================================================================================
 *
 *       Filename:  gray.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/10/2012 03:55:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  qinyuchun (), beerium@126.com
 *        Company:  www.feinno.com
 *
 * =====================================================================================
 */

#ifndef __GRAY_H__
#define __GRAY_H__


#include <antlr3.h>
#include "../include/list.h"

enum operand_type_t {INT_TYPE, FLOAT_TYPE, STRING_TYPE} ;
enum operation_t {OPR_AND, OPR_OR, OPR_NOT, OPR_GROUP, OPR_DEFAULT};

typedef struct common_token_s
{ 
    pANTLR3_COMMON_TOKEN tokens;
    list_head_t list;
}common_token_t;

typedef struct args_value_s
{
    char* value;
    list_head_t list;
}args_value_t;

typedef struct func_base_s
{
    args_value_t *args;
    enum operand_type_t type;
    char func_name[10];
    int (*apply) (const char *field_value, const args_value_t *args, enum operand_type_t type);
    void (*func_free) (args_value_t *args);
}func_base_t;

typedef struct cond_s cond_t;

struct cond_s
{
    cond_t *lvalue;
    cond_t *rvalue;
    enum operation_t operation;
    func_base_t func;
    char* field_name;
    // in fact , should input ctx;
    int (*apply) (const cond_t *lvalue, const cond_t *rvalue, const char *field_value,
            enum operation_t operation);
    void (*cond_free) (cond_t *lvalue, cond_t *rvalue, enum operation_t operation);
};

typedef struct gray_user_context_s gray_user_context_t;

struct gray_user_context_s
{
    char *uid;
    char *sid;
    char *mobile_no;
    char *user_region;
    char *user_carrier;
    char *client_type;
    char *client_region;
    char *client_version;
    char * (*get_value)(gray_user_context_t *ctx, const char *field_name);
 };

int gray(char *expr, gray_user_context_t ctx);

int cond_apply(const cond_t *lvalue, const cond_t *rvalue, const char *field_value,  
        enum operation_t operation);

void cond_free(cond_t *lvalue, cond_t *rvalue, enum operation_t operation);

void free_gray_package();

#define token_list_for_each(tk, tklist)         \
list_for_each_entry((tk),list,&(tklist)->list)

#define token_list_append(tk,tklist)       \
list_append(&(tk)->list,&(tklist)->list)

#define token_list_remove(tk)         \
list_remove(&(tk)->list)

#define args_list_for_each(arg, arglist)         \
list_for_each_entry((arg),list,&(arglist)->list)

#define args_list_for_each_safe(arg, arg_t, arglist)         \
list_for_each_entry_safe((arg), (arg_t), &(arglist)->list ,list)

#define args_list_append(arg,arglist)       \
list_append(&(arg)->list,&(arglist)->list)

#define args_list_remove(arg)         \
list_remove(&(arg)->list)

#endif
