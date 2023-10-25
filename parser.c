#include "thrvcc.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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
	bool is_extern; // Whether it is extern variable
	int align;
};

// variable initializer. this is a tree.
// Initializers can be nested
// int x[2][2] = {{1, 2}, {3, 4}}
struct Initializer {
	struct Initializer *next;
	struct Type *type;
	struct Token *token;
	bool is_flexible;

	struct AstNode *expr;

	struct Initializer **children;
};

struct InitDesig {
	struct InitDesig *next;
	int idx;
	struct Member *member;
	struct Obj_Var *var;
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

// goto and label list in cur func
static struct AstNode *Gotos;
static struct AstNode *Labels;

// current target that 'break'
static char *BrkLabel;
// current target that 'continue'
static char *CtueLabel;

// if a switch statement is being parsed, \
// it points to the node representing the switch.
// else, NULL.
static struct AstNode *CurSwitch;

// program = (typedef | functionDefinition | globalVariable)*
// functionDefinition = declspec declarator "{" compoundStmt*
// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "typedef" | "static" | "extern"
//             | "_Alignas" ("(" typeName | constExpr ")")
//             | structDecl | unionDecl | typedefName
//             | enumSpecifier)+
// enumSpecifier = ident? "{" enumList? "}"
//                 | ident ("{" enumList? "}")?
// enumList = ident ("=" constExpr)? ("," ident ("=" constExpr)?)* ","?
// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) typeSuffix
// typeSuffix = "(" funcParams | "[" arrayDimensions | ε
// arrayDimensions = constExpr? "]" typeSuffix
// funcParams = ("void" | param ("," param)* ("," "...")?)? ")"
// param = declspec declarator

// compoundStmt = (typedef | declaration | stmt)* "}"
// declaration = declspec (declarator ("=" initializer)?
//                         ("," declarator ("=" initializer)?)*)? ";"
// initializer = stringInitializer | arrayInitializer | structInitializer
//             | unionInitializer |assign
// stringInitializer = stringLiteral

// arrayInitializer = arrayInitializer1 | arrayInitializer2
// arrayInitializer1 = "{" initializer ("," initializer)* ","? "}"
// arrayIntializer2 = initializer ("," initializer)* ","?

// structInitializer = structInitializer1 | structInitializer2
// structInitializer1 = "{" initializer ("," initializer)* ","? "}"
// structIntializer2 = initializer ("," initializer)* ","?

// unionInitializer = "{" initializer "}"
// stmt = "return" expr? ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "switch" "(" expr ")" stmt
//        | "case" constExpr ":" stmt
//        | "default" ":" stmt
//        | "for" "(" exprStmt expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | "do" stmt "while" "(" expr ")" ";"
//        | "goto" ident ";"
//        | "break" ";"
//        | "continue" ";"
//        | ident ":" stmt
//        | "{" compoundStmt
//        | exprStmt
// expr stmt = expr? ;
// expr = assign ("," expr)?
// assign = conditional (assignOp assign)?
// conditional = logOr ("?" expr ":" conditional)?
// logOr = logAnd ("||" logAnd)*
// logAnd = bitOr ("&&" bitOr)*
// bitOr = bitXor ("|" bitXor)*
// bitXor = bitAnd ("^" bitAnd)*
// bitAnd = equality ("&" equality)*
// assignOp = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
//          | "<<=" | ">>="
// equality = relational ("==" relational | "!=" relational)*
// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
// shift = add ("<<" add | ">>" add)*
// add = mul ("+" mul | "-" mul)*
// mul = cast ("*" cast | "/" cast | "%" cast)*
// cast = "(" typeName ")" cast | unary
// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
//       | postfix
// structMembers = (declspec declarator ( "," declarator)* ";")*
// structDecl = structUnionDecl
// unionDecl = structUnionDecl
// structUnionDecl = ident? ("{" structMembers)?
// postfix = "(" typeName ")" "{" initializerList "}"
//         | primary ("[" expr "]" | "." ident)* | "->" ident | "++" | "--")*
// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" typeName ")"
//         | "sizeof" unary
//         | "_Alignof" "(" typeName ")"
//         | "_Alignof" unary
//         | ident funcArgs?
//         | str
//         | num
// typeName = declspec abstractDeclarator
// abstractDeclarator = "*"* ("(" abstractDeclarator ")")? typeSuffix

// funcall = ident "(" (assign ("," assign)*)? ")"

static bool is_typename(struct Token *token);
static struct Type *declspec(struct Token **rest, struct Token *token,
			     struct VarAttr *attr);
static struct Type *type_name(struct Token **rest, struct Token *token);
static struct Type *enum_specifier(struct Token **rest, struct Token *token);
static struct Type *type_suffix(struct Token **rest, struct Token *token,
				struct Type *type);
static struct Type *declarator(struct Token **rest, struct Token *token,
			       struct Type *ty);
static struct AstNode *compoundstmt(struct Token **rest, struct Token *token);
static struct AstNode *declaration(struct Token **rest, struct Token *token,
				   struct Type *base_ty, struct VarAttr *attr);
static void initializer2(struct Token **rest, struct Token *token,
			 struct Initializer *init);
static struct Initializer *initializer(struct Token **rest, struct Token *token,
				       struct Type *type,
				       struct Type **new_type);
static struct AstNode *
lvar_initializer(struct Token **rest, struct Token *token, struct Obj_Var *var);
static void gvar_initializer(struct Token **rest, struct Token *token,
			     struct Obj_Var *var);
static struct AstNode *stmt(struct Token **rest, struct Token *token);
static struct AstNode *exprstmt(struct Token **rest, struct Token *token);
static struct AstNode *expr(struct Token **rest, struct Token *token);
static int64_t eval(struct AstNode *node);
static int64_t eval2(struct AstNode *node, char **label);
static int64_t eval_rval(struct AstNode *node, char **label);
static int64_t const_expr(struct Token **rest, struct Token *token);
static struct AstNode *assign(struct Token **rest, struct Token *token);
static struct AstNode *conditional(struct Token **rest, struct Token *token);
static struct AstNode *log_or(struct Token **rest, struct Token *token);
static struct AstNode *log_and(struct Token **rest, struct Token *token);
static struct AstNode *bit_or(struct Token **rest, struct Token *token);
static struct AstNode *bit_xor(struct Token **rest, struct Token *token);
static struct AstNode *bit_and(struct Token **rest, struct Token *token);
static struct AstNode *equality(struct Token **rest, struct Token *token);
static struct AstNode *relational(struct Token **rest, struct Token *token);
static struct AstNode *shift(struct Token **rest, struct Token *token);
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
static bool is_function(struct Token *token);
static struct Token *function(struct Token *token, struct Type *base_ty,
			      struct VarAttr *attr);
static struct Token *global_variable(struct Token *token, struct Type *base_ty,
				     struct VarAttr *attr);

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

// new initializer
static struct Initializer *new_initializer(struct Type *type, bool is_flexible)
{
	struct Initializer *init = calloc(1, sizeof(struct Initializer));
	// store original type
	init->type = type;

