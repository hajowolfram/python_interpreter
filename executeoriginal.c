/*execute.c*/

//
// Executes nuPython program, given as a Program Graph.
// 
// Solution by Prof. Joe Hummel

// Northwestern University
// CS 211
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // true, false
#include <string.h>
#include <assert.h>
#include <math.h>

#include "programgraph.h"
#include "ram.h"
#include "execute.h"


//
// Private functions:
//

//
// get_element_value
//
// Given a basic element of an expression --- an identifier
// "x" or some kind of literal like 123 --- the value of 
// this identifier or literal is returned via the reference 
// parameter. Returns true if successful, false if not.
//
// Why would it fail? If the identifier does not exist in 
// memory. This is a semantic error, and an error message is 
// output before returning.
//
static bool get_element_value(struct STMT* stmt, struct RAM* memory, struct ELEMENT* element, struct RAM_VALUE* value)
{
  if (element->element_type == ELEMENT_INT_LITERAL) {

    char* literal = element->element_value;
    value->value_type = RAM_TYPE_INT;
    value->types.i = atoi(literal);
  }
  else if (element->element_type == ELEMENT_REAL_LITERAL) {

    char* literal = element->element_value;
    value->value_type = RAM_TYPE_REAL;
    value->types.d = atof(literal);
  }
  else if (element->element_type == ELEMENT_STR_LITERAL) {

    char* literal = element->element_value;
    value->value_type = RAM_TYPE_STR;
    value->types.s = literal;
  }
  else if (element->element_type == ELEMENT_TRUE) {

    char* literal = element->element_value;
    value->value_type = RAM_TYPE_BOOLEAN;
    value->types.i = 1;
  }
  else if (element->element_type == ELEMENT_FALSE) {

    char* literal = element->element_value;
    value->value_type = RAM_TYPE_BOOLEAN;
    value->types.i = 0;
  }
  
  else {
    //
    // identifier => variable
    //
    assert(element->element_type == ELEMENT_IDENTIFIER);
    char* var_name = element->element_value;
    struct RAM_VALUE* ram_value = ram_read_cell_by_id(memory, var_name);

    if (ram_value == NULL) {
      printf("**SEMANTIC ERROR: name '%s' is not defined (line %d)\n", var_name, stmt->line);
      return false;
    }
    if (ram_value->value_type == RAM_TYPE_INT) {
      value->value_type = ram_value->value_type;
      value->types.i = ram_value->types.i;
    }
    else if (ram_value->value_type == RAM_TYPE_REAL) {
      value->value_type = ram_value->value_type;
      value->types.d = ram_value->types.d;
    }
    else if (ram_value->value_type == RAM_TYPE_STR) {
      value->value_type = ram_value->value_type;
      value->types.s = ram_value->types.s;
    }
    else if (ram_value->value_type == RAM_TYPE_BOOLEAN) {
      value->value_type = ram_value->value_type;
      value->types.i = ram_value->types.i;
    }
    
    //printf("check1");
    
  }

  return true;
}


//
// get_unary_value
//
// Given a unary expr, returns the value that it represents.
// This could be the result of a literal 123 or the value
// from memory for an identifier such as "x". Unary values
// may have unary operators, such as + or -, applied.
// This value is "returned" via the reference parameter.
// Returns true if successful, false if not.
//
// Why would it fail? If the identifier does not exist in 
// memory. This is a semantic error, and an error message is 
// output before returning.
//
static bool get_unary_value(struct STMT* stmt, struct RAM* memory, struct UNARY_EXPR* unary, struct RAM_VALUE* value)
{
  //
  // we only have simple elements so far (no unary operators):
  //
  assert(unary->expr_type == UNARY_ELEMENT);

  struct ELEMENT* element = unary->element;

  bool success = get_element_value(stmt, memory, element, value);

  return success;
}

