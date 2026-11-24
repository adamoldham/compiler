/*
 * idtable.cpp
 *
 * Written:            1 June 2020
 * Last modified:      1 June 2023
 *      Author: Michael Oudshoorn
 *
 * Modified by: Adam Oldham, CSC-4510
 * Phase 3 (semantic analysis). The symbol table is an array of BSTs
 * table[i] holds the identifiers declared at scope level i. Level 0 holds the
 * program name and the predefined conversion functions.
 */

#include <iostream>
#include <string>

#include "token.h"
#include "error_handler.h"
#include "id_table.h"

using namespace std;


id_table::id_table(error_handler* err)
{
    error = err;
    for (int i = 0; i < MAX_LEVELS; i++)
    {
        table[i] = NULL; // every scope starts with an empty tree
        next_offset[i] = 0;
    }
    current_level = 0; // level 0 is the outermost (global) scope

    // predefined functions, each takes one value parameter. the handout types the
    // argument as ARITH, so an integer or a real is accepted.
    add_builtin("INT2REAL", lille_type::type_arith, lille_type::type_real);
    add_builtin("REAL2INT", lille_type::type_arith, lille_type::type_integer);
    add_builtin("INT2STRING", lille_type::type_arith, lille_type::type_string);
    add_builtin("REAL2STRING", lille_type::type_arith, lille_type::type_string);
}

// destructor runs when table is destroyed and frees any scopes still open
id_table::~id_table()
{
    for (int i = 0; i < MAX_LEVELS; i++)
        bst_free(table[i]);
}


void id_table::add_builtin(string name, lille_type::lille_ty param_ty, lille_type::lille_ty ret_ty)
{
    id_node* n = insert(name, 0, 0, lille_type(lille_type::type_func), lille_kind(lille_kind::unknown));
    n->params.push_back({"X", lille_type(param_ty), lille_kind(lille_kind::value_param)});
    n->return_type = lille_type(ret_ty);
}


int id_table::get_level()
{
    return current_level;
}


void id_table::enter_scope()
{
    if (current_level + 1 >= MAX_LEVELS)
        throw lille_exception("Internal compiler error. Scopes nested too deeply.");
    current_level++; // table[current_level] is already NULL, so the scope starts empty
}


void id_table::exit_scope()
{
    dump_id_table(true); // print BEFORE the identifiers are removed
    bst_free(table[current_level]);
    table[current_level] = NULL;
    next_offset[current_level] = 0;
    if (current_level > 0)
        current_level--;
}


id_node* id_table::insert(string name, int line, int pos, lille_type ty, lille_kind kind)
{
    id_node* n = new id_node;
    n->name = name;
    n->type = ty;
    n->kind = kind;
    n->level = current_level;
    n->line = line;
    n->pos = pos;
    n->offset = next_offset[current_level]++;
    n->trace = false;
    n->int_val = 0;
    n->real_val = 0.0f;
    n->bool_val = false;
    n->left = n->right = NULL;

    if (bst_insert(table[current_level], n) == NULL)
    {
        delete n; // duplicate in this scope, caller reports error 82
        next_offset[current_level]--;
        return NULL;
    }
    return n;
}


id_node* id_table::lookup(string name)
{
    // innermost scope first, so an inner declaration hides an outer one
    for (int lvl = current_level; lvl >= 0; lvl--)
    {
        id_node* n = bst_find(table[lvl], name);
        if (n != NULL)
            return n;
    }
    return NULL;
}


id_node* id_table::lookup_current(string name)
{
    return bst_find(table[current_level], name);
}


id_node* id_table::bst_insert(id_node*& root, id_node* n)
{
    // walks down by name, the reference lets us attach n where the walk ends
    if (root == NULL)
    {
        root = n;
        return n;
    }
    if (n->name < root->name)
        return bst_insert(root->left, n);
    if (n->name > root->name)
        return bst_insert(root->right, n);
    return NULL; // equal names, already declared in this scope
}


id_node* id_table::bst_find(id_node* root, const string& name)
{
    while (root != NULL)
    {
        if (name == root->name)
            return root;
        root = (name < root->name) ? root->left : root->right;
    }
    return NULL;
}


void id_table::bst_free(id_node* root)
{
    if (root == NULL)
        return;
    bst_free(root->left); // postorder so children go before the parent
    bst_free(root->right);
    delete root;
}


void id_table::print_node(id_node* n)
{
    cout << "Token Name: " << n->name
         << " Line No: " << n->line
         << " Position: " << n->pos
         << "  Type: " << n->type.to_string()
         << "  Kind: " << n->kind.to_string()
         << "  Level: " << n->level
         << "  Offset: " << n->offset
         << "  Trace?: " << n->trace;
    if (n->type.is_type(lille_type::type_proc) || n->type.is_type(lille_type::type_func))
    {
        cout << "  #params: " << n->params.size();
        if (n->type.is_type(lille_type::type_func))
            cout << "  Return ty: " << n->return_type.to_string();
    }
    cout << endl;
}


void id_table::bst_print(id_node* root)
{
    if (root == NULL)
        return;
    bst_print(root->left); // inorder: left, node, right gives alphabetical order
    print_node(root);
    bst_print(root->right);
}


void id_table::print_level(int level)
{
    cout << "scope level " << level << endl;
    cout << "------------------" << endl;
    bst_print(table[level]);
}


void id_table::dump_id_table(bool dump_all)
{
	if (!dump_all)
	{
		cout << "Dump of idtable for current scope only." << endl;
		cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		print_level(current_level);
	}
	else
	{
		cout << "Dump of the entire symbol table." << endl;
		cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		for (int lvl = 0; lvl <= current_level; lvl++)
			print_level(lvl);
	}
}