	// deal with array type
	if (type->kind == TY_ARRAY) {
		// Determine
		// if the number of array elements needs to be adjusted
		// &&
		// the array is incomplete.
		if (is_flexible && type->size < 0) {
			// Set the initializer to be adjustable,
			// and then construct the initializer
			// after performing the calculation of the number of array elements.
			init->is_flexible = true;
			return init;
		}

		// allocate space for each of the outermost elements of the array
		init->children =
			calloc(type->array_len, sizeof(struct Initializer *));
		// iterate through the outermost elements of the array, and parse it
		for (int i = 0; i < type->array_len; ++i)
			init->children[i] = new_initializer(type->base, false);
	}

	// deal with structure and union
	if (type->kind == TY_STRUCT || type->kind == TY_UNION) {
		// caculate the num of struct member
		int len = 0;
		for (struct Member *member = type->member; member;
		     member = member->next)
			++len;

		// initialize initializer's children
		init->children = calloc(len, sizeof(struct Initializer *));

		// iterate children and assign it
		for (struct Member *member = type->member; member;
		     member = member->next) {
			// determines whether a structure is flexible
			// while a member is also flexible and is the last.
			// constructed directly here
			// to avoid parsing of flexible arrays.
			if (is_flexible && type->is_flexible && !member->next) {
				struct Initializer *child =
					calloc(1, sizeof(struct Initializer));
				child->type = member->type;
				child->is_flexible = true;
				init->children[member->idx] = child;
			} else {
				// assign values to non-flexible members
				init->children[member->idx] =
					new_initializer(member->type, false);
			}
		}
		return init;
	}
	return init;
}

// construct a new variable
static struct Obj_Var *new_var(char *name, struct Type *type)
{
	struct Obj_Var *var = calloc(1, sizeof(struct Obj_Var));
	var->name = name;
	var->type = type;
	// set variable's default align as their type's align
	var->align = type->align;
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
	// static
	var->is_static = true;
	// definitions exist
	var->is_definition = true;
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

static void push_tag_scope(struct Token *token, struct Type *type)
{
	struct TagScope *tsp = calloc(1, sizeof(struct TagScope));
	tsp->name = strndup(token->location, token->len);
	tsp->type = type;
	tsp->next = Scp->tags;
	Scp->tags = tsp;
}

// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "typedef" | "static" | "extern"
//             | "_Alignas" ("(" typeName | constExpr ")")
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
		if (equal(token, "typedef") || equal(token, "static") ||
		    equal(token, "extern")) {
			if (!attr)
				error_token(
					token,
					"storage class specifier is not allowed in this context");
			if (equal(token, "typedef"))
				attr->is_typedef = true;
			else if (equal(token, "static"))
				attr->is_static = true;
			else
				attr->is_extern = true;

			// "typedef" should not be used in conjunction with \
			// "static" or "extern"
			if (attr->is_typedef &&
			    (attr->is_static || attr->is_extern))
				error_token(
					token,
					"'typedef' should not be used in conjunction whith 'static'/'extern'");
			token = token->next;
			continue;
		}

