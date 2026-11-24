/*
 * parser.cpp
 *
 * Author: Adam Oldham
 * Course: CSC-4510 Programming Language Design and Translation
 *
 * Phase 2 (parser). LL(1) recursive-descent parser for lille.
 *
 * One parse function per grammar nonterminal. Optional parts [ ] are written as
 * an if and repetition { } as a while. At each choice point one token of
 * lookahead is tested with have() using the FIRST sets, and the FOLLOW sets
 * decide when an optional or empty part is skipped. expect() wraps must_be()
 * and stops at the first syntax error.
 *
 * Phase 3 (semantic analysis), Adam Oldham, CSC-4510. Semantic actions are added
 * to the parse functions: scopes are opened and closed in parse_block and the
 * program name is declared in parse_prog. Declarations (variables, constants,
 * procedures, functions and parameters) are entered in the symbol table and
 * checked for duplicates and constant rules in parse_declaration. Each expression
 * function returns the lille_type of the expression it parsed and checks operand
 * types against the lille type rules. A for loop opens a scope holding its control
 * variable. Assignments, procedure calls, function calls and read, write and writeln
 * are checked against the symbol table (assignable target, compatible types, and
 * argument count, type and mode). Conditions must be boolean, exit must be inside a
 * loop, and return is checked against the enclosing function's return type. A function
 * with no return statement is not reported at compile time (the handout allows delaying it).
 */

#include "parser.h"
#include "lille_exception.h"

using namespace std;

//////////////////////////////////////// setup and helpers ////////////////////////////////////////

static bool is_arith(lille_type t)
// ARITH is INT | REAL
{
	return t.is_type(lille_type::type_integer) or t.is_type(lille_type::type_real);
}

static bool assign_ok(lille_type to, lille_type from)
// can a value of type from be stored in a location of type to? an integer may be
// stored in a real (automatic conversion), the reverse needs int2real/real2int.
// ARITH is the parameter type of the predefined conversion functions.
{
	if (to.is_type(lille_type::type_arith) or to.is_type(lille_type::type_real))
		return is_arith(from);
	return to.get_type() == from.get_type();
}

static lille_type arith_result(lille_type a, lille_type b)
// INT op INT is INT, any REAL operand makes the result REAL
{
	if (a.is_type(lille_type::type_integer) and b.is_type(lille_type::type_integer))
		return lille_type(lille_type::type_integer);
	return lille_type(lille_type::type_real);
}

parser::parser(scanner* s, error_handler* e, id_table* t) : scan(s), err(e), id_tab(t), primary_count(0), last_ident(NULL), loop_depth(0), current_routine(NULL)
{
}

void parser::expect(symbol::symbol_type s)
// require the current token to be s and advance past it. must_be() reports a
// mismatch but neither advances nor throws, so compare the error count before
// and after and stop at the first error.
{
	int errors_before = err->error_count();
	scan->must_be(s);
	if (err->error_count() > errors_before)
		throw syntax_abort();
}

void parser::syntax_error(int error_no)
// report a syntax error at the current token (message, line and position come
// from the error handler), then stop parsing
{
	err->flag(scan->this_token(), error_no);
	throw syntax_abort();
}

void parser::semantic_error(token* t, int error_no)
// report a semantic error at token t, then stop parsing. unlike syntax_error the
// position is the offending identifier or literal, which may be earlier than the
// current token.
{
	err->flag(t, error_no);
	throw syntax_abort();
}

id_node* parser::declare_procedure(token& name, lille_type ty, lille_type ret)
// declare a procedure or function in the enclosing scope using the formals saved in
// pending_params. this happens before its block is parsed so the body can call itself.
{
	id_node* n = id_tab->insert(name.get_identifier_value(), name.get_line_number(),
		name.get_pos_on_line(), ty, lille_kind());
	if (n == NULL)
		semantic_error(&name, 82); // declared twice in the same block
	for (param_decl& p : pending_params)
	{
		if (ty.is_type(lille_type::type_func) and p.kind.is_kind(lille_kind::ref_param))
			semantic_error(&p.tok, 123); // functions may only have value parameters
		n->params.push_back({p.tok.get_identifier_value(), p.type, p.kind});
	}
	n->return_type = ret; // unknown for a procedure
	return n;
}

void parser::parse_routine_block(id_node* routine, token& name)
// parses the block of a procedure or function. remembers which routine the body belongs
// to so return can be checked, and restores the outer routine afterwards because routines
// can be nested.
{
	id_node* outer_routine = current_routine;
	current_routine = routine;

	parse_block(name.get_identifier_value(), 107);

	// whether a function actually executes a return is left until run time, the handout
	// allows delaying that check, so there is no compile-time check for it here
	current_routine = outer_routine;
}

void parser::parse_condition()
// an expression used as a condition (if, elsif, while, exit when) must be boolean
{
	token start = *scan->this_token();
	if (!parse_expr().is_type(lille_type::type_boolean))
		semantic_error(&start, 103);
}

