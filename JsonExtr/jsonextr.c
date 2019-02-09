#define _CRT_SECURE_NO_WARNINGS //suppress sscanf warnings

#include "jsonextr.h"
#include <stdbool.h>
#include <string.h> //for strlen, strchr, memcmp
#include <stdio.h> //for sscanf

static inline size_t skipspace(const char *input, size_t offset, size_t length) {
	if (offset == (size_t)-1) {
		return (size_t)-1;
	}
	while (input[offset] == '\x09'
		|| input[offset] == '\x0A'
		|| input[offset] == '\x0D'
		|| input[offset] == '\x20') {
		offset++;
		if (offset >= length) {
			return (size_t)-1;
		}
	}
	return offset;
}

static inline size_t skipstring(const char *input, size_t offset, size_t length) {
	while (input[offset] != '"' || input[offset - 1] == '\\') {
		offset++;
		if (offset >= length) {
			return (size_t)-1;
		}
	}
	return offset + 1;
}

static size_t skipitem(const char *input, size_t offset, size_t length) {
	offset = skipspace(input, offset, length);
	if (offset == (size_t)-1) {
		return (size_t)-1;
	}
	switch (input[offset]) {
	case '[':
		//array as item
		//[item, item, ... ]
		offset = skipspace(input, offset + 1, length);
		if (offset == (size_t)-1) {
			return (size_t)-1;
		}
		if (input[offset] == ']') {
			//empty array
			return offset + 1;
		}
		while (true) {
			offset = skipitem(input, offset, length);
			offset = skipspace(input, offset, length);
			if (offset == (size_t)-1) {
				return (size_t)-1;
			}
			if (input[offset] != ',') {
				if (input[offset] == ']') {
					return offset + 1;
				}
				return (size_t)-1;
			}
			offset += 1;
		}
		break;
	case '{':
		//object as item
		//{"string":item, ...}
		offset = skipspace(input, offset + 1, length);
		if (offset == (size_t)-1) {
			return (size_t)-1;
		}
		if (input[offset] == '}') {
			//empty object
			return offset + 1;
		}
		while (true) {
			//skip name(string)
			if (offset == (size_t)-1 || input[offset] != '"') {
				return (size_t)-1;
			}
			offset = skipstring(input, offset + 1, length);
			offset = skipspace(input, offset, length);
			if (offset == (size_t)-1 || input[offset] != ':') {
				return (size_t)-1;
			}
			//skip value
			offset = skipitem(input, offset + 1, length);
			offset = skipspace(input, offset, length);
			if (offset == (size_t)-1) {
				return (size_t)-1;
			}
			if (input[offset] != ',') {
				if (input[offset] == '}') {
					return offset + 1;
				}
				return (size_t)-1;
			}
			offset = skipspace(input, offset + 1, length);
		}
		break;
	case '"':
		//string as item
		return skipstring(input, offset + 1, length);
	default:
		//normal data (num, bool, none)
		while (input[offset] != ','
			&& input[offset] != '}'
			&& input[offset] != ']') {
			offset++;
			if (offset >= length) {
				return (size_t)-1;
			}
		}
		//find last non-space char
		do {
			offset--;
		} while (input[offset] == '\x09'
			|| input[offset] == '\x0A'
			|| input[offset] == '\x0D'
			|| input[offset] == '\x20');
		return offset + 1;
	case ',': case '}': case ']':
		//bad json
		return (size_t)-1;
	}
	return 0;
}

