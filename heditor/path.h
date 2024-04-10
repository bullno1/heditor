#ifndef HEDITOR_PATH_H
#define HEDITOR_PATH_H

#include "allocator/allocator.h"
#include <stdbool.h>

typedef struct hed_path_s hed_path_t;

hed_path_t*
hed_path_resolve(hed_allocator_t* alloc, const char* path);

hed_path_t*
hed_path_dirname(hed_allocator_t* alloc, const hed_path_t* file);

const char*
hed_path_basename(hed_allocator_t* alloc, const hed_path_t* file);

hed_path_t*
hed_path_join(hed_allocator_t* alloc, const hed_path_t* base, const char* parts[]);

bool
hed_path_is_root(const hed_path_t* path);

const char*
hed_path_as_str(const hed_path_t* path);

int
hed_path_len(const hed_path_t* path);

#endif
