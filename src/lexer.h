#ifndef _LEXER_H__
#define _LEXER_H__

#include <types.h>

#define LEX_ERR_UNEXPECTED_TOKEN -10

typedef struct {
	int line_num;
	size_t error_pos;
	size_t start_line;
	size_t end_line;
} Span;

typedef struct {
	size_t offset;
	size_t len;
	Span span;
} Token;

typedef struct {
	const char *stream;
	size_t len;
	size_t cur;
	bool in_parsable;
	int level;
} Lexer;

// init the lexer for this char stream
int lexer_init(Lexer *l, const char *stream, size_t len);
// get next token in the stream. Note that we skip over regular c
// tokens and only return tokens that are in our extensions:
// @use abc::def::ghi
// or
// @trait my_trait { ... }
// or
// @ my_type { ... }
int lexer_next(Lexer *l, Token *token);

#endif	// _LEXER_H__
