#include "thrvcc.h"

// maceo variable
struct Macro {
	struct Macro *next;
	char *name;
	struct Token *body;
	bool deleted;
};

// global Macro stack
static struct Macro *Macros;

// #if can be nested, so use the stack to hold nested #if
struct CondIncl {
	struct CondIncl *next;
	enum { IN_THEN, IN_ELIF, IN_ELSE } ctx; // type
	struct Token *token;
	bool included;
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
	if (token1->kind == TK_EOF)
		return token2;

	struct Token head = {};
	struct Token *cur = &head;

	for (; token1->kind != TK_EOF; token1 = token1->next)
		cur = cur->next = copy_token(token1);

	cur->next = token2;
	return head.next;
}

// skip #if and #endif
static struct Token *skip_condincl2(struct Token *token)
{
	while (token->kind != TK_EOF) {
		if (is_begin_hash(token) && equal(token->next, "if")) {
			token = skip_condincl2(token->next->next);
			continue;
		}
		if (is_begin_hash(token) && equal(token->next, "endif"))
			return token->next->next;
		token = token->next;
	}
	return token;
}

// if #if is empty, skip to #endif
// skip nest #if too
static struct Token *skip_condincl(struct Token *token)
{
	while (token->kind != TK_EOF) {
		// skip #if
		if (is_begin_hash(token) && equal(token->next, "if")) {
			token = skip_condincl2(token->next->next);
			continue;
		}
		// #else #endif
		if (is_begin_hash(token) &&
		    (equal(token->next, "elif") || equal(token->next, "else") ||
		     equal(token->next, "endif")))
			break;
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

static struct CondIncl *push_condincl(struct Token *token, bool included)
{
	struct CondIncl *ci = calloc(1, sizeof(struct CondIncl));
	ci->next = CondIncls;
	ci->ctx = IN_THEN;
	ci->token = token;
	ci->included = included;
	CondIncls = ci;
	return ci;
}

static struct Macro *find_macro(struct Token *token)
{
	// if not identifier, error
	if (token->kind != TK_IDENT)
		return NULL;

	// iterate Macro stack, if match return matched marco value
	for (struct Macro *m = Macros; m; m = m->next)
		if (strlen(m->name) == token->len &&
		    !strncmp(m->name, token->location, token->len))
			return m->deleted ? NULL : m;
	return NULL;
}

static struct Macro *push_macro(char *name, struct Token *body)
{
	struct Macro *m = calloc(1, sizeof(struct Macro));
	m->next = Macros;
	m->name = name;
	m->body = body;
	Macros = m;
	return m;
}

// if Macro var, expand, return true
// else NULL false
static bool expand_macro(struct Token **rest, struct Token *token)
{
	struct Macro *m = find_macro(token);
	if (!m)
		return false;
	*rest = append(m->body, token->next);
	return true;
}

// iterate terminator, process with macro and directive
static struct Token *preprocess(struct Token *token)
{
	struct Token head = {};
	struct Token *cur = &head;

	while (token->kind != TK_EOF) {
		if (expand_macro(&token, token))
			continue;

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

		// #define
		if (equal(token, "define")) {
			token = token->next;
			if (token->kind != TK_IDENT)
				error_token(token,
					    "macro name must be an identifier");
			char *name = strndup(token->location, token->len);
			push_macro(name, copy_line(&token, token->next));
			continue;
		}

		// #undef
		if (equal(token, "undef")) {
			token = token->next;
			if (token->kind != TK_IDENT)
				error_token(token,
					    "macro name must be an identifier");
			char *name = strndup(token->location, token->len);

			token = skip_line(token->next);

			struct Macro *m = push_macro(name, NULL);
			m->deleted = true;
			continue;
		}

		// #if
		if (equal(token, "if")) {
			long val = eval_constexpr(&token, token);
			push_condincl(start, val);

			// #if false
			if (!val)
				token = skip_condincl(token);
			continue;
		}

		// #elif
		if (equal(token, "elif")) {
			if (!CondIncls || CondIncls->ctx == IN_ELSE)
				error_token(start, "stray #elif");
			CondIncls->ctx = IN_ELIF;

			if (!CondIncls->included &&
			    eval_constexpr(&token, token))
				// #if false #elif true
				CondIncls->included = true;
			else
				token = skip_condincl(token);
			continue;
		}

		// #else
		if (equal(token, "else")) {
			if (!CondIncls || CondIncls->ctx == IN_ELSE)
				error_token(start, "stray #else");
			CondIncls->ctx = IN_ELSE;

			token = skip_line(token->next);

			// #if true #else, else skip
			if (CondIncls->included)
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