void parser::parse()
// entry point -- the scanner does not read a token on construction, so the first
// token is fetched here before parsing starts. syntax_abort is caught here so
// the driver can still print its final error count.
{
	try
	{
		scan->get_token();
		parse_prog();
	}
	catch (syntax_abort&)
	{
		// the error has already been reported
	}
}

bool parser::first_of_expr()
// FIRST(expr) = FIRST(simple_expr) = FIRST(expr2) = FIRST(term) = FIRST(factor)
//             = { +, -, not, odd, (, identifier, integer, real_num, strng, true, false }
// used to decide whether an optional expression is present
{
	return scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym)
		or scan->have(symbol::not_sym) or scan->have(symbol::odd_sym)
		or scan->have(symbol::left_paren_sym) or scan->have(symbol::identifier)
		or scan->have(symbol::integer) or scan->have(symbol::real_num)
		or scan->have(symbol::strng) or scan->have(symbol::true_sym)
		or scan->have(symbol::false_sym);
}

bool parser::first_of_statement()
// FIRST(statement) = { identifier, exit, return, read, write, writeln, null, if, loop, for, while }
// used to decide whether another statement follows in a statement_list.
// FOLLOW(statement_list) = { end, elsif, else } is disjoint from this set.
{
	return scan->have(symbol::identifier) or scan->have(symbol::exit_sym)
		or scan->have(symbol::return_sym) or scan->have(symbol::read_sym)
		or scan->have(symbol::write_sym) or scan->have(symbol::writeln_sym)
		or scan->have(symbol::null_sym) or scan->have(symbol::if_sym)
		or scan->have(symbol::loop_sym) or scan->have(symbol::for_sym)
		or scan->have(symbol::while_sym);
}

vector<arg_info> parser::parse_expr_list()
// <expr> { , <expr> }*

// helper, not a grammar nonterminal. a comma is the only token that continues the list.
{
	vector<arg_info> args;
	args.push_back(parse_arg());
	while (scan->have(symbol::comma_sym))
	{
		expect(symbol::comma_sym);
		args.push_back(parse_arg());
	}
	return args;
}

arg_info parser::parse_arg()
// parses one expression and also records whether it was just a single identifier. a ref
// parameter needs that, since only a variable can be passed by reference. the check is:
// the argument starts with an identifier, contains exactly one primary, and that primary
// was a plain identifier (not a call or a literal).
{
	arg_info a;
	a.tok = *scan->this_token();
	bool starts_with_id = scan->have(symbol::identifier);
	int saved = primary_count; // an argument can contain a call with its own arguments
	primary_count = 0;
	last_ident = NULL;
	a.type = parse_expr();
	a.ident = (starts_with_id and primary_count == 1) ? last_ident : NULL;
	primary_count = saved;
	return a;
}

void parser::check_arguments(id_node* callee, vector<arg_info>& args, token& name)
// compare the actual arguments of a call with the formals saved in the callee's node
{
	vector<param_info>& formals = callee->params;
	if (args.size() > formals.size())
		semantic_error(&args[formals.size()].tok, 100); // first extra argument
	if (args.size() < formals.size())
		semantic_error(&name, 97); // too few arguments
	for (size_t i = 0; i < formals.size(); i++)
	{
		param_info& f = formals[i];
		arg_info& a = args[i];
		if (f.kind.is_kind(lille_kind::ref_param))
		{
			// assigning to a ref formal changes the actual, so the actual must be a
			// modifiable variable of exactly the same type (no int to real conversion)
			bool modifiable = a.ident != NULL and (a.ident->kind.is_kind(lille_kind::variable)
				or a.ident->kind.is_kind(lille_kind::value_param)
				or a.ident->kind.is_kind(lille_kind::ref_param));
			if (!modifiable)
				semantic_error(&a.tok, 99); // a ref parameter needs a variable
			if (!f.type.is_type(a.type))
				semantic_error(&a.tok, 98);
		}
		else if (!assign_ok(f.type, a.type))
			semantic_error(&a.tok, 98); // value parameter, the type must be assignable
	}
}

void parser::check_output(vector<arg_info> args)
// write and writeln accept a string, integer or real, not a boolean
{
	for (arg_info& a : args)
		if (a.type.is_type(lille_type::type_boolean))
			semantic_error(&a.tok, 84);
}

void parser::parse_read_ident()
// one identifier of a read statement. it must be declared, be modifiable and be an
// integer or real (only numbers can be read)
{
	token name = *scan->this_token();
	expect(symbol::identifier);
	id_node* id = id_tab->lookup(name.get_identifier_value());
	if (id == NULL)
		semantic_error(&name, 81);
	if (!(id->kind.is_kind(lille_kind::variable) or id->kind.is_kind(lille_kind::value_param)
		or id->kind.is_kind(lille_kind::ref_param)))
		semantic_error(&name, 85); // constants, loop variables and procedures cannot be read into
	if (!is_arith(id->type))
		semantic_error(&name, 86);
}

