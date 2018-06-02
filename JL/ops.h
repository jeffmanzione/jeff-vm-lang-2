/*
 * ops.h
 *
 *  Created on: Dec 11, 2016
 *      Author: Jeff
 */

#ifndef OPS_H_
#define OPS_H_

#include "element.h"
#include "error.h"

#define VAL_OF(val) ((INT == val.type)   ? val.int_val  : \
                    ((FLOAT == val.type) ? val.float_val :  val.char_val))
#define VAL_OF_INT(val) ((INT == val.type) ? val.int_val :  val.char_val)
#define APPLY(lhv, rhv, op) (VAL_OF(lhv) op VAL_OF(rhv))
#define APPLY_INT(lhv, rhv, op) (VAL_OF_INT(lhv) op VAL_OF_INT(rhv))
#define OPDEF(name) Element operator_##name(Element lhe, Element rhe)
#define OPDEF_BOOLTYPE(name) Element operator_##name(VM *vm, Element lhe, Element rhe)
#define OPDEF_SINGLE(name) Element operator_##name(Element elt)

#define OPFUNC_INTTYPE(op, name)                           \
  OPDEF(name) {                                            \
    if (lhe.type == OBJECT || lhe.type == NONE             \
     || rhe.type == OBJECT || rhe.type == NONE             \
     || lhe.val.type == FLOAT || rhe.val.type == FLOAT)    \
      ERROR("Invalid operand (FLOAT or NONE).");           \
    if (lhe.val.type == INT || rhe.val.type == INT)        \
      return create_int(APPLY_INT(lhe.val, rhe.val, op));  \
    return create_char(APPLY_INT(lhe.val, rhe.val,  op));  \
  }


/*    if (lhs.type == OBJECT && rhs.type == OBJECT &&       \
        lhs.obj == class_string.obj && rhs.obj == class_string.obj) \
      return string_##name(lhs, rhs);                     \ */

#define OPFUNC(op, name)                                   \
  OPDEF(name) {                                            \
     if (lhe.type == OBJECT || lhe.type == NONE            \
     || rhe.type == OBJECT || rhe.type == NONE)            \
      ERROR("Invalid operand (None).");                    \
    if (lhe.val.type == FLOAT || rhe.val.type == FLOAT)    \
      return create_float(APPLY(lhe.val, rhe.val, op));    \
    if (lhe.val.type == INT || rhe.val.type == INT)        \
      return create_int(APPLY(lhe.val, rhe.val, op));      \
    return create_char(APPLY(lhe.val, rhe.val,  op));      \
  }

#define OPFUNC_BOOLTYPE(op, name)                          \
  OPDEF_BOOLTYPE(name) {                                   \
    if (lhe.type == OBJECT || lhe.type == NONE             \
     || rhe.type == OBJECT || rhe.type == NONE)            \
      ERROR("Invalid operand (None).");                    \
    return APPLY(lhe.val, rhe.val,  op) == 0               \
      ? element_false(vm) : element_true(vm);              \
  }

#define OPFUNC_BOOLTYPE_SINGLE(op, name)                   \
  OPDEF_SINGLE(name) {                                     \
    if (elt.type == OBJECT || elt.type == NONE)            \
      ERROR("Invalid operand (None).");                    \
    return (op (VAL_OF(elt.val))) == 0                     \
      ? create_none() : create_int(1);                     \
  }

#define OPFUNC_BOOLTYPE_SINGLE_NIL(op, name)               \
  OPDEF_SINGLE(name) {                                     \
    if (elt.type == OBJECT)                                \
      ERROR("Invalid operand (Object).");                  \
    return (op (elt));                                     \
  }

OPDEF(add);
OPDEF(sub);
OPDEF(mult);
OPDEF(div);
OPDEF(mod);
OPDEF_BOOLTYPE(eq);
OPDEF_BOOLTYPE(neq);
OPDEF_BOOLTYPE(gt);
OPDEF_BOOLTYPE(gte);
OPDEF_BOOLTYPE(lt);
OPDEF_BOOLTYPE(lte);
OPDEF_BOOLTYPE(and);
OPDEF_BOOLTYPE(or);
//OPDEF_SINGLE(not);
OPDEF_SINGLE(notc);

#endif /* OPS_H_ */
