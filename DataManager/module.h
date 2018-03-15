/*
 * module.h
 *
 *  Created on: Jan 1, 2017
 *      Author: Dad
 */

#ifndef MODULE_H_
#define MODULE_H_

#include <stdint.h>
#include <stdio.h>

#include "instruction.h"
#include "map.h"
#include "tape.h"
#include "tokenizer.h"

typedef struct Module_ Module;

Module *module_create(const char fn[]);
Module *module_create_file(FILE *file);
Module *module_create_tape(FileInfo *fi, Tape *tape);
const char *module_filename(const Module const *m);
const char *module_name(const Module const *m);
FileInfo *module_fileinfo(const Module const *m);
//void module_load(Module *m);
Ins module_ins(const Module *m, uint32_t index);
const InsContainer *module_insc(const Module *m, uint32_t index);
const Map *module_refs(const Module *m);
//Set *module_literals(const Module *m);
//Set *module_vars(const Module *m);
//Map *module_refs(const Module *m);
const Map *module_classes(const Module *m);
int32_t module_ref(const Module *m, const char ref_name[]);
uint32_t module_size(const Module *m);
void module_delete(Module *module);

#endif /* MODULE_H_ */
