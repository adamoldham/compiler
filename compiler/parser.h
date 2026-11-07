/*
 * parser.h
 *
 * Author: Adam Oldham
 * Course: CSC-4510 Programming Language Design and Translation
 *
 * Phase 2 (parser). Declares the LL(1) recursive-descent parser for lille.
 * One parse function per grammar nonterminal. Syntax only, no semantic checks.
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "scanner.h"
#include "error_handler.h"
#include "symbol.h"

using namespace std;

// thrown after the first syntax error is reported (error recovery is phase 4).
struct syntax_abort {};

class parser {
private:
	scanner* scan; // supplies tokens: have(), must_be(), this_token()
	error_handler* err; // reports errors with line and position

	// helpers
	void expect(symbol::symbol_type s); // must_be(s), then abort if it reported an error
	void syntax_error(int error_no); // flag error at current token, then abort
	bool first_of_expr(); // current token in FIRST(expr)?
	bool first_of_statement(); // current token in FIRST(statement)?
	void parse_expr_list(); // parses expr { , expr }*

	// one function per nonterminal
	void parse_prog();
	void parse_block();
	void parse_declaration();
	void parse_type();
	void parse_param_list();
	void parse_param();
	void parse_ident_list();
	void parse_param_kind();
	void parse_statement_list();
	void parse_statement();
	void parse_simple_statement();
	void parse_compound_statement();
	void parse_if_statement();
	void parse_while_statement();
	void parse_for_statement();
	void parse_loop_statement();
	void parse_range();
	void parse_expr();
	void parse_bool();
	void parse_simple_expr();
	void parse_expr2();
	void parse_term();
	void parse_factor();
	void parse_primary();

public:
	parser(scanner* s, error_handler* e);

	void parse(); // parse a whole program. Returns normally, even after a syntax error.
};

#endif /* PARSER_H_ */
