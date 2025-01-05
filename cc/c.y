%code requires{
	#include "tree.hpp"
	
}

%{
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "tree.hpp"

#define YYDEBUG 1

using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
 
void yyerror(const char *s);

struct TreeNode *node ;

%}


%union {
	int number;
	float float_type;
	char* str;
	TreeNode* node_type;
}

%token<str> IDENTIFIER STRING_LITERAL FUNC_NAME 
%token<number> I_CONSTANT ENUMERATION_CONSTANT
%token<float_type> F_CONSTANT  
%token  SIZEOF
%token	PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME 

%token	TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token	CONST RESTRICT VOLATILE
%token	BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token	COMPLEX IMAGINARY 
%token	STRUCT UNION ENUM ELLIPSIS

%token	CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

%type<node_type> primary_expression constant enumeration_constant string generic_selection generic_assoc_list generic_association postfix_expression argument_expression_list unary_expression unary_operator cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression logical_and_expression logical_or_expression conditional_expression assignment_expression assignment_operator expression constant_expression declaration declaration_specifiers init_declarator_list init_declarator storage_class_specifier type_specifier struct_or_union_specifier struct_or_union struct_declaration_list struct_declaration specifier_qualifier_list struct_declarator_list struct_declarator enum_specifier enumerator_list enumerator atomic_type_specifier type_qualifier function_specifier alignment_specifier declarator direct_declarator pointer type_qualifier_list parameter_type_list parameter_list parameter_declaration identifier_list type_name abstract_declarator direct_abstract_declarator initializer initializer_list designation designator designator_list static_assert_declaration statement labeled_statement compound_statement block_item_list block_item expression_statement selection_statement iteration_statement jump_statement translation_unit external_declaration function_definition declaration_list

%start translation_unit
%%

primary_expression
	: IDENTIFIER {$$=new TreeNode($1);}
	| constant {$$=$1;}
	| string {$$=$1;}
	| '(' expression ')' {$$=$2;}
	| generic_selection {$$=$1;}
	;

constant
	: I_CONSTANT		/* includes character_constant */ {$$=new TreeNode($1);}
	| F_CONSTANT {$$=new TreeNode($1);}
	| ENUMERATION_CONSTANT  /*after it has been defined as such*/  {$$=new TreeNode($1);}
	;

enumeration_constant		/* before it has been defined as such */
	: IDENTIFIER {$$=new TreeNode($1);}
	;

string
	: STRING_LITERAL {$$=new TreeNode($1);}
	| FUNC_NAME {$$=new TreeNode($1);}
	;


generic_selection
	: GENERIC '(' assignment_expression ',' generic_assoc_list ')'  {$$= new TreeNode("generic_selection"); $$->children.push_back($3); $$->children.push_back($5); }
	;

generic_assoc_list
	: generic_association {$$=$1;}
	| generic_assoc_list ',' generic_association {$$= $1; $$->children.push_back($3);}
	;

generic_association
	: type_name ':' assignment_expression {$$= new TreeNode(":"); $$->children.push_back($1); $$->children.push_back($3); }
	| DEFAULT ':' assignment_expression {$$= new TreeNode(":"); $$->children.push_back(new TreeNode("DEFAULT")); $$->children.push_back($3); }
	;


postfix_expression
	: primary_expression {$$=$1;}
	| postfix_expression '[' expression ']' {$$=$1; $$->children.push_back($3);}
	| postfix_expression '(' ')' {$$= $1;}
	| postfix_expression '(' argument_expression_list ')' {$$=$1; $$->children.push_back($3);}
	| postfix_expression '.' IDENTIFIER {$$= $1; $$->children.push_back(new TreeNode($3)); }
	| postfix_expression PTR_OP IDENTIFIER {$$=new TreeNode("PTR_OP"); $$->children.push_back($1); $$->children.push_back(new TreeNode($3));}
	| postfix_expression INC_OP  {$$= new TreeNode("INC_OP"); $$->children.push_back($1);}
	| postfix_expression DEC_OP {$$=new TreeNode("DEC_OP"); $$->children.push_back($1);}
	| '(' type_name ')' '{' initializer_list '}' {$$=new TreeNode("postfix_expression");$$->children.push_back($2);$$->children.push_back($5);}
	| '(' type_name ')' '{' initializer_list ',' '}' {$$=new TreeNode("postfix_expression");$$->children.push_back($2);$$->children.push_back($5);}
	;