JsonExtrStru json_extract(const char *input, size_t length, const char *path) {
	if (length == 0) {
		length = strlen(input);
	}
	bool running = true;
	size_t pathindex = 0;
	size_t jsonindex = 0;
	do {
		//strip spaces
		jsonindex = skipspace(input, jsonindex, length);
		if (jsonindex == (size_t)-1) {
			return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
		}
		//parse path
		bool isarrayindex = false;
		size_t arrayindex = 0;
		if (path[pathindex] == JSON_PATH_SEPARATER) {
			//double separater: array index
			isarrayindex = true;
			pathindex++;
			//parse array index
			int ret = sscanf(&path[pathindex], "%zd", &arrayindex);
			if (ret < 1) {
				//ERROR: can't parse array index
				return (JsonExtrStru){jsonindex, pathindex, jerror_arrayindex};
			}
		}
		const char *nextpath = strchr(&path[pathindex], JSON_PATH_SEPARATER);
		if (nextpath == NULL) {
			running = false;
			nextpath = strchr(&path[pathindex], '\0');
		}
		if (input[jsonindex] == '[') {
			if (!isarrayindex) {
				return (JsonExtrStru){jsonindex, pathindex, jerror_indextype};
			}
			jsonindex++;
			//skip item
			for (size_t i = 0; i < arrayindex; i++) {
				jsonindex = skipitem(input, jsonindex, length);
				jsonindex = skipspace(input, jsonindex, length);
				if (jsonindex == (size_t)-1) {
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				if (input[jsonindex] != ',') {
					if (input[jsonindex] == ']') {
						return (JsonExtrStru) { jsonindex, pathindex, jerror_arrayrange };
					}
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				jsonindex++;
			}
			size_t end;
			jsonindex = skipspace(input, jsonindex, length);
			end = skipitem(input, jsonindex, length);
			if (end == (size_t)-1) {
				return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
			}
			length = end;
		} else if (input[jsonindex] == '{') {
			size_t namelength = (nextpath - path) - pathindex;
			jsonindex = skipspace(input, jsonindex + 1, length);
			bool found = false;
			while (!isarrayindex || arrayindex > 0) {
				//compare name
				if (jsonindex == (size_t)-1 || input[jsonindex] != '"') {
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				size_t jsonindex1 = skipstring(input, jsonindex + 1, length);//after name
				if (!isarrayindex) {
					size_t realnamelength = jsonindex1 - jsonindex - 2;
					if (realnamelength == namelength && memcmp(&path[pathindex], &input[jsonindex + 1], namelength) == 0) {
						//hit
						found = true;
					}
				}

				jsonindex = skipspace(input, jsonindex1, length);
				if (jsonindex == (size_t)-1 || input[jsonindex] != ':') {
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				//skip value
				jsonindex++;
				if (found) break;
				jsonindex = skipitem(input, jsonindex, length);
				jsonindex = skipspace(input, jsonindex, length);
				if (jsonindex == (size_t)-1) {
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				if (input[jsonindex] != ',') {
					if (input[jsonindex] == '}') {
						if (isarrayindex) {
							return (JsonExtrStru){jsonindex, pathindex, jerror_arrayrange};
						} else {
							return (JsonExtrStru){jsonindex, pathindex, jerror_objectname};
						}
					}
					return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
				}
				jsonindex = skipspace(input, jsonindex + 1, length);
				arrayindex--;
			}
			//found
			size_t end;
			jsonindex = skipspace(input, jsonindex, length);
			end = skipitem(input, jsonindex, length);
			if (end == (size_t)-1) {
				return (JsonExtrStru){jsonindex, pathindex, jerror_badjson};
			}
			length = end;
		} else {
			return (JsonExtrStru){jsonindex, pathindex, jerror_indextype};
		}
		pathindex = nextpath - path + 1;
	} while (running);
	//done
	return (JsonExtrStru){jsonindex, length - jsonindex, json_getType(&input[jsonindex], length - jsonindex)};
}

JsonType json_getType(const char *input, size_t length) {
	switch (input[0]) {
	case '[':
		return jtype_array;
	case '{':
		return jtype_object;
	case '"':
		return jtype_string;
	case 't':
		if (length == 4 && memcmp(&input[1], "rue", 3) == 0) {
			return jtype_boolean_true;
		} else {
			return jtype_badtype;
		}
	case 'f':
		if (length == 5 && memcmp(&input[1], "alse", 4) == 0) {
			return jtype_boolean_false;
		} else {
			return jtype_badtype;
		}
	case 'n':
		if (length == 4 && memcmp(&input[1], "ull", 3) == 0) {
			return jtype_null;
		} else {
			return jtype_badtype;
		}
	default:
		if (input[0] >= '0' && input[0] <= '9' || input[0] == '-') {
			return jtype_number;
		} else {
			return jtype_badtype;
		}
	}
}
