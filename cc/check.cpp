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
#include<map>
#include<algorithm>

#include "tree.hpp"

std::map<std::string , int> func_param; //no of parameters in function
std::map<std::string , int> undecl_func_param; //no of parameters in function

std::list<std::string> declared_variables;

std::list<std::string> declared_functions;
std::list<std::string> undeclared_functions;

//support scope checking and declaration before use

int curr_scope=0;
int flag=0;

bool ifpresent(std::string id, std::list<std::string> list)
{
    std::list<std::string> temp= list;

    while (temp.size()!=0)
    {   
        std::string curr= (temp.front());
        
        temp.pop_front();
        if (id==curr)
        {
            return true;
        }
    }
    
    return false;
}

bool check_blk(struct TreeNode *ast_node, std::list<std::string> dec_variables)
{
    std::list<TreeNode*> children=ast_node->children;

    if (children.size()==0 && ast_node->category=="str")
        {   
            std::string id=ast_node->name;

            if (ifpresent(ast_node->name,declared_functions) && (func_param[ast_node->name]==0 || func_param[ast_node->name]==-1 ))
            {

            }


            else if (ifpresent(ast_node->name,dec_variables)==false && ifpresent(ast_node->name,declared_variables)==false)
            {   
                std::cout<<"Variable not declared: "<<ast_node->name <<"\n";
                flag=1;
                return false;
            }
        }

    while (children.size()!=0)
    {
        TreeNode* child= children.front();
        children.pop_front();

        if (child->children.size()==0 && child->category=="str")
        {   
            

            if (ifpresent(child->name,declared_functions) && (func_param[child->name]==0 || func_param[child->name]==-1 ))
            {

            }

            else if (ifpresent(child->name,dec_variables)==false && ifpresent(child->name,declared_variables)==false)
            {   
                std::cout<<"Variable not declared: "<<child->name <<"\n";
                flag=1;
                return false;
            }
        }

        bool err= check_blk(child,dec_variables);
        if (err == false)
        {   flag=1;
            return false;
        }
    }

    return true;
}

void check_declaration(struct TreeNode *ast_node, std::list<std::string>& dec_variables)
{
    std::string type=" ";
    std::string func;
    int i=0;

    TreeNode *temp= ast_node;

    std:: string category=temp->category;
        
    std::list<TreeNode*> child= temp->children;
    
    while (child.size()!=0)
    {   
        TreeNode *it=child.front();
        child.pop_front();
        
        if (it->children.size()==0)
        {   
            if (i==0)
            {
                type= it->name;
            }
            
        }

        else
        {
            func=it->name;
            if (func==("init_declarator_list"))
            {
                std::list<TreeNode*> child_temp= it->children;

                while (child_temp.size()!=0)
                {   
                    TreeNode *it_temp=child_temp.front();
                    child_temp.pop_front();
                    
                    if (it_temp->children.size()==0)
                    {   
                        
                        dec_variables.push_back(it_temp->name);
                        
                    }
                    else if ((it_temp->name==("init_declarator")))
                    {
                        if (it_temp->children.size()==2)
                        {   
                            if (it_temp->children.front()->name=="declarator")

                            {
                             std::list<TreeNode*> curr= it_temp->children.front()->children;
                             dec_variables.push_back(curr.back()->name);

                             std::string id=it_temp->children.back()->name;
                             bool iserror= ifpresent(id,dec_variables)|| ifpresent(id,declared_variables) || id.find("'")!=std::string::npos || id.find('"')!=std::string::npos;
                            if (iserror==false && it_temp->children.back()->children.size()==0)
                            {
                                flag=1;
                            }
                            else if (it_temp->children.back()->children.size()>0)
                            {
                                check_blk(it_temp->children.back()->children.front(),dec_variables);
                            }
                        }
                        
                           else 
                           {
                            dec_variables.push_back(it_temp->children.front()->name);
                            bool iserror= check_blk(it_temp->children.back(), dec_variables);
                            
                            if (iserror==false)
                            {
                                flag=1;
                            }

                           } 
                            
                        }

                        else 
                        {
                             
                             dec_variables.push_back(it_temp->children.front()->name);
                             
                        }
                        
                    }
                    else if ( it_temp->name=="declarator")
                    {
                        dec_variables.push_back(it_temp->children.back()->name);
                    }
                    
                    else // function name
                    {
                        declared_functions.push_back(it_temp->name);
                        
                        std::list<TreeNode*> param=it_temp->children.front()->children;
                        int num=0;
                        bool isellipsis=false;
                        while(param.size()!=0)
                        {
                            TreeNode *func_temp=param.front();
                            param.pop_front();
                            if ((func_temp->name==("parameter_declaration")))
                            {
                                num=num+1;
                            }
                            else if ((func_temp->name==("ELLIPSIS")))
                            {
                                isellipsis=true;
                            }
                        }

                        if (isellipsis)
                        {
                            func_param.insert({it_temp->name,-1});
                            
                        }
                        else
                        {
                            func_param.insert({it_temp->name,num});
                            
                        }

                        
                    }
                    
                    
                }


            }
            
        }
        
        i=i+1;
    }
   
}