		// _Alignas "(" typeName | constExpr ")"
		if (equal(token, "_Alignas")) {
			// cant set align when var's causality not exist
			if (!attr)
				error_token(
					token,
					"_Alignas is not allowed in this context");
			token = skip(token->next, "(");

			// determine if it's a type name, or a constant expression
			if (is_typename(token))
				attr->align = type_name(&token, token)->align;
			else
				attr->align = const_expr(&token, token);
			token = skip(token, ")");
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

// funcParams = ("void" | param ("," param)* ("," "...")?)? ")"
// param = declspec declarator
static struct Type *func_params(struct Token **rest, struct Token *token,
				struct Type *type)
{
	// "void"
	if (equal(token, "void") && equal(token->next, ")")) {
		*rest = token->next->next;
		return func_type(type);
	}

	struct Type head = {};
	struct Type *cur = &head;
	bool is_variadic = false;

	while (!equal(token, ")")) {
		// func_params = param ("," param)*
		// param = declspec declarator
		if (cur != &head)
			token = skip(token, ",");

		// ("," "...")?
		if (equal(token, "...")) {
			is_variadic = true;
			token = token->next;
			skip(token, ")");
			break;
		}

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
	// pass is_variadic parameters
	type->is_variadic = is_variadic;
	*rest = token->next;
	return type;
}

// Array Dimension
// arrayDimensions = constExpr? "]" typeSuffix
static struct Type *array_dimensions(struct Token **rest, struct Token *token,
				     struct Type *type)
{
	// ']' , "[]" for dimensionless array
	if (equal(token, "]")) {
		type = type_suffix(rest, token->next, type);
		return array_of(type, -1);
	}

	// the case with array dimensions
	int sz = const_expr(&token, token);
	token = skip(token, "]");
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

// determine if the terminator matches the end
static bool is_end(struct Token *token)
{
	// "}" | ",}"
	return equal(token, "}") ||
	       (equal(token, ",") && equal(token->next, "}"));
}

// consume the terminator at the end
// "}" | ",}"
static bool consume_end(struct Token **rest, struct Token *token)
{
	// "}"
	if (equal(token, "}")) {
		*rest = token->next;
		return true;
	}

	// ",}"
	if (equal(token, ",") && equal(token->next, "}")) {
		*rest = token->next->next;
		return true;
	}

	// do not consume those characters
	return false;
}

// get enum type val
// enumSpecifier = ident? "{" enumList? "}"
//               | ident ("{" enumList? "}")?
// enumList      = ident ("=" constExpr)? ("," ident ("=" constExpr)?)* ","?
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
	while (!consume_end(rest, token)) {
		if (I++ > 0)
			token = skip(token, ",");

		char *name = get_ident(token);
		token = token->next;

		// determine if an assignment exists
		if (equal(token, "="))
			val = const_expr(&token, token->next);

		// store enum constant
		struct VarScope *vsp = push_scope(name);
		vsp->_enum = type;
		vsp->enum_val = val++;
	}

	if (tag)
		push_tag_scope(tag, type);
	return type;
}

// declaration = declspec (declarator ("=" initializer)?
//                         ("," declarator ("=" initializer)?)*)? ";"
static struct AstNode *declaration(struct Token **rest, struct Token *token,
				   struct Type *base_ty, struct VarAttr *attr)
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
		if (type->kind == TY_VOID)
			error_token(token, "variable declared void");

		if (attr && attr->is_static) {
			// static local variable
			struct Obj_Var *var = new_anon_gvar(type);
			push_scope(get_ident(type->name))->var = var;
			if (equal(token, "="))
				gvar_initializer(&token, token->next, var);
			continue;
		}

		struct Obj_Var *var = new_lvar(get_ident(type->name), type);
		// read if variable's align exist
		if (attr && attr->align)
			var->align = attr->align;

		// if not exist "=", its var declaration, no need to gen AstNode,
		// already stored in Locals.
		if (equal(token, "=")) {
			// parse variable initializer
			struct AstNode *expr =
				lvar_initializer(&token, token->next, var);
			// stored in expr stmt
			cur->next =
				new_unary_tree_node(ND_EXPR_STMT, expr, token);
			cur = cur->next;
		}

		if (var->type->size < 0)
			error_token(type->name, "variable has incomplete type");
		if (var->type->kind == TY_VOID)
			error_token(type->name, "variable declared void");
	}

	// store all expr stmt in code block
	struct AstNode *node = new_astnode(ND_BLOCK, token);
	node->body = head.next;
	*rest = token->next;
	return node;
}

// skip excess elements
static struct Token *skip_excess_element(struct Token *token)
{
	if (equal(token, "{")) {
		token = skip_excess_element(token->next);
		return skip(token, "}");
	}

	// parse and discard excess elements
	assign(&token, token);
	return token;
}

// stringInitializer = stringLiteral
static void string_initializer(struct Token **rest, struct Token *token,
			       struct Initializer *init)
{
	// If it is adjustable, construct an initializer that contains an array.
	// String literals have been added '\0' in the lexing section.
	if (init->is_flexible)
		*init = *new_initializer(array_of(init->type->base,
						  token->type->array_len),
					 false);

	// take the shortest lengths of arrays and strings
	int len = MIN(init->type->array_len, token->type->array_len);
	// iterate and assign values
	for (int i = 0; i < len; i++)
		init->children[i]->expr = new_num_astnode(token->str[i], token);
	*rest = token->next;
}

// Count the number of initialize elements of an array.
static int count_array_init_elements(struct Token *token, struct Type *type)
{
	struct Initializer *dummy = new_initializer(type->base, false);
	// count
	int i = 0;

	for (; !consume_end(&token, token); i++) {
		if (i > 0)
			token = skip(token, ",");
		initializer2(&token, token, dummy);
	}
	return i;
}

// arrayInitializer1 = "{" initializer ("," initializer)* ","? "}"
static void array_initializer1(struct Token **rest, struct Token *token,
			       struct Initializer *init)
{
	token = skip(token, "{");

	// If the array is adjustable,
	// then count the number of elements in the array
	// and then do initializer construction.
	if (init->is_flexible) {
		int len = count_array_init_elements(token, init->type);
		*init = *new_initializer(array_of(init->type->base, len),
					 false);
	}

	// iterate array
	for (int i = 0; !consume_end(rest, token); i++) {
		if (i > 0)
			token = skip(token, ",");

		// normal parsing elements
		if (i < init->type->array_len)
			initializer2(&token, token, init->children[i]);

		// skip excess elements
		else
			token = skip_excess_element(token);
	}
}

// arrayIntializer2 = initializer ("," initializer)* ","?
static void array_intializer2(struct Token **rest, struct Token *token,
			      struct Initializer *init)
{
	// if array is adjustable, calculate array len,
	// then construct initializer
	if (init->is_flexible) {
		int len = count_array_init_elements(token, init->type);
		*init = *new_initializer(array_of(init->type->base, len),
					 false);
	}

	// iterate through array
	for (int i = 0; i < init->type->array_len && !is_end(token); i++) {
		if (i > 0)
			token = skip(token, ",");
		initializer2(&token, token, init->children[i]);
	}
	*rest = token;
}

// structInitializer1 = "{" initializer ("," initializer)* ","? "}"
static void struct_initializer1(struct Token **rest, struct Token *token,
				struct Initializer *init)
{
	token = skip(token, "{");

	// struct member list
	struct Member *member = init->type->member;

