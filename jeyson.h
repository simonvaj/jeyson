#ifndef JEYSON_H
#define JEYSON_H

#include "core.h"

enum
{
	JEYSON_TYPE_NONE,
	JEYSON_TYPE_STRING,
	JEYSON_TYPE_INTEGER,
	JEYSON_TYPE_FLOAT,
	JEYSON_TYPE_ARRAY,
	JEYSON_TYPE_NUMBER,
	JEYSON_TYPE_ELEMENT,
	JEYSON_TYPE_BOOL,
	JEYSON_TYPE_NULL,
};
typedef u32 Jeyson_Type;

struct Jeyson_Element;

typedef union
{
	i64 i;
	f64 f;
	String string;
	struct Jeyson_Element* child;
	b32 b;
}
Jeyson_Value;

struct Jeyson_Element
{
	String label;
	Jeyson_Value value;
	Jeyson_Type type;
	struct Jeyson_Element* next;
};

typedef struct Jeyson_Element Jeyson_Element;

Jeyson_Element* jeyson_parse_source(Arena* arena, char const* data, u32 size);
Jeyson_Element* jeyson_child(Jeyson_Element* element);
Jeyson_Element* jeyson_next(Jeyson_Element* element);
Jeyson_Element* jeyson_find(Arena* arena, Jeyson_Element* element, String label);

#endif
