#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _JsonType {
	jtype_null = 0,
	jtype_object = 1, //dict
	jtype_array = 2,
	jtype_string = 3,
	jtype_number = 4,
	jtype_boolean_true = 5,
	jtype_boolean_false = 6,
	jtype_badtype = 7,
	//extract done, but can't determine value type

	jerror_indextype = -1,
	//using object index on array
	jerror_arrayindex = -2,
	//can't parse array index
	jerror_objectname = -3,
	//can't find object name
	jerror_arrayrange = -4,
	//array out of range
	jerror_unextractable = -5,
	//unextractable object
	jerror_badjson = -6,
	//malformed json string
} JsonType;

typedef struct _JsonExtrStru {
	size_t start;
	size_t len;
	JsonType type;
} JsonExtrStru;

#define JSON_PATH_SEPARATER '\n'

// input: json string
// length: input length, 0: end with '\0'
// path: path\nto\ndestionation or array\nindex\n\n0
// return value: 
//     OK: start = start index, len = length
//     Error: start = input index at error, len = path index at error
// hint: use index on object to retrieve object name at index
JsonExtrStru json_extract(const char *input, size_t length, const char *path);

#define json_isExtractError(jsonextstru) ((jsonextstru).type < 0)

JsonType json_getType(const char *input, size_t length);

#ifdef __cplusplus
}
#endif