//
// given two ram values and a opoeration operator, performs calculations using the values and returns the result
//
double expression_helper(struct RAM_VALUE* lhs, int operator, struct RAM_VALUE* rhs){
  
  double lhs_value;
  double rhs_value;
  double result;

  if (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_INT) {
    lhs_value = (double)(lhs->types.i);
    rhs_value = (double)(rhs->types.i);
  }
  else if (lhs->value_type == RAM_TYPE_REAL && rhs->value_type == RAM_TYPE_INT) {
    lhs_value = lhs->types.d;
    rhs_value = (double)(rhs->types.i);
  }
  else if (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_REAL) {
    lhs_value = (double)(lhs->types.i);
    rhs_value = rhs->types.d;
  }
  else {
    lhs_value = lhs->types.d;
    rhs_value = rhs->types.d;
  }
  
  switch (operator)
    {
  case OPERATOR_PLUS:
    result = lhs_value + rhs_value;
    break;
      
  case OPERATOR_MINUS:
    result = lhs_value - rhs_value;
    break;
      
  case OPERATOR_ASTERISK:
    result = lhs_value * rhs_value;
    break;

  case OPERATOR_POWER:
    result = pow(lhs_value, rhs_value);
    break;

  case OPERATOR_MOD:
    result = fmod(lhs_value, rhs_value);
    break;

  case OPERATOR_DIV:
    result = lhs_value/rhs_value;
    break;
    }
  
  return result;
}

//
// checks for cases in which the input to assignment helper contains zeros and returns true if this is the case.
//
bool string_check_helper(char* input){
  while (*input != '\0') {
    if (*input != '0' && *input != '.') {
      return false;
    }
    input++;
  }
  return true;
}

//
// given two ram values and a comparison operator, performs comparisons using the values and returns the boolean result
//
bool conditional_eval_helper(struct RAM_VALUE* lhs, int operator, struct RAM_VALUE* rhs) {
  double lhs_value;
  double rhs_value;

  if (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_INT) {
    lhs_value = (double)lhs->types.i;
    rhs_value = (double)rhs->types.i;
  }
  else if (lhs->value_type == RAM_TYPE_REAL && rhs->value_type == RAM_TYPE_INT) {
    lhs_value = lhs->types.d;
    rhs_value = (double)rhs->types.i;
  }
  else if (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_REAL) {
    lhs_value = (double)lhs->types.i;
    rhs_value = rhs->types.d;
  }
  else {
    lhs_value = lhs->types.d;
    rhs_value = rhs->types.d;
  }

  switch (operator)
    {
  case OPERATOR_EQUAL:
    return (lhs_value == rhs_value);
    break;
      
  case OPERATOR_NOT_EQUAL:
    return (double)(lhs_value != rhs_value);
    break;

  case OPERATOR_LT:
    return (double)(lhs_value < rhs_value);
    break;

  case OPERATOR_GT:
    return (double)(lhs_value > rhs_value);
    break;

  case OPERATOR_LTE:
    return (double)(lhs_value <= rhs_value);
    break;

  case OPERATOR_GTE:
    return (double)(lhs_value >= rhs_value);
    break;
    }
  return 0;
}

//
// given an operator, returns whether it is a comparison or calculation operator
//
int operator_helper(int operator){
if
  (operator == OPERATOR_PLUS || operator == OPERATOR_MINUS || operator == OPERATOR_ASTERISK ||operator ==OPERATOR_POWER || operator ==OPERATOR_MOD || operator ==OPERATOR_DIV){
    return 1;
  }
  else if(operator == OPERATOR_EQUAL ||operator == OPERATOR_NOT_EQUAL ||operator == OPERATOR_LT ||operator == OPERATOR_LTE ||operator == OPERATOR_GT ||operator == OPERATOR_GTE){
    return 2;

  }
  else{
    return 3;
  }
}

//
// given two ram values, performs relational operators and returns the boolean result
//
bool string_comp_helper(struct RAM_VALUE* lhs, int operator, struct RAM_VALUE* rhs) {
  
  char* lhs_value = lhs->types.s;
  char* rhs_value = rhs->types.s;
  
  switch (operator)
    {
  case OPERATOR_EQUAL:
    return (strcmp(lhs_value,rhs_value)==0);
    break;

  case OPERATOR_NOT_EQUAL:
    return (strcmp(lhs_value,rhs_value)!=0);
    break;

  case OPERATOR_LT:
    return (strcmp(lhs_value,rhs_value)<0);
    break;

  case OPERATOR_GT:
    return (strcmp(lhs_value,rhs_value)>0);
    break;

  case OPERATOR_LTE:
    return (strcmp(lhs_value,rhs_value)<=0);
    break;

  case OPERATOR_GTE:
    return (strcmp(lhs_value,rhs_value)>=0);
    break;
    }

  if (operator == OPERATOR_PLUS){
    int bytes = strlen(lhs->types.s) + strlen(rhs->types.s) + 1;  
    char* concat = (char*)malloc(bytes);
  
    strcpy(concat, lhs->types.s);
    strcat(concat, rhs->types.s);
    lhs->types.s = concat;
    return true;
  }
    

  return 0;
}

