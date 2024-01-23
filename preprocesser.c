#include "thrvcc.h"
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

// #if can be nested, so use the stack to hold nested #if
// conditional inclusion
struct CondIncl {
	struct CondIncl *next;
	struct Token *token;
};

// global #if stack
static struct CondIncl *CondIncls;

// if # at the begin of line
static bool is_begin_hash(struct Token *token)
{
	return token->at_bol && equal(token, "#");
}

// some preprocesser allow directives like #include to \
// have redundant terminators before line breaks
// skip those terminators
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

static struct Token *new_eof(struct Token *token)
{
	struct Token *t = copy_token(token);
	t->kind = TK_EOF;
	t->len = 0;
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

// if #if is empty, skip to #endif
static struct Token *skip_condincl(struct Token *token)
{
	while (token->kind != TK_EOF) {
		if (is_begin_hash(token) && equal(token->next, "endif"))
			return token;
		token = token->next;
	}
	return token;
}

// copy all terminators between the current tok and line breaks and \
// end them with EOF terminators
// this function analyzes parameters for #if
static struct Token *copy_line(struct Token **rest, struct Token *token)
{
	struct Token head = {};
	struct Token *cur = &head;

	// iterate and copy terminators
	for (; !token->at_bol; token = token->next)
		cur = cur->next = copy_token(token);

	cur->next = new_eof(token);
	*rest = token;
	return head.next;
}

// read and evaluate constant expressions.
static long eval_constexpr(struct Token **rest, struct Token *token)
{
	struct Token *start = token;
	struct Token *expr = copy_line(rest, token->next);

	// if empty directive, error out
	if (expr->kind == TK_EOF)
		error_token(start, "no expression");

	// evaluate value of constexpr
	struct Token *rest2;
	long val = const_expr(&rest2, expr);
	// if have redundant terminators, error out
	if (rest2->kind != TK_EOF)
		error_token(rest2, "extra token");

	return val;
}

static struct CondIncl *push_condincl(struct Token *token)
{
	struct CondIncl *ci = calloc(1, sizeof(struct CondIncl));
	ci->next = CondIncls;
	ci->token = token;
	CondIncls = ci;
	return ci;
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

		// start terminator
		struct Token *start = token;
		// next terminator
		token = token->next;

		// #include
		if (equal(token, "include")) {
			// jump "
			token = token->next;

			if (token->kind != TK_STR)
				error_token(token, "expected a filename");

			char *path;
			if (token->str[0] == '/')
				// absolute path
				path = token->str;
			else
				path = format(
					"%s/%s",
					dirname(strdup(token->file->name)),
					token->str);

			struct Token *token2 = lexer_file(path);
			if (!token2)
				error_token(token, "%s", strerror(errno));
			token = skip_line(token->next);
			token = append(token2, token);
			continue;
		}

		// #if
		if (equal(token, "if")) {
			long val = eval_constexpr(&token, token);
			push_condincl(start);

			// #if false
			if (!val)
				token = skip_condincl(token);
			continue;
		}

		// #endif
		if (equal(token, "endif")) {
			if (!CondIncls)
				error_token(start, "stray #endif");
			CondIncls = CondIncls->next;

			token = skip_line(token->next);
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
	// if have #if left, error
	if (CondIncls)
		error_token(CondIncls->token,
			    "unterminated conditional directive");
	convert_keywords(token);
	return token;
}
