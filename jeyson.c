#include "jeyson.h"

#include <stdio.h>

typedef struct
{
	Arena* arena;
	String source;
	u32 pos;
	Jeyson_Element* root;
	u32 line_index;
	b32 had_error;
}
Jeyson_Parser;

void jeyson_log_error(Jeyson_Parser* parser);
void jeyson_error(Jeyson_Parser* parser);
b32 jeyson_parser_finished(Jeyson_Parser* parser);
b32 jeyson_parse_bool(Jeyson_Parser* parser, Jeyson_Element* element);
b32 jeyson_parse_keyword(Jeyson_Parser* parser, Jeyson_Element* element, String* keyword);
b32 jeyson_parse_null(Jeyson_Parser* parser);
b32 jeyson_parse_colon(Jeyson_Parser* parser);
b32 jeyson_parse_comma(Jeyson_Parser* parser);
b32 jeyson_parse_quote(Jeyson_Parser* parser);
b32 jeyson_parse_whitespaces(Jeyson_Parser* parser);
b32 jeyson_parse_string(Jeyson_Parser* parser, String* string);
b32 jeyson_parse_minus(Jeyson_Parser* parser);
b32 jeyson_parse_plus(Jeyson_Parser* parser);
b32 jeyson_parse_token(Jeyson_Parser* parser, char token);
b32 jeyson_parse_number(Jeyson_Parser* parser, Jeyson_Element* element);
b32 jeyson_parse_number(Jeyson_Parser* parser, Jeyson_Element* element);
b32 jeyson_parse_value(Jeyson_Parser* parser, Jeyson_Element* element);
b32 jeyson_parse_element(Jeyson_Parser* parser, Jeyson_Element* element, b32 has_label);
b32 jeyson_parse_elements(Jeyson_Parser* parser, Jeyson_Element* element, char end_token, b32 have_labels);
void jeyson_init_element(Jeyson_Element* element);

// Get a line in `str`, starting from character at position `offset`.
// If `offset` is at 3rd line, and `line_index` is -1, this will return
// the second line.
b32 jeyson_get_line_relative_index(String* output, String* str, u32 offset, int line_index)
{
	b32 result = false;
	int count = 0;
	int pos = offset;
	int line = 0;
	if (line_index < 0)
	{
		line_index = -line_index;
		u32 index = str_find_char_from_reverse(str, '\n', offset);
		while (line_index && index < str->len && index > 0)
		{
			offset = index - 1;
			index = str_find_char_from_reverse(str, '\n', offset);
			line_index--;
		}
		if (line_index == 0)
		{
			if (index < str->len)
			{
				index++;
				output->buf = str->buf + index;
				u32 end_index = str_find_char_from(str, '\n', index);
				output->len = end_index - index;
			}
			else
			{
				output->buf = str->buf;
				output->len = str_find_char(str, '\n');
			}
			result = true;
		}
	}
	else if (line_index == 0)
	{
		u32 index = str_find_char_from_reverse(str, '\n', offset);
		if (index < str->len)
		{
			index++;
			output->buf = str->buf + index;
			u32 end_index = str_find_char_from(str, '\n', index);
			output->len = end_index - index;
		}
		else
		{
			output->buf = str->buf;
			output->len = str_find_char(str, '\n');
		}
		result = true;
	}
	else
	{
		u32 index = str_find_char_from(str, '\n', offset);
		while (line_index && index < str->len)
		{
			line_index--;
			offset = index + 1;
			index = str_find_char_from(str, '\n', offset);
		}
		if (line_index == 0 && offset < str->len)
		{
			output->buf = str->buf + offset;
			output->len = index - offset;
			result = true;
		}
	}
	return result;
}