//
// takes in a stmt poiter, ram pointer, and ram_value pointer and checks which function to perform and then carries out the function, returns true if succesful and returns false with an error message if not
//
bool assignment_helper(struct STMT* stmt, struct RAM* ram, struct RAM_VALUE* ram_value){
  
  char* func_name = stmt->types.assignment->rhs->types.function_call->function_name;
  char* input = stmt->types.assignment->rhs->types.function_call->parameter->element_value;
  struct VALUE* function = stmt->types.assignment->rhs;
  
  if(strcmp(func_name, "input") == 0){
    printf("%s",input);
    char line[256];
    fgets(line,sizeof(line), stdin);

    // delete EOL chars from input:
    line[strcspn(line, "\r\n")] = '\0';
    
    ram_value->value_type = RAM_TYPE_STR;
    ram_value->types.s = line;
    return ram_write_cell_by_id(ram,*ram_value,stmt->types.assignment->var_name);

  }

  else if(strcmp(func_name, "int") == 0) {
    ram_value->value_type = RAM_TYPE_INT;
    int val_type = stmt->types.assignment->rhs->types.function_call->parameter->element_type;
    struct RAM_VALUE* val = ram_read_cell_by_id(ram,function->types.function_call->parameter->element_value);

    if (val_type == ELEMENT_IDENTIFIER) {
      if (val->value_type == RAM_TYPE_INT) {
        ram_value->types.i = val->types.i;
        return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);
      }
      else if (val->value_type == RAM_TYPE_STR) {
        if (atoi(val->types.s) == 0 && !string_check_helper(val->types.s)){
          printf("**SEMANTIC ERROR: invalid string for int() (line %d)\n", stmt->line);
          return false;
        }
        ram_value->types.i = atoi(val->types.s);
        return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);

      }
      else {
        return false;
        
      }
    }
    else if (val_type == ELEMENT_INT_LITERAL) {
      ram_value->types.i = atoi(input);
      return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);
    }
    else if (val_type == ELEMENT_STR_LITERAL) {
      if (atoi(input) == 0 && !string_check_helper(input)) {
      printf("**SEMANTIC ERROR: invalid string for int() (line %d)\n", stmt->line);
      return false;

      }
      ram_value->types.i = atoi(input);
      return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);

    }

  }
  else if(strcmp(func_name, "float") == 0) {
    ram_value->value_type = RAM_TYPE_REAL;
    int val_type = stmt->types.assignment->rhs->types.function_call->parameter->element_type;
    struct RAM_VALUE* val = ram_read_cell_by_id(ram,function->types.function_call->parameter->element_value);

    if (val_type == ELEMENT_IDENTIFIER) {
      if (val->value_type == RAM_TYPE_REAL) {
        ram_value->types.d = val->types.d;
        return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);
      }
      else if (val->value_type == RAM_TYPE_STR) {
        if (atof(val->types.s) == 0 && !string_check_helper(val->types.s)) {
          printf("**SEMANTIC ERROR: invalid string for float() (line %d)\n", stmt->line);
          return false;
        }
        ram_value->types.d = atof(val->types.s);
        return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);
      }
      else {
        return false;
        
      }
    }
    else if (val_type == ELEMENT_REAL_LITERAL) {
      ram_value->types.d = atof(input);
      return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);
    }
    else if (val_type == ELEMENT_STR_LITERAL) {
      if (atoi(val->types.s) == 0 && !string_check_helper(input)) {
      printf("**SEMANTIC ERROR: invalid string for float() (line %d)\n", stmt->line);
      return false;

      }
      ram_value->types.d = atof(input);
      return ram_write_cell_by_id(ram, *ram_value, stmt->types.assignment->var_name);

    
    }

  }

  return false;
}