//////////////////////////////////////// program and block ////////////////////////////////////////

void parser::parse_prog()
// <prog> ::= program <ident> is <block> ;

// FIRST(prog) = { program }    FOLLOW(prog) = { end_of_program }
{
	expect(symbol::program_sym);

	// read the name before expect() advances past it, then declare it at level 0
	token* name = scan->this_token();
	string prog_name = name->get_identifier_value(); // saved, the scanner overwrites its token
	id_tab->insert(prog_name, name->get_line_number(),
		name->get_pos_on_line(), lille_type(lille_type::type_prog), lille_kind());
	expect(symbol::identifier);
	expect(symbol::is_sym);
	parse_block(prog_name, 75);
	expect(symbol::semicolon_sym);

	// FOLLOW(prog) is only end_of_program, so nothing may come after the final ';'
	if (!scan->have(symbol::end_of_program))
		syntax_error(77); // no symbols permitted after end of program
}

void parser::parse_block(const string& block_name, int mismatch_error)
// <block> ::= { <declaration> }* begin <statement_list> end [ <ident> ]

// FIRST(block) = { identifier, procedure, function, begin }    FOLLOW(block) = { ; }
// the declarations loop runs while the token is in FIRST(declaration). because it
// is a loop, declarations can only come before begin: after begin the parser reads
// statements, so a declaration there fails as a statement. if the token is not a
// declaration start it must be begin (the only other member of FIRST(block)).
// the name after end is optional and is taken when the token is an identifier,
// since FOLLOW(block) = { ; } does not contain identifier. block_name is the program,
// procedure or function this block belongs to, the name after end must match it.
{
	id_tab->enter_scope(); // everything declared in this block lives one level deeper

	// the formals of a procedure or function were parsed before this scope existed,
	// so they are declared here, at the start of the procedure or function's own scope
	for (param_decl& p : pending_params)
	{
		id_node* n = id_tab->insert(p.tok.get_identifier_value(), p.tok.get_line_number(),
			p.tok.get_pos_on_line(), p.type, p.kind);
		if (n == NULL)
			semantic_error(&p.tok, 82); // two parameters with the same name
	}
	pending_params.clear(); // nested procedures and functions start with their own list

	while (scan->have(symbol::identifier) or scan->have(symbol::procedure_sym)
		or scan->have(symbol::function_sym))
		parse_declaration();

	if (!scan->have(symbol::begin_sym))
		syntax_error(78); // declaration or 'begin' expected
	expect(symbol::begin_sym);
	parse_statement_list();
	expect(symbol::end_sym);
	if (scan->have(symbol::identifier))
	{
		if (scan->this_token()->get_identifier_value() != block_name)
			semantic_error(scan->this_token(), mismatch_error); // end name must match the block
		expect(symbol::identifier);
	}

	id_tab->exit_scope(); // prints the active scopes, then discards this block's scope
}

//////////////////////////////////////// declarations ////////////////////////////////////////

void parser::parse_declaration()
// <declaration> ::= <ident_list> : [ constant ] <type> [ := <number> | := <string> | := <bool> ] ;
//                 | procedure <ident> [ ( <param_list> ) ] is <block> ;
//                 | function <ident> [ ( <param_list> ) ] return <type> is <block> ;