	while (!consume_end(rest, token)) {
		// if [member] not pointer to [init->type->member],
		// means that [member] performed <next> operation,
		// it is not the first
		if (member != init->type->member)
			token = skip(token, ",");

		if (member) {
			// deal with member
			initializer2(&token, token,
				     init->children[member->idx]);
			member = member->next;
		} else {
			// deal with excess member
			token = skip_excess_element(token);
		}
	}
}

// structIntializer2 = initializer ("," initializer)* ","?
static void struct_initializer2(struct Token **rest, struct Token *token,
				struct Initializer *init)
{
	bool first = true;

	// iterate all member
	for (struct Member *member = init->type->member;
	     member && !is_end(token); member = member->next) {
		if (!first)
			token = skip(token, ",");
		first = false;
		initializer2(&token, token, init->children[member->idx]);
	}

	*rest = token;
}

// unionInitializer = "{" initializer "}"
static void union_initializer(struct Token **rest, struct Token *token,
			      struct Initializer *init)
{
	// union only accepts the first member for initialization
	if (equal(token, "{")) {
		// if {} exist
		initializer2(&token, token->next, init->children[0]);
		// ","?
		consume(&token, token, ",");
		*rest = skip(token, "}");
	} else {
		// {} not exist
		initializer2(rest, token, init->children[0]);
	}
}

// initializer = stringInitializer | arrayInitializer | structInitializer
//             | unionInitializer |assign
static void initializer2(struct Token **rest, struct Token *token,
			 struct Initializer *init)
{
	// Initialization of string literals
	if (init->type->kind == TY_ARRAY && token->kind == TK_STR) {
		string_initializer(rest, token, init);
		return;
	}
	// Initialization of array
	if (init->type->kind == TY_ARRAY) {
		if (equal(token, "{"))
			array_initializer1(rest, token, init);
		else
			array_intializer2(rest, token, init);
		return;
	}
	// Initialization of structure
	if (init->type->kind == TY_STRUCT) {
		// Match assignments using other structures,
		// which need to be parsed first.
		if (equal(token, "{")) {
			struct_initializer1(rest, token, init);
			return;
		}

		struct AstNode *expr = assign(rest, token);
		add_type(expr);
		if (expr->type->kind == TY_STRUCT) {
			init->expr = expr;
			return;
		}

		struct_initializer2(rest, token, init);
		return;
	}

	// Initialization of union
	if (init->type->kind == TY_UNION) {
		union_initializer(rest, token, init);
		return;
	}

	// deal with braces outside of scalar, e.g. int x = {3};
	if (equal(token, "{")) {
		initializer2(&token, token->next, init);
		*rest = skip(token, "}");
		return;
	}

	// assign
	// store expr for node
	init->expr = assign(rest, token);
}

// copy the type of a structure
static struct Type *copy_struct_type(struct Type *type)
{
	type = copy_type(type);

	// copy the type of struct members
	struct Member head = {};
	struct Member *cur = &head;
	// iterate through members
	for (struct Member *member = type->member; member;
	     member = member->next) {
		struct Member *m = calloc(1, sizeof(struct Member));
		*m = *member;
		cur->next = m;
		cur = cur->next;
	}

	type->member = head.next;
	return type;
}

// initializer
static struct Initializer *initializer(struct Token **rest, struct Token *token,
				       struct Type *type,
				       struct Type **new_type)
{
	// Create a new initializer with resolved types
	struct Initializer *init = new_initializer(type, true);
	// Parsing needs to be assigned to Init
	initializer2(rest, token, init);

	if ((type->kind == TY_STRUCT || type->kind == TY_UNION) &&
	    type->is_flexible) {
		type = copy_struct_type(type);

		struct Member *member = type->member;
		// iterate till end
		while (member->next)
			member = member->next;
		// flexible array type replacement with actual array type
		member->type = init->children[member->idx]->type;
		// add structure type size
		type->size += member->type->size;

		// return new type into variable
		*new_type = type;
		return init;
	}

	// Pass the new type back to the variable.
	*new_type = init->type;
	return init;
}

// assign initialize expr
static struct AstNode *init_desig_expr(struct InitDesig *desig,
				       struct Token *token)
{
	// return variable in desig
	if (desig->var)
		return new_var_astnode(desig->var, token);

	// return member of [Desig]
	if (desig->member) {
		struct AstNode *node = new_unary_tree_node(
			ND_MEMBER, init_desig_expr(desig->next, token), token);
		node->member = desig->member;
		return node;
	}

	// Name of the variable to be assigned
	// Recursive to the next outer Desig,
	// at this point in outermost there is a [Desig->Var] or a [Desig->member]
	// The offsets are then calculated layer by layer
	struct AstNode *lhs = init_desig_expr(desig->next, token);
	// offset
	struct AstNode *rhs = new_num_astnode(desig->idx, token);
	// Returns the address of the variable after the offset
	return new_unary_tree_node(ND_DEREF, new_add(lhs, rhs, token), token);
}

// Creating initialization of local variables
static struct AstNode *create_lvar_init(struct Initializer *init,
					struct Type *type,
					struct InitDesig *desig,
					struct Token *token)
{
	if (type->kind == TY_ARRAY) {
		// The case of null expressions
		struct AstNode *node = new_astnode(ND_NULL_EXPR, token);
		for (int i = 0; i < type->array_len; i++) {
			// Here next points to information about \
			// the previous level of Desig, and the offset in it.
			struct InitDesig desig2 = { desig, i };
			// Local variables for initialization
			struct AstNode *rhs = create_lvar_init(
				init->children[i], type->base, &desig2, token);
			// Construct a binary tree of the form: NULL_EXPR, EXPR1, EXPR2...
			node = new_binary_tree_node(ND_COMMA, node, rhs, token);
		}
		return node;
	}

	// If the value has been assigned to another structure,
	// then there will be an [expr] and it will not be parsed.
	if (type->kind == TY_STRUCT && !init->expr) {
		// construct initializer for struct
		struct AstNode *node = new_astnode(ND_NULL_EXPR, token);

		for (struct Member *member = type->member; member;
		     member = member->next) {
			// desig2 stored struct member
			struct InitDesig desig2 = { desig, 0, member };
			struct AstNode *rhs =
				create_lvar_init(init->children[member->idx],
						 member->type, &desig2, token);
			node = new_binary_tree_node(ND_COMMA, node, rhs, token);
		}
		return node;
	}

