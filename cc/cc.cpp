#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "c.tab.hpp"

#include "tree.hpp"

#include "check.cpp"

#include "code_generation.cpp"

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
extern "C" TreeNode *node;


static void usage()
{
  printf("Usage: cc <prog.c>\n");
}

int indentation=0;

void print_ast(struct TreeNode *ast_node)
{
    if (ast_node== NULL)
    {
        //printf("Empty Tree");
        std::cout<< "-- End --";
    }

    else
    {   

        std::list<TreeNode*> visit;
        visit.push_back(ast_node);

        int start=indentation;

       
            TreeNode *temp= visit.front();
            
            visit.pop_front();

            std:: string category=temp->category;
            
            start=indentation; 
        while (start>=0)
        {
            std::cout<< " ";
            start=start-1;
        }

        
            if (category=="str")
            {   
                
                //printf("\n Class Name = %s \n",node->name);
                std::cout<< temp->name<<"\n";
                
            }

            else if (category=="int")
            {   
                
                //printf("\n Class Name = %d \n",node->id);
                std::cout<< temp->id<<"\n";
            }

            else

            {   
                
                //printf("\n Class Name = %d \n",node->num);
                std::cout<< temp->num<<"\n";
            }

            
            std::list<TreeNode*> child= temp->children;

            
            indentation+=1;

            while (child.size()!=0)
            {   
                TreeNode *it=child.front();
                child.pop_front();
                print_ast(it);
                indentation-=1;
               
            }
            


        }
        
    }


int
main(int argc, char **argv)
{
  if (argc != 2) {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);
  int ret = yyparse();
  printf("retv = %d\n", ret);
  printf("\n Printing the AST \n");
  print_ast(node);
  
  printf("Performing Semantic Analysis \n");

  if (final_check(node)==true && ret==0)
  { 
    printf("Well-defined code \n");

    printf("Code Generation: Generated code (with dead code elimination + local optimisations) dumped at output.ll \n");

    final(node);

  }

  else
  { 
    printf("Ill-defined code \n");
    printf("Therefore, no code generation (IR bitcode gets emitted only for well-formed C code) \n");
  }
  

  
  exit(0);
}