// FIRST(declaration) = { identifier, procedure, function }. each alternative has a
// different first token, so one token of lookahead picks the branch.
// FOLLOW(declaration) = FIRST(declaration) + { begin }.
{
	if (scan->have(symbol::procedure_sym))
	{
		expect(symbol::procedure_sym);
		token name = *scan->this_token(); // copy, the scanner overwrites its token on advance
		expect(symbol::identifier);
		if (scan->have(symbol::left_paren_sym)) // optional parameter list, FOLLOW is { is }
		{
			expect(symbol::left_paren_sym);
			parse_param_list();
			expect(symbol::right_paren_sym);
		}
		id_node* routine = declare_procedure(name, lille_type(lille_type::type_proc), lille_type());
		expect(symbol::is_sym);
		parse_routine_block(routine, name);
		expect(symbol::semicolon_sym);
	}
	else if (scan->have(symbol::function_sym))
	{
		expect(symbol::function_sym);
		token name = *scan->this_token(); // copy, the scanner overwrites its token on advance
		expect(symbol::identifier);
		if (scan->have(symbol::left_paren_sym)) // optional parameter list, FOLLOW is { return }
		{
			expect(symbol::left_paren_sym);
			parse_param_list();
			expect(symbol::right_paren_sym);
		}
		expect(symbol::return_sym);
		lille_type ret = parse_type();
		id_node* routine = declare_procedure(name, lille_type(lille_type::type_func), ret);
		expect(symbol::is_sym);
		parse_routine_block(routine, name);
		expect(symbol::semicolon_sym);
	}
	else if (scan->have(symbol::identifier))
	{
		// variable or constant declaration
		vector<token> ids; // names being declared, with their positions
		parse_ident_list(ids);
		expect(symbol::colon_sym);
		bool is_const = false;
		if (scan->have(symbol::constant_sym))
		{
			expect(symbol::constant_sym);
			is_const = true;
		}
		lille_type ty = parse_type();

		// optional initial value. the grammar only allows a number, string or bool after
		// :=, the semantic rules (constants only, type must match) are checked below.
		bool has_value = false;
		lille_type lit_ty; // type of the literal
		token lit_tok; // copy of the literal token, for its value and error position
		int ival = 0;
		float rval = 0.0f;
		string sval;
		bool bval = false;
		if (scan->have(symbol::becomes_sym))
		{
			expect(symbol::becomes_sym);
			has_value = true;
			lit_tok = *scan->this_token();
			if (scan->have(symbol::integer))
			{
				lit_ty = lille_type(lille_type::type_integer);
				ival = lit_tok.get_integer_value();
				expect(symbol::integer);
			}
			else if (scan->have(symbol::real_num))
			{
				lit_ty = lille_type(lille_type::type_real);
				rval = lit_tok.get_real_value();
				expect(symbol::real_num);
			}
			else if (scan->have(symbol::strng))
			{
				lit_ty = lille_type(lille_type::type_string);
				sval = lit_tok.get_string_value();
				expect(symbol::strng);
			}
			else if (scan->have(symbol::true_sym) or scan->have(symbol::false_sym))
			{
				lit_ty = lille_type(lille_type::type_boolean);
				bval = scan->have(symbol::true_sym);
				parse_bool();
			}
			else
				syntax_error(112); // literal expected
		}
		expect(symbol::semicolon_sym);

		// semantic rules for declarations
		if (is_const and !has_value)
			semantic_error(&ids[0], 110); // a constant needs := and a value
		if (!is_const and has_value)
			semantic_error(&lit_tok, 91); // lille has no variable initialisation
		bool int_to_real = false; // an integer literal may initialise a real constant
		if (is_const)
		{
			int_to_real = ty.is_type(lille_type::type_real) and lit_ty.is_type(lille_type::type_integer);
			if (ty.get_type() != lit_ty.get_type() and !int_to_real)
				semantic_error(&lit_tok, 111); // literal does not match the declared type
		}

		// declare each name only after the whole declaration parsed, so a name is not
		// visible inside its own declaration
		for (token& id : ids)
		{
			id_node* n = id_tab->insert(id.get_identifier_value(), id.get_line_number(),
				id.get_pos_on_line(), ty,
				is_const ? lille_kind(lille_kind::constant) : lille_kind(lille_kind::variable));
			if (n == NULL)
				semantic_error(&id, 82); // already declared in this block
			if (is_const)
			{
				n->int_val = ival;
				n->real_val = int_to_real ? float(ival) : rval;
				n->str_val = sval;
				n->bool_val = bval;
			}
		}
	}
	else
		syntax_error(106); // variable, procedure or function declaration expected
}

lille_type parser::parse_type()
// <type> ::= integer | real | string | boolean

// FIRST(type) = { integer, real, string, boolean }
// FOLLOW(type) = { :=, ;, is, ) }
{
	lille_type ty;
	if (scan->have(symbol::integer_sym))
		ty = lille_type(lille_type::type_integer);
	else if (scan->have(symbol::real_sym))
		ty = lille_type(lille_type::type_real);
	else if (scan->have(symbol::string_sym))
		ty = lille_type(lille_type::type_string);
	else if (scan->have(symbol::boolean_sym))
		ty = lille_type(lille_type::type_boolean);
	else
		syntax_error(96); // type name integer, real, string or boolean expected
	scan->get_token(); // consume the type keyword (already identified by have())
	return ty;
}

void parser::parse_param_list()
// <param_list> ::= <param> { ; <param> }*

// FIRST(param_list) = { identifier }    FOLLOW(param_list) = { ) }
// a ';' continues the list. ')' (in FOLLOW) ends it.
{
	parse_param();
	while (scan->have(symbol::semicolon_sym))
	{
		expect(symbol::semicolon_sym);
		parse_param();
	}
}

void parser::parse_param()
// <param> ::= <ident_list> : <param_kind> <type>
{
	vector<token> ids;
	parse_ident_list(ids);
	expect(symbol::colon_sym);
	lille_kind kind = parse_param_kind();
	lille_type ty = parse_type();
	// the procedure or function's scope does not exist yet, so save the formals for parse_block
	for (token& id : ids)
		pending_params.push_back({id, ty, kind});
}

void parser::parse_ident_list(vector<token>& ids)
// <ident_list> ::= <ident> { , <ident> }*

