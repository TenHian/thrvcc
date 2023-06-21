#include "thrvcc.h"
#include <math.h>
#include <stdbool.h>
#include <time.h>

// local/global variable, typedef or enum constant scope
struct VarScope {
	struct VarScope *next; // next variable scope
	char *name; // variable scope name
	struct Obj_Var *var; // variable
	struct Type *_typedef; // alias
	struct Type *_enum; // enum type
	int enum_val; // enum constant value
};

// scope of structure label, union label or enum label
struct TagScope {
	struct TagScope *next;
	char *name;
	struct Type *type;
};

// block scope
struct Scope {
	struct Scope *next; // pointing to a higher-level scope

	// scope type : var(or typedef aka type alias) scope or\
	// structure(or union) scope
	struct VarScope *vars; // variables in current scope
	struct TagScope *tags; // label of the structure in the current scope
};

// Variable attributes
struct VarAttr {
	bool is_typedef; // Whether it is a type alias
	bool is_static; // Whether it is in file scope
};

// all var add in this list while parse
// means Local Variables
// used by func:
//         static struct Obj_Var *find_var(struct Token *token)
//         static struct Obj_Var *new_lvar(char *name, struct Type *type)
//         static struct Function *function(struct Token **rest, struct Token *token)
struct Obj_Var *Locals;
struct Obj_Var *Globals;

// linked list for all scope
static struct Scope *Scp = &(struct Scope){};

// point to current parsing function
static struct Obj_Var *CurParseFn;

// program = (typedef | functionDefinition | globalVariable)*
// functionDefinition = declspec declarator "{" compoundStmt*
// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "typedef" | "static"
//             | structDecl | unionDecl | typedefName
//             | enumSpecifier)+
// enumSpecifier = ident? "{" enumList? "}"
//                 | ident ("{" enumList? "}")?
// enumList = ident ("=" num)? ("," ident ("=" num)?)*
// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) typeSuffix
// typeSuffix = "(" funcParams | "[" arrayDimensions | ε
// arrayDimensions = num? "]" typeSuffix
// funcParams = (param ("," param)*)? ")"
// param = declspec declarator

// compoundStmt = (typedef | declaration | stmt)* "}"
// declaration =
// 	declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" exprStmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compoundStmt
//      | exprStmt
// expr stmt = expr? ;
// expr = assign ("," expr)?
// assign = logOr (assignOp assign)?
// logOr = logAnd ("||" logAnd)*
// logAnd = bitOr ("&&" bitOr)*
// bitOr = bitXor ("|" bitXor)*
// bitXor = bitAnd ("^" bitAnd)*
// bitAnd = equality ("&" equality)*
// assignOp = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
// equality = relational ("==" | "!=")
// relational = add ("<" | "<=" | ">" | ">=")
// add = mul ("+" | "-")
// mul = cast ("*" cast | "/" cast | "%" cast)*
// cast = "(" typeName ")" cast | unary
// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
// structMembers = (declspec declarator ( "," declarator)* ";")*
// structDecl = structUnionDecl
// unionDecl = structUnionDecl
// structUnionDecl = ident? ("{" structMembers)?
// postfix = primary ("[" expr "]" | "." ident)* | "->" ident | "++" | "--")*
// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" typeName ")"
//         | "sizeof" unary
//         | ident funcArgs?
//         | str
//         | num
// typeName = declspec abstractDeclarator
// abstractDeclarator = "*"* ("(" abstractDeclarator ")")? typeSuffix

// funcall = ident "(" (assign ("," assign)*)? ")"

static bool is_typename(struct Token *token);
static struct Type *declspec(struct Token **rest, struct Token *token,
			     struct VarAttr *attr);
static struct Type *enum_specifier(struct Token **rest, struct Token *token);
static struct Type *type_suffix(struct Token **rest, struct Token *token,
				struct Type *type);
static struct Type *declarator(struct Token **rest, struct Token *token,
			       struct Type *ty);
static struct AstNode *compoundstmt(struct Token **rest, struct Token *token);
static struct AstNode *declaration(struct Token **rest, struct Token *token,
				   struct Type *base_ty);
static struct AstNode *stmt(struct Token **rest, struct Token *token);
static struct AstNode *exprstmt(struct Token **rest, struct Token *token);
static struct AstNode *assign(struct Token **rest, struct Token *token);
static struct AstNode *log_or(struct Token **rest, struct Token *token);
static struct AstNode *log_and(struct Token **rest, struct Token *token);
static struct AstNode *bit_or(struct Token **rest, struct Token *token);
static struct AstNode *bit_xor(struct Token **rest, struct Token *token);
static struct AstNode *bit_and(struct Token **rest, struct Token *token);
static struct AstNode *equality(struct Token **rest, struct Token *token);
static struct AstNode *relational(struct Token **rest, struct Token *token);
static struct AstNode *expr(struct Token **rest, struct Token *token);
static struct AstNode *add(struct Token **rest, struct Token *token);
static struct AstNode *new_add(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token);
static struct AstNode *new_sub(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token);
static struct AstNode *mul(struct Token **rest, struct Token *token);
static struct AstNode *cast(struct Token **rest, struct Token *token);
static struct Type *struct_decl(struct Token **rest, struct Token *token);
static struct Type *union_decl(struct Token **rest, struct Token *token);
static struct AstNode *unary(struct Token **rest, struct Token *token);
static struct AstNode *postfix(struct Token **rest, struct Token *token);
static struct AstNode *primary(struct Token **rest, struct Token *token);
static bool is_typename(struct Token *token);
static struct Token *parse_typedef(struct Token *token, struct Type *base_ty);

// enter scope
static void enter_scope(void)
{
	struct Scope *sp = calloc(1, sizeof(struct Scope));
	// link stack
	sp->next = Scp;
	Scp = sp;
}

// leave current scope
static void leave_scope(void)
{
	Scp = Scp->next;
}

