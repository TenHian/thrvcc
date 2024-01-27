#include "thrvcc.h"

struct MacroParam {
	struct MacroParam *next;
	char *name;
};

struct MacroArg {
	struct MacroArg *next;
	char *name;
	struct Token *token;
};

// maceo variable
struct Macro {
	struct Macro *next;
	char *name;
	bool is_obj_like; // macro var->true, macro func->false
	struct MacroParam *params;
	struct Token *body;
	bool deleted;
};

// #if can be nested, so use the stack to hold nested #if
struct CondIncl {
	struct CondIncl *next;
	enum { IN_THEN, IN_ELIF, IN_ELSE } ctx; // type
	struct Token *token;
	bool included;
};

// global Macro stack
static struct Macro *Macros;
// global #if stack
static struct CondIncl *CondIncls;

static struct Macro *find_macro(struct Token *token);

static struct Token *preprocess(struct Token *token);

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

static struct HideSet *new_hideset(char *name)
{
	struct HideSet *hs = calloc(1, sizeof(struct HideSet));
	hs->name = name;
	return hs;
}

static struct HideSet *hideset_union(struct HideSet *hs1, struct HideSet *hs2)
{
	struct HideSet head = {};
	struct HideSet *cur = &head;

	for (; hs1; hs1 = hs1->next)
		cur = cur->next = new_hideset(hs1->name);
	cur->next = hs2;
	return head.next;
}

// if already in hideset
static bool hideset_contains(struct HideSet *hs, char *s, int len)
{
	for (; hs; hs = hs->next)
		if (strlen(hs->name) == len && !strncmp(hs->name, s, len))
			return true;
	return false;
}

static struct HideSet *hideset_intersection(struct HideSet *hs1,
					    struct HideSet *hs2)
{
	struct HideSet head = {};
	struct HideSet *cur = &head;

	for (; hs1; hs1 = hs1->next)
		if (hideset_contains(hs2, hs1->name, strlen(hs1->name)))
			cur = cur->next = new_hideset(hs1->name);
	return head.next;
}

// iterate all terminators after token, assign hs for every terminator
static struct Token *add_hideset(struct Token *token, struct HideSet *hs)
{
	struct Token head = {};
	struct Token *cur = &head;