// FIRST(ident_list) = { identifier }    FOLLOW(ident_list) = { : }
// a ',' continues the list. ':' (in FOLLOW) ends it.
{
	ids.push_back(*scan->this_token()); // copy the token before expect() advances
	expect(symbol::identifier);
	while (scan->have(symbol::comma_sym))
	{
		expect(symbol::comma_sym);
		ids.push_back(*scan->this_token());
		expect(symbol::identifier);
	}
}

lille_kind parser::parse_param_kind()
// <param_kind> ::= value | ref

// FIRST(param_kind) = { value, ref }
{
	lille_kind kind;
	if (scan->have(symbol::value_sym))
		kind = lille_kind(lille_kind::value_param);
	else if (scan->have(symbol::ref_sym))
		kind = lille_kind(lille_kind::ref_param);
	else
		syntax_error(94); // parameter mode expected
	scan->get_token(); // consume value or ref (already identified by have())
	return kind;
}

//////////////////////////////////////// statements ////////////////////////////////////////

void parser::parse_statement_list()
// <statement_list> ::= <statement> ; { <statement> ; }*

// at least one statement is required, so FIRST(statement) is checked first.
// every statement must end with ';'. the loop continues while the next token
// can start another statement and stops on FOLLOW(statement_list) = { end, elsif, else }.
{
	if (!first_of_statement())
		syntax_error(80); // statement expected
	do
	{
		parse_statement();
		expect(symbol::semicolon_sym);
	} while (first_of_statement());
}

void parser::parse_statement()
// <statement> ::= <simple_statement> | <compound_statement>

// FIRST(compound_statement) = { if, loop, for, while } picks the compound branch.
// every other token in FIRST(statement) starts a simple statement.
{
	if (scan->have(symbol::if_sym) or scan->have(symbol::loop_sym)
		or scan->have(symbol::for_sym) or scan->have(symbol::while_sym))
		parse_compound_statement();
	else
		parse_simple_statement();
}

void parser::parse_simple_statement()
// <simple_statement> ::= <ident> [ ( <expr> { , <expr> }* ) ]
//                      | <ident> := <expr>
//                      | exit [ when <expr> ] | return [ <expr> ]
//                      | read [ ( ] <ident> { , <ident> }* [ ) ]
//                      | write [ ( ] <expr> { , <expr> }* [ ) ]
//                      | writeln [ ( ] [ <expr> { , <expr> }* ] [ ) ]
//                      | null

// each alternative starts with its own token, so one token of lookahead picks it.
// the two <ident> alternatives share the prefix <ident>, so after it the next token decides:
//   :=  assignment      (  call with arguments      ;  call with no arguments
// FOLLOW(simple_statement) = { ; }
{
	if (scan->have(symbol::identifier))
	{
		token name = *scan->this_token(); // copy, the scanner overwrites its token on advance
		expect(symbol::identifier);
		id_node* id = id_tab->lookup(name.get_identifier_value());
		if (id == NULL)
			semantic_error(&name, 81); // not declared in this or any enclosing scope

		if (scan->have(symbol::becomes_sym))
		{
			// assignment, the target must be a variable or parameter
			if (!(id->kind.is_kind(lille_kind::variable) or id->kind.is_kind(lille_kind::value_param)
				or id->kind.is_kind(lille_kind::ref_param)))
				semantic_error(&name, 85); // constants, loop variables, procedures and functions
			token op = *scan->this_token();
			expect(symbol::becomes_sym);
			lille_type rhs = parse_expr();
			if (!assign_ok(id->type, rhs))
				semantic_error(&op, 93); // right side does not fit the left side
		}
		else
		{
			// procedure call, with or without arguments
			if (!id->type.is_type(lille_type::type_proc))
				semantic_error(&name, 90); // only a procedure can be called as a statement
			vector<arg_info> args;
			if (scan->have(symbol::left_paren_sym))
			{
				expect(symbol::left_paren_sym);
				args = parse_expr_list();
				expect(symbol::right_paren_sym);
			}
			// else: empty. a call with no arguments. the next token must be ';' (FOLLOW).
			check_arguments(id, args, name);
		}
	}
	else if (scan->have(symbol::exit_sym))
	{
		token ex = *scan->this_token();
		expect(symbol::exit_sym);
		if (loop_depth == 0)
			semantic_error(&ex, 89); // exit leaves a loop, so it must be inside one
		if (scan->have(symbol::when_sym))
		{
			expect(symbol::when_sym); // safe: have() matched. must_be() text for 'when' is wrong in the starter code.
			parse_condition();
		}
	}
	else if (scan->have(symbol::return_sym))
	{
		token ret = *scan->this_token();
		expect(symbol::return_sym);
		bool in_function = current_routine != NULL
			and current_routine->type.is_type(lille_type::type_func);
		if (first_of_expr()) // FIRST(expr) is disjoint from FOLLOW = { ; }
		{
			// only a function returns a value, and it must fit the declared return type
			lille_type t = parse_expr();
			if (current_routine == NULL)
				semantic_error(&ret, 88); // the main program cannot return a value
			if (!in_function or !assign_ok(current_routine->return_type, t))
				semantic_error(&ret, 87); // a procedure has no value to return, or the type is wrong
		}
		else if (in_function)
			semantic_error(&ret, 87); // a function's return needs a value
	}
	else if (scan->have(symbol::read_sym))
	{
		expect(symbol::read_sym);
		// the ( is the optional wrapper paren. an identifier follows, never an
		// expression, so there is no ambiguity here.
		bool paren = scan->have(symbol::left_paren_sym);
		if (paren)
			expect(symbol::left_paren_sym);
		parse_read_ident();
		while (scan->have(symbol::comma_sym))
		{
			expect(symbol::comma_sym);
			parse_read_ident();
		}
		if (paren)
			expect(symbol::right_paren_sym);
	}
	else if (scan->have(symbol::write_sym))
	{
		expect(symbol::write_sym); // safe: have() matched. must_be() text for 'write' is wrong in the starter code.
		// ( is in FIRST(expr) and is also the optional wrapper paren. decision:
		// a ( directly after write/writeln is always the wrapper paren, and a
		// matching ) is then required. so "write (a+b)*c" is not accepted.
		bool paren = scan->have(symbol::left_paren_sym);
		if (paren)
			expect(symbol::left_paren_sym);
		check_output(parse_expr_list());
		if (paren)
			expect(symbol::right_paren_sym);
	}
	else if (scan->have(symbol::writeln_sym))
	{
		expect(symbol::writeln_sym); // safe: have() matched. must_be() text for 'writeln' is wrong in the starter code.
		bool paren = scan->have(symbol::left_paren_sym); // same wrapper paren rule as write
		if (paren)
			expect(symbol::left_paren_sym);
		if (first_of_expr()) // the expression list is optional, FOLLOW = { ; ) }
			check_output(parse_expr_list());
		if (paren)
			expect(symbol::right_paren_sym);
	}
	else if (scan->have(symbol::null_sym))
		expect(symbol::null_sym);
	else
		syntax_error(83); // a simple statement expected
}