// find a local var by search name
static struct VarScope *find_var(struct Token *token)
{
	// the more scopes matched first, the deeper
	for (struct Scope *sp = Scp; sp; sp = sp->next)
		// iterate over all variables in the scope
		for (struct VarScope *vsp = sp->vars; vsp; vsp = vsp->next)
			if (equal(token, vsp->name))
				return vsp;
	return NULL;
}

// find a tag by search Token
static struct Type *find_tag(struct Token *token)
{
	for (struct Scope *sp = Scp; sp; sp = sp->next)
		for (struct TagScope *tsp = sp->tags; tsp; tsp = tsp->next)
			if (equal(token, tsp->name))
				return tsp->type;
	return NULL;
}

static struct AstNode *new_astnode(enum NodeKind kind, struct Token *token)
{
	struct AstNode *node = calloc(1, sizeof(struct AstNode));
	node->kind = kind;
	node->tok = token;
	return node;
}

static struct AstNode *new_unary_tree_node(enum NodeKind kind,
					   struct AstNode *expr,
					   struct Token *token)
{
	struct AstNode *node = new_astnode(kind, token);
	node->lhs = expr;
	return node;
}

static struct AstNode *new_binary_tree_node(enum NodeKind kind,
					    struct AstNode *lhs,
					    struct AstNode *rhs,
					    struct Token *token)
{
	struct AstNode *node = new_astnode(kind, token);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static struct AstNode *new_num_astnode(int64_t val, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_NUM, token);
	node->val = val;
	return node;
}

// construct a new long type node
static struct AstNode *new_long(int64_t val, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_NUM, token);
	node->val = val;
	node->type = TyLong;
	return node;
}

static struct AstNode *new_var_astnode(struct Obj_Var *var, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_VAR, token);
	node->var = var;
	return node;
}

// new cast
struct AstNode *new_cast(struct AstNode *expr, struct Type *type)
{
	add_type(expr);

	struct AstNode *node = calloc(1, sizeof(struct AstNode));
	node->kind = ND_CAST;
	node->tok = expr->tok;
	node->lhs = expr;
	node->type = copy_type(type);
	return node;
}

// push variable into current scope
static struct VarScope *push_scope(char *name)
{
	struct VarScope *vsp = calloc(1, sizeof(struct VarScope));
	vsp->name = name;
	// link stack
	vsp->next = Scp->vars;
	Scp->vars = vsp;
	return vsp;
}

// construct a new variable
static struct Obj_Var *new_var(char *name, struct Type *type)
{
	struct Obj_Var *var = calloc(1, sizeof(struct Obj_Var));
	var->name = name;
	var->type = type;
	push_scope(name)->var = var;
	return var;
}

// add a local var into var list
static struct Obj_Var *new_lvar(char *name, struct Type *type)
{
	struct Obj_Var *var = new_var(name, type);
	var->is_local = true;
	// insert into head
	var->next = Locals;
	Locals = var;
	return var;
}

// add a global var into var list
static struct Obj_Var *new_gvar(char *name, struct Type *type)
{
	struct Obj_Var *var = new_var(name, type);
	var->next = Globals;
	Globals = var;
	return var;
}

// new unique name
static char *new_unique_name(void)
{
	static int id = 0;
	return format(".L..%d", id++);
}

// add new anonymous global var
static struct Obj_Var *new_anon_gvar(struct Type *type)
{
	return new_gvar(new_unique_name(), type);
}

// new string literal
static struct Obj_Var *new_string_literal(char *str, struct Type *type)
{
	struct Obj_Var *var = new_anon_gvar(type);
	var->init_data = str;
	return var;
}

// get identifier
static char *get_ident(struct Token *token)
{
	if (token->kind != TK_IDENT)
		error_token(token, "expected an identifier");
	return strndup(token->location, token->len);
}

// search for type alias
static struct Type *find_typedef(struct Token *token)
{
	// A type alias is an identifier
	if (token->kind == TK_IDENT) {
		// Find if a variable exists in the scope
		struct VarScope *vsp = find_var(token);
		if (vsp)
			return vsp->_typedef;
	}
	return NULL;
}

// get number
static long get_number(struct Token *token)
{
	if (token->kind != TK_NUM)
		error_token(token, "expected a number");
	return token->val;
}

static void push_tag_scope(struct Token *token, struct Type *type)
{
	struct TagScope *tsp = calloc(1, sizeof(struct TagScope));
	tsp->name = strndup(token->location, token->len);
	tsp->type = type;
	tsp->next = Scp->tags;
	Scp->tags = tsp;
}

// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "typedef" | "static"
//             | structDecl | unionDecl | typedefName
//             | enumSpecifier)+
// declarator specifier
static struct Type *declspec(struct Token **rest, struct Token *token,
			     struct VarAttr *attr)
{
	// combinations of types are represented as for example: LONG+LONG=1<<9
	// tt is known that 'long int' and 'int long' are equivalent.
	enum {
		VOID = 1 << 0,
		BOOL = 1 << 2,
		CHAR = 1 << 4,
		SHORT = 1 << 6,
		INT = 1 << 8,
		LONG = 1 << 10,
		OTHER = 1 << 12,
	};

	struct Type *type = TyInt;
	int Counter = 0; // record type summed values