//
// execute_binary_expr
//
// Given two values and an operator, performs the operation
// and updates the value in the lhs (which can be updated
// because a pointer to the value is passed in). Returns
// true if successful and false if not.
//
static bool execute_binary_expr(struct RAM_VALUE* lhs, int operator, struct RAM_VALUE* rhs, struct STMT* stmt, struct RAM* ram) 
{
  assert(operator != OPERATOR_NO_OP);

  //
  // perform the operation:
  //
  struct RAM_VALUE lhs_help;
  struct RAM_VALUE rhs_help;

  if (lhs->value_type == RAM_TYPE_NONE || rhs->value_type == RAM_TYPE_NONE){
    //printf("**EXECUTION ERROR: unexpected operator (%d) in execute_binary_expr\n", operator);
    return false;
  }

  if (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_INT && operator_helper(operator) != 3){
    if (operator_helper(operator) == 1) {
      lhs->types.i = expression_helper(lhs, operator, rhs);
    }
    else {
      lhs->types.i = conditional_eval_helper(lhs, operator, rhs);
      lhs->value_type = RAM_TYPE_BOOLEAN;
    }
  }

  else if (((lhs->value_type == RAM_TYPE_REAL && rhs->value_type == RAM_TYPE_REAL) || (lhs->value_type == RAM_TYPE_INT && rhs->value_type == RAM_TYPE_REAL) || (lhs->value_type == RAM_TYPE_REAL && rhs->value_type == RAM_TYPE_INT)) && operator_helper(operator) != 3){
    if (operator_helper(operator) == 1) {
      //printf("%d, %d\n");
      lhs->types.d = expression_helper(lhs, operator, rhs);
      lhs->value_type = RAM_TYPE_REAL;
    }
    else {
      lhs->types.i = conditional_eval_helper(lhs, operator, rhs);
      lhs->value_type = RAM_TYPE_BOOLEAN;
    }
  }

  else if (rhs->value_type == RAM_TYPE_STR && lhs->value_type == RAM_TYPE_STR && (operator == OPERATOR_PLUS || operator_helper(operator) != 3)){
    
    if (operator_helper(operator) == 1) {
      string_comp_helper(lhs, operator, rhs);
      lhs->value_type = RAM_TYPE_STR;
    }
    else if (operator_helper(operator) == 2){
      //printf("2\n");
      lhs->types.i = string_comp_helper(lhs, operator, rhs);
      lhs->value_type = RAM_TYPE_BOOLEAN;
    }
  }
  else {
    printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", stmt->line);
    return false;
  }
  
  return ram_write_cell_by_id(ram, *lhs, stmt->types.assignment->var_name);
}


//
// execute_assignment
//
// Executes an assignment statement, returning true if 
// successful and false if not (an error message will be
// output before false is returned, so the caller doesn't
// need to output anything).
// 
// Examples: x = 123
//           y = x ** 2
//
static bool execute_assignment(struct STMT* stmt, struct RAM* memory)
{
  struct STMT_ASSIGNMENT* assign = stmt->types.assignment;
  struct RAM_VALUE* value = malloc(sizeof(struct RAM_VALUE));
  char* var_name = assign->var_name;
  //struct RAM_VALUE value;

  //
  // no pointers yet:
  //
  assert(assign->isPtrDeref == false);

  //
  // we only have expressions on the RHS, no function calls:
  //

  
  if (stmt->stmt_type == STMT_ASSIGNMENT){
    if (stmt->types.assignment->rhs->value_type == VALUE_FUNCTION_CALL) {
      return assignment_helper(stmt, memory, value);
    }
  }
  
  
  assert(assign->rhs->value_type == VALUE_EXPR);

  struct VALUE_EXPR* expr = assign->rhs->types.expr;

  //
  // we always have a LHS:
  //
  assert(expr->lhs != NULL);
    
  bool success = get_unary_value(stmt, memory, expr->lhs, value);

  if (!success)  // semantic error? If so, return now:
    return false;

  //
  // do we have a binary expression?
  //

  
  if (expr->isBinaryExpr)
  {
    assert(expr->rhs != NULL);  // we must have a RHS
    assert(expr->operator != OPERATOR_NO_OP);  // we must have an operator

    struct RAM_VALUE* rhs_value = malloc(sizeof(struct RAM_VALUE));
    //struct RAM_VALUE rhs_value;

    success = get_unary_value(stmt, memory, expr->rhs, rhs_value);

    if (!success) {  // semantic error? If so, return now:
      return false;
    }

    //
    // perform the operation, updating value:
    //
    bool success = execute_binary_expr(value, expr->operator, rhs_value, stmt, memory);

    if (!success) {
      return false;
    }

    //
    // success! Fall through and write value to memory:
    //
  }
  

  //
  // write the value to memory:
  //

  success = ram_write_cell_by_id(memory, *value, var_name);

  return success;
}