	if (type->kind == TY_UNION) {
		// desig2 stored union member
		struct InitDesig desig2 = { desig, 0, type->member };
		// process with first member only
		return create_lvar_init(init->children[0], type->member->type,
					&desig2, token);
	}

	// If the expr to be used as the right value is null, \
	// it is set to the null expr
	if (!init->expr)
		return new_astnode(ND_NULL_EXPR, token);

	// Left values that can be assigned directly to variables, etc.
	struct AstNode *lhs = init_desig_expr(desig, token);
	// Initialized right value
	return new_binary_tree_node(ND_ASSIGN, lhs, init->expr, token);
}

// Local variable initializers
static struct AstNode *
lvar_initializer(struct Token **rest, struct Token *token, struct Obj_Var *var)
{
	// Get the initializer to correspond the values to the \
	// data structure one by one
	struct Initializer *init =
		initializer(rest, token, var->type, &var->type);
	// assign initialize
	struct InitDesig desig = { NULL, 0, NULL, var };

	// First assign 0 to all elements, \
	// then assign values to those that have a specified value
	struct AstNode *lhs = new_astnode(ND_MEMZERO, token);
	lhs->var = var;

	// creat initialize for local variable
	struct AstNode *rhs = create_lvar_init(init, var->type, &desig, token);
	// left part is all zeroized \
	// and the right part is the part that needs to be assigned a value
	return new_binary_tree_node(ND_COMMA, lhs, rhs, token);
}

// temporarily converted to [buf] type to store "val"
static void write_buf(char *buf, uint64_t val, int size)
{
	if (size == 1)
		*buf = val;
	else if (size == 2)
		*(uint16_t *)buf = val;
	else if (size == 4)
		*(uint32_t *)buf = val;
	else if (size == 8)
		*(uint64_t *)buf = val;
	else
		unreachable();
}

// write data into global variable initializer(GVI)
static struct Relocation *write_gvar_data(struct Relocation *cur,
					  struct Initializer *init,
					  struct Type *type, char *buf,
					  int offset)
{
	// process array
	if (type->kind == TY_ARRAY) {
		int size = type->base->size;
		for (int i = 0; i < type->array_len; i++)
			cur = write_gvar_data(cur, init->children[i],
					      type->base, buf,
					      offset + size * i);
		return cur;
	}

	// process structure
	if (type->kind == TY_STRUCT) {
		for (struct Member *member = type->member; member;
		     member = member->next)
			cur = write_gvar_data(cur, init->children[member->idx],
					      member->type, buf,
					      offset + member->offset);
		return cur;
	}

	// process union
	if (type->kind == TY_UNION) {
		return write_gvar_data(cur, init->children[0],
				       type->member->type, buf, offset);
	}

	// if return here, make buf 0
	if (!init->expr)
		return cur;

	// preset the other global variables' name that to be used
	char *label = NULL;
	uint64_t val = eval2(init->expr, &label);

	// if label not exist, means could calculate const expr directly
	if (!label) {
		write_buf(buf + offset, val, type->size);
		return cur;
	}

	// if label exist, means it used other global variables
	struct Relocation *rel = calloc(1, sizeof(struct Relocation));
	rel->offset = offset;
	rel->label = label;
	rel->addend = val;

	// push into list top
	cur->next = rel;
	return cur->next;
}

// global variables need to \
// have their initialized values calculated at compile time \
// and then written to the .data segment.
static void gvar_initializer(struct Token **rest, struct Token *token,
			     struct Obj_Var *var)
{
	// get initializer
	struct Initializer *init =
		initializer(rest, token, var->type, &var->type);