argument_expression_list
	: assignment_expression {$$=new TreeNode("argument_expression_list"); $$->children.push_back($1);}
	| argument_expression_list ',' assignment_expression {$$=new TreeNode("argument_expression_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3);}
	;

unary_expression
	: postfix_expression {$$=$1;}
	| INC_OP unary_expression {$$= new TreeNode("INC_OP"); $$->children.push_back($2);}
	| DEC_OP unary_expression {$$= new TreeNode("DEC_OP"); $$->children.push_back($2);}
	| unary_operator cast_expression {$$= new TreeNode("unary_expression"); $$->children.push_back($1); $$->children.push_back($2);}
	| SIZEOF unary_expression  {$$= new TreeNode("SIZEOF"); $$->children.push_back($2); }
	| SIZEOF '(' type_name ')' {$$= new TreeNode("SIZEOF"); $$->children.push_back($3); }
	| ALIGNOF '(' type_name ')' {$$= new TreeNode("ALIGNOF"); $$->children.push_back($3); }
	;

unary_operator
	: '&' {$$= new TreeNode("&");}
	| '*' {$$= new TreeNode("*");}
	| '+' {$$= new TreeNode("+");}
	| '-' {$$= new TreeNode("-");}
	| '~' {$$= new TreeNode("~");}
	| '!' {$$= new TreeNode("!");}
	;

cast_expression
	: unary_expression {$$= $1;}
	| '(' type_name ')' cast_expression {$$= new TreeNode("cast_expression"); $$->children.push_back($2); $$->children.push_back($4); }
	;

multiplicative_expression
	: cast_expression {$$= $1;}
	| multiplicative_expression '*' cast_expression {$$= new TreeNode("Mult"); $$->children.push_back($1); $$->children.push_back($3);}
	| multiplicative_expression '/' cast_expression {$$= new TreeNode("DIV"); $$->children.push_back($1); $$->children.push_back($3);}
	| multiplicative_expression '%' cast_expression {$$= new TreeNode("mod"); $$->children.push_back($1); $$->children.push_back($3);}
	;

additive_expression
	: multiplicative_expression {$$=$1;}
	| additive_expression '+' multiplicative_expression {$$= new TreeNode("Add"); $$->children.push_back($1); $$->children.push_back($3);}
	| additive_expression '-' multiplicative_expression {$$= new TreeNode("Sub"); $$->children.push_back($1); $$->children.push_back($3);}
	;

shift_expression
	: additive_expression {$$=$1;}
	| shift_expression LEFT_OP additive_expression {$$= new TreeNode("LEFT_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	| shift_expression RIGHT_OP additive_expression {$$= new TreeNode("RIGHT_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	;

relational_expression
	: shift_expression {$$=$1;}
	| relational_expression '<' shift_expression {$$= new TreeNode("LessThan"); $$->children.push_back($1); $$->children.push_back($3);}
	| relational_expression '>' shift_expression {$$= new TreeNode("GreaterThan"); $$->children.push_back($1); $$->children.push_back($3);}
	| relational_expression LE_OP shift_expression {$$= new TreeNode("LE_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	| relational_expression GE_OP shift_expression {$$= new TreeNode("GE_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	;

equality_expression
	: relational_expression {$$=$1;}
	| equality_expression EQ_OP relational_expression {$$= new TreeNode("EQ_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	| equality_expression NE_OP relational_expression {$$= new TreeNode("NE_OP"); $$->children.push_back($1); $$->children.push_back($3);}
	;

and_expression
	: equality_expression {$$=$1;}
	| and_expression '&' equality_expression {$$= new TreeNode("And"); $$->children.push_back($1); $$->children.push_back($3);}
	;

exclusive_or_expression
	: and_expression {$$=$1;}
	| exclusive_or_expression '^' and_expression {$$= new TreeNode("xor"); $$->children.push_back($1); $$->children.push_back($3);}
	;

inclusive_or_expression
	: exclusive_or_expression {$$=$1;}
	| inclusive_or_expression '|' exclusive_or_expression {$$= new TreeNode("inclusive_or"); $$->children.push_back($1); $$->children.push_back($3);}
	;

logical_and_expression
	: inclusive_or_expression {$$=$1;}
	| logical_and_expression AND_OP inclusive_or_expression {$$= new TreeNode("Logical_And"); $$->children.push_back($1); $$->children.push_back($3);}
	;

logical_or_expression
	: logical_and_expression {$$=$1;}
	| logical_or_expression OR_OP logical_and_expression {$$= new TreeNode("Logical_OR"); $$->children.push_back($1); $$->children.push_back($3);}
	;

conditional_expression
	: logical_or_expression {$$=$1;}
	| logical_or_expression '?' expression ':' conditional_expression {$$= new TreeNode("?:"); $$->children.push_back($1); $$->children.push_back($3); $$->children.push_back($5);}
	;

assignment_expression
	:  conditional_expression {$$=$1;}
	|  unary_expression assignment_operator assignment_expression {$$= $2; $$-> children.push_back($1); $$->children.push_back($3);}
	;

assignment_operator
	: '=' {$$=new TreeNode("=");}
	| MUL_ASSIGN {$$= new TreeNode("MUL_ASSIGN"); }
	| DIV_ASSIGN {$$= new TreeNode("DIV_ASSIGN"); }
	| MOD_ASSIGN {$$= new TreeNode("MOD_ASSIGN");}
	| ADD_ASSIGN {$$= new TreeNode("ADD_ASSIGN"); }
	| SUB_ASSIGN {$$= new TreeNode("SUB_ASSIGN"); }
	| LEFT_ASSIGN {$$= new TreeNode("LEFT_ASSIGN");}
	| RIGHT_ASSIGN {$$= new TreeNode("RIGHT_ASSIGN"); }
	| AND_ASSIGN {$$= new TreeNode("ADD_ASSIGN"); }
	| XOR_ASSIGN {$$= new TreeNode("XOR_ASSIGN"); }
	| OR_ASSIGN {$$= new TreeNode("OR_ASSIGN"); }
	;

expression
	: assignment_expression {$$=new TreeNode("expression"); $$->children.push_back($1);}
	| expression ',' assignment_expression {$$=new TreeNode("expression"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3); }
	;

constant_expression
	: conditional_expression	/* with constraints */ {$$=$1;}
	;

declaration
	: declaration_specifiers ';' {$$=$1;}
	| declaration_specifiers init_declarator_list ';' {$$= new TreeNode("declaration"); $$->children.push_back($1); $$->children.push_back($2);}
	| static_assert_declaration {$$=$1;}
	;

declaration_specifiers
	: storage_class_specifier declaration_specifiers {$$= new TreeNode("declaration_specifiers"); $$->children.push_back($1); $$->children.push_back($2);}
	| storage_class_specifier {$$=$1;}
	| type_specifier declaration_specifiers {$$= new TreeNode("declaration_specifiers"); $$->children.push_back($1); $$->children.push_back($2);}
	| type_specifier {$$=$1;}
	| type_qualifier declaration_specifiers {$$= new TreeNode("declaration_specifiers"); $$->children.push_back($1); $$->children.push_back($2);}
	| type_qualifier {$$=$1;}
	| function_specifier declaration_specifiers {$$= new TreeNode("declaration_specifiers"); $$->children.push_back($1); $$->children.push_back($2);}
	| function_specifier {$$= $1;}
	| alignment_specifier declaration_specifiers {$$= new TreeNode("declaration_specifiers"); $$->children.push_back($1); $$->children.push_back($2);}
	| alignment_specifier {$$=$1;}
	;

init_declarator_list
	: init_declarator {$$=new TreeNode("init_declarator_list"); $$->children.push_back($1);}
	| init_declarator_list ',' init_declarator {$$=new TreeNode("init_declarator_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3);}
	;

init_declarator
	: declarator '=' initializer {$$= new TreeNode("init_declarator"); $$->children.push_back($1); $$->children.push_back($3);}
	| declarator {$$=$1;}
	;

storage_class_specifier
	: TYPEDEF	/* identifiers must be flagged as TYPEDEF_NAME */ {$$=new TreeNode("TYPEDEF");}
	| EXTERN {$$=new TreeNode("EXTERN");}
	| STATIC {$$=new TreeNode("STATIC");}
	| THREAD_LOCAL {$$= new TreeNode("THREAD_LOCAL");}
	| AUTO {$$=new TreeNode("AUTO");}
	| REGISTER {$$= new TreeNode("REGISTER");}
	;

type_specifier
	: VOID {$$=new TreeNode("VOID");}
	| CHAR {$$=new TreeNode("CHAR");}
	| SHORT {$$=new TreeNode("SHORT");}
	| INT {$$=new TreeNode("INT");}
	| LONG {$$=new TreeNode("LONG");}
	| FLOAT {$$=new TreeNode("FLOAT");}
	| DOUBLE {$$=new TreeNode("DOUBLE");}
	| SIGNED {$$=new TreeNode("SIGNED");}
	| UNSIGNED {$$=new TreeNode("UNSIGNED");}
	| BOOL {$$=new TreeNode("BOOL");}
	| COMPLEX {$$=new TreeNode("COMPLEX");}
	| IMAGINARY	  	/* non-mandated extension */ {$$=new TreeNode("IMAGINARY");}
	| atomic_type_specifier {$$=$1;}
	| struct_or_union_specifier {$$=$1;}
	| enum_specifier {$$=$1;}
	| TYPEDEF_NAME		/* after it has been defined as such */ {$$=new TreeNode("TYPEDEF_NAME");}
	;

struct_or_union_specifier
	: struct_or_union '{' struct_declaration_list '}' {$$= new TreeNode("struct_or_union_specifier"); $$->children.push_back($1); $$->children.push_back($3);}
	| struct_or_union IDENTIFIER '{' struct_declaration_list '}' {$$= new TreeNode("struct_or_union_specifier"); $$->children.push_back($1); $$->children.push_back(new TreeNode($2)); $$->children.push_back($4);}
	| struct_or_union IDENTIFIER {$$= new TreeNode("struct_or_union_specifier"); $$->children.push_back($1); $$->children.push_back(new TreeNode($2));}
	;

struct_or_union
	: STRUCT {$$=new TreeNode("STRUCT");}
	| UNION {$$= new TreeNode("UNION");}
	;

struct_declaration_list
	: struct_declaration {$$=$1;}
	| struct_declaration_list struct_declaration {$$= $1; $$->children.push_back($2); }
	;

struct_declaration
	: specifier_qualifier_list ';'	/* for anonymous struct/union */ {$$=$1;}
	| specifier_qualifier_list struct_declarator_list ';' {$$= new TreeNode("struct_declaration"); $$->children.push_back($1); $$->children.push_back($2);}
	| static_assert_declaration {$$=$1;}
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list {$$= $2; $$->children.push_back($1);}
	| type_specifier {$$=$1;}
	| type_qualifier specifier_qualifier_list {$$= $2; $$->children.push_back($1);}
	| type_qualifier {$$=$1;}
	;

struct_declarator_list
	: struct_declarator {$$=$1;}
	| struct_declarator_list ',' struct_declarator {$$= $1; $$->children.push_back($3);}
	;

struct_declarator
	: ':' constant_expression {$$= $2; }
	| declarator ':' constant_expression {$$= new TreeNode("struct_declarator"); $$->children.push_back($1); $$->children.push_back($3);}
	| declarator {$$=$1;}
	;

enum_specifier
	: ENUM '{' enumerator_list '}' {$$=$3;}
	| ENUM '{' enumerator_list ',' '}' {$$=$3;}
	| ENUM IDENTIFIER '{' enumerator_list '}' {$$= new TreeNode("enum_specifier"); $$->children.push_back(new TreeNode($2)); $$->children.push_back($4);}
	| ENUM IDENTIFIER '{' enumerator_list ',' '}' {$$= new TreeNode("enum_specifier"); $$->children.push_back(new TreeNode($2)); $$->children.push_back($4);}
	| ENUM IDENTIFIER {$$= new TreeNode($2);}
	;

enumerator_list
	: enumerator {$$=$1;}
	| enumerator_list ',' enumerator {$$= $1; $$->children.push_back($3);}
	;

enumerator	/* identifiers must be flagged as ENUMERATION_CONSTANT */
	: enumeration_constant '=' constant_expression {$$= new TreeNode("enumerator"); $$->children.push_back($1); $$->children.push_back($3);}
	| enumeration_constant {$$=$1;}
	;

atomic_type_specifier
	: ATOMIC '(' type_name ')' {$$=$3;}
	;

type_qualifier
	: CONST {$$= new TreeNode("CONST"); }
	| RESTRICT {$$= new TreeNode("RESTRICT"); }
	| VOLATILE {$$= new TreeNode("VOLATILE"); }
	| ATOMIC {$$= new TreeNode("ATOMIC"); }
	;

function_specifier
	: INLINE {$$= new TreeNode("INLINE"); }
	| NORETURN {$$= new TreeNode("NORETURN"); }
	;

alignment_specifier
	: ALIGNAS '(' type_name ')' {$$=$3;}
	| ALIGNAS '(' constant_expression ')' {$$=$3;}
	;

declarator
	: pointer direct_declarator {$$=new TreeNode("declarator");$$->children.push_back($1);$$->children.push_back($2);}
	| direct_declarator {$$=$1;}
	;

direct_declarator
	: IDENTIFIER {$$= new TreeNode($1); }
	| '(' declarator ')' {$$= $2; }
	| direct_declarator '[' ']' {$$= $1; }
	| direct_declarator '[' '*' ']' {$$= $1; }
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']' {$$= $1; $$->children.push_back($4); $$->children.push_back($5);}
	| direct_declarator '[' STATIC assignment_expression ']' {$$= $1; $$->children.push_back($4);}
	| direct_declarator '[' type_qualifier_list '*' ']' {$$= $1; $$->children.push_back($3);}
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']' {$$= $1; $$->children.push_back($3); $$->children.push_back($5);}
	| direct_declarator '[' type_qualifier_list assignment_expression ']' {$$= $1; $$->children.push_back($3); $$->children.push_back($4);}
	| direct_declarator '[' type_qualifier_list ']' {$$= $1; $$->children.push_back($3);}
	| direct_declarator '[' assignment_expression ']' {$$= $1; $$->children.push_back($3);}
	| direct_declarator '(' parameter_type_list ')' {$$= $1; $$->children.push_back($3);}
	| direct_declarator '(' ')' {$$= $1; }
	| direct_declarator '(' identifier_list ')' {$$= $1; $$->children.push_back($3);}
	;

pointer
	: '*' type_qualifier_list pointer {$$= new TreeNode("pointer"); $$->children.push_back($2); $$->children.push_back($3);}
	| '*' type_qualifier_list {$$= new TreeNode("pointer"); $$->children.push_back($2); }
	| '*' pointer {$$= new TreeNode("pointer"); $$->children.push_back($2); }
	| '*' {$$= new TreeNode("pointer");}
	;

type_qualifier_list
	: type_qualifier {$$=new TreeNode("type_qualifier_list"); $$->children.push_back($1);}
	| type_qualifier_list type_qualifier {$$=new TreeNode("type_qualifier_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($2); }
	;


parameter_type_list
	: parameter_list ',' ELLIPSIS {$$=$1; $$->children.push_back(new TreeNode("ELLIPSIS"));}
	| parameter_list {$$=$1;}
	;

parameter_list
	: parameter_declaration {$$=new TreeNode("parameter_list");$$->children.push_back($1);}
	| parameter_list ',' parameter_declaration {$$=new TreeNode("parameter_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3);}
	;

parameter_declaration
	: declaration_specifiers declarator {$$= new TreeNode("parameter_declaration"); $$->children.push_back($1); $$->children.push_back($2); }
	| declaration_specifiers abstract_declarator {$$= new TreeNode("parameter_declaration"); $$->children.push_back($1); $$->children.push_back($2); }
	| declaration_specifiers {$$=new TreeNode("parameter_declaration"); $$->children.push_back($1);}
	;

identifier_list
	: IDENTIFIER {$$=new TreeNode("identifier_list"); $$->children.push_back(new TreeNode($1));}
	| identifier_list ',' IDENTIFIER {$$=new TreeNode("identifier_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back(new TreeNode($3)); }
	;

type_name
	: specifier_qualifier_list abstract_declarator {$$= new TreeNode("type_name"); $$->children.push_back($1); $$->children.push_back($2); }
	| specifier_qualifier_list {$$=new TreeNode("type_name"); $$->children.push_back($1);}
	;

abstract_declarator
	: pointer direct_abstract_declarator {$$= new TreeNode("abstract_declarator"); $$->children.push_back($1); $$->children.push_back($2); }
	| pointer {$$=$1;}
	| direct_abstract_declarator {$$=$1;}
	;

direct_abstract_declarator
	: '(' abstract_declarator ')' {$$=$2;}
	| '[' ']' {$$=new TreeNode("[]");}
	| '[' '*' ']' {$$= new TreeNode("[*]"); }
	| '[' STATIC type_qualifier_list assignment_expression ']' {$$= new TreeNode("direct_abstract_declarator"); $$->children.push_back($3); $$->children.push_back($4); }
	| '[' STATIC assignment_expression ']' {$$=$3;}
	| '[' type_qualifier_list STATIC assignment_expression ']' {$$= new TreeNode("direct_abstract_declarator"); $$->children.push_back($2); $$->children.push_back($4); }
	| '[' type_qualifier_list assignment_expression ']' {$$= new TreeNode("direct_abstract_declarator"); $$->children.push_back($2); $$->children.push_back($3); }
	| '[' type_qualifier_list ']' {$$=$2;}
	| '[' assignment_expression ']' {$$=$2;}
	| direct_abstract_declarator '[' ']' {$$=$1;}
	| direct_abstract_declarator '[' '*' ']' {$$=$1;}
	| direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']' {$$= $1; $$->children.push_back($4); $$->children.push_back($5); }
	| direct_abstract_declarator '[' STATIC assignment_expression ']' {$$= $1; $$->children.push_back($4); }
	| direct_abstract_declarator '[' type_qualifier_list assignment_expression ']' {$$= $1; $$->children.push_back($3); $$->children.push_back($4); }
	| direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']' {$$= $1; $$->children.push_back($3); $$->children.push_back($5); }
	| direct_abstract_declarator '[' type_qualifier_list ']' {$$= $1; $$->children.push_back($3);}
	| direct_abstract_declarator '[' assignment_expression ']' {$$= $1; $$->children.push_back($3);}
	| '(' ')' {$$= new TreeNode("()");  }
	| '(' parameter_type_list ')' {$$= $2; }
	| direct_abstract_declarator '(' ')' {$$= $1; }
	| direct_abstract_declarator '(' parameter_type_list ')' {$$= $1; $$->children.push_back($3);}
	;

initializer
	: '{' initializer_list '}' {$$=$2;}
	| '{' initializer_list ',' '}' {$$=$2;}
	| assignment_expression {$$=$1;}
	;

initializer_list
	: designation initializer {$$=new TreeNode("initializer_list"); $$->children.push_back($1); $$->children.push_back($2);}
	| initializer {$$=new TreeNode("initializer_list"); $$->children.push_back($1);}
	| initializer_list ',' designation initializer {$$=new TreeNode("initializer_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3); $$->children.push_back($4);}
	| initializer_list ',' initializer { $$=new TreeNode("initializer_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($3); }
	;

designation
	: designator_list '=' {$$=$1;}
	;

designator_list
	: designator {$$=new TreeNode("designator_list"); $$->children.push_back($1);}
	| designator_list designator {$$=new TreeNode("designator_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($2); }
	;

designator
	: '[' constant_expression ']' {$$=$2;}
	| '.' IDENTIFIER {$$= new TreeNode($2);}
	;

static_assert_declaration
	: STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' ';' {$$= new TreeNode("STATIC_ASSERT"); $$->children.push_back($3); $$->children.push_back(new TreeNode($5)); }
	;

statement
	: labeled_statement {$$=$1;}
	| compound_statement {$$=$1;}
	| expression_statement {$$=$1;}
	| selection_statement {$$=$1;}
	| iteration_statement {$$=$1;}
	| jump_statement {$$=$1;}
	;

labeled_statement
	: IDENTIFIER ':' statement {$$= new TreeNode(":"); $$->children.push_back(new TreeNode($1)); $$->children.push_back($3); }
	| CASE constant_expression ':' statement {$$= new TreeNode(":"); $$->children.push_back($2); $$->children.push_back($4); }
	| DEFAULT ':' statement {$$= new TreeNode(":"); $$->children.push_back(new TreeNode("DEFAULT")); $$->children.push_back($3); }
	;

compound_statement
	: '{' '}' {$$= new TreeNode("{}"); }
	| '{'  block_item_list '}' {$$= $2; }
	;

block_item_list
	: block_item {$$=new TreeNode("block_item_list"); $$->children.push_back($1);}
	| block_item_list block_item {
		$$=new TreeNode("block_item_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($2);
	 }
	;

block_item
	: declaration {$$=$1;}
	| statement {$$=$1;}
	;

expression_statement
	: ';' {$$= new TreeNode(";");}
	| expression ';' {$$=$1;}
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement {$$= new TreeNode("ifelse"); $$->children.push_back($3); $$->children.push_back($5);$$->children.push_back($7); }
	| IF '(' expression ')' statement {$$= new TreeNode("if"); $$->children.push_back($3); $$->children.push_back($5); }
	| SWITCH '(' expression ')' statement {$$= new TreeNode("switch"); $$->children.push_back($3); $$->children.push_back($5); }
	;

iteration_statement
	: WHILE '(' expression ')' statement {$$= new TreeNode("WHILE"); $$->children.push_back($3); $$->children.push_back($5); }
	| DO statement WHILE '(' expression ')' ';' {$$= new TreeNode("DoWhile"); $$->children.push_back($2); $$->children.push_back($5); }
	| FOR '(' expression_statement expression_statement ')' statement {$$= new TreeNode("for_loop"); $$->children.push_back($3); $$->children.push_back($4); $$->children.push_back($6);}
	| FOR '(' expression_statement expression_statement expression ')' statement {$$= new TreeNode("for_loop"); $$->children.push_back($3); $$->children.push_back($4); $$->children.push_back($5); $$->children.push_back($7);}
	| FOR '(' declaration expression_statement ')' statement {$$= new TreeNode("for_loop"); $$->children.push_back($3); $$->children.push_back($4); $$->children.push_back($6);}
	| FOR '(' declaration expression_statement expression ')' statement {$$= new TreeNode("for_loop"); $$->children.push_back($3); $$->children.push_back($4); $$->children.push_back($5); $$->children.push_back($7);}
	;

jump_statement
	: GOTO IDENTIFIER ';' {$$=new TreeNode($2);}
	| CONTINUE ';' {$$=new TreeNode("CONTINUE");}
	| BREAK ';' {$$= new TreeNode("BREAK");}
	| RETURN ';' {$$=new TreeNode("RETURN");}
	| RETURN expression ';' {$$= new TreeNode("RETURN"); $$->children.push_back($2); }
	;

translation_unit
	: external_declaration {$$= new TreeNode("Building AST"); $$->children.push_back($1); node=$$;}
	| translation_unit external_declaration {$$= $1;
    $$->children.push_back($2);}
	;

external_declaration
	: function_definition {$$=$1;}
	| declaration {$$=$1; }
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement {$$= new TreeNode("function_definition"); $$->children.push_back($1); $$->children.push_back($2); $$->children.push_back($3); $$->children.push_back($4);}
	| declaration_specifiers declarator compound_statement {$$= new TreeNode("function_definition"); $$->children.push_back($1); $$->children.push_back($2); $$->children.push_back($3);}
	;

declaration_list
	: declaration {$$=new TreeNode("declaration_list"); $$->children.push_back($1);}
	| declaration_list declaration {$$=new TreeNode("declaration_list"); std::list<TreeNode*> child= $1->children;
	while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                $$->children.push_back(it);
               
            }  
    $$->children.push_back($2);}
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "*** %s\n", s);
}
