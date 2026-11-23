/*
 * parser.h
 *
 * Author: Adam Oldham
 * Course: CSC-4510 Programming Language Design and Translation
 *
 * Phase 2 (parser). Declares the LL(1) recursive-descent parser for lille.
 * One parse function per grammar nonterminal. Syntax only, no semantic checks.
 *
 * Phase 3 (semantic analysis), Adam Oldham, CSC-4510. The parser now holds a
 * pointer to the symbol table so that it can declare and look up identifiers.
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "scanner.h"
#include "error_handler.h"
#include "symbol.h"
#include <vector>
#include "id_table.h"

using namespace std;

// thrown after the first syntax error is reported
struct syntax_abort {};

// one formal parameter, kept until the procedure or function's own scope exists
struct param_decl {
	token tok; // name, line and position
	lille_type type;
	lille_kind kind; // value_param or ref_param
};

class parser {
private:
	scanner* scan; // supplies tokens: have(), must_be(), this_token()
	error_handler* err; // reports errors with line and position
	id_table* id_tab; // symbol table, one BST per scope level
	vector<param_decl> pending_params; // formals parsed but not yet declared (see parse_block)

	// helpers
	void expect(symbol::symbol_type s); // must_be(s), then abort if it reported an error
	void syntax_error(int error_no); // flag error at current token, then abort
	void semantic_error(token* t, int error_no); // flag error at token t, then abort
	void declare_procedure(token& name, lille_type ty, lille_type ret); // declare procedure or function
	bool first_of_expr(); // current token in FIRST(expr)?
	bool first_of_statement(); // current token in FIRST(statement)?
	void parse_expr_list(); // parses expr { , expr }*

	// one function per nonterminal
	void parse_prog();
	void parse_block(const string& block_name, int mismatch_error);
	void parse_declaration();
	lille_type parse_type();
	void parse_param_list();
	void parse_param();
	void parse_ident_list(vector<token>& ids);
	lille_kind parse_param_kind();
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
	parser(scanner* s, error_handler* e, id_table* t);

	void parse(); // parse whole program
};

#endif /* PARSER_H_ */
