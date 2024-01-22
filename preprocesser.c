#include "thrvcc.h"

// preprocesser entry func
struct Token *preprocesser(struct Token *token)
{
	convert_keywords(token);
	return token;
}
