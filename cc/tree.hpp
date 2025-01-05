#ifndef __TREE_HPP
#define __TREE_HPP

#include <list>
#include <vector>
#include <stack>
#include <string>
#include <stdlib.h>
#include <iostream>
#include<stdio.h>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>


//AST node
struct TreeNode
{   
    //char* name;
    std::string name;
    std::list<TreeNode*> children;
    std::string category=" ";
    int id=0;
    float num=0;
    
    
    TreeNode(const char* operand)
    {   category="str";
        //name= operand ? strdup(operand) : strdup(" ");
        name=operand;
    }

    TreeNode(int operand)
    {
        category="int";
        id=operand;

    }   

    TreeNode(float operand)
    {
        category="float";
        num=operand;
    }

};



#endif
