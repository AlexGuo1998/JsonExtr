# JsonExtr
A very lightweight _(300 lines)_ JSON parser for `C`

# Usage
Add `jsonextr.c` to your build environment, and `#include "jsonextr.h"`.

Alternatively, open `JsonExtracter.sln` with `Visual Studio 2017` to try tests.

## Extract value from objects (dicts)

Example: get `json["key1"]["b"]` from

```
{"key1":{"a":"a","b":"b"},"key2":"val2"}
```

Code:

```
const char *json = "{\"key1\":{\"a\":\"a\",\"b\":\"b\"},\"key2\":\"val2\"}";
const char *path = "key1\nb";
JsonExtrStru s = json_extract(json, 0, path);
if (json_isExtractError(s)) {
	printf("Error #%d\n", -s.type);
} else {
	printf("%s\n", json);
	for (size_t i = 0; i < s.start; i++) putchar(' ');
	for (size_t i = 0; i < s.len; i++) putchar('~');
	printf("\nType: %d\n", s.type);
}
```

Output:
```
{"key1":{"a":"a","b":"b"},"key2":"val2"}
                     ~~~
Type: 3
```

## Extract value from lists

Example: get `json[0][1]` from

```
[[0,1,2],[3,4,5]]
```

Code:

```
const char *path = "\n0\n\n1";
```

Notice the additional `\n`.

_Hint: use numeric key on objects to retrieve object name._

## Iterate object properties (dict keys)

Code: (See `test.c`)