	// write data after calculate
	// creat a new relocation list
	struct Relocation head = {};
	char *buf = calloc(1, var->type->size);
	write_gvar_data(&head, init, var->type, buf, 0);
	// global variables's data
	var->init_data = buf;
	// return head.next, list head is empty
	var->rel = head.next;
}

// determine if it is a type name
static bool is_typename(struct Token *token)
{
	static char *keyword[] = {
		"void",	  "_Bool",  "char",	"short",   "int",
		"long",	  "struct", "union",	"typedef", "enum",
		"static", "extern", "_Alignas",
	};

	for (int i = 0; i < sizeof(keyword) / sizeof(*keyword); ++i) {
		if (equal(token, keyword[i]))
			return true;
	}

	// Find out if it is a type alias
	return find_typedef(token);
}

// parse stmt
// stmt = "return" expr? ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "switch" "(" expr ")" stmt
//        | "case" constExpr ":" stmt
//        | "default" ":" stmt
//        | "for" "(" exprStmt expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | "do" stmt "while" "(" expr ")" ";"
//        | "goto" ident ";"
//        | "break" ";"
//        | "continue" ";"
//        | ident ":" stmt
//        | "{" compoundStmt
//        | exprStmt
static struct AstNode *stmt(struct Token **rest, struct Token *token)
{
	// "return" expr ";"
	if (equal(token, "return")) {
		struct AstNode *node = new_astnode(ND_RETURN, token);

		// none return stmt
		if (consume(rest, token->next, ";"))
			return node;

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
	// "switch" "(" expr ")" stmt
	if (equal(token, "switch")) {
		struct AstNode *node = new_astnode(ND_SWITCH, token);
		token = skip(token->next, "(");
		node->condition = expr(&token, token);
		token = skip(token, ")");

		// reg current CurSwitch
		struct AstNode *sw = CurSwitch;
		// set current CurSwitch
		CurSwitch = node;

		// store current brk_label
		char *brk = BrkLabel;
		// set current brk_label
		BrkLabel = node->brk_label = new_unique_name();

		// enter and parse all cases
		// stmt
		node->then_ = stmt(rest, token);

		// restore current CurSwitch
		CurSwitch = sw;
		// restore current brk_label
		BrkLabel = brk;
		return node;
	}
	// "case" constExpr ":" stmt
	if (equal(token, "case")) {
		if (!CurSwitch)
			error_token(token, "stray case");

		struct AstNode *node = new_astnode(ND_CASE, token);
		// the val after case
		int val = const_expr(&token, token->next);
		token = skip(token, ":");
		node->label = new_unique_name();
		// stmt in case
		node->lhs = stmt(rest, token);
		// val that case mathes
		node->val = val;
		// store old CurSwitch list header into node->case_next
		node->case_next = CurSwitch->case_next;
		// store node into CurSwitch->case_next
		CurSwitch->case_next = node;
		return node;
	}
	// "default" ":" stmt
	if (equal(token, "default")) {
		if (!CurSwitch)
			error_token(token, "stray default");

		struct AstNode *node = new_astnode(ND_CASE, token);
		token = skip(token->next, ":");
		node->label = new_unique_name();
		node->lhs = stmt(rest, token);
		// store into CurSwitch->default_case
		CurSwitch->default_case = node;
		return node;
	}
	// "for" "(" exprStmt expr? ";" expr? ")" stmt
	if (equal(token, "for")) {
		struct AstNode *node = new_astnode(ND_FOR, token);
		// "("
		token = skip(token->next, "(");

		// enter 'for' loop scope
		enter_scope();

		// store current brk_label and ctue_label
		char *brk = BrkLabel;
		char *ctue = CtueLabel;
		// set name for brk_label and ctue_label
		BrkLabel = node->brk_label = new_unique_name();
		CtueLabel = node->ctue_label = new_unique_name();

		// exprStmt
		if (is_typename(token)) {
			// Initialize loop control variables
			struct Type *base_ty = declspec(&token, token, NULL);
			node->init = declaration(&token, token, base_ty, NULL);
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
		// restore the previous brk_label and ctue_label
		BrkLabel = brk;
		CtueLabel = ctue;

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

		// store current brk_label and ctue_label
		char *brk = BrkLabel;
		char *ctue = CtueLabel;
		// set name for brk_label and ctue_label
		BrkLabel = node->brk_label = new_unique_name();
		CtueLabel = node->ctue_label = new_unique_name();

		// stmt
		node->then_ = stmt(rest, token);

		// restore the previous brk_label and ctue_label
		BrkLabel = brk;
		CtueLabel = ctue;

		return node;
	}

	// "goto" ident ";"
	if (equal(token, "goto")) {
		struct AstNode *node = new_astnode(ND_GOTO, token);
		node->label = get_ident(token->next);
		// store 'node' into Gotos, used to parse UniqueLabel at the end
		node->goto_next = Gotos;
		Gotos = node;
		*rest = skip(token->next->next, ";");
		return node;
	}

	// "do" stmt "while" "(" expr ")" ";"
	if (equal(token, "do")) {
		struct AstNode *node = new_astnode(ND_DO, token);

		// store break/continue label's name before this
		char *brk = BrkLabel;
		char *ctue = CtueLabel;
		// set break/continue label's name
		BrkLabel = node->brk_label = new_unique_name();
		CtueLabel = node->ctue_label = new_unique_name();

		// stmt
		// 'do' code block stmt
		node->then_ = stmt(&token, token->next);

		// restore break/continue before
		BrkLabel = brk;
		CtueLabel = ctue;

		// "while" "(" expr ")" ";"
		token = skip(token, "while");
		token = skip(token, "(");
		// expr
		// conditional expression that 'while' use
		node->condition = expr(&token, token);
		token = skip(token, ")");
		*rest = skip(token, ";");
		return node;
	}

	// "break" ";"
	if (equal(token, "break")) {
		if (!BrkLabel)
			error_token(token, "stray break");
		// jump to brk_label
		struct AstNode *node = new_astnode(ND_GOTO, token);
		node->unique_label = BrkLabel;
		*rest = skip(token->next, ";");
		return node;
	}

	// "continue" ";"
	if (equal(token, "continue")) {
		if (!CtueLabel)
			error_token(token, "stray continue");
		// jump to ctue_label
		struct AstNode *node = new_astnode(ND_GOTO, token);
		node->unique_label = CtueLabel;
		*rest = skip(token->next, ";");
		return node;
	}

	// ident ":" stmt
	if (token->kind == TK_IDENT && equal(token->next, ":")) {
		struct AstNode *node = new_astnode(ND_LABEL, token);
		node->label = strndup(token->location, token->len);
		node->unique_label = new_unique_name();
		node->lhs = stmt(rest, token->next->next);
		// store 'node' into Gotos, used to parse UniqueLabel at the end
		node->goto_next = Labels;
		Labels = node;
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
		if (is_typename(token) && !equal(token->next, ":")) {
			struct VarAttr attr = {};
			struct Type *base_ty = declspec(&token, token, &attr);

			// parse 'typedef' stmt
			if (attr.is_typedef) {
				token = parse_typedef(token, base_ty);
				continue;
			}

			// parse function
			if (is_function(token)) {
				token = function(token, base_ty, &attr);
				continue;
			}

			// parse extern global variable
			if (attr.is_extern) {
				token = global_variable(token, base_ty, &attr);
				continue;
			}

			// parse variable declaration statements
			cur->next = declaration(&token, token, base_ty, &attr);
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

static int64_t eval(struct AstNode *node)
{
	return eval2(node, NULL);
}

// Compute a constant expression for a given node
// const expr can be number or addr +/- offser
static int64_t eval2(struct AstNode *node, char **label)
{
	add_type(node);

	switch (node->kind) {
	case ND_ADD:
		return eval2(node->lhs, label) + eval(node->rhs);
	case ND_SUB:
		return eval2(node->lhs, label) - eval(node->rhs);
	case ND_MUL:
		return eval(node->lhs) * eval(node->rhs);
	case ND_DIV:
		return eval(node->lhs) / eval(node->rhs);
	case ND_NEG:
		return -eval(node->lhs);
	case ND_MOD:
		return eval(node->lhs) % eval(node->rhs);
	case ND_BITAND:
		return eval(node->lhs) & eval(node->rhs);
	case ND_BITOR:
		return eval(node->lhs) | eval(node->rhs);
	case ND_BITXOR:
		return eval(node->lhs) ^ eval(node->rhs);
	case ND_SHL:
		return eval(node->lhs) << eval(node->rhs);
	case ND_SHR:
		return eval(node->lhs) >> eval(node->rhs);
	case ND_EQ:
		return eval(node->lhs) == eval(node->rhs);
	case ND_NE:
		return eval(node->lhs) != eval(node->rhs);
	case ND_LT:
		return eval(node->lhs) < eval(node->rhs);
	case ND_LE:
		return eval(node->lhs) <= eval(node->rhs);
	case ND_COND:
		return eval(node->condition) ? eval2(node->then_, label) :
					       eval2(node->else_, label);
	case ND_COMMA:
		return eval2(node->rhs, label);
	case ND_NOT:
		return !eval(node->lhs);
	case ND_BITNOT:
		return ~eval(node->lhs);
	case ND_LOGAND:
		return eval(node->lhs) && eval(node->rhs);
	case ND_LOGOR:
		return eval(node->lhs) || eval(node->rhs);
	case ND_CAST: {
		int64_t val = eval2(node->lhs, label);
		if (is_integer(node->type)) {
			switch (node->type->size) {
			case 1:
				return (uint8_t)val;
			case 2:
				return (uint16_t)val;
			case 4:
				return (uint32_t)val;
			}
		}
		return val;
	}
	case ND_ADDR:
		return eval_rval(node->lhs, label);
	case ND_MEMBER:
		// no label address, means not a const expr
		if (!label)
			error_token(node->tok, "not a compile-time constant");
		// could not be an array
		if (node->type->kind != TY_ARRAY)
			error_token(node->tok, "invalid initializer");
		// return left value(and parse label) + member offset
		return eval_rval(node->lhs, label) + node->member->offset;
	case ND_VAR:
		// no label address, means not a const expr
		if (!label)
			error_token(node->tok, "not a compile-time constant");
		// could not be an array or function
		if (node->var->type->kind != TY_ARRAY &&
		    node->var->type->kind != TY_FUNC)
			error_token(node->tok, "invalid initializer");
		*label = node->var->name;
		return 0;
	case ND_NUM:
		return node->val;
	default:
		break;
	}

	error_token(node->tok, "not a compile-time constant");
	return -1;
}

// calculate relocation variables
static int64_t eval_rval(struct AstNode *node, char **label)
{
	switch (node->kind) {
	case ND_VAR:
		// local variable could not take part in global variable initialize
		if (node->var->is_local)
			error_token(node->tok, "not a compile-time constant");
		*label = node->var->name;
		return 0;
	case ND_DEREF:
		// enter into dereference address
		return eval2(node->lhs, label);
	case ND_MEMBER:
		// plus member offset
		return eval_rval(node->lhs, label) + node->member->offset;
	default:
		break;
	}

	error_token(node->tok, "invalid initializer");
	return -1;
}

// Parsing constant expressions
static int64_t const_expr(struct Token **rest, struct Token *token)
{
	// construct constant expression
	struct AstNode *node = conditional(rest, token);
	// compute constant expression
	return eval(node);
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

// assign = conditional (assignOp assign)?
// assignOp = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
//          | "<<=" | ">>="
static struct AstNode *assign(struct Token **rest, struct Token *token)
{
	// conditional
	struct AstNode *node = conditional(&token, token);

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

	// ("<<=" assign)?
	if (equal(token, "<<="))
		return to_assign(new_binary_tree_node(
			ND_SHL, node, assign(rest, token->next), token));

	// (">>=" assign)?
	if (equal(token, ">>="))
		return to_assign(new_binary_tree_node(
			ND_SHR, node, assign(rest, token->next), token));

	*rest = token;
	return node;
}

// parse conditional operator
// conditional = logOr ("?" expr ":" conditional)?
static struct AstNode *conditional(struct Token **rest, struct Token *token)
{
	// lohOr
	struct AstNode *cond = log_or(&token, token);

	// "?"
	if (!equal(token, "?")) {
		*rest = token;
		return cond;
	}

	// expr ":" conditional
	struct AstNode *node = new_astnode(ND_COND, token);
	node->condition = cond;
	// expr ":"
	node->then_ = expr(&token, token->next);
	token = skip(token, ":");
	// conditional, could not be parsed to assign stmt
	node->else_ = conditional(rest, token);
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

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static struct AstNode *relational(struct Token **rest, struct Token *token)
{
	// shift
	struct AstNode *node = shift(&token, token);

	// ("<" shift | "<=" shift | ">" shift | ">=" shift)*
	while (true) {
		struct Token *start = token;
		// "<" shift
		if (equal(token, "<")) {
			node = new_binary_tree_node(
				ND_LT, node, shift(&token, token->next), start);
			continue;
		}
		// "<=" shift
		if (equal(token, "<=")) {
			node = new_binary_tree_node(
				ND_LE, node, shift(&token, token->next), start);
			continue;
		}
		// ">" shift, X>Y equal Y<X
		if (equal(token, ">")) {
			node = new_binary_tree_node(
				ND_LT, shift(&token, token->next), node, start);
			continue;
		}
		// ">=" shift
		if (equal(token, ">=")) {
			node = new_binary_tree_node(
				ND_LE, shift(&token, token->next), node, start);
			continue;
		}

		*rest = token;
		return node;
	}
}

// parse shift
// shift = add ("<<" add | ">>" add)*
static struct AstNode *shift(struct Token **rest, struct Token *token)
{
	// add
	struct AstNode *node = add(&token, token);

	while (true) {
		struct Token *start = token;

		// "<<" add
		if (equal(token, "<<")) {
			node = new_binary_tree_node(
				ND_SHL, node, add(&token, token->next), start);
			continue;
		}

		// ">>" add
		if (equal(token, ">>")) {
			node = new_binary_tree_node(
				ND_SHR, node, add(&token, token->next), start);
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

		// compound literals
		if (equal(token, "{"))
			return unary(rest, start);

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
	// record the index value of a member
	int idx = 0;

	while (!equal(token, "}")) {
		// declspec
		struct VarAttr attr = {};
		struct Type *base_ty = declspec(&token, token, &attr);
		int first = true;

		while (!consume(&token, token, ";")) {
			if (!first)
				token = skip(token, ",");
			first = false;

			struct Member *member =
				calloc(1, sizeof(struct Member));
			// declarator
			member->type = declarator(&token, token, base_ty);
			member->name = member->type->name;
			// the index value corresponding to the member
			member->idx = idx++;
			// set align
			member->align = attr.align ? attr.align :
						     member->type->align;
			cur = cur->next = member;
		}
	}

	// parsing flexible array member with array size set to 0
	if (cur != &head && cur->type->kind == TY_ARRAY &&
	    cur->type->array_len < 0) {
		cur->type = array_of(cur->type->base, 0);
		// set type flexible
		type->is_flexible = true;
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

	// construct incomplete struct
	if (tag && !equal(token, "{")) {
		*rest = token;

		struct Type *ty = find_tag(tag);
		if (ty)
			return ty;

		ty = struct_type();
		ty->size = -1;
		push_tag_scope(tag, ty);
		return ty;
	}

	// ("{" structMembers)?
	token = skip(token, "{");

	// construct a struct
	struct Type *ty = struct_type();
	struct_members(rest, token, ty);

	// if have a tag, reg the structure type
	if (tag) {
		for (struct TagScope *tsp = Scp->tags; tsp; tsp = tsp->next) {
			if (equal(tag, tsp->name)) {
				*tsp->type = *ty;
				return tsp->type;
			}
		}
		push_tag_scope(tag, ty);
	}
	return ty;
}

// structDecl = structUnionDecl
static struct Type *struct_decl(struct Token **rest, struct Token *token)
{
	struct Type *ty = struct_union_decl(rest, token);
	ty->kind = TY_STRUCT;

	// incomplete struct
	if (ty->size < 0)
		return ty;

	// caculate the offset of struct members
	int offset = 0;
	for (struct Member *mem = ty->member; mem; mem = mem->next) {
		offset = align_to(offset, mem->align);
		mem->offset = offset;
		offset += mem->type->size;

		if (ty->align < mem->align)
			ty->align = mem->align;
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
		if (ty->align < mem->align)
			ty->align = mem->align;
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

// postfix = "(" typeName ")" "{" initializerList "}"
//         | primary ("[" expr "]" | "." ident)* | "->" ident | "++" | "--")*
static struct AstNode *postfix(struct Token **rest, struct Token *token)
{
	// "(" typeName ")" "{" initializerList "}"
	if (equal(token, "(") && is_typename(token->next)) {
		// compound literals
		struct Token *start = token;
		struct Type *type = type_name(&token, token->next);
		token = skip(token, ")");

		if (Scp->next == NULL) {
			struct Obj_Var *var = new_anon_gvar(type);
			gvar_initializer(rest, token, var);
			return new_var_astnode(var, start);
		}

		struct Obj_Var *var = new_lvar("", type);
		struct AstNode *lhs = lvar_initializer(rest, token, var);
		struct AstNode *rhs = new_var_astnode(var, token);
		return new_binary_tree_node(ND_COMMA, lhs, rhs, start);
	}

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
//         | "_Alignof" "(" typeName ")"
//         | "_Alignof" unary
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
	// "_Alignof" "(" typeName ")"
	// read type's align
	if (equal(token, "_Alignof") && equal(token->next, "(") &&
	    is_typename(token->next->next)) {
		struct Type *type = type_name(&token, token->next->next);
		*rest = skip(token, ")");
		return new_num_astnode(type->align, token);
	}
	// "_Alignof" unary
	// read variable's align
	if (equal(token, "_Alignof")) {
		struct AstNode *node = unary(rest, token->next);
		add_type(node);
		return new_num_astnode(node->type->align, token);
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

// match goto and label
// Because the label may appear after goto, \
// you have to parse the function before parsing the goto and label
static void resolve_goto_labels(void)
{
	// iterate through all goto to match label
	for (struct AstNode *X = Gotos; X; X = X->goto_next) {
		for (struct AstNode *Y = Labels; Y; Y = Y->goto_next) {
			if (!strcmp(X->label, Y->label)) {
				X->unique_label = Y->unique_label;
				break;
			}
		}

		if (X->unique_label == NULL)
			error_token(X->tok->next, "use of undeclared label");
	}

	Gotos = NULL;
	Labels = NULL;
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
	// process goto and label
	resolve_goto_labels();
	return token;
}

// construct globalVariable
static struct Token *global_variable(struct Token *token,
				     struct Type *base_type,
				     struct VarAttr *attr)
{
	bool first = true;

	while (!consume(&token, token, ";")) {
		if (!first)
			token = skip(token, ",");
		first = false;

		struct Type *type = declarator(&token, token, base_type);
		// global variable initialize
		struct Obj_Var *var = new_gvar(get_ident(type->name), type);
		// whether definitions exist
		var->is_definition = !attr->is_extern;
		// whether transfered is static
		var->is_static = attr->is_static;
		// if set, cover global variable's align
		if (attr->align)
			var->align = attr->align;

		if (equal(token, "="))
			gvar_initializer(&token, token->next, var);
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
		token = global_variable(token, base_type, &attr);
	}

	return Globals;
}
