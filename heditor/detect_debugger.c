#include "detect_debugger.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __linux__

bool
is_debugger_attached(void) {
	char buf[4096];

	FILE* file = fopen("/proc/self/status", "rb");
	if (file == NULL) { return false; }

	size_t size = fread(buf, sizeof(char), sizeof(buf), file);
	fclose(file);

	if (size == 0) { return false; }
	buf[size - 1] = '\0';

	const char* pos = strstr(buf, "TracerPid:");
	if (pos == NULL) { return false; }
	pos += sizeof("TracerPid:");

	while (pos < buf + sizeof(buf)) {
		if (isspace(*pos)) {
			++pos;
			continue;
		} else if (*pos == '0') {
			return false;
		} else {
			return true;
		}
	}

	return false;
}

#endif