void jeyson_log_error(Jeyson_Parser* parser)
{
	u32 count = 0;
	u32 pos = parser->pos;
	while (pos > 0 && parser->source.buf[pos] != '\n')
	{
		count++;
		pos--;
	}
	ScratchArena scratch = scratch_begin(parser->arena);
	u32 char_count = 120;
	u32 max_char_count_left = 80;
	char* line = ARENA_PUSH_ARRAY(parser->arena, char, char_count);
	u32 i = pos;
	if (count > max_char_count_left)
	{
		i = parser->pos - max_char_count_left;
	}
	else
	{
		i = parser->pos - count + 1;
	}
	u32 j = 0;
	char* whitespaces = ARENA_PUSH_ARRAY(parser->arena, char, char_count);
	u32 whitespace_count = 0;
	for (; count && parser->source.buf[i] != '\n'; i++, j++, count--)
	{
		line[j] = parser->source.buf[i];
		if (i == parser->pos)
		{
			whitespaces[whitespace_count] = '^';
			whitespace_count++;
		}
		else
		{
			if (line[j] == '\t')
			{
				whitespaces[whitespace_count] = '\t';
				whitespace_count++;
			}
			else if (line[j] == '\r')
			{
			}
			else
			{
				whitespaces[whitespace_count] = ' ';
				whitespace_count++;
			}
		}
	}
	u32 extra_line_count = 2;
	Str_Builder builder;
	str_builder_alloc(&builder, parser->arena, char_count * 6 * extra_line_count);
	str_append(&builder, "Jeyson parsing error on line %u, at %u: Unexpected token '%c':\n",
		parser->line_index + 1, whitespace_count, parser->source.buf[parser->pos]);
	char const* bold_red = "\033[1;31m";
	char const* gray = "\033[90m";
	char const* reset = "\033[0m";
	str_append(&builder, "%s", bold_red);
	str_append_char_count(&builder, '_', char_count);
	str_append_char(&builder, '\n');
	int line_index = (int)extra_line_count;
	line_index = -line_index;
	str_append(&builder, "%s", gray);
	for (int i = line_index; i < 0; i++)
	{
		String next_line;
		if (jeyson_get_line_relative_index(&next_line, &parser->source, parser->pos, i))
		{
			str_append_string(&builder, &next_line);
			str_append_char(&builder, '\n');
		}
	}
	str_append(&builder, "%s", bold_red);
	str_append(&builder, "%B\n", line, j);
	str_append(&builder, "%B\n", whitespaces, whitespace_count);
	str_append(&builder, "%s", gray);
	line_index = (int)extra_line_count;
	for (int i = 1; i <= line_index; i++)
	{
		String next_line;
		if (jeyson_get_line_relative_index(&next_line, &parser->source, parser->pos, i))
		{
			str_append_string(&builder, &next_line);
			str_append_char(&builder, '\n');
		}
	}
	str_append(&builder, "%s", bold_red);
	str_append_char_count(&builder, '_', char_count);
	str_append_char(&builder, '\n');
	str_append(&builder, "%s", reset);
	String message = str_build(&builder);
	log_err("%B", message.buf, message.len);
	scratch_end(&scratch);
}

void jeyson_error(Jeyson_Parser* parser)
{
	if (!parser->had_error)
	{
		jeyson_log_error(parser);
		parser->had_error = true;
	}
}

b32 jeyson_parser_finished(Jeyson_Parser* parser)
{
	b32 result = parser->pos >= parser->source.len;
	return result;
}

b32 jeyson_parse_bool(Jeyson_Parser* parser, Jeyson_Element* element)
{
	b32 result = false;
	char const* false_str = "false";
	char const* true_str = "true";
	u32 false_len = cstr_length(false_str);
	u32 true_len = cstr_length(true_str);
	if (str_equal(parser->source.buf + parser->pos, false_str, false_len))
	{
		parser->pos += false_len;
		element->value.b = false;
		result = true;
	}
	else if (str_equal(parser->source.buf + parser->pos, true_str, true_len))
	{
		parser->pos += true_len;
		element->value.b = true;
		result = true;
	}
	return result;
}

b32 jeyson_parse_keyword(Jeyson_Parser* parser, Jeyson_Element* element, String* keyword)
{
	b32 result = false;
	if (str_equal(parser->source.buf + parser->pos, keyword->buf, (u32)keyword->len))
	{
		parser->pos += (u32)keyword->len;
		element->value.b = false;
		result = true;
	}
	return result;
}

b32 jeyson_parse_null(Jeyson_Parser* parser)
{
	b32 result = false;
	char const* null_str = "null";
	u32 null_len = cstr_length(null_str);
	if (str_equal(parser->source.buf + parser->pos, null_str, null_len))
	{
		parser->pos += null_len;
		result = true;
	}
	return result;
}

b32 jeyson_parse_colon(Jeyson_Parser* parser)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == ':')
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_comma(Jeyson_Parser* parser)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == ',')
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_quote(Jeyson_Parser* parser)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == '"')
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_whitespaces(Jeyson_Parser* parser)
{
	b32 result = false;
	b32 has_whitespace;
	do
	{
		has_whitespace = false;
		char value = parser->source.buf[parser->pos];
		if (value == '\n')
		{
			parser->pos++;
			parser->line_index++;
			has_whitespace = true;
			result = true;
		}
		else if (value == ' ' || value == '\t' || value == '\r')
		{
			parser->pos++;
			has_whitespace = true;
			result = true;
		}
	} while (!jeyson_parser_finished(parser) && has_whitespace);
	return result;
}

void jeyson_init_element(Jeyson_Element* element)
{
	element->label.buf = 0;
	element->label.len = 0;
	element->value.i = 0;
	element->type = 0;
	element->next = 0;
}

