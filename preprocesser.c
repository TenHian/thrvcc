#include "thrvcc.h"
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

// if # at the begin of line
static bool is_begin_hash(struct Token *token)
{
	return token->at_bol && equal(token, "#");
}

// some preprocesser allow directives like #include to \
// have redundant terminators before line breaks
// skip those therminators
static struct Token *skip_line(struct Token *token)
{
	if (token->at_bol)
		return token;
	warn_token(token, "extra token");
	while (!token->at_bol)
		token = token->next;
	return token;
}
static struct Token *copy_token(struct Token *token)
{
	struct Token *t = calloc(1, sizeof(struct Token));
	*t = *token;
	t->next = NULL;
	return t;
}

static struct Token *append(struct Token *token1, struct Token *token2)
{
	if (!token1 || token1->kind == TK_EOF)
		return token2;

	struct Token head = {};
	struct Token *cur = &head;

	for (; token1 && token1->kind != TK_EOF; token1 = token1->next)
		cur = cur->next = copy_token(token1);

	cur->next = token2;
	return head.next;
}

// iterate terminator, process with macro and directive
static struct Token *preprocess(struct Token *token)
{
	struct Token head = {};
	struct Token *cur = &head;

	while (token->kind != TK_EOF) {
		if (!is_begin_hash(token)) {
			cur->next = token;
			cur = cur->next;
			token = token->next;
			continue;
		}

		token = token->next;

		// #include
		if (equal(token, "include")) {
			// jump "
			token = token->next;

			if (token->kind != TK_STR)
				error_token(token, "expected a filename");

			char *path = format("%s/%s",
					    dirname(strdup(token->file->name)),
					    token->str);
			struct Token *token2 = lexer_file(path);
			if (!token2)
				error_token(token, "%s", strerror(errno));
			token = skip_line(token->next);
			token = append(token2, token);
			continue;
		}

		if (token->at_bol)
			continue;

		error_token(token, "invalid preprocesser directive");
	}

	cur->next = token;
	return head.next;
}
// preprocesser entry func
struct Token *preprocesser(struct Token *token)
{
	token = preprocess(token);
	convert_keywords(token);
	return token;
}