	// iterate through all Token of type name
	while (is_typename(token)) {
		// Handling the typedef keyword
		if (equal(token, "typedef") || equal(token, "static")) {
			if (!attr)
				error_token(
					token,
					"storage class specifier is not allowed in this context");
			if (equal(token, "typedef"))
				attr->is_typedef = true;
			else
				attr->is_static = true;

			// "typedef" should not be used in conjunction with "static"
			if (attr->is_typedef && attr->is_static)
				error_token(
					token,
					"'typedef' should not be used in conjunction whith 'static'");
			token = token->next;
			continue;
		}

		// Handling user-defined types
		struct Type *type2 = find_typedef(token);
		if (equal(token, "struct") || equal(token, "union") ||
		    equal(token, "enum") || type2) {
			if (Counter)
				break;

			if (equal(token, "struct"))
				type = struct_decl(&token, token->next);
			else if (equal(token, "union"))
				type = union_decl(&token, token->next);
			else if (equal(token, "enum"))
				type = enum_specifier(&token, token->next);
			else {
				// Set the type to the type pointed to by the type alias
				type = type2;
				token = token->next;
			}

			Counter += OTHER;
			continue;
		}

		// Add Counter for the type name that appears
		// Each step of Counter needs to have a legal value
		if (equal(token, "void"))
			Counter += VOID;
		else if (equal(token, "_Bool"))
			Counter += BOOL;
		else if (equal(token, "char"))
			Counter += CHAR;
		else if (equal(token, "short"))
			Counter += SHORT;
		else if (equal(token, "int"))
			Counter += INT;
		else if (equal(token, "long"))
			Counter += LONG;
		else
			unreachable();

		// Mapping to the corresponding Type according to the Counter value
		switch (Counter) {
		case VOID:
			type = TyVoid;
			break;
		case BOOL:
			type = TyBool;
			break;
		case CHAR:
			type = TyChar;
			break;
		case SHORT:
		case SHORT + INT:
			type = TyShort;
			break;
		case INT:
			type = TyInt;
			break;
		case LONG:
		case LONG + INT:
		case LONG + LONG:
		case LONG + LONG + INT:
			type = TyLong;
			break;
		default:
			error_token(token, "invalid type");
		}

		token = token->next;
	}

	*rest = token;
	return type;
}

// funcParams = (param ("," param)*)? ")"
// param = declspec declarator
static struct Type *func_params(struct Token **rest, struct Token *token,
				struct Type *type)
{
	struct Type head = {};
	struct Type *cur = &head;

	while (!equal(token, ")")) {
		// func_params = param ("," param)*
		// param = declspec declarator
		if (cur != &head)
			token = skip(token, ",");
		struct Type *type2 = declspec(&token, token, NULL);
		type2 = declarator(&token, token, type2);

		// An array of type T is converted to type T*
		if (type2->kind == TY_ARRAY) {
			struct Token *name = type2->name;
			type2 = pointer_to(type2->base);
			type2->name = name;
		}

		// copy to parameters list
		cur->next = copy_type(type2);
		cur = cur->next;
	}

	// wrap a func node
	type = func_type(type);
	// pass parameters
	type->params = head.next;
	*rest = token->next;
	return type;
}

// Array Dimension
// arrayDimensions = num? "]" typeSuffix
static struct Type *array_dimensions(struct Token **rest, struct Token *token,
				     struct Type *type)
{
	// ']' , "[]" for dimensionless array
	if (equal(token, "]")) {
		type = type_suffix(rest, token->next, type);
		return array_of(type, -1);
	}

	// the case with array dimensions
	int sz = get_number(token);
	token = skip(token->next, "]");
	type = type_suffix(rest, token, type);
	return array_of(type, sz);
}

// typeSuffix = "(" funcParams | "[" arrayDimensions | ε
static struct Type *type_suffix(struct Token **rest, struct Token *token,
				struct Type *ty)
{
	// "(" funcParams
	if (equal(token, "("))
		return func_params(rest, token->next, ty);

	// "[" arrayDimensions
	if (equal(token, "["))
		return array_dimensions(rest, token->next, ty);

	*rest = token;
	return ty;
}

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) typeSuffix
static struct Type *declarator(struct Token **rest, struct Token *token,
			       struct Type *type)
{
	// "*"*
	// construct all (multiple) pointers
	while (consume(&token, token, "*"))
		type = pointer_to(type);

	// "(" declarator ")"
	if (equal(token, "(")) {
		// reg "(" position
		struct Token *start = token;
		struct Type dummy = {};
		// make token forword after ")"
		declarator(&token, start->next, &dummy);
		token = skip(token, ")");
		// get the type suffix after the brackets, \
		// type is the finished type, rest points to the semicolon
		type = type_suffix(rest, token, type);
		// parse type as a whole as base to construct and return the value of type
		return declarator(&token, start->next, type);
	}

	if (token->kind != TK_IDENT)
		error_token(token, "expected a variable name");

	// type_suffix
	type = type_suffix(rest, token->next, type);
	// ident
	// variable name or func name
	type->name = token;
	return type;
}

// abstractDeclarator = "*"* ("(" abstractDeclarator ")")? typeSuffix
static struct Type *abstract_declarator(struct Token **rest,
					struct Token *token, struct Type *type)
{
	// "*"*
	while (equal(token, "*")) {
		type = pointer_to(type);
		token = token->next;
	}

	// ("(" abstractDeclarator ")")?
	if (equal(token, "(")) {
		struct Token *start = token;
		struct Type dummy = {};
		// make token forword after ")"
		abstract_declarator(&token, start->next, &dummy);
		token = skip(token, ")");
		// get the type suffix after the brackets, \
		// type is the finished type, rest points to the semicolon
		type = type_suffix(rest, token, type);
		// parse type as a whole as base to construct and return the value of type
		return abstract_declarator(&token, start->next, type);
	}

	// type_suffix
	return type_suffix(rest, token, type);
}

// typeName = declspec abstractDeclarator
// Get information about the type
static struct Type *type_name(struct Token **rest, struct Token *token)
{
	// declspec
	struct Type *type = declspec(&token, token, NULL);
	// abstractDeclarator
	return abstract_declarator(rest, token, type);
}

// get enum type val
// enumSpecifier = ident? "{" enumList? "}"
//               | ident ("{" enumList? "}")?
// enumList      = ident ("=" num)? ("," ident ("=" num)?)*
static struct Type *enum_specifier(struct Token **rest, struct Token *token)
{
	struct Type *type = enum_type();

	// read label
	// ident?
	struct Token *tag = NULL;
	if (token->kind == TK_IDENT) {
		tag = token;
		token = token->next;
	}

	// process with case none {}
	if (tag && !equal(token, "{")) {
		struct Type *ty = find_tag(tag);
		if (!ty)
			error_token(tag, "unknown enum type");
		if (ty->kind != TY_ENUM)
			error_token(tag, "not an enum tag");
		*rest = token;
		return ty;
	}

	// "{" enumList? "}"
	token = skip(token, "{");