b32 jeyson_parse_string(Jeyson_Parser* parser, String* string)
{
	b32 result = false;
	if (jeyson_parse_quote(parser))
	{
		b32 next_quote = false;
		// The string points at original source at the beginning, to be packed later in string arena
		string->buf = parser->source.buf + parser->pos;
		while (!jeyson_parser_finished(parser) && !next_quote)
		{
			if (parser->source.buf[parser->pos] == '"')
			{
				next_quote = true;
			}
			else
			{
				string->len++;
				parser->pos++;
			}
		}
		if (!jeyson_parser_finished(parser) && jeyson_parse_quote(parser))
		{
			result = true;
		}
	}
	return result;
}

b32 jeyson_parse_minus(Jeyson_Parser* parser)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == '-')
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_plus(Jeyson_Parser* parser)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == '+')
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_token(Jeyson_Parser* parser, char token)
{
	b32 result = false;
	if (parser->source.buf[parser->pos] == token)
	{
		parser->pos++;
		result = true;
	}
	return result;
}

b32 jeyson_parse_number(Jeyson_Parser* parser, Jeyson_Element* element)
{
	b32 result = false;
	b32 force_parse_number = false;
	b32 negative = jeyson_parse_minus(parser);
	if (negative)
	{
		if (jeyson_parse_plus(parser))
		{
			jeyson_error(parser);
		}
		else
		{
			force_parse_number = true;
		}
	}
	else if (jeyson_parse_plus(parser))
	{
		if (jeyson_parse_minus(parser))
		{
			jeyson_error(parser);
		}
		else
		{
			force_parse_number = true;
		}
	}
	if (!parser->had_error)
	{
		char const* buf = parser->source.buf + parser->pos;
		if ((*buf >= '0' && *buf <= '9') || *buf == '.')
		{
			char const* start = buf;
			char const* stop = 0;
			u64 number;
			if (str_parse_u64(&number, buf, &stop))
			{
				buf = (char*)stop;
				if (*buf == '.')
				{
					element->type = JEYSON_TYPE_FLOAT;
					buf++;
					f64 a = 1.0 / 10.0;
					f64 f = (f64)number;
					while (*buf >= '0' && *buf <= '9')
					{
						f64 i = (f64)((int)(*buf) - (int)'0');
						f += a * i;
						a *= 1.0 / 10.0;
						buf++;
					}
					if (negative)
					{
						f = -f;
					}
					element->value.f = f;
				}
				else
				{
					element->type = JEYSON_TYPE_INTEGER;
					element->value.i = (i64)number;
					if (negative)
					{
						element->value.i = -element->value.i;
					}
				}
				u32 count = (u32)(buf - start);
				parser->pos += count;
				result = true;
			}
		}
		else if (force_parse_number)
		{
			jeyson_error(parser);
		}
	}
	return result;
}

b32 jeyson_parse_value(Jeyson_Parser* parser, Jeyson_Element* element)
{
	b32 result = true;
	if (jeyson_parse_token(parser, '{'))
	{
		jeyson_parse_whitespaces(parser);
		b32 have_labels = true;
		element->type = JEYSON_TYPE_ELEMENT;
		element->value.child = ARENA_PUSH(parser->arena, Jeyson_Element);
		jeyson_init_element(element->value.child);
		if (!jeyson_parse_elements(parser, element->value.child, '}', have_labels))
		{
			jeyson_error(parser);
			result = false;
		}
	}
	else if (jeyson_parse_token(parser, '['))
	{
		jeyson_parse_whitespaces(parser);
		b32 have_labels = false;
		element->type = JEYSON_TYPE_ARRAY;
		element->value.child = ARENA_PUSH(parser->arena, Jeyson_Element);
		jeyson_init_element(element->value.child);
		if (!jeyson_parse_elements(parser, element->value.child, ']', have_labels))
		{
			jeyson_error(parser);
			result = false;
		}
	}
	else
	{
		if (jeyson_parse_number(parser, element))
		{
		}
		else if (jeyson_parse_string(parser, &element->value.string))
		{
			element->type = JEYSON_TYPE_STRING;
		}
		else if (jeyson_parse_bool(parser, element))
		{
			element->type = JEYSON_TYPE_BOOL;
		}
		else if (jeyson_parse_null(parser))
		{
			element->type = JEYSON_TYPE_NULL;
		}
		else
		{
			result = false;
		}
	}
	return result;
}

