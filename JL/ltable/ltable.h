/*
 * ltable.h
 *
 *  Created on: Nov 17, 2018
 *      Author: Jeffrey
 */

#ifndef LTABLE_LTABLE_H_
#define LTABLE_LTABLE_H_

// Ordered by most common lookups.
typedef enum {
  CKey_INVALID = -1,

  CKey_$ip,
  CKey_$module,
  CKey_$has_error,
  CKey_len,
  CKey_class,
  CKey_$resval,
  CKey_$adr,
  CKey_module,
  CKey_name,
  CKey_$parent,
  CKey_parents,
  CKey_parent_class,

  CKey_END,
} CommonKey;

void CKey_init();
void CKey_finalize();

CommonKey CKey_lookup_key(const char *str);
const char *CKey_lookup_str(CommonKey key);

#endif /* LTABLE_LTABLE_H_ */
