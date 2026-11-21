/*
 * idtable.h
 *
 * Written:            1 June 2020
 * Last modified:      1 June 2023
 *      Author: Michael Oudshoorn
 *
 * Modified by: Adam Oldham, CSC-4510
 * Phase 3 (semantic analysis). Symbol table implemented as an array of
 * binary search trees, one BST per scope level.
 */

#ifndef ID_TABLE_H_
#define ID_TABLE_H_

#include <iostream>
#include <string>
#include <vector>

#include "token.h"
#include "error_handler.h"
#include "lille_type.h"
#include "lille_kind.h"

using namespace std;

// one formal parameter of a procedure or function
struct param_info {
	string name;
	lille_type type;
	lille_kind kind; // value_param or ref_param
};

// one identifier, and one node of a scope's BST
struct id_node {
	string name; // identifiers are already upper case from the scanner
	lille_type type; // integer, real, string, boolean, proc, func, prog
	lille_kind kind; // variable, constant, value_param, ref_param, for_ident
	int level; // scope level where declared
	int line; // declaration line
	int pos; // declaration position on the line
	int offset; // not meaningful until code generation
	bool trace; // pragma trace flag

	// constant values, valid when kind is constant
	int int_val;
	float real_val;
	string str_val;
	bool bool_val;

	// procedures and functions only
	vector<param_info> params;
	lille_type return_type;

	id_node* left; // BST links, ordered by name
	id_node* right;
};

class id_table {
private:
	static const int MAX_LEVELS = 100;

	error_handler* error;
	id_node* table[MAX_LEVELS]; // table[i] is the root of the BST for scope level i
	int next_offset[MAX_LEVELS]; // next free offset in each scope
	int current_level; // innermost active scope

	id_node* bst_insert(id_node*& root, id_node* n); // false result is a duplicate
	id_node* bst_find(id_node* root, const string& name);
	void bst_print(id_node* root); // inorder traversal
	void bst_free(id_node* root);
	void print_node(id_node* n);
	void print_level(int level);
	void add_builtin(string name, lille_type::lille_ty param_ty, lille_type::lille_ty ret_ty);

public:
	id_table(error_handler* err);
	~id_table();

	int get_level(); // current scope level
	void enter_scope(); // open a new, empty scope
	void exit_scope(); // dump all active scopes, then discard the innermost

	// returns the new node, or NULL if name is already declared in the current scope
	id_node* insert(string name, int line, int pos, lille_type ty, lille_kind kind);
	id_node* lookup(string name); // current scope outward to level 0, NULL if not found
	id_node* lookup_current(string name); // current scope only, NULL if not found

	void dump_id_table(bool dump_all = true);
};

#endif /* ID_TABLE_H_ */
