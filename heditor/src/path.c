#define _DEFAULT_SOURCE 1
#include "path.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

#if defined(__unix__)
#	define PATH_SEP '/'
#	include <unistd.h>
#elif defined(_WIN32)
#	define PATH_SEP '\\'
#	error "TODO: Windows long path"
#else
#	error "Unsupported platform"
#endif

struct hed_path_s {
	int len;
	char name[];
};

HED_PRIVATE hed_path_t*
hed_path_alloc(hed_allocator_t* alloc, const char* content, int len) {
	hed_path_t* result = hed_malloc(sizeof(hed_path_t) + len + 1, alloc);
	result->len = len;
	memcpy(result->name, content, len);
	result->name[len] = '\0';
	return result;
}

hed_path_t*
hed_path_resolve(hed_allocator_t* alloc, const char* path) {
	// realpath will always alloc.
	// There is no safe way to use it otherwise.
	char* tmp = realpath(path, NULL);
	if (tmp == NULL) { return NULL; }

	hed_path_t* result = hed_path_alloc(alloc, tmp, (int)strlen(tmp));
	free(tmp);
	return result;
}

const char*
hed_path_as_str(const hed_path_t* path) {
	return path != NULL ? path->name : NULL;
}

int
hed_path_len(const hed_path_t* path) {
	return path != NULL ? path->len : 0;
}

hed_path_t*
hed_path_dirname(hed_allocator_t* alloc, const hed_path_t* file) {
	if (file == NULL || hed_path_is_root(file)) { return NULL; }

	int i;
	for (i = file->len - 1; i >= 0; --i) {
		if (file->name[i] == PATH_SEP) { break; }
	}

	if (i <= 0) {
#if defined(__linux__)
		return hed_path_alloc(alloc, "/", 1);
#elif defined(_WIN32)
		return NULL;
#endif
	}

	return hed_path_alloc(alloc, file->name, i);
}

const char*
hed_path_basename(hed_allocator_t* alloc, const hed_path_t* file) {
#if defined(__linux__)
	if (file == NULL || hed_path_is_root(file)) { return NULL; }

	int i;
	for (i = file->len - 1; i >= 0; --i) {
		if (file->name[i] == PATH_SEP) { break; }
	}

	if (i <= 0) { return NULL; }

	int name_len = file->len - i;
	char* name_buf = hed_malloc(name_len + 1, alloc);
	memcpy(name_buf, file->name + i + 1, name_len);
	name_buf[name_len] = '\0';
	return name_buf;
#else
#endif
}

hed_path_t*
hed_path_join(hed_allocator_t* alloc, const hed_path_t* base, const char* parts[]) {
	int tmp_buf_size = base->len + 1;
	for (int i = 0; parts[i] != NULL; ++i) {
		tmp_buf_size += strlen(parts[i]) + 1;
	}

	char* tmp_buf = hed_malloc(tmp_buf_size, alloc);
	char* pos = tmp_buf;
	memcpy(pos, base->name, base->len);
	pos += base->len;

	for (int i = 0; parts[i] != NULL; ++i) {
		pos[0] = PATH_SEP;
		++pos;

		int len = strlen(parts[i]);
		memcpy(pos, parts[i], len);
		pos += len;
	}
	pos[0] = '\0';

	hed_path_t* result = hed_path_resolve(alloc, tmp_buf);
	hed_free(tmp_buf, alloc);
	return result;
}

bool
hed_path_is_root(const hed_path_t* path) {
#if defined(__linux__)
	return path->len == 1 && path->name[0] == '/';
#else
#	error "TODO: Windows long path"
#endif
}

hed_path_t*
hed_path_current(hed_allocator_t* alloc) {
#if defined(__linux__)
	char* dir = getcwd(NULL, 0);
	hed_path_t* result = hed_path_alloc(alloc, dir, strlen(dir));
	free(dir);
	return result;
#else
#	error "TODO: Windows long path"
#endif
}

hed_path_t*
hed_path_dup(hed_allocator_t* alloc, const hed_path_t* src) {
	hed_path_t* dup = hed_malloc(sizeof(hed_path_t) + src->len + 1, alloc);
	dup->len = src->len;
	memcpy(dup->name, src->name, src->len + 1);
	return dup;
}