bool check_block(struct TreeNode *ast_node, std::list<std::string>& declared_var)
{
    TreeNode *temp= ast_node;
    std::list<TreeNode*> child= temp->children;
    std:: string category=temp->category;
    
    while (child.size()!=0)
    {   
        TreeNode *blk_node= child.front();
        child.pop_front();

            if (category=="str" && blk_node->name!="")
        {
            std::string id= blk_node->name;
            
            if ((id==("declaration")))
            {
                check_declaration(blk_node,declared_var);
                
                blk_node->children.pop_front();

            }

            else if (id==":")
            {
                declared_var.push_back(blk_node->children.front()->name);
                
            }
      
            else if (id=="pointer" )
            {

            }

            else if (blk_node->children.size()==0 )
            {
                //bool var_present= ifpresent(id,declared_var) || ifpresent(id, declared_variables);
                
                if  (id.find("'")!=std::string::npos || id.find('"')!=std::string::npos)
                {

                }
            
                
                else if (ifpresent(id,declared_functions) && (func_param[id]==0 || func_param[id]==-1 ))
                {
                    
                }
                else if (ifpresent(id,declared_var) == true) 
                {   
                }

                else if (ifpresent(id, declared_variables) == true )
                {

                }

                else
                {   std::cout<<"variable not declared: "<< id<<"\n";
                    
                    return false;
                }
            }

            else if ((blk_node->children.front()->name==("argument_expression_list")))
            {
                bool func_present=ifpresent(id, declared_functions);
                int num_params= blk_node->children.front()->children.size();

                if (func_present==false)
                {
                    undeclared_functions.push_back(id);
                    undecl_func_param.insert({id,num_params});
                }

                else
                {
                    int req_params= func_param[id];
                    if (req_params!= -1 && req_params!=num_params)
                    {  
                        std::cout<<"Mismatch in function parameters: "<< id <<"\n";

                        return false;
                    }
                    
                    if (req_params==-1)
                    {
                        std::list<TreeNode*> temp_hold=blk_node->children.front()->children;
                        std::list<TreeNode*> hold_nodes;
                        while (temp_hold.size()!=0)
                        {
                            TreeNode *temp_node=temp_hold.front();
                            temp_hold.pop_front();
                            if (temp_node->children.size()!=0)
                            {
                                hold_nodes.push_back(temp_node);
                            }
                        }

                        blk_node->children=hold_nodes;
                    }
                   
                }

                
            }
            

        bool iserror=check_block(blk_node,declared_var);
            if (iserror==false)
            {   
                return false;
            }

            }
        }
    
    

    
    return true;
    
}

bool check_function(struct TreeNode *ast_node)
{
    TreeNode *temp= ast_node;

    std:: string category=temp->category;
         
    std::list<TreeNode*> child= temp->children;

    std::list<std::string> declared_var;
    int i=0;
    while (child.size()!=0)
    {
        TreeNode *fn= child.front();
        child.pop_front();
        if (i!=0)
        {
            if ((fn->name!=("block_item_list")) && (fn->name!=("{}")))
            {   

                if (fn->name=="declarator")
                {
                    fn=fn->children.back();
                }

                declared_functions.push_back(fn->name);

            

                if (fn->children.size()!=0)
                {       
                    
                        std::list<TreeNode*> param=fn->children.front()->children;
                        int num=0;
                        bool isellipsis=false;

                        while(param.size()!=0)
                        {
                            TreeNode *func_temp=param.front();
                            param.pop_front();
                            if ((func_temp->name==("parameter_declaration")))
                            {
                                num=num+1;
                                std::list<TreeNode*> it=func_temp->children;

                                while (it.size()!=0)
                                {    
                                    TreeNode* it_child=it.front();
                                    it.pop_front();

                                    if (((it_child)->name==("declarator")))
                                    {
                                        declared_var.push_back((it_child)->children.back()->name);
                                    }

                                    else if ((it_child)->children.size()==0)
                                    {
                                        declared_var.push_back((it_child)->name);
                                    }
                                }

                        
                            }
                            else if ((func_temp->name==("ELLIPSIS")))
                            {
                                isellipsis=true;
                            }
                        }

                        if (isellipsis)
                        {
                            func_param.insert({fn->name,-1});
                    
                        }
                        else
                        {
                            func_param.insert({fn->name,num});
                            
                            
                        }
                }
                else
                {   
                    func_param.insert({fn->name,0});
                    
                }

            }

            else if ((fn->name==("block_item_list"))) //block
            {   
                
                bool iserror=check_block(fn,declared_var);
                if (iserror==false)
                {   
                    return false;
                }
            }
            
        }

        i=i+1;
    }

    return true;


}

void check_scope (struct TreeNode *ast_node)
{
   
        TreeNode *temp= ast_node;

        std:: string category=temp->category;
        
        
        std::list<TreeNode*> child= temp->children;
        

        if (category=="str")
            {   

                std::string id= temp->name;

                if ( (id==("Building AST")))
                {
                    curr_scope=0;
                    
                    while (child.size()!=0)
                    {   
                        TreeNode *it=child.front();
                        child.pop_front();
                        
                        check_scope(it);
                        
                        
                    }

                }

                else if ((id==("declaration")))
                {
                    
                    check_declaration(ast_node,declared_variables);
                }
                
                else if ((id==("function_definition")))
                {   

                    bool iserror= check_function(ast_node);
                    if (iserror==false)
                    {
                        flag=1;
                        return;
                    }
                }
                
            }
        

        
}

bool final_check(struct TreeNode *ast_node)
{
    check_scope(ast_node);
    if (flag==1)
    {
        return false;
        
    }
    else if (undeclared_functions.size()!=0)
    {
        std::list<std::string> undec=undeclared_functions;
        while (undec.size()!=0)
        {
            std::string func= (undec.front());
            undec.pop_front();

            if (ifpresent(func,declared_functions))
            {
                int valid_param= func_param[func];
                int param= undecl_func_param[func];
                if (valid_param!=-1 && valid_param!=param)
                {   std::cout<<"Mismatch in function parameters: "<<func <<"\n";
                    return false;
                }
            }

            else
            {   std::cout<<"function not declared: "<< func <<"\n";
                return false;
            }
            
        }
        
    }
    return true;
}

