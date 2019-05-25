/*
 * ltable.h
 *
 *  Created on: Nov 17, 2018
 *      Author: Jeffrey
 */

#ifndef LTABLE_LTABLE_H_
#define LTABLE_LTABLE_H_

typedef enum {
  CKey_INVALID = -1,
  CKey_resval,
  CKey_name,
  CKey_class,
  CKey_parent,
  CKey_parents,
  CKey_parent_class,
  CKey_END,
} CommonKey;

void CKey_init();
void CKey_finalize();

void CKey_set(CommonKey key, const char *str);
CommonKey CKey_lookup_key(const char *str);
const char *CKey_lookup_str(CommonKey key);

#endif /* LTABLE_LTABLE_H_ */