//
// prints the passed in ram_value's value and returns true if succesful
//
bool print_helper(struct RAM_VALUE* value){

  if (value->value_type == RAM_TYPE_INT) {
    printf("%d\n", value->types.i);
    return true;
  }
  else if (value->value_type == RAM_TYPE_STR) {
    printf("%s\n", value->types.s);
    return true;
  }
  else if (value->value_type == RAM_TYPE_REAL){
    printf("%f\n", value->types.d);
    return true;
  }
  else if (value->value_type == RAM_TYPE_BOOLEAN){
    printf("%s\n", value->types.i ? "True" : "False");
    return true;
  }
  else 
  return false;
}

//
// execute_function_call
//
// Executes a function call statement, returning true if 
// successful and false if not (an error message will be
// output before false is returned, so the caller doesn't
// need to output anything).
// 
// Examples: print()
//           print(x)
//           print(123)
//
static bool execute_function_call(struct STMT* stmt, struct RAM* memory)
{
  struct STMT_FUNCTION_CALL* call = stmt->types.function_call;
  //
  // for now we are assuming it's a call to print:
  //
  char* function_name = call->function_name;

  if (strcmp(function_name, "print") == 0) {
  
  if (call->parameter == NULL) {
    printf("\n");
    return true;
  }
  else {
    //
    // we have a parameter, which type of parameter?
    // Note that a parameter is a simple element, i.e.
    // identifier or literal (or True, False, None):
    //
    struct RAM_VALUE* value = malloc(sizeof(struct RAM_VALUE));
    //struct RAM_VALUE value;

    bool success = get_element_value(stmt, memory, call->parameter, value);
    if(!success){
      return false;
    }
    
    success = print_helper(value);
    return true;
    
    }//else
  }
  return true;
}


//
// Public functions:
//

//
// execute
//
// Given a nuPython program graph and a memory, 
// executes the statements in the program graph.
// If a semantic error occurs (e.g. type error),
// an error message is output, execution stops,
// and the function returns.
//
void execute(struct STMT* program, struct RAM* memory)
{
  struct STMT* stmt = program;
  //
  // traverse through the program statements:
  //
  while (stmt != NULL) {

    if (stmt->stmt_type == STMT_ASSIGNMENT) {

      bool success = execute_assignment(stmt, memory);

      if (!success)
        return;

      stmt = stmt->types.assignment->next_stmt;  // advance
    }
    else if (stmt->stmt_type == STMT_FUNCTION_CALL) {
      
      bool success = execute_function_call(stmt, memory);

      if (!success)
        return;

      stmt = stmt->types.function_call->next_stmt;
    }
    else if (stmt->stmt_type == STMT_WHILE_LOOP) {
      struct RAM_VALUE* lhs_value = malloc(sizeof(struct RAM_VALUE));
      struct RAM_VALUE* rhs_value = malloc(sizeof(struct RAM_VALUE));
      
      struct VALUE_EXPR* condition = stmt->types.while_loop->condition;
      struct UNARY_EXPR* lhs = condition->lhs;
      struct UNARY_EXPR* rhs = condition->rhs;
      int operator = condition->operator;
      get_element_value(stmt, memory, lhs->element, lhs_value);
      get_element_value(stmt, memory, rhs->element, rhs_value);

      
      enum RAM_VALUE_TYPES lhs_type = lhs_value->value_type;
      enum RAM_VALUE_TYPES rhs_type = rhs_value->value_type;
      
      if (lhs_type == RAM_TYPE_STR && rhs_type == RAM_TYPE_STR) {
        if (string_comp_helper(lhs_value, operator, rhs_value)) {
          execute(stmt->types.while_loop->loop_body, memory);
          break;
        }
        else {
          stmt = stmt->types.function_call->next_stmt;
        }
      }
      else if ((lhs_type == RAM_TYPE_INT || lhs_type == RAM_TYPE_REAL) && (rhs_type == RAM_TYPE_REAL || rhs_type == RAM_TYPE_INT)) {
        if (conditional_eval_helper(lhs_value, operator, rhs_value)) {
          execute(stmt->types.while_loop->loop_body, memory);
          break;
        }
        else {
          stmt = stmt->types.function_call->next_stmt;
        }
      }
      else {
        printf("**SEMANTIC ERROR: invalid operand types (line %d)\n", stmt->line);
        break;
      }
    }
    else {
      assert(stmt->stmt_type == STMT_PASS);

      //
      // nothing to do!
      //

      stmt = stmt->types.pass->next_stmt;
    }
  }//while
  
  //
  // done:
  //
  return;
}