void parser::parse_compound_statement()
// <compound_statement> ::= <if_statement> | <loop_statement> | <for_statement> | <while_statement>

// one keyword each: if, loop, for, while
{
	if (scan->have(symbol::if_sym))
		parse_if_statement();
	else if (scan->have(symbol::loop_sym))
		parse_loop_statement();
	else if (scan->have(symbol::for_sym))
		parse_for_statement();
	else if (scan->have(symbol::while_sym))
		parse_while_statement();
	else
		syntax_error(101); // compound statement expected
}

void parser::parse_if_statement()
// <if_statement> ::= if <expr> then <statement_list>
//                    { elsif <expr> then <statement_list> }*
//                    [ else <statement_list> ]
//                    end if

// the elsif loop runs while the next token is elsif and the else part is taken
// only on else. otherwise the token must be in FOLLOW(statement_list) = { end }.
// there is no dangling-else problem because every if is closed by "end if".
{
	expect(symbol::if_sym);
	parse_condition();
	expect(symbol::then_sym);
	parse_statement_list();
	while (scan->have(symbol::elsif_sym))
	{
		expect(symbol::elsif_sym);
		parse_condition();
		expect(symbol::then_sym);
		parse_statement_list();
	}
	if (scan->have(symbol::else_sym))
	{
		expect(symbol::else_sym);
		parse_statement_list();
	}
	expect(symbol::end_sym);
	expect(symbol::if_sym);
}

void parser::parse_while_statement()
// <while_statement> ::= while <expr> <loop_statement>
{
	expect(symbol::while_sym); // safe: only called after have(while_sym). must_be() text for 'while' is wrong in the starter code.
	parse_condition();
	parse_loop_statement();
}

void parser::parse_for_statement()
// <for_statement> ::= for <ident> in [ reverse ] <range> <loop_statement>

// reverse is optional. FIRST(range) = FIRST(simple_expr) does not contain reverse.
{
	expect(symbol::for_sym);
	token var = *scan->this_token(); // copy, the scanner overwrites its token on advance
	expect(symbol::identifier);
	expect(symbol::in_sym);
	if (scan->have(symbol::reverse_sym))
		expect(symbol::reverse_sym);
	parse_range(); // parsed before the variable exists, so the bounds cannot use it

	// the control variable is an implicit integer constant visible only inside the loop
	id_tab->enter_scope();
	id_tab->insert(var.get_identifier_value(), var.get_line_number(), var.get_pos_on_line(),
		lille_type(lille_type::type_integer), lille_kind(lille_kind::for_ident));
	parse_loop_statement();
	id_tab->exit_scope(); // prints the symbol table, then the loop variable disappears
}

void parser::parse_loop_statement()
// <loop_statement> ::= loop <statement_list> end loop
{
	expect(symbol::loop_sym);
	loop_depth++; // exit is legal anywhere inside, including nested loops
	parse_statement_list();
	loop_depth--;
	expect(symbol::end_sym);
	expect(symbol::loop_sym);
}

