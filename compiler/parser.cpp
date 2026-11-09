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
// from the error handler), then stop parsing
{
	err->flag(scan->this_token(), error_no);
	throw syntax_abort();
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

void parser::parse_expr_list()
// <expr> { , <expr> }*

// helper, not a grammar nonterminal. a comma is the only token that continues the list.
{
	parse_expr();
	while (scan->have(symbol::comma_sym))
	{
		expect(symbol::comma_sym);
		parse_expr();
	}
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
// <block> ::= { <declaration> }* begin <statement_list> end [ <ident> ]

// FIRST(block) = { identifier, procedure, function, begin }    FOLLOW(block) = { ; }
// the declarations loop runs while the token is in FIRST(declaration). because it
// is a loop, declarations can only come before begin: after begin the parser reads
// statements, so a declaration there fails as a statement. if the token is not a
// declaration start it must be begin (the only other member of FIRST(block)).
// the name after end is optional and is taken when the token is an identifier,
// since FOLLOW(block) = { ; } does not contain identifier.
{
	while (scan->have(symbol::identifier) or scan->have(symbol::procedure_sym)
		or scan->have(symbol::function_sym))
		parse_declaration();

	if (!scan->have(symbol::begin_sym))
		syntax_error(78); // declaration or 'begin' expected
	expect(symbol::begin_sym);
	parse_statement_list();
	expect(symbol::end_sym);
	if (scan->have(symbol::identifier))
		expect(symbol::identifier);
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
		expect(symbol::identifier);
		if (scan->have(symbol::left_paren_sym)) // optional parameter list, FOLLOW is { is }
		{
			expect(symbol::left_paren_sym);
			parse_param_list();
			expect(symbol::right_paren_sym);
		}
		expect(symbol::is_sym);
		parse_block();
		expect(symbol::semicolon_sym);
	}
	else if (scan->have(symbol::function_sym))
	{
		expect(symbol::function_sym);
		expect(symbol::identifier);
		if (scan->have(symbol::left_paren_sym)) // optional parameter list, FOLLOW is { return }
		{
			expect(symbol::left_paren_sym);
			parse_param_list();
			expect(symbol::right_paren_sym);
		}
		expect(symbol::return_sym);
		parse_type();
		expect(symbol::is_sym);
		parse_block();
		expect(symbol::semicolon_sym);
	}
	else if (scan->have(symbol::identifier))
	{
		// variable or constant declaration
		parse_ident_list();
		expect(symbol::colon_sym);
		if (scan->have(symbol::constant_sym))
			expect(symbol::constant_sym);
		parse_type();
		// optional initial value. whether := is legal (constants only) is a semantic
		// check for phase 3. the grammar only allows a number, string or bool after it.
		if (scan->have(symbol::becomes_sym))
		{
			expect(symbol::becomes_sym);
			if (scan->have(symbol::integer))
				expect(symbol::integer);
			else if (scan->have(symbol::real_num))
				expect(symbol::real_num);
			else if (scan->have(symbol::strng))
				expect(symbol::strng);
			else if (scan->have(symbol::true_sym) or scan->have(symbol::false_sym))
				parse_bool();
			else
				syntax_error(112); // literal expected
		}
		expect(symbol::semicolon_sym);
	}
	else
		syntax_error(106); // variable, procedure or function declaration expected
}

void parser::parse_type()
// <type> ::= integer | real | string | boolean

// FIRST(type) = { integer, real, string, boolean }
// FOLLOW(type) = { :=, ;, is, ) }
{
	if (scan->have(symbol::integer_sym) or scan->have(symbol::real_sym)
		or scan->have(symbol::string_sym) or scan->have(symbol::boolean_sym))
		scan->get_token(); // consume the type keyword (already identified by have())
	else
		syntax_error(96); // type name integer, real, string or boolean expected
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
	parse_ident_list();
	expect(symbol::colon_sym);
	parse_param_kind();
	parse_type();
}

void parser::parse_ident_list()
// <ident_list> ::= <ident> { , <ident> }*

// FIRST(ident_list) = { identifier }    FOLLOW(ident_list) = { : }
// a ',' continues the list. ':' (in FOLLOW) ends it.
{
	expect(symbol::identifier);
	while (scan->have(symbol::comma_sym))
	{
		expect(symbol::comma_sym);
		expect(symbol::identifier);
	}
}

void parser::parse_param_kind()
// <param_kind> ::= value | ref

// FIRST(param_kind) = { value, ref }
{
	if (scan->have(symbol::value_sym) or scan->have(symbol::ref_sym))
		scan->get_token(); // consume value or ref (already identified by have())
	else
		syntax_error(94); // parameter mode expected
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
		expect(symbol::identifier);
		if (scan->have(symbol::becomes_sym))
		{
			expect(symbol::becomes_sym);
			parse_expr();
		}
		else if (scan->have(symbol::left_paren_sym))
		{
			expect(symbol::left_paren_sym);
			parse_expr_list();
			expect(symbol::right_paren_sym);
		}
		// else: empty. a call with no arguments. the next token must be ';' (FOLLOW).
	}
	else if (scan->have(symbol::exit_sym))
	{
		expect(symbol::exit_sym);
		if (scan->have(symbol::when_sym))
		{
			expect(symbol::when_sym); // safe: have() matched. must_be() text for 'when' is wrong in the starter code.
			parse_expr();
		}
	}
	else if (scan->have(symbol::return_sym))
	{
		expect(symbol::return_sym);
		if (first_of_expr()) // FIRST(expr) is disjoint from FOLLOW = { ; }
			parse_expr();
	}
	else if (scan->have(symbol::read_sym))
	{
		expect(symbol::read_sym);
		// the ( is the optional wrapper paren. an identifier follows, never an
		// expression, so there is no ambiguity here.
		bool paren = scan->have(symbol::left_paren_sym);
		if (paren)
			expect(symbol::left_paren_sym);
		expect(symbol::identifier);
		while (scan->have(symbol::comma_sym))
		{
			expect(symbol::comma_sym);
			expect(symbol::identifier);
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
		parse_expr_list();
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
			parse_expr_list();
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
	parse_expr();
	expect(symbol::then_sym);
	parse_statement_list();
	while (scan->have(symbol::elsif_sym))
	{
		expect(symbol::elsif_sym);
		parse_expr();
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
	parse_expr();
	parse_loop_statement();
}

void parser::parse_for_statement()
// <for_statement> ::= for <ident> in [ reverse ] <range> <loop_statement>

// reverse is optional. FIRST(range) = FIRST(simple_expr) does not contain reverse.
{
	expect(symbol::for_sym);
	expect(symbol::identifier);
	expect(symbol::in_sym);
	if (scan->have(symbol::reverse_sym))
		expect(symbol::reverse_sym);
	parse_range();
	parse_loop_statement();
}

void parser::parse_loop_statement()
// <loop_statement> ::= loop <statement_list> end loop
{
	expect(symbol::loop_sym);
	parse_statement_list();
	expect(symbol::end_sym);
	expect(symbol::loop_sym);
}

//////////////////////////////////////// expressions ////////////////////////////////////////

void parser::parse_range()
// <range> ::= <simple_expr> .. <simple_expr>

// FIRST(range) = FIRST(simple_expr)    FOLLOW(range) = FOLLOW(expr) + { loop }
{
	parse_simple_expr();
	expect(symbol::range_sym);
	parse_simple_expr();
}

void parser::parse_expr()
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
	parse_simple_expr();
	if (scan->have(symbol::equals_sym) or scan->have(symbol::not_equals_sym)
		or scan->have(symbol::less_than_sym) or scan->have(symbol::less_or_equal_sym)
		or scan->have(symbol::greater_than_sym) or scan->have(symbol::greater_or_equal_sym))
	{
		scan->get_token(); // consume the relop (already identified by have())
		parse_simple_expr();
	}
	else if (scan->have(symbol::in_sym))
	{
		expect(symbol::in_sym);
		parse_range();
	}
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

void parser::parse_simple_expr()
// <simple_expr> ::= <expr2> { <stringop> <expr2> }*      <stringop> ::= &

// the loop repeats while the next token is &
{
	parse_expr2();
	while (scan->have(symbol::ampersand_sym))
	{
		expect(symbol::ampersand_sym);
		parse_expr2();
	}
}

void parser::parse_expr2()
// <expr2> ::= <term> { { <addop> | or } <term> }*       <addop> ::= + | -

// repeat while the next token is +, - or or. a loop (not recursion) avoids
// left recursion and makes the operators left associative.
{
	parse_term();
	while (scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym)
		or scan->have(symbol::or_sym))
	{
		scan->get_token(); // consume + - or or (already identified by have())
		parse_term();
	}
}

void parser::parse_term()
// <term> ::= <factor> { { <multop> | and } <factor> }*  <multop> ::= * | /

// repeat while the next token is *, / or and
{
	parse_factor();
	while (scan->have(symbol::asterisk_sym) or scan->have(symbol::slash_sym)
		or scan->have(symbol::and_sym))
	{
		scan->get_token(); // consume * / or and (already identified by have())
		parse_factor();
	}
}

void parser::parse_factor()
// <factor> ::= <primary> [ ** <primary> ]
//            | [ <addop> ] <primary>

// FIRST(primary) does not contain + or -, so a leading sign selects the second
// alternative. only the first alternative may be followed by **.
{
	if (scan->have(symbol::plus_sym) or scan->have(symbol::minus_sym))
	{
		scan->get_token(); // consume the unary sign
		parse_primary();
	}
	else
	{
		parse_primary();
		if (scan->have(symbol::power_sym)) // ** is optional
		{
			expect(symbol::power_sym);
			parse_primary();
		}
	}
}

void parser::parse_primary()
// <primary> ::= not <expr> | odd <expr> | ( <simple_expr> )
//             | <ident> [ ( <expr> { , <expr> }* ) ]
//             | <number> | <string> | <bool>

// every alternative starts with a different token, so one token of lookahead
// chooses. for <ident>, a following ( means a function call with arguments,
// otherwise the identifier stands alone (( is not in FOLLOW(primary)).
// eof is not part of the grammar, so it is not accepted here.
{
	if (scan->have(symbol::not_sym))
	{
		expect(symbol::not_sym);
		parse_expr();
	}
	else if (scan->have(symbol::odd_sym))
	{
		expect(symbol::odd_sym);
		parse_expr();
	}
	else if (scan->have(symbol::left_paren_sym))
	{
		expect(symbol::left_paren_sym);
		parse_simple_expr(); // the grammar allows a simple_expr here, not a full expr
		expect(symbol::right_paren_sym);
	}
	else if (scan->have(symbol::identifier))
	{
		expect(symbol::identifier);
		if (scan->have(symbol::left_paren_sym))
		{
			expect(symbol::left_paren_sym);
			parse_expr_list();
			expect(symbol::right_paren_sym);
		}
	}
	else if (scan->have(symbol::integer))
		expect(symbol::integer);
	else if (scan->have(symbol::real_num))
		expect(symbol::real_num);
	else if (scan->have(symbol::strng))
		expect(symbol::strng);
	else if (scan->have(symbol::true_sym) or scan->have(symbol::false_sym))
		parse_bool();
	else
		syntax_error(92); // string or expression expected
}
