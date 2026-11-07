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
{
	// TODO: must_be(s), compare error_count() before and after, throw syntax_abort
}

void parser::syntax_error(int error_no)
{
	// TODO: err->flag(scan->this_token(), error_no); throw syntax_abort
}

void parser::parse()
{
	// TODO: get_token() once to prime the scanner, then parse_prog() inside try/catch(syntax_abort)
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
{
	// TODO: <prog> ::= program <ident> is <block> ;
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