//////////////////////////////////////// expressions ////////////////////////////////////////

void parser::parse_range()
// <range> ::= <simple_expr> .. <simple_expr>

// FIRST(range) = FIRST(simple_expr)    FOLLOW(range) = FOLLOW(expr) + { loop }
{
	token start = *scan->this_token();
	lille_type lo = parse_simple_expr();
	expect(symbol::range_sym);
	lille_type hi = parse_simple_expr();
	if (!lo.is_type(lille_type::type_integer) or !hi.is_type(lille_type::type_integer))
		semantic_error(&start, 104); // both bounds must be integers
}

lille_type parser::parse_expr()
// <expr> ::= <simple_expr> [ <relop> <simple_expr> ]
//          | <simple_expr> in <range>

// both alternatives start with <simple_expr> (common prefix), so it is parsed
// once (left factoring) and the next token picks the rest:
//   FIRST(relop) = { =, <>, <, <=, >, >= }    relational comparison
//   { in }                                    range test
//   anything else: the optional part is empty (token must be in FOLLOW(expr))
// relop and in are also in FOLLOW(primary) because of "not <expr>" and
// "odd <expr>". this is resolved greedily: the innermost expr takes the
// operator, so "not a = b" means "not (a = b)".
{
	lille_type left = parse_simple_expr();
	if (scan->have(symbol::equals_sym) or scan->have(symbol::not_equals_sym)
		or scan->have(symbol::less_than_sym) or scan->have(symbol::less_or_equal_sym)
		or scan->have(symbol::greater_than_sym) or scan->have(symbol::greater_or_equal_sym))
	{
		token op = *scan->this_token(); // copy, kept for the error position
		scan->get_token(); // consume the relop (already identified by have())
		lille_type right = parse_simple_expr();
		// ARITH relop ARITH or BOOLEAN relop BOOLEAN, the result is BOOLEAN
		bool ok = (is_arith(left) and is_arith(right))
			or (left.is_type(lille_type::type_boolean) and right.is_type(lille_type::type_boolean));
		if (!ok)
			semantic_error(&op, 114); // operand types do not match
		return lille_type(lille_type::type_boolean);
	}
	else if (scan->have(symbol::in_sym))
	{
		token op = *scan->this_token();
		expect(symbol::in_sym);
		if (!left.is_type(lille_type::type_integer))
			semantic_error(&op, 102); // the tested value must be an integer
		parse_range(); // checks both bounds are integers
		return lille_type(lille_type::type_boolean);
	}
	return left; // no relop or in, the type is just the simple_expr's
}

void parser::parse_bool()
// <bool> ::= true | false

// FIRST(bool) = { true, false }
{
	if (scan->have(symbol::true_sym))
		expect(symbol::true_sym);
	else if (scan->have(symbol::false_sym))
		expect(symbol::false_sym);
	else
		syntax_error(112); // literal expected
}

lille_type parser::parse_simple_expr()
// <simple_expr> ::= <expr2> { <stringop> <expr2> }*      <stringop> ::= &

// the loop repeats while the next token is &
{
	lille_type result = parse_expr2();
	while (scan->have(symbol::ampersand_sym))
	{
		token op = *scan->this_token();
		expect(symbol::ampersand_sym);
		lille_type right = parse_expr2();
		if (!result.is_type(lille_type::type_string) or !right.is_type(lille_type::type_string))
			semantic_error(&op, 115); // & needs two strings
		// STRING & STRING is STRING, so result stays STRING
	}
	return result;
}

lille_type parser::parse_expr2()
// <expr2> ::= <term> { { <addop> | or } <term> }*       <addop> ::= + | -

// repeat while the next token is +, - or or. a loop (not recursion) avoids
// left recursion and makes the operators left associative.
{
	lille_type result = parse_term();
	while (scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym)
		or scan->have(symbol::or_sym))
	{
		token op = *scan->this_token();
		scan->get_token(); // consume + - or or (already identified by have())
		lille_type right = parse_term();
		if (op.get_sym() == symbol::or_sym)
		{
			// BOOLEAN or BOOLEAN is BOOLEAN
			if (!result.is_type(lille_type::type_boolean) or !right.is_type(lille_type::type_boolean))
				semantic_error(&op, 117);
		}
		else
		{
			// INT + INT is INT, anything with a REAL is REAL
			if (!is_arith(result) or !is_arith(right))
				semantic_error(&op, 116);
			result = arith_result(result, right);
		}
	}
	return result;
}

lille_type parser::parse_term()
// <term> ::= <factor> { { <multop> | and } <factor> }*  <multop> ::= * | /

