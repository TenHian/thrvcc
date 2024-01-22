#include "thrvcc.h"

// if # at the begin of line
static bool is_begin_hash(struct Token *token)
{
	return token->at_bol && equal(token, "#");
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