	// enumList
	// read enum list
	int I = 0; // enum constants counter
	int val = 0; // enum constants val
	while (!equal(token, "}")) {
		if (I++ > 0)
			token = skip(token, ",");

		char *name = get_ident(token);
		token = token->next;

		// determine if an assignment exists
		if (equal(token, "=")) {
			val = get_number(token->next);
			token = token->next->next;
		}

		// store enum constant
		struct VarScope *vsp = push_scope(name);
		vsp->_enum = type;
		vsp->enum_val = val++;
	}

	*rest = token->next;

	if (tag)
		push_tag_scope(tag, type);
	return type;
}

// declaration =
//    declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static struct AstNode *declaration(struct Token **rest, struct Token *token,
				   struct Type *base_ty)
{
	struct AstNode head = {};
	struct AstNode *cur = &head;
	// count the var declaration times
	int decl_count = 0;

	// (declarator ("=" expr)? ("," declarator ("=" expr)?)*)?
	while (!equal(token, ";")) {
		// the first var not have to match ","
		if (decl_count++ > 0)
			token = skip(token, ",");

		// declarator
		// declare the var-type that got, include var name
		struct Type *type = declarator(&token, token, base_ty);
		if (type->size < 0)
			error_token(token, "variable has incomplete type");
		if (type->kind == TY_VOID)
			error_token(token, "variable declared void");
		struct Obj_Var *var = new_lvar(get_ident(type->name), type);

		// if not exist "=", its var declaration, no need to gen AstNode,
		// already stored in Locals.
		if (!equal(token, "="))
			continue;

		// parse tokens after "="
		struct AstNode *lhs = new_var_astnode(var, type->name);
		// parsing recursive assignment statements
		struct AstNode *rhs = assign(&token, token->next);
		struct AstNode *node =
			new_binary_tree_node(ND_ASSIGN, lhs, rhs, token);
		// stored it in expr stmt
		cur->next = new_unary_tree_node(ND_EXPR_STMT, node, token);
		cur = cur->next;
	}
	// store all expr stmt in code block
	struct AstNode *node = new_astnode(ND_BLOCK, token);
	node->body = head.next;
	*rest = token->next;
	return node;
}

// determine if it is a type name
static bool is_typename(struct Token *token)
{
	static char *keyword[] = {
		"void",	  "_Bool", "char",    "short", "int",	 "long",
		"struct", "union", "typedef", "enum",  "static",
	};

	for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); ++i) {
		if (equal(token, keyword[i]))
			return true;
	}

	// Find out if it is a type alias
	return find_typedef(token);
}

// parse stmt
// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" exprStmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compoundStmt
//      | exprStmt
static struct AstNode *stmt(struct Token **rest, struct Token *token)
{
	// "return" expr ";"
	if (equal(token, "return")) {
		struct AstNode *node = new_astnode(ND_RETURN, token);
		struct AstNode *exp = expr(&token, token->next);
		*rest = skip(token, ";");

		add_type(exp);
		// type conversion for return type
		node->lhs = new_cast(exp, CurParseFn->type->return_type);
		return node;
	}
	// "if" "(" expr ")" stmt ("else" stmt)?
	if (equal(token, "if")) {
		struct AstNode *node = new_astnode(ND_IF, token);
		// conditionial stmt
		token = skip(token->next, "(");
		node->condition = expr(&token, token);
		token = skip(token, ")");
		// then stmt
		node->then_ = stmt(&token, token);
		// ("else" stmt)?
		if (equal(token, "else"))
			node->else_ = stmt(&token, token->next);
		*rest = token;
		return node;
	}
	// "for" "(" exprStmt expr? ";" expr? ")" stmt
	if (equal(token, "for")) {
		struct AstNode *node = new_astnode(ND_FOR, token);
		// "("
		token = skip(token->next, "(");

		// enter 'for' loop scope
		enter_scope();

		// exprStmt
		if (is_typename(token)) {
			// Initialize loop control variables
			struct Type *base_ty = declspec(&token, token, NULL);
			node->init = declaration(&token, token, base_ty);
		} else {
			node->init = exprstmt(&token, token);
		}

		// expr?
		if (!equal(token, ";"))
			node->condition = expr(&token, token);
		// ";"
		token = skip(token, ";");

		// expr?
		if (!equal(token, ")"))
			node->increase = expr(&token, token);
		// ")"
		token = skip(token, ")");

		// stmt
		node->then_ = stmt(rest, token);

		// leave 'for' loop scope
		leave_scope();

		return node;
	}
	// "while" "(" expr ")" stmt
	if (equal(token, "while")) {
		struct AstNode *node = new_astnode(ND_FOR, token);
		// "("
		token = skip(token->next, "(");
		// expr
		node->condition = expr(&token, token);
		// ")"
		token = skip(token, ")");
		// stmt
		node->then_ = stmt(rest, token);
		return node;
	}

	// "{" compoundStmt
	if (equal(token, "{"))
		return compoundstmt(rest, token->next);
	// exprStmt
	return exprstmt(rest, token);
}

// parse compound stmt
// compoundStmt = (typedef | declaration | stmt)* "}"
static struct AstNode *compoundstmt(struct Token **rest, struct Token *token)
{
	struct AstNode *node = new_astnode(ND_BLOCK, token);
	// unidirectional linked list
	struct AstNode head = {};
	struct AstNode *cur = &head;

	// enter new scope
	enter_scope();

	// (declaration | stmt)* "}"
	while (!equal(token, "}")) {
		// declaration
		if (is_typename(token)) {
			struct VarAttr attr = {};
			struct Type *base_ty = declspec(&token, token, &attr);

			// parse 'typedef' stmt
			if (attr.is_typedef) {
				token = parse_typedef(token, base_ty);
				continue;
			}

			// parse variable declaration statements
			cur->next = declaration(&token, token, base_ty);
		}
		// stmt
		else
			cur->next = stmt(&token, token);
		cur = cur->next;
		// add var kind to nodes, after constructed AST
		add_type(cur);
	}

	// leave current scope
	leave_scope();

