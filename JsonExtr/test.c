#include "jsonextr.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void printpath(const char *path) {
	bool running = true;
	do {
		bool isarray = false;
		if (path[0] == '\n') {
			isarray = true;
			path++;
		}
		const char *nextpath = strchr(path, '\n');
		if (nextpath == NULL) {
			running = false;
			nextpath = strchr(path, '\0');
		}
		if (isarray) {
			printf("[%.*s]", (int)(nextpath - path), path);
		} else {
			printf("[\"%.*s\"]", (int)(nextpath - path), path);
		}
		path = nextpath + 1;
	} while (running);
	printf("\n");
}

void extracttest(const char *json, const char *path) {
	const char types[8][10] = {"null", "object", "array", "string", "number", "true", "false", "badtype"};
	const char errors[6][20] = {"index type", "bad array index", "object name", "array range", "unextractable", "bad json"};
	printf("%s -> j", json);
	printpath(path);
	JsonExtrStru s = json_extract(json, 0, path);
	if (json_isExtractError(s)) {
		printf("Error #%d: %s\n", -s.type, errors[-1 - s.type]);
	} else {
		for (size_t i = 0; i < s.start; i++) putchar(' ');
		for (size_t i = 0; i < s.len; i++) putchar('~');
		printf("\nType: %s\n", types[s.type]);
	}
	printf("\n");
}

void iterate_helper(const char *json, size_t jsonlen, JsonType type, size_t indent) {
	const char types[8][10] = {"null", "object", "array", "string", "number", "true", "false", "badtype"};
	char path[32];
	size_t index = 0;
	JsonExtrStru s, s1;
	switch (type) {
	case jtype_array:
		while (true) {
			snprintf(path, 32, "\n%d", index);
			s = json_extract(json, jsonlen, path);
			if (json_isExtractError(s)) break;
			for (size_t i = 0; i < indent; i++) putchar(' ');
			printf("#%d: %.*s (%s)\n", index, s.len, json + s.start, types[s.type]);
			iterate_helper(json + s.start, s.len, s.type, indent + 4);
			index++;
		}
		break;
	case jtype_object:
		while (true) {
			//get property name
			snprintf(path, 32, "\n%d", index);
			s1 = json_extract(json, jsonlen, path);
			if (s1.type != jtype_string) break;
			//get content
			snprintf(path, 32, "%.*s", s1.len - 2, json + s1.start + 1);
			s = json_extract(json, jsonlen, path);
			for (size_t i = 0; i < indent; i++) putchar(' ');
			printf("\"%.*s\": %.*s (%s)\n", s1.len - 2, json + s1.start + 1, s.len, json + s.start, types[s.type]);
			iterate_helper(json + s.start, s.len, s.type, indent + 4);
			index++;
		}
		break;
	default:
		break;
	}
}

void iterate(const char *json) {
	size_t len = strlen(json);
	iterate_helper(json, len, json_getType(json, len), 0);
}

int main() {
	const char json[] = "{ \"key1\" : [0,1,2, 3 ,4] , \"key2\":{\"2\": true } }";
	extracttest(json, "\n0");
	extracttest(json, "key1");
	extracttest(json, "key1\n\n3");
	extracttest(json, "key1\n\n4");
	extracttest(json, "key1\n\n5");
	extracttest(json, "key1\n3");
	extracttest(json, "\n1");
	extracttest(json, "key2");
	extracttest(json, "key2\n\n0");
	extracttest(json, "key2\n2");
	extracttest(json, "key2\n\n2");
	extracttest(json, "\n2");
	extracttest("[ ]", "\n0");
	extracttest("[ ]", "\n1");
	extracttest("{ }", "\n0");
	extracttest("{ }", "\n1");

	const char json1[] = "[0,1,2,[3,4],{\"key1\":[\"zero\",-1,2,[],{}],\"key2\":{\"x\":true,\"y\":false,\"z\":null,\"test\":\"a\\\"\\\\b\\\\\"}}]";
	printf("%s\n", json1);
	iterate(json1);

	getchar();
	return 0;
}