// repeat while the next token is *, / or and
{
	lille_type result = parse_factor();
	while (scan->have(symbol::asterisk_sym) or scan->have(symbol::slash_sym)
		or scan->have(symbol::and_sym))
	{
		token op = *scan->this_token();
		scan->get_token(); // consume * / or and (already identified by have())
		lille_type right = parse_factor();
		if (op.get_sym() == symbol::and_sym)
		{
			// BOOLEAN and BOOLEAN is BOOLEAN
			if (!result.is_type(lille_type::type_boolean) or !right.is_type(lille_type::type_boolean))
				semantic_error(&op, 120);
		}
		else
		{
			// same result rules as + and -
			if (!is_arith(result) or !is_arith(right))
				semantic_error(&op, 116);
			result = arith_result(result, right);
		}
	}
	return result;
}

lille_type parser::parse_factor()
// <factor> ::= <primary> [ ** <primary> ]
//            | [ <addop> ] <primary>

// FIRST(primary) does not contain + or -, so a leading sign selects the second
// alternative. only the first alternative may be followed by **.
{
	if (scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym))
	{
		token op = *scan->this_token();
		scan->get_token(); // consume the unary sign
		lille_type t = parse_primary();
		if (!is_arith(t))
			semantic_error(&op, 118); // a sign needs an integer or real
		return t; // the sign does not change the type
	}
	lille_type base = parse_primary();
	if (scan->have(symbol::power_sym)) // ** is optional
	{
		token op = *scan->this_token();
		expect(symbol::power_sym);
		lille_type exponent = parse_primary();
		if (!is_arith(base))
			semantic_error(&op, 116);
		if (!exponent.is_type(lille_type::type_integer))
			semantic_error(&op, 119); // the exponent must be an integer
		return base; // INT ** INT is INT, REAL ** INT is REAL
	}
	return base;
}

lille_type parser::parse_primary()
// <primary> ::= not <expr> | odd <expr> | ( <simple_expr> )
//             | <ident> [ ( <expr> { , <expr> }* ) ]
//             | <number> | <string> | <bool>

// every alternative starts with a different token, so one token of lookahead
// chooses. for <ident>, a following ( means a function call with arguments,
// otherwise the identifier stands alone (( is not in FOLLOW(primary)).
// eof is not part of the grammar, so it is not accepted here.
{
	primary_count++; // parse_arg uses these two to tell whether an argument is a bare identifier
	last_ident = NULL;
	if (scan->have(symbol::not_sym))
	{
		token op = *scan->this_token();
		expect(symbol::not_sym);
		if (!parse_expr().is_type(lille_type::type_boolean))
			semantic_error(&op, 120); // not needs a boolean
		return lille_type(lille_type::type_boolean);
	}
	else if (scan->have(symbol::odd_sym))
	{
		token op = *scan->this_token();
		expect(symbol::odd_sym);
		if (!parse_expr().is_type(lille_type::type_integer))
			semantic_error(&op, 119); // odd needs an integer
		return lille_type(lille_type::type_boolean);
	}
	else if (scan->have(symbol::left_paren_sym))
	{
		expect(symbol::left_paren_sym);
		lille_type t = parse_simple_expr(); // the grammar allows a simple_expr here, not a full expr
		expect(symbol::right_paren_sym);
		return t;
	}
	else if (scan->have(symbol::identifier))
	{
		token name = *scan->this_token(); // copy, the scanner overwrites its token on advance
		expect(symbol::identifier);
		id_node* id = id_tab->lookup(name.get_identifier_value());
		if (id == NULL)
			semantic_error(&name, 81); // not declared in this or any enclosing scope

		vector<arg_info> args;
		bool has_args = scan->have(symbol::left_paren_sym);
		if (has_args)
		{
			if (!id->type.is_type(lille_type::type_func))
				semantic_error(&name, 121); // only a function can be followed by arguments
			expect(symbol::left_paren_sym);
			args = parse_expr_list();
			expect(symbol::right_paren_sym);
		}
		if (id->type.is_type(lille_type::type_func))
		{
			check_arguments(id, args, name); // also catches a function with formals called without ()
			last_ident = NULL; // a call is not a variable
			return id->return_type; // a call's type is the function's return type
		}
		if (id->type.is_type(lille_type::type_proc) or id->type.is_type(lille_type::type_prog))
			semantic_error(&name, 91); // a procedure or the program name is not a value
		last_ident = id; // a plain identifier, parse_arg may need it for ref parameters
		return id->type; // variable, constant or parameter
	}
	else if (scan->have(symbol::integer))
	{
		expect(symbol::integer);
		return lille_type(lille_type::type_integer);
	}
	else if (scan->have(symbol::real_num))
	{
		expect(symbol::real_num);
		return lille_type(lille_type::type_real);
	}
	else if (scan->have(symbol::strng))
	{
		expect(symbol::strng);
		return lille_type(lille_type::type_string);
	}
	else if (scan->have(symbol::true_sym) or scan->have(symbol::false_sym))
	{
		parse_bool();
		return lille_type(lille_type::type_boolean);
	}
	else
		syntax_error(92); // string or expression expected
	return lille_type(); // not reached, syntax_error throws
}