	// mov stmt that in "{}" to parser
	node->body = head.next;
	*rest = token->next;
	return node;
}

// parse expr
// exprStmt = expr ;
static struct AstNode *exprstmt(struct Token **rest, struct Token *token)
{
	// ";"
	if (equal(token, ";")) {
		*rest = token->next;
		return new_astnode(ND_BLOCK, token);
	}
	// expr ";"
	struct AstNode *node = new_astnode(ND_EXPR_STMT, token);
	node->lhs = expr(&token, token);
	*rest = skip(token, ";");
	return node;
}

// expr = assign ("," expr)?
static struct AstNode *expr(struct Token **rest, struct Token *token)
{
	struct AstNode *node = assign(&token, token);

	if (equal(token, ","))
		return new_binary_tree_node(ND_COMMA, node,
					    expr(rest, token->next), token);

	*rest = token;
	return node;
}

// convert "A op= B" to "TMP = &A, *TMP = *TMP op B"
static struct AstNode *to_assign(struct AstNode *binary)
{
	// A
	add_type(binary->lhs);
	// B
	add_type(binary->rhs);
	struct Token *token = binary->tok;

	// TMP
	struct Obj_Var *var = new_lvar("", pointer_to(binary->lhs->type));

	// TMP = &A
	struct AstNode *expr1 = new_binary_tree_node(
		ND_ASSIGN, new_var_astnode(var, token),
		new_unary_tree_node(ND_ADDR, binary->lhs, token), token);

	// *TMP = *TMP op B
	struct AstNode *expr2 = new_binary_tree_node(
		ND_ASSIGN,
		new_unary_tree_node(ND_DEREF, new_var_astnode(var, token),
				    token),
		new_binary_tree_node(
			binary->kind,
			new_unary_tree_node(ND_DEREF,
					    new_var_astnode(var, token), token),
			binary->rhs, token),
		token);

	// TMP = &A, *TMP = *TMP op B
	return new_binary_tree_node(ND_COMMA, expr1, expr2, token);
}

// assign = logOr (assignOp assign)?
// assignOp = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
static struct AstNode *assign(struct Token **rest, struct Token *token)
{
	// equality
	struct AstNode *node = log_or(&token, token);

	// there may be recursive assignments, such as a=b=1
	// ("=" assign)?
	if (equal(token, "="))
		return node = new_binary_tree_node(ND_ASSIGN, node,
						   assign(rest, token->next),
						   token);
	// ("+=" assign)?
	if (equal(token, "+="))
		return to_assign(
			new_add(node, assign(rest, token->next), token));
	// ("-=" assign)?
	if (equal(token, "-="))
		return to_assign(
			new_sub(node, assign(rest, token->next), token));
	// ("*=" assign)?
	if (equal(token, "*="))
		return to_assign(new_binary_tree_node(
			ND_MUL, node, assign(rest, token->next), token));
	// ("/=" assign)?
	if (equal(token, "/="))
		return to_assign(new_binary_tree_node(
			ND_DIV, node, assign(rest, token->next), token));
	// ("%=" assign)?
	if (equal(token, "%="))
		return to_assign(new_binary_tree_node(
			ND_MOD, node, assign(rest, token->next), token));
	// ("&=" assign)?
	if (equal(token, "&="))
		return to_assign(new_binary_tree_node(
			ND_BITAND, node, assign(rest, token->next), token));
	// ("!=" assign)?
	if (equal(token, "|="))
		return to_assign(new_binary_tree_node(
			ND_BITOR, node, assign(rest, token->next), token));

	// ("^=" assign)?
	if (equal(token, "^="))
		return to_assign(new_binary_tree_node(
			ND_BITXOR, node, assign(rest, token->next), token));

	*rest = token;
	return node;
}

// logOr = logAnd ("||" logAnd)*
static struct AstNode *log_or(struct Token **rest, struct Token *token)
{
	struct AstNode *node = log_and(&token, token);
	while (equal(token, "||")) {
		struct Token *start = token;
		node = new_binary_tree_node(
			ND_LOGOR, node, log_and(&token, token->next), start);
	}
	*rest = token;
	return node;
}

// logAnd = bitOr ("&&" bitOr)*
static struct AstNode *log_and(struct Token **rest, struct Token *token)
{
	struct AstNode *node = bit_or(&token, token);
	while (equal(token, "&&")) {
		struct Token *start = token;
		node = new_binary_tree_node(ND_LOGAND, node,
					    bit_or(&token, token->next), start);
	}
	*rest = token;
	return node;
}

// bitOr = bitXor ("|" bitXor)*
static struct AstNode *bit_or(struct Token **rest, struct Token *token)
{
	struct AstNode *node = bit_xor(&token, token);
	while (equal(token, "|")) {
		struct Token *start = token;
		node = new_binary_tree_node(
			ND_BITOR, node, bit_xor(&token, token->next), start);
	}
	*rest = token;
	return node;
}

// bitXor = bitAnd ("^" bitAnd)*
static struct AstNode *bit_xor(struct Token **rest, struct Token *token)
{
	struct AstNode *node = bit_and(&token, token);
	while (equal(token, "^")) {
		struct Token *start = token;
		node = new_binary_tree_node(
			ND_BITXOR, node, bit_and(&token, token->next), start);
	}
	*rest = token;
	return node;
}

// bitAnd = equality ("&" equality)*
static struct AstNode *bit_and(struct Token **rest, struct Token *token)
{
	struct AstNode *node = equality(&token, token);
	while (equal(token, "&")) {
		struct Token *start = token;
		node = new_binary_tree_node(
			ND_BITAND, node, equality(&token, token->next), start);
	}
	*rest = token;
	return node;
}

// equality = relational ("==" relational | "!=" relational)*
static struct AstNode *equality(struct Token **rest, struct Token *token)
{
	// relational
	struct AstNode *node = relational(&token, token);

