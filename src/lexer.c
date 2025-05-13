#include <error.h>
#include <lexer.h>
#include <sys.h>
#include <util.h>

static bool lexer_is_whitespace(Lexer *l) {
	return l->stream[l->cur] == ' ' || l->stream[l->cur] == '\t' ||
	       l->stream[l->cur] == '\r' || l->stream[l->cur] == '\n' ||
	       l->stream[l->cur] == '\v' || l->stream[l->cur] == '\f';
}

static bool lexer_is_ident_char(char ch, bool is_first) {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
	       (ch >= '0' && ch <= '9' && !is_first) || ch == '_';
}

static int lexer_parsable(Lexer *l, Token *token) {
	size_t start = l->cur;
	while (l->cur < l->len && !lexer_is_whitespace(l)) {
		if (start == l->cur) {
			if (!lexer_is_ident_char(l->stream[l->cur], true)) {
				l->cur++;
				break;
			}
		} else if (!lexer_is_ident_char(l->stream[l->cur], false)) {
			break;
		}

		l->cur++;
	}
	token->len = l->cur - start;
	token->offset = start;

	if (token->len == 1 && l->stream[start] == '{')
		l->level++;
	else if (token->len == 1 && l->stream[start] == '}') {
		if (l->level == 0) {
			err = LEX_ERR_UNEXPECTED_TOKEN;
			return -1;
		}
		l->level--;
		if (l->level == 0) l->in_parsable = false;
	} else if (token->len == 1 && l->stream[start] == ';' &&
		   l->level == 0) {
		l->in_parsable = false;
	}

	return 0;
}

static int lexer_unparsable(Lexer *l, Token *token) {
	token->len = 0;
	while (l->cur < l->len && !l->in_parsable) {
		if (l->stream[l->cur] == '@') {
			token->offset = l->cur;
			token->len = 1;
			l->in_parsable = true;
		}
		l->cur++;
	}
	return 0;
}

static void lexer_skip_whitespace(Lexer *l) {
	while (l->cur < l->len) {
		if (!lexer_is_whitespace(l)) break;
		l->cur++;
	}
}

int lexer_init(Lexer *l, const char *stream, size_t len) {
	if (l == NULL || stream == NULL) {
		err = ERR_EINVAL;
		return err;
	}
	l->stream = stream;
	l->cur = 0;
	l->len = len;
	l->in_parsable = false;
	l->level = 0;
	return 0;
}

int lexer_next(Lexer *l, Token *token) {
	if (l == NULL || token == NULL) {
		err = ERR_EINVAL;
		return err;
	}
	lexer_skip_whitespace(l);

	if (l->cur == l->len) {
		token->len = 0;
		return 0;
	}

	if (l->in_parsable)
		return lexer_parsable(l, token);
	else
		return lexer_unparsable(l, token);
}

int printf(const char *, ...);
#define MAX_LEN 1024

int lexer(int argc, char **argv) {
	printf("ps=%i\n", PAGE_SIZE);
	if (argc != 2) {
		printf("usage: lexer [file]\n");
		return -1;
	}
	int fd = openfd(argv[1], OPEN_RDONLY);
	if (fd < 0) {
		printf("error opening file: %i\n", fd);
		return -1;
	}
	char *data = fmap(fd, 0, 1);
	if (data == NULL) {
		printf("error mapping file to memory: %i\n", err);
		return -1;
	}

	Lexer l;
	if (lexer_init(&l, data, fsize(fd))) {
		printf("error initializing lexer: %i", err);
		return -1;
	}

	Token token = {0};
	int token_count = 0;
	char buf[MAX_LEN];
	while (true) {
		if (lexer_next(&l, &token)) {
			printf("parse err=%i\n", err);
			break;
		}

		copy_bytes((byte *)buf, (byte *)(data + token.offset),
			   token.len);
		buf[token.len] = '\0';
		printf("token[%i]='%s' (%zu)\n", token_count, buf, token.len);

		if (token.len == 0) break;
		token_count++;
	}

	closefd(fd);

	return 0;
}
