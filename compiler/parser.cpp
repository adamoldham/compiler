/*
 * parser.cpp
 *
 * Author: Adam Oldham
 * Course: CSC-4510 Programming Language Design and Translation
 *
 * Phase 2 (parser). LL(1) recursive-descent parser for lille.
 * SKELETON: every function is an empty stub to be filled in.
 */

#include "parser.h"
#include "lille_exception.h"

using namespace std;

//////////////////////////////////////// setup and helpers ////////////////////////////////////////

parser::parser(scanner* s, error_handler* e) : scan(s), err(e)
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
// from the error handler), then stop parsing.
{
	err->flag(scan->this_token(), error_no);
	throw syntax_abort();
}

void parser::parse()
// entry point. the scanner does not read a token on construction, so the first
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
{
	// TODO
	return false;
}

bool parser::first_of_statement()
{
	// TODO
	return false;
}

void parser::parse_expr_list()
{
	// TODO: <expr> { , <expr> }*
}

//////////////////////////////////////// program and block ////////////////////////////////////////

void parser::parse_prog()
// <prog> ::= program <ident> is <block> ;
// FIRST(prog) = { program }    FOLLOW(prog) = { end_of_program }
{
	expect(symbol::program_sym);
	expect(symbol::identifier);
	expect(symbol::is_sym);
	parse_block();
	expect(symbol::semicolon_sym);

	// FOLLOW(prog) is only end_of_program, so nothing may come after the final ';'
	if (!scan->have(symbol::end_of_program))
		syntax_error(77); // no symbols permitted after end of program
}

void parser::parse_block()
{
	// TODO: <block> ::= { <declaration> }* begin <statement_list> end [ <ident> ]
}

//////////////////////////////////////// declarations ////////////////////////////////////////

void parser::parse_declaration()
{
	// TODO: <ident_list> : [ constant ] <type> [ := literal ] ; | procedure ... | function ...
}

void parser::parse_type()
{
	// TODO: integer | real | string | boolean
}

void parser::parse_param_list()
{
	// TODO: <param> { ; <param> }*
}

void parser::parse_param()
{
	// TODO: <ident_list> : <param_kind> <type>
}

void parser::parse_ident_list()
{
	// TODO: <ident> { , <ident> }*
}

void parser::parse_param_kind()
{
	// TODO: value | ref
}

//////////////////////////////////////// statements ////////////////////////////////////////

void parser::parse_statement_list()
{
	// TODO: <statement> ; { <statement> ; }*
}

void parser::parse_statement()
{
	// TODO: <simple_statement> | <compound_statement>
}

void parser::parse_simple_statement()
{
	// TODO: call/assign, exit, return, read, write, writeln, null
}

void parser::parse_compound_statement()
{
	// TODO: if | loop | for | while
}

void parser::parse_if_statement()
{
	// TODO
}

void parser::parse_while_statement()
{
	// TODO
}

void parser::parse_for_statement()
{
	// TODO
}

void parser::parse_loop_statement()
{
	// TODO
}

//////////////////////////////////////// expressions ////////////////////////////////////////

void parser::parse_range()
{
	// TODO: <simple_expr> .. <simple_expr>
}

void parser::parse_expr()
{
	// TODO: <simple_expr> [ <relop> <simple_expr> ] | <simple_expr> in <range>
}

void parser::parse_bool()
{
	// TODO: true | false
}

void parser::parse_simple_expr()
{
	// TODO: <expr2> { & <expr2> }*
}

void parser::parse_expr2()
{
	// TODO: <term> { { + | - | or } <term> }*
}

void parser::parse_term()
{
	// TODO: <factor> { { * | / | and } <factor> }*
}

void parser::parse_factor()
{
	// TODO: <primary> [ ** <primary> ] | [ + | - ] <primary>
}

void parser::parse_primary()
{
	// TODO: not | odd | ( simple_expr ) | ident [ ( args ) ] | number | string | bool
}