	// ("==" relational | "!=" relational)*
	while (true) {
		struct Token *start = token;
		// "==" relational
		if (equal(token, "==")) {
			node = new_binary_tree_node(
				ND_EQ, node, relational(&token, token->next),
				start);
			continue;
		}
		// "!=" relational
		if (equal(token, "!=")) {
			node = new_binary_tree_node(
				ND_NE, node, relational(&token, token->next),
				start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static struct AstNode *relational(struct Token **rest, struct Token *token)
{
	// add
	struct AstNode *node = add(&token, token);

	// ("<" add | "<=" add | ">" add | ">=" add)*
	while (true) {
		struct Token *start = token;
		// "<" add
		if (equal(token, "<")) {
			node = new_binary_tree_node(
				ND_LT, node, add(&token, token->next), start);
			continue;
		}
		// "<=" add
		if (equal(token, "<=")) {
			node = new_binary_tree_node(
				ND_LE, node, add(&token, token->next), start);
			continue;
		}
		// ">" add, X>Y equal Y<X
		if (equal(token, ">")) {
			node = new_binary_tree_node(
				ND_LT, add(&token, token->next), node, start);
			continue;
		}
		// ">=" add
		if (equal(token, ">=")) {
			node = new_binary_tree_node(
				ND_LE, add(&token, token->next), node, start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// parse various add
static struct AstNode *new_add(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token)
{
	// add type for both left side and right side
	add_type(lhs);
	add_type(rhs);
	// num + num
	if (is_integer(lhs->type) && is_integer(rhs->type))
		return new_binary_tree_node(ND_ADD, lhs, rhs, token);
	// if "ptr + ptr", error
	if (lhs->type->base && rhs->type->base)
		error_token(token, "invalid operands");
	// turn "num + ptr" into "ptr + num"
	if (!lhs->type->base && rhs->type->base) {
		struct AstNode *tmp = lhs;
		lhs = rhs;
		rhs = tmp;
	}
	// ptr + num
	// pointer additon, ptr+1, the 1 in here is not a char,
	// is 1 item's space, so, need to *size
	// pointer stored with long type now
	rhs = new_binary_tree_node(
		ND_MUL, rhs, new_long(lhs->type->base->size, token), token);
	return new_binary_tree_node(ND_ADD, lhs, rhs, token);
}

// parse various sub
static struct AstNode *new_sub(struct AstNode *lhs, struct AstNode *rhs,
			       struct Token *token)
{
	add_type(lhs);
	add_type(rhs);

	// num - num
	if (is_integer(lhs->type) && is_integer(rhs->type))
		return new_binary_tree_node(ND_SUB, lhs, rhs, token);
	// ptr - num
	if (lhs->type->base && is_integer(rhs->type)) {
		// pointer stored with long type now
		rhs = new_binary_tree_node(
			ND_MUL, rhs, new_long(lhs->type->base->size, token),
			token);
		add_type(rhs);
		struct AstNode *node =
			new_binary_tree_node(ND_SUB, lhs, rhs, token);
		// node type is pointer
		node->type = lhs->type;
		return node;
	}
	// ptr - ptr, return how many items between 2 pointers
	if (lhs->type->base && rhs->type->base) {
		struct AstNode *node =
			new_binary_tree_node(ND_SUB, lhs, rhs, token);
		node->type = TyInt;
		return new_binary_tree_node(
			ND_DIV, node,
			new_num_astnode(lhs->type->base->size, token), token);
	}
	error_token(token, "invalid operands");
	return NULL;
}

// expr = mul ("+" mul | "-" mul)*
static struct AstNode *add(struct Token **rest, struct Token *token)
{
	struct AstNode *node = mul(&token, token);

	while (true) {
		struct Token *start = token;
		if (equal(token, "+")) {
			node = new_add(node, mul(&token, token->next), start);
			continue;
		}
		if (equal(token, "-")) {
			node = new_sub(node, mul(&token, token->next), start);
			continue;
		}
		*rest = token;
		return node;
	}
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static struct AstNode *mul(struct Token **rest, struct Token *token)
{
	// cast
	struct AstNode *node = cast(&token, token);

	// ("*" cast | "/" cast | "%" cast)*
	while (true) {
		struct Token *start = token;
		// "*" cast
		if (equal(token, "*")) {
			node = new_binary_tree_node(
				ND_MUL, node, cast(&token, token->next), start);
			continue;
		}
		// "/" cast
		if (equal(token, "/")) {
			node = new_binary_tree_node(
				ND_DIV, node, cast(&token, token->next), start);
			continue;
		}
		// "%" cast
		if (equal(token, "%")) {
			node = new_binary_tree_node(
				ND_MOD, node, cast(&token, token->next), start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// Parsing type conversions(cast)
// cast = "(" typeName ")" cast | unary
static struct AstNode *cast(struct Token **rest, struct Token *token)
{
	// cast = "(" typeName ")" cast
	if (equal(token, "(") && is_typename(token->next)) {
		struct Token *start = token;
		struct Type *type = type_name(&token, token->next);
		token = skip(token, ")");
		// Parsing nested type conversions
		struct AstNode *node = new_cast(cast(rest, token), type);
		node->tok = start;
		return node;
	}

	// unary
	return unary(rest, token);
}

// parse unary operators
// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
static struct AstNode *unary(struct Token **rest, struct Token *token)
{
	// "+" cast
	if (equal(token, "+"))
		return cast(rest, token->next);
	// "-" cast
	if (equal(token, "-"))
		return new_unary_tree_node(ND_NEG, cast(rest, token->next),
					   token);
	// "&" cast
	if (equal(token, "&"))
		return new_unary_tree_node(ND_ADDR, cast(rest, token->next),
					   token);
	// "*" cast
	if (equal(token, "*"))
		return new_unary_tree_node(ND_DEREF, cast(rest, token->next),
					   token);
	// "!" cast
	if (equal(token, "!"))
		return new_unary_tree_node(ND_NOT, cast(rest, token->next),
					   token);
	// "~" cast
	if (equal(token, "~"))
		return new_unary_tree_node(ND_BITNOT, cast(rest, token->next),
					   token);
	// convert '++i' to 'i+=1'
	// '++' unary
	if (equal(token, "++"))
		return to_assign(new_add(unary(rest, token->next),
					 new_num_astnode(1, token), token));
	// convert '--i' to 'i-=1'
	// '--' unary
	if (equal(token, "--"))
		return to_assign(new_sub(unary(rest, token->next),
					 new_num_astnode(1, token), token));
	// postfix
	return postfix(rest, token);
}

// structMembers = (declspec declarator (","  declarator)* ";")*
static void struct_members(struct Token **rest, struct Token *token,
			   struct Type *type)
{
	struct Member head = {};
	struct Member *cur = &head;

	while (!equal(token, "}")) {
		// declspec
		struct Type *base_ty = declspec(&token, token, NULL);
		int first = true;

		while (!consume(&token, token, ";")) {
			if (!first)
				token = skip(token, ",");
			first = false;

			struct Member *mem = calloc(1, sizeof(struct Member));
			// declarator
			mem->type = declarator(&token, token, base_ty);
			mem->name = mem->type->name;
			cur = cur->next = mem;
		}
	}
	*rest = token->next;
	type->member = head.next;
}

// structUnionDecl = ident? ("{" structMembers)?
static struct Type *struct_union_decl(struct Token **rest, struct Token *token)
{
	// read label
	struct Token *tag = NULL;
	if (token->kind == TK_IDENT) {
		tag = token;
		token = token->next;
	}

	if (tag && !equal(token, "{")) {
		struct Type *ty = find_tag(tag);
		if (!ty)
			error_token(tag, "unknown struct type");
		*rest = token;
		return ty;
	}

	// construct a struct
	struct Type *ty = calloc(1, sizeof(struct Type));
	ty->kind = TY_STRUCT;
	struct_members(rest, token->next, ty);
	ty->align = 1;

	// if have a tag, reg the structure type
	if (tag)
		push_tag_scope(tag, ty);
	return ty;
}

// structDecl = structUnionDecl
static struct Type *struct_decl(struct Token **rest, struct Token *token)
{
	struct Type *ty = struct_union_decl(rest, token);
	ty->kind = TY_STRUCT;

	// caculate the offset of struct members
	int offset = 0;
	for (struct Member *mem = ty->member; mem; mem = mem->next) {
		offset = align_to(offset, mem->type->align);
		mem->offset = offset;
		offset += mem->type->size;

		if (ty->align < mem->type->align)
			ty->align = mem->type->align;
	}
	ty->size = align_to(offset, ty->align);

	return ty;
}

// unionDecl = structUnionDecl
static struct Type *union_decl(struct Token **rest, struct Token *token)
{
	struct Type *ty = struct_union_decl(rest, token);
	ty->kind = TY_UNION;

	// union needs to be set to the maximum alignment and size,\
	// and the variable offsets all default to 0
	for (struct Member *mem = ty->member; mem; mem = mem->next) {
		if (ty->align < mem->type->align)
			ty->align = mem->type->align;
		if (ty->size < mem->type->size)
			ty->size = mem->type->size;
	}
	// align
	ty->size = align_to(ty->size, ty->align);
	return ty;
}

// get struct member
static struct Member *get_struct_member(struct Type *type, struct Token *token)
{
	for (struct Member *mem = type->member; mem; mem = mem->next)
		if (mem->name->len == token->len &&
		    !strncmp(mem->name->location, token->location, token->len))
			return mem;
	error_token(token, "no such member");
	return NULL;
}

// construct struct member node
static struct AstNode *struct_ref(struct AstNode *lhs, struct Token *token)
{
	add_type(lhs);
	if (lhs->type->kind != TY_STRUCT && lhs->type->kind != TY_UNION)
		error_token(lhs->tok, "not a struct nor a union");

	struct AstNode *node = new_unary_tree_node(ND_MEMBER, lhs, token);
	node->member = get_struct_member(lhs->type, token);
	return node;
}

// convert 'i++' to '(typeof i)((i += 1) - 1)'
// increment decrement
static struct AstNode *new_inc_dec(struct AstNode *node, struct Token *token,
				   int add_end)
{
	add_type(node);
	return new_cast(
		new_add(to_assign(new_add(node, new_num_astnode(add_end, token),
					  token)),
			new_num_astnode(-add_end, token), token),
		node->type);
}

// postfix = primary ("[" expr "]" | "." ident)*
static struct AstNode *postfix(struct Token **rest, struct Token *token)
{
	// primary
	struct AstNode *node = primary(&token, token);

	// ("[" expr "]")*
	while (true) {
		if (equal(token, "[")) {
			// x[y] equal to *(x+y)
			struct Token *start = token;
			struct AstNode *idx = expr(&token, token->next);
			token = skip(token, "]");
			node = new_unary_tree_node(
				ND_DEREF, new_add(node, idx, start), start);
			continue;
		}

		if (equal(token, ".")) {
			node = struct_ref(node, token->next);
			token = token->next->next;
			continue;
		}

		if (equal(token, "->")) {
			// x->y equals to (*x).y
			node = new_unary_tree_node(ND_DEREF, node, token);
			node = struct_ref(node, token->next);
			token = token->next->next;
			continue;
		}

		if (equal(token, "++")) {
			node = new_inc_dec(node, token, 1);
			token = token->next;
			continue;
		}

		if (equal(token, "--")) {
			node = new_inc_dec(node, token, -1);
			token = token->next;
			continue;
		}

		*rest = token;
		return node;
	}
}

// parse func call
// funcall = ident "(" (assign ("," assign)*)? ")"
static struct AstNode *func_call(struct Token **rest, struct Token *token)
{
	struct Token *start = token;
	token = token->next->next;

	// search function name
	struct VarScope *vsp = find_var(start);
	if (!vsp)
		error_token(start, "implicit declaration of a function");
	if (!vsp->var || vsp->var->type->kind != TY_FUNC)
		error_token(start, "not a function");

	// func name type
	struct Type *type = vsp->var->type;
	// func parameters type
	struct Type *param_type = type->params;

	struct AstNode head = {};
	struct AstNode *cur = &head;

	while (!equal(token, ")")) {
		if (cur != &head)
			token = skip(token, ",");
		// assign
		struct AstNode *arg = assign(&token, token);
		add_type(arg);

		if (param_type) {
			if (param_type->kind == TY_STRUCT ||
			    param_type->kind == TY_UNION)
				error_token(
					arg->tok,
					"passing struct or union is not supported yet");
			// type conversion for parameters node
			arg = new_cast(arg, param_type);
			// forword, next parameter type
			param_type = param_type->next;
		}
		// store parameter
		cur->next = arg;
		cur = cur->next;
		add_type(cur);
	}

	*rest = skip(token, ")");

	struct AstNode *node = new_astnode(ND_FUNCALL, start);

	// ident
	node->func_name = strndup(start->location, start->len);
	// func type
	node->func_type = type;
	// return type
	node->type = type->return_type;
	node->args = head.next;
	return node;
}

// parse "(" ")" | num | variables
// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" typeName ")"
//         | "sizeof" unary
//         | ident funcArgs?
//         | str
//         | num
static struct AstNode *primary(struct Token **rest, struct Token *token)
{
	struct Token *start = token;

	// "(" "{" stmt+ "}" ")"
	if (equal(token, "(") && equal(token->next, "{")) {
		// GNU statement expression
		struct AstNode *node = new_astnode(ND_STMT_EXPR, token);
		node->body = compoundstmt(&token, token->next->next)->body;
		*rest = skip(token, ")");
		return node;
	}
	// "(" expr ")"
	if (equal(token, "(")) {
		struct AstNode *node = expr(&token, token->next);
		*rest = skip(token, ")");
		return node;
	}
	// "sizeof" "(" typeName ")"
	if (equal(token, "sizeof") && equal(token->next, "(") &&
	    is_typename(token->next->next)) {
		struct Type *type = type_name(&token, token->next->next);
		*rest = skip(token, ")");
		return new_num_astnode(type->size, start);
	}
	// "sizeof" unary
	if (equal(token, "sizeof")) {
		struct AstNode *node = unary(rest, token->next);
		add_type(node);
		return new_num_astnode(node->type->size, token);
	}
	// ident args?
	if (token->kind == TK_IDENT) {
		// func call
		if (equal(token->next, "("))
			return func_call(rest, token);

		// ident
		// find var or enum constant
		struct VarScope *vsp = find_var(token);
		// if var or enum constant not exist, creat a new var in global var list
		if (!vsp || (!vsp->var && !vsp->_enum))
			error_token(token, "undefined variable");

		struct AstNode *node;
		// whether it is a variable
		if (vsp->var)
			node = new_var_astnode(vsp->var, token);
		// else, enum constant
		else
			node = new_num_astnode(vsp->enum_val, token);

		*rest = token->next;
		return node;
	}
	// str
	if (token->kind == TK_STR) {
		struct Obj_Var *var =
			new_string_literal(token->str, token->type);
		*rest = token->next;
		return new_var_astnode(var, token);
	}
	// num
	if (token->kind == TK_NUM) {
		struct AstNode *node = new_num_astnode(token->val, token);
		*rest = token->next;
		return node;
	}

	error_token(token, "expected an expression");
	return NULL;
}

// Parse type alias
static struct Token *parse_typedef(struct Token *token, struct Type *base_ty)
{
	bool first = true;

	while (!consume(&token, token, ";")) {
		if (!first)
			token = skip(token, ",");
		first = false;

		struct Type *type = declarator(&token, token, base_ty);
		// The variable name of the type alias is stored in the variable scope, \
		// and the type is set
		push_scope(get_ident(type->name))->_typedef = type;
	}
	return token;
}

// add params into Locals
static void create_params_lvars(struct Type *param)
{
	if (param) {
		// Recursive to the lowest of the form parameter
		// Add the bottom ones to the Locals first,
		// then add all the subsequent ones to the top one by one,
		// keeping the same order
		create_params_lvars(param->next);
		// add into Locals
		new_lvar(get_ident(param->name), param);
	}
}

// functionDefinition = declspec declarator "{" compoundStmt*
static struct Token *function(struct Token *token, struct Type *base_type,
			      struct VarAttr *attr)
{
	struct Type *type = declarator(&token, token, base_type);

	struct Obj_Var *fn = new_gvar(get_ident(type->name), type);
	fn->is_function = true;
	fn->is_definition = !consume(&token, token, ";");
	fn->is_static = attr->is_static;

	// determine if there is no function definition
	if (!fn->is_definition)
		return token;

	CurParseFn = fn;
	// clean global Locals
	Locals = NULL;
	// enter new scope
	enter_scope();
	// func parameters
	create_params_lvars(type->params);
	fn->params = Locals;

	token = skip(token, "{");
	// func body store AST, Locals store var
	fn->body = compoundstmt(&token, token);
	fn->locals = Locals;
	// leave current scope
	leave_scope();
	return token;
}

// construct globalVariable
static struct Token *global_variable(struct Token *token,
				     struct Type *base_type)
{
	bool first = true;

	while (!consume(&token, token, ";")) {
		if (!first)
			token = skip(token, ",");
		first = false;

		struct Type *type = declarator(&token, token, base_type);
		new_gvar(get_ident(type->name), type);
	}
	return token;
}

// distinguish between functions and global variables
static bool is_function(struct Token *token)
{
	if (equal(token, ";"))
		return false;

	struct Type dummy = {};
	struct Type *type = declarator(&token, token, &dummy);
	return type->kind == TY_FUNC;
}

// parser
// program = (typedef | functionDefinition | globalVariable)*
struct Obj_Var *parse(struct Token *token)
{
	Globals = NULL;

	while (token->kind != TK_EOF) {
		struct VarAttr attr = {};
		struct Type *base_type = declspec(&token, token, &attr);

		// typedef
		if (attr.is_typedef) {
			token = parse_typedef(token, base_type);
			continue;
		}

		// function
		if (is_function(token)) {
			token = function(token, base_type, &attr);
			continue;
		}
		// global variable
		token = global_variable(token, base_type);
	}

	return Globals;
}