	for (; token; token = token->next) {
		struct Token *t = copy_token(token);
		t->hideset = hideset_union(t->hideset, hs);
		cur = cur->next = t;
	}
	return head.next;
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
		if (is_begin_hash(token) &&
		    (equal(token->next, "if") || equal(token->next, "ifdef") ||
		     equal(token->next, "ifndef"))) {
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
		if (is_begin_hash(token) &&
		    (equal(token->next, "if") || equal(token->next, "ifdef") ||
		     equal(token->next, "ifndef"))) {
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

static char *quote_string(char *str)
{
	int buf_size = 3;
	// if have \ or ", then need one more space to store escape sign
	for (int i = 0; str[i]; i++) {
		if (str[i] == '\\' || str[i] == '"')
			buf_size++;
		buf_size++;
	}
	char *buf = calloc(1, buf_size);
	char *p = buf;
	// " at begin
	*p++ = '"';
	for (int i = 0; str[i]; i++) {
		if (str[i] == '\\' || str[i] == '"')
			*p++ = '\\';
		*p++ = str[i];
	}
	// "\0 at end
	*p++ = '"';
	*p++ = '\0';
	return buf;
}

// construct a new str terminator
static struct Token *new_str_token(char *str, struct Token *tmpl)
{
	char *buf = quote_string(str);
	// pass the string and the corresponding macro name into
	// the lexical analysis to parse it
	return lexer(new_file(tmpl->file->name, tmpl->file->file_no, buf));
}

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

static struct Token *new_num_token(int val, struct Token *tmpl)
{
	char *buf = format("%d\n", val);
	return lexer(new_file(tmpl->file->name, tmpl->file->file_no, buf));
}

static struct Token *read_const_expr(struct Token **rest, struct Token *token)
{
	token = copy_line(rest, token);
	struct Token head = {};
	struct Token *cur = &head;

	while (token->kind != TK_EOF) {
		// "defined(foo)" or "defined foo", if foo exists, 1 ,else 0.
		if (equal(token, "defined")) {
			struct Token *start = token;
			// consume (
			bool has_paren = consume(&token, token->next, "(");

			if (token->kind != TK_IDENT)
				error_token(start,
					    "macro name must be an identifier");
			struct Macro *m = find_macro(token);
			token = token->next;

			if (has_paren)
				token = skip(token, ")");

			cur = cur->next = new_num_token(m ? 1 : 0, start);
			continue;
		}
		cur = cur->next = token;
		token = token->next;
	}
	cur->next = token;
	return head.next;
}

// read and evaluate constant expressions.
static long eval_constexpr(struct Token **rest, struct Token *token)
{
	struct Token *start = token;
	struct Token *expr = read_const_expr(rest, token->next);
	// parse Macro var
	expr = preprocess(expr);

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

static struct Macro *push_macro(char *name, bool is_obj_like,
				struct Token *body)
{
	struct Macro *m = calloc(1, sizeof(struct Macro));
	m->next = Macros;
	m->name = name;
	m->is_obj_like = is_obj_like;
	m->body = body;
	Macros = m;
	return m;
}

static struct MacroParam *read_macro_params(struct Token **rest,
					    struct Token *token)
{
	struct MacroParam head = {};
	struct MacroParam *cur = &head;

	while (!equal(token, ")")) {
		if (cur != &head)
			token = skip(token, ",");
		if (token->kind != TK_IDENT)
			error_token(token, "expected an identifier");
		struct MacroParam *m = calloc(1, sizeof(struct MacroParam));
		m->name = strndup(token->location, token->len);
		cur = cur->next = m;
		token = token->next;
	}
	*rest = token->next;
	return head.next;
}

static void read_macro_def(struct Token **rest, struct Token *token)
{
	if (token->kind != TK_IDENT)
		error_token(token, "marco name must be an identifier");
	char *name = strndup(token->location, token->len);
	token = token->next;
	// determine if macro var of macro func.
	// macro func if there no space before ()
	if (!token->has_space && equal(token, "(")) {
		// construct params
		struct MacroParam *params =
			read_macro_params(&token, token->next);
		// push macro func
		struct Macro *m =
			push_macro(name, false, copy_line(rest, token));
		m->params = params;
	} else {
		push_macro(name, true, copy_line(rest, token));
	}
}

static struct MacroArg *read_one_macro_arg(struct Token **rest,
					   struct Token *token)
{
	struct Token head = {};
	struct Token *cur = &head;
	int level = 0;

	while (level > 0 || !equal(token, ",") && !equal(token, ")")) {
		if (token->kind == TK_EOF)
			error_token(token, "premature end of input");

		if (equal(token, "("))
			level++;
		else if (equal(token, ")"))
			level--;

		cur = cur->next = copy_token(token);
		token = token->next;
	}

	cur->next = new_eof(token);

	struct MacroArg *arg = calloc(1, sizeof(struct MacroArg));
	arg->token = head.next;
	*rest = token;
	return arg;
}

static struct MacroArg *read_macro_args(struct Token **rest,
					struct Token *token,
					struct MacroParam *params)
{
	struct Token *start = token;
	token = token->next->next;

	struct MacroArg head = {};
	struct MacroArg *cur = &head;

	struct MacroParam *mp = params;
	for (; mp; mp = mp->next) {
		if (cur != &head)
			token = skip(token, ",");
		cur = cur->next = read_one_macro_arg(&token, token);
		cur->name = mp->name;
	}
	// if left, error
	if (mp)
		error_token(start, "too many arguments");
	skip(token, ")");
	// return )
	*rest = token;
	return head.next;
}

static struct MacroArg *find_macro_arg(struct MacroArg *args,
				       struct Token *token)
{
	for (struct MacroArg *ma = args; ma; ma = ma->next)
		if (token->len == strlen(ma->name) &&
		    !strncmp(token->location, ma->name, token->len))
			return ma;
	return NULL;
}

// concatenates all the terminators in the terminator chain table
// and returns a new string
static char *join_tokens(struct Token *token)
{
	// calculate the length of the final terminator
	int len = 1;
	for (struct Token *t = token; t && t->kind != TK_EOF; t = t->next) {
		// not the first, and preceded by a space, \
		// the count is increased by one
		if (t != token && t->has_space)
			len++;
		len += t->len;
	}
	char *buf = calloc(1, len);
	// copy
	int pos = 0;
	for (struct Token *t = token; t && t->kind != TK_EOF; t = t->next) {
		// not the first, and preceded by a space, \
		// set space
		if (t != token && t->has_space)
			buf[pos++] = ' ';
		strncpy(buf + pos, t->location, t->len);
		pos += t->len;
	}
	// end with '\0'
	buf[pos] = '\0';
	return buf;
}

// concatenates the terminators in all args and \
// returns a string terminator
static struct Token *stringize(struct Token *hash, struct Token *arg)
{
	char *s = join_tokens(arg);
	return new_str_token(s, hash);
}

// splicing two terminators to build a new one
static struct Token *splice_terminators(struct Token *lhs, struct Token *rhs)
{
	// splice
	char *buf = format("%.*s%.*s", lhs->len, lhs->location, rhs->len,
			   rhs->location);

	// lex buf, get token stream
	struct Token *token =
		lexer(new_file(lhs->file->name, lhs->file->file_no, buf));
	if (token->next->kind != TK_EOF)
		error_token(lhs, "splicing froms '%s', an invalid token", buf);
	return token;
}

// replace macro params with args
static struct Token *subst(struct Token *token, struct MacroArg *args)
{
	struct Token head = {};
	struct Token *cur = &head;

	while (token->kind != TK_EOF) {
		// #macro arg, replace with corresponding str
		if (equal(token, "#")) {
			struct MacroArg *arg =
				find_macro_arg(args, token->next);
			if (!arg)
				error_token(
					token->next,
					"'#' is not followed by a macro parameter");
			// stringize
			cur = cur->next = stringize(token, arg->token);
			token = token->next->next;
			continue;
		}

		// ##__, used for splice terminators
		if (equal(token, "##")) {
			if (cur == &head)
				error_token(
					token,
					"'##' cannot appear at start of macro expansion");
			if (token->next->kind == TK_EOF)
				error_token(
					token,
					"'##' cannot appear at end of macro expansion");
			// search next terminator
			// if (##__) macro args
			struct MacroArg *arg =
				find_macro_arg(args, token->next);
			if (arg) {
				if (arg->token->kind != TK_EOF) {
					// splice
					*cur = *splice_terminators(cur,
								   arg->token);
					// the left terminators after splice
					for (struct Token *t = arg->token->next;
					     t->kind != TK_EOF; t = t->next)
						cur = cur->next = copy_token(t);
				}
				token = token->next->next;
				continue;
			}
			// if not (##__) macro args, splice direct splicing
			*cur = *splice_terminators(cur, token->next);
			token = token->next->next;
			continue;
		}

		struct MacroArg *arg = find_macro_arg(args, token);

		// __##, used for splice terminator
		if (arg && equal(token->next, "##")) {
			// read ##'s right terminators
			struct Token *rhs = token->next->next;

			// case ## left args is empty
			if (arg->token->kind == TK_EOF) {
				// search ## right args
				struct MacroArg *arg2 =
					find_macro_arg(args, rhs);
				if (arg2) {
					// if args
					for (struct Token *t = arg2->token;
					     t->kind != TK_EOF; t = t->next)
						cur = cur->next = copy_token(t);
				} else {
					// if not args
					cur = cur->next = copy_token(rhs);
				}
				token = rhs->next;
				continue;
			}
			// case ## left args not empty
			for (struct Token *t = arg->token; t->kind != TK_EOF;
			     t = t->next)
				cur = cur->next = copy_token(t);
			token = token->next;
			continue;
		}

		//process macro terminator
		if (arg) {
			struct Token *t = preprocess(arg->token);
			for (; t->kind != TK_EOF; t = t->next)
				cur = cur->next = copy_token(t);
			token = token->next;
			continue;
		}

		// process other terminator
		cur = cur->next = copy_token(token);
		token = token->next;
		continue;
	}

	cur->next = token;
	return head.next;
}

// if Macro var and expand success, return true
static bool expand_macro(struct Token **rest, struct Token *token)
{
	// determined if in hideset
	if (hideset_contains(token->hideset, token->location, token->len))
		return false;

	// determined if macro var
	struct Macro *m = find_macro(token);
	if (!m)
		return false;

	// if macro var
	if (m->is_obj_like) {
		// macro expand once, add it into hideset
		struct HideSet *hs =
			hideset_union(token->hideset, new_hideset(m->name));
		// after process this macro var, pass hidset to terminators behind
		struct Token *body = add_hideset(m->body, hs);
		*rest = append(body, token->next);
		return true;
	}

	// if there no args list behind macro func, process it as normal identifier
	if (!equal(token->next, "("))
		return false;

	// process macro func, and link after token
	// read macro func, here macro func's hideset
	struct Token *macro_token = token;
	struct MacroArg *args = read_macro_args(&token, token, m->params);
	// here return ), here macro arg's hideset
	struct Token *r_paren = token;
	// macro functions may have different hidden sets between them, \
	// and the new terminator would not know which hidden set to use.
	// we take the intersection of the macro terminator and the right bracket \
	// and use it as the new hidden set.
	struct HideSet *hs =
		hideset_intersection(macro_token->hideset, r_paren->hideset);

	// add cur func into hideset
	hs = hideset_union(hs, new_hideset(m->name));
	// replace macro func params with args
	struct Token *body = subst(m->body, args);
	// set macro func inner hideset
	body = add_hideset(body, hs);
	// append
	*rest = append(body, token->next);
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
			read_macro_def(&token, token->next);
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

			struct Macro *m = push_macro(name, true, NULL);
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

		// #ifdef
		if (equal(token, "ifdef")) {
			bool defined = find_macro(token->next);
			// push #if stack
			push_condincl(token, defined);
			token = skip_line(token->next->next);
			// if not defined, skip
			if (!defined)
				token = skip_condincl(token);
			continue;
		}

		// #ifndef
		if (equal(token, "ifndef")) {
			bool defined = find_macro(token->next);
			push_condincl(token, !defined);
			token = skip_line(token->next->next);
			if (defined)
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