b32 jeyson_parse_element(Jeyson_Parser* parser, Jeyson_Element* element, b32 has_label)
{
	b32 result = false;
	if (has_label)
	{
		if (jeyson_parse_string(parser, &element->label))
		{
			jeyson_parse_whitespaces(parser);
			if (jeyson_parse_colon(parser))
			{
				jeyson_parse_whitespaces(parser);
			}
			else
			{
				jeyson_error(parser);
			}
		}
		else
		{
			jeyson_error(parser);
		}
	}
	if (!parser->had_error)
	{
		if (jeyson_parse_value(parser, element))
		{
			result = true;
		}
		else
		{
			jeyson_error(parser);
		}
	}
	return result;
}

b32 jeyson_parse_elements(Jeyson_Parser* parser, Jeyson_Element* element, char end_token, b32 have_labels)
{
	Assert(element);
	b32 result = false;
	b32 done = false;
	while (!jeyson_parser_finished(parser) && !parser->had_error && !done)
	{
		if (jeyson_parse_element(parser, element, have_labels))
		{
			if (jeyson_parse_comma(parser))
			{
				jeyson_parse_whitespaces(parser);
				element->next = ARENA_PUSH(parser->arena, Jeyson_Element);
				element = element->next;
			}
			else
			{
				done = true;
				jeyson_parse_whitespaces(parser);
				if (jeyson_parse_token(parser, end_token))
				{
					result = true;
				}
				else
				{
					jeyson_error(parser);
				}
			}
		}
	}
	return result;
}

void jeyson_pack_string(Arena* arena, String* string, WriteBuffer* string_buffer)
{
	String s{(char*)string_buffer->base, (u32)string_buffer->pos};
	u32 index = str_find(&s, string);
	if (index < (u32)string_buffer->pos)
	{
		string->buf = (char*)(string_buffer->base + index);
	}
	else if (string->len)
	{
		char* pointer = (char*)mim_write_buffer_pointer(string_buffer);
		mim_write_data(string_buffer, (u8*)string->buf, string->len);
		string->buf = pointer;
	}
}

/// Store strings from source to arena. If a duplicate string is found, use the previously stored string.
void jeyson_pack_strings(Arena* arena, Jeyson_Element* element, WriteBuffer* string_buffer)
{
	while (element)
	{
		jeyson_pack_string(arena, &element->label, string_buffer);
		if (element->type == JEYSON_TYPE_STRING)
		{
			jeyson_pack_string(arena, &element->value.string, string_buffer);
		}
		else if (element->type == JEYSON_TYPE_ELEMENT || element->type == JEYSON_TYPE_ARRAY)
		{
			jeyson_pack_strings(arena, element->value.child, string_buffer);
		}
		element = element->next;
	}
}

Jeyson_Element* jeyson_parse_source(Arena* arena, char const* data, u32 size)
{
	Assert(arena);
	Assert(data);
	Jeyson_Element* result = 0;
	Jeyson_Parser parser = {0};
	parser.arena = arena;
	parser.source.buf = data;
	parser.source.len = size;
	Jeyson_Element* root = ARENA_PUSH(arena, Jeyson_Element);
	jeyson_init_element(root);
	if (parser.source.buf[0] == '{' && jeyson_parse_value(&parser, root))
	{
		WriteBuffer string_buffer;
		mim_serialize_begin(&string_buffer, arena);
		jeyson_pack_strings(arena, root, &string_buffer);
		mim_serialize_end(&string_buffer);
		result = root;
	}
	else if (!parser.had_error)
	{
		jeyson_error(&parser);
	}
	return result;
}

Jeyson_Element* jeyson_child(Jeyson_Element* element)
{
	Jeyson_Element* result = 0;
	if (element->type == JEYSON_TYPE_ELEMENT || element->type == JEYSON_TYPE_ARRAY)
	{
		result = element->value.child;
	}
	return result;
}

Jeyson_Element* jeyson_next(Jeyson_Element* element)
{
	Jeyson_Element* result = element->next;
	return result;
}

Jeyson_Element* jeyson_find(Arena* arena, Jeyson_Element* element, String label)
{
	Jeyson_Element* result = 0;
	u64 max_count = arena_space(arena) / sizeof(Jeyson_Element*);
	Jeyson_Element** unvisited_elements = (Jeyson_Element**)arena_pointer(arena);
	*unvisited_elements = element;
	u32 unvisited_count = 1;
	Jeyson_Element* next = 0;
	while ((unvisited_count || next) && !result)
	{
		if (next)
		{
			element = next;
		}
		else
		{
			unvisited_count--;
			element = unvisited_elements[unvisited_count];
		}
		if (str_equal_strings(&element->label, &label))
		{
			result = element;
		}
		else
		{
			next = jeyson_next(element);
			Jeyson_Element* child = jeyson_child(element);
			if (child && unvisited_count < max_count)
			{
				unvisited_elements[unvisited_count] = child;
				unvisited_count++;
			}
		}
	}
	return result;
}
