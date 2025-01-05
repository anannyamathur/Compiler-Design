#include <list>
#include <stack>
#include <stdlib.h>
#include <iostream>
#include<stdio.h>
#include <string.h>
#include <cstring>

#include "tree.hpp"

//main header files
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

//optimisation

#include "llvm/IR/Instructions.h"

//reading files

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"

//

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::sys;


static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<Module> TheModule;
static std::unique_ptr<IRBuilder<>> Builder;
static std::map<std::string, Value *> NamedValues;

// Declare initial modules:

Value* LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}


Value *LogErrorV(std::string Str) {

  const char *str=Str.c_str();
  LogError(str);
  return nullptr;
}

Value *codegen_int (int Val)
{ 
  return ConstantInt::get(*TheContext, APInt(32, Val));
}

Value *codegen_float (float Val)
{ int d=Val;

  return ConstantInt::get(*TheContext, APInt(32, d));
}

Value *codegen_var (std::string Val)
{ 
  
  //return ConstantFP::get(*TheContext, APFloat(0.0));
  return ConstantInt::get((*TheContext), APInt(32,0));
}

Value *codegen_Variable (std::string Name, std::map<std::string, Value *> &DecValues)
{
  // Look this variable up in the function.
  Value *V = DecValues[Name];
  if (!V)
  {
    Value *GV= NamedValues[Name];

    if (!GV)
    {   
        std::string err= "Unknown variable name/ Variable has not been allotted a value yet: " + Name + "=> Alloting 0 to the variable";
        DecValues[Name]=ConstantInt::get((*TheContext), APInt(32,0));
        LogErrorV(err);
        return DecValues[Name];
    }

    else
    {
        return GV;
    }
  }
    
  return V;
}


//To check

bool ifpresent_cgen(std::string id, std::list<std::string> list)
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
    
    return false; //id does not exist 
}

// operators

std::list<std::string> used_op;

//scope + declaration before use already checked
//Proceed to Code Generation 

//to allot: global+local variables
static std::map<std::string, AllocaInst*> NamedValuesAllot;

std::list<std::string> alloted_var;
std::map<std::string , int> func_param_cgen; //no of parameters in function
std::map<std::string , int> undecl_func_param_cgen; //no of parameters in function

std::list<std::string> declared_functions_cgen;
std::list<std::string> undeclared_functions_cgen;

// goto blocks

static std::map<std::string, BasicBlock *> goto_blk;

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(Type::getInt32Ty(*TheContext), nullptr,
                           VarName);
}

Value *check_blk_cgen(struct TreeNode *ast_node, std::map<std::string, Value *> &NamedValues,  std::map<std::string, AllocaInst*> NamedValuesAllot,std::list<std::string> alloted);

void allot_value(std::string Val, std::string Variable, std::map<std::string, Value *> &NamedVal)
{
    if (!NamedVal[Val])
    {
        NamedVal[Variable]=NamedValues[Val];
    }
    else
    {
        NamedVal[Variable]=NamedVal[Val];
    }

    
}

Function *getFunction(std::string Name) {
  // if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  return nullptr;
}

Value *CallFunction (struct TreeNode *ast_node, std::map<std::string, Value *> NamedValues, std::list<std::string> alloted) 
{
  // Look up the name in the global module table.
  std::string Callee= ast_node->name;
  Function *CalleeF = getFunction(Callee);
  int Args= ast_node->children.front()->children.size();

  if (!CalleeF)
  {
    std::string err= "Unknown function referenced: " + Callee;
    return LogErrorV(err);
  }
    

  // If argument mismatch error.
  if (func_param_cgen[Callee] != Args && func_param_cgen[Callee]!=-1)
  {
    std::string err= "Incorrect number of arguments passed: " + Callee;
    return LogErrorV(err);
  }


  std::vector<Value *> ArgsV;

  std::list<TreeNode*> temp= ast_node->children.front()->children;

  while (temp.size()!=0) 
  { 
    TreeNode* child_arg= temp.front();
    temp.pop_front();
    ArgsV.push_back(check_blk_cgen(child_arg,NamedValues,NamedValuesAllot,alloted));

    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *CallEmptyFunction (struct TreeNode *ast_node, std::map<std::string, Value *> NamedValues, std::list<std::string> alloted) 
{
  // Look up the name in the global module table.
  std::string Callee= ast_node->name;
  Function *CalleeF = getFunction(Callee);
  int Args= 0;

  if (!CalleeF)
  {
    std::string err= "Unknown function referenced: " + Callee;
    return LogErrorV(err);
  }
    

  // If argument mismatch error.
  if (func_param_cgen[Callee] != Args && func_param_cgen[Callee]!=-1)
  {
    std::string err= "Incorrect number of arguments passed: " + Callee;
    return LogErrorV(err);
  }


  std::vector<Value *> ArgsV;

  ArgsV.push_back((ConstantInt::get(*TheContext, APInt(32,0))));
  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value *check_blk_cgen(struct TreeNode *ast_node, std::map<std::string, Value *> &NamedValues, std::map<std::string, AllocaInst*> NamedInst,std::list<std::string> alloted)
{
    std::list<TreeNode*> children=ast_node->children;

    if (children.size()==0)
    {   
        if (ast_node->category=="str")
        {
            std::string id= ast_node->name;

            if (NamedInst[id])
            {   
                AllocaInst *Alloca= NamedInst[id];
                return Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, id);
            }

            else if (ifpresent_cgen(id,alloted) || ifpresent_cgen(id,alloted_var))
            {
                return codegen_Variable(id,NamedValues);
            }

            else if (id.find("'")!=std::string::npos || id.find('"')!=std::string::npos)
            {
                return codegen_var(id);
            }

            else if (ifpresent_cgen(id,declared_functions_cgen) && (func_param_cgen[id]==0 || func_param_cgen[id]==-1))
            {
                
                return CallEmptyFunction(ast_node,NamedValues,alloted);
                //return nullptr;
            }

            else
            {
                std::string err= "No value allotted to the variable yet: " + id + " => Alloting 0 to the variable";
                NamedValues[id]= (ConstantInt::get(*TheContext, APInt(32,0)));
    
                LogErrorV(err);
                return NamedValues[id];
                
            }
            

        }

        else if (ast_node->category=="int")
        {   
           
            return codegen_int(ast_node->id);
        }

        else
        {   
            
            return codegen_float(ast_node->num);
        }
    }

    else if (ifpresent_cgen(ast_node->name,used_op))
    {
        std::string op_name= ast_node->name;
        Value *L=check_blk_cgen(children.front(),NamedValues, NamedInst, alloted);
        Value *R=check_blk_cgen(children.back(),NamedValues, NamedInst,alloted);

        if (!L || !R)
        {
            return nullptr;
        }

    if (op_name=="Add") 
        return Builder->CreateAdd(L, R, "addtmp");
    else if (op_name=="DIV") 
        return Builder->CreateUDiv(L, R, "divtmp");
    else if (op_name=="Sub")
        return Builder->CreateSub(L, R, "subtmp");
    else if (op_name=="Mult")
        return Builder->CreateMul(L, R, "multtmp");
    else if (op_name=="LessThan")
    {
        L = Builder->CreateICmpULT(L, R, "lessthantmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return L;
    }
        
    else if (op_name=="GreaterThan")
    {
         L = Builder->CreateICmpUGT(L, R, "greaterthantmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return L;

    }
   
    else if (op_name=="GE_OP") 
    {
        L = Builder->CreateICmpUGE(L, R, "ge_optmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return L;

    }
    
    else if (op_name=="LE_OP")
    {
        L = Builder->CreateICmpULE(L, R, "le_optmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return L;
    }
    
    else if (op_name=="Logical_OR")
    {  
       if (L== (ConstantInt::get(*TheContext, APInt(32,1))) or R== (ConstantInt::get(*TheContext, APInt(32,1))))
       {
        return (ConstantInt::get(*TheContext, APInt(32,1)));
       }
       return Builder->CreateOr(L, R, "logical_ortmp");

    }
    
    else if (op_name=="Logical_And")
    {    
        if (L== (ConstantInt::get(*TheContext, APInt(32,0))) or R== (ConstantInt::get(*TheContext, APInt(32,0))))
       {
        return (ConstantInt::get(*TheContext, APInt(32,0)));
       }
         return Builder->CreateAnd(L, R, "logical_andtmp");
    }

     else if (op_name=="And")
    {   
        if (L== (ConstantInt::get(*TheContext, APInt(32,0))) or R== (ConstantInt::get(*TheContext, APInt(32,0))))
       {
        return (ConstantInt::get(*TheContext, APInt(32,0)));
       }

        return Builder->CreateAnd(L, R, "logical_andtmp");
    }
    
    else if (op_name=="inclusive_or")
    {   
        if (L== (ConstantInt::get(*TheContext, APInt(32,1))) or R== (ConstantInt::get(*TheContext, APInt(32,1))))
       {
        return (ConstantInt::get(*TheContext, APInt(32,1)));
       }

        return Builder->CreateOr(L, R, "inclusive_ortmp");
     
    }

    else if (op_name=="LEFT_OP")
        return Builder->CreateShl(L, R, "shift_lefttmp");

    else 
        return Builder->CreateAShr(L, R, "shift_righttmp");

    }

    
    
    else if (ifpresent_cgen(ast_node->name,declared_functions_cgen))
    {   
        return CallFunction(ast_node,NamedValues,alloted);
    }

    else if (ast_node->name=="expression")
    {
        
        return check_blk_cgen(ast_node->children.front(),NamedValues,NamedInst,alloted);
    }

    else 
    {  
        std::string err= "Invalid operator: " + ast_node->name;
        return LogErrorV(err);
        
    }
}

Function *generate_variadicfunction(std::list<std::string> Args, std::string Name)
 {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(Args.size(), Type::getInt32Ty(*TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getInt32Ty(*TheContext), Doubles, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  std::list<std::string> arg= Args;
  
  for (auto &Arg : F->args())
   {
    std::string arg_temp=arg.front();
   arg.pop_front();
   Arg.setName(arg_temp);

   
   }
    
    
  return F;
}

Function *generate_function(std::list<std::string> Args, std::string Name)
 {
  // Make the function type:  double(double,double) etc.
  std::vector<Type *> Doubles(Args.size(), Type::getInt32Ty(*TheContext));
  FunctionType *FT =
      FunctionType::get(Type::getInt32Ty(*TheContext), Doubles, false);

  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  std::list<std::string> arg= Args;
  
  for (auto &Arg : F->args())
   {
   std::string arg_temp=arg.front();
   arg.pop_front();
   Arg.setName(arg_temp);
   }
    
    
  return F;
}

void check_declaration_cgen(struct TreeNode *ast_node, std::list<std::string> &alloted_variables, std::map<std::string, AllocaInst*> &ValuesAllot, std::map<std::string, Value *> &NamedVal)
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
                        
                        //dec_variables.push_back(it_temp->name);
                        NamedVal[it_temp->name]=codegen_var(it_temp->name);
                        
                    }
                    else if ((it_temp->name==("init_declarator")))
                    {
                        if (it_temp->children.size()==2)
                        {   
                            if (it_temp->children.front()->name=="declarator")

                            {
                             
                             std::list<TreeNode*> curr= it_temp->children.front()->children;
                             //dec_variables.push_back(curr.back()->name);
                             NamedVal[curr.back()->name]=codegen_var(curr.back()->name);

                             std::string id=it_temp->children.back()->name;

                             bool iserror= id.find("'")!=std::string::npos || id.find('"')!=std::string::npos;

                            if (iserror==false && it_temp->children.back()->children.size()==0)
                            {   
                                if (!codegen_Variable(id,NamedVal))
                                {   
                                    std::string err= "Unknown variable accessed: " + id;
                                    LogErrorV(err);
                                
                                }
                                else
                                {
                                    allot_value((id),(curr.back()->name),NamedVal); 
                                     // Value* Val, Value* Variable
                                    alloted_variables.push_back(curr.back()->name);
                                }
                                
                            }
                            else if (it_temp->children.back()->children.size()>0)
                            {
                                Value *var_assign=check_blk_cgen(it_temp->children.back()->children.front(),NamedVal,ValuesAllot, alloted_variables);
                                NamedVal[curr.back()->name]=var_assign;
                                alloted_variables.push_back(curr.back()->name);
                            }
                            else if (iserror==true)
                            {
                                allot_value((id),(curr.back()->name),NamedVal); 
                                alloted_variables.push_back(curr.back()->name);
                            }
                             
                        }
                        
                           else 
                           {
                            //dec_variables.push_back(it_temp->children.front()->name);
                            NamedVal[it_temp->children.front()->name]=codegen_var(it_temp->children.front()->name);

                            Value *val_assign= check_blk_cgen(it_temp->children.back(),NamedVal,ValuesAllot,alloted_variables);
                            
                            NamedVal[it_temp->children.front()->name]=val_assign;
                            
                            alloted_variables.push_back(it_temp->children.front()->name);
                           

                           } 
                            
                        }

                        else 
                        {
                             
                             //dec_variables.push_back(it_temp->children.front()->name);
                             NamedVal[it_temp->children.front()->name]=codegen_var(it_temp->children.front()->name);
                             
                        }
                        
                    }
                    else if ( it_temp->name=="declarator")
                    {
                        //dec_variables.push_back(it_temp->children.back()->name);
                        NamedVal[it_temp->children.back()->name]=codegen_var(it_temp->children.back()->name);
                    }
                    
                    else // function name
                    {
                        declared_functions_cgen.push_back(it_temp->name);
                        
                        std::list<TreeNode*> param=it_temp->children.front()->children;
                        int num=0;
                        bool isellipsis=false;

                        std::list<std::string> args;

                         while(param.size()!=0)
                        {
                            TreeNode *func_temp=param.front();
                            param.pop_front();
                            if ((func_temp->name==("parameter_declaration")))
                            {
                                num=num+1;
                                std::list<TreeNode*> it_=func_temp->children;
                                
                                int track=0;
                                while (it_.size()!=0)
                                {    
                                    TreeNode* it_child=it_.front();
                                    it_.pop_front();

                                    if (((it_child)->name==("declarator")))
                                    {
                                        //declared_var.push_back((it_child)->children.back()->name);
                                        //NamedVal[(it_child)->children.back()->name]=codegen_var((it_child)->children.back()->name);
                                        args.push_back((it_child)->children.back()->name);
                                        //alloted_variables.push_back((it_child)->children.back()->name);
                                    }

                                    else if ((it_child)->children.size()==0 and track==1)
                                    {
                                        //declared_var.push_back((it_child)->name);
                                        //NamedVal[(it_child)->name]=codegen_var((it_child)->name);
                                        args.push_back((it_child)->name);
                                        //alloted_variables.push_back((it_child)->name);
                                    }

                                    track=track+1;
                                }

                        
                            }
                            else if ((func_temp->name==("ELLIPSIS")))
                            {
                                isellipsis=true;
                            }
                        }

                        if (isellipsis)
                        {
                            func_param_cgen.insert({it_temp->name,-1});
                            

                            Function *TheFunction = generate_variadicfunction(args, it_temp->name);

           BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
            Builder->SetInsertPoint(BB);
            Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                            
                        }
                        else
                        {
                            func_param_cgen.insert({it_temp->name,num});
                            Function *TheFunction = generate_function(args, it_temp->name);

            BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
            Builder->SetInsertPoint(BB);
             Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                            
                            
                        }

                        
                        
                        
                    }
                    
                    
                }


            }
            
        }
        
        i=i+1;
    }

   
}


//compound statements...

Value* check_block_cgen(BasicBlock * &BB, Function *TheFunction, struct TreeNode *ast_node, std::list<std::string> &alloted_variables, 
std::map<std::string, AllocaInst*> &ValuesAllot, std::map<std::string, Value *> &NamedVal, int &flag_loop)

{   
    flag_loop=0;
    Value* V=nullptr;

    
    if (ast_node->name !="block_item_list")
    {
        TreeNode* ast_temp=new TreeNode("block_item_list");
        ast_temp->children.push_back(ast_node);
        ast_node=ast_temp;
    }

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
                check_declaration_cgen(blk_node, alloted_variables, ValuesAllot,NamedVal);
                
                blk_node->children.pop_front();

            }

            else if (id==("RETURN"))
            {   
                //BB = BasicBlock::Create(*TheContext, "return", TheFunction);
                Builder->SetInsertPoint(BB);
                
                if (flag_loop==0)
                {
                    Value *ret= check_blk_cgen(blk_node->children.front(), NamedVal, ValuesAllot,alloted_variables);
                //Builder->CreateRet(ret);
                    Type* type = ret->getType();
                    if (type->isIntegerTy()) {

                        IntegerType* it = (IntegerType*) type;

                        if (it->getBitWidth() != 32)
                     // convert to  int32
                            ret=Builder->CreateZExt(ret, Builder->getInt32Ty(), "booltoint");
                    }

                    V=ret;
                    flag_loop=1;
                    
                }
                
                

            }

            else if (id=="if")
            {   
                //

                std::map<std::string, Value*> old_binding_if;
                old_binding_if=NamedVal;
                
                std::list<TreeNode*> cond_children= blk_node->children;
                
                TreeNode* condition=cond_children.front();
                TreeNode* block=cond_children.back();

                Value *CondV= check_blk_cgen(condition, NamedVal, ValuesAllot,alloted_variables);
                
                // Convert condition to a bool by comparing non-equal to 0.0.
                CondV = Builder->CreateICmpNE(CondV, ConstantInt::get(*TheContext, APInt(1,0)), "ifcond");

                // Emit then value.
                BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
                BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else", TheFunction);
                BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont", TheFunction);

                Builder->CreateCondBr(CondV, ThenBB, ElseBB);

                
                // Emit then value.
                Builder->SetInsertPoint(ThenBB);

                Value* ThenV= check_block_cgen(ThenBB,TheFunction,block, alloted_variables,ValuesAllot, old_binding_if,flag_loop);

                
                Builder->CreateBr(MergeBB);

                 // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
                ThenBB = Builder->GetInsertBlock();

                // Emit else block.
                
                Builder->SetInsertPoint(ElseBB);
                
                TreeNode *else_block= new TreeNode("block_item_list");
                else_block->children= child;

                //

                std::map<std::string, Value*>::iterator it = old_binding_if.begin(); 

                if (ThenV==nullptr)
                {
                    Builder->CreateBr(MergeBB);
                    Builder->SetInsertPoint(MergeBB);
                }
  
                // Iterating over the map using Iterator till map end. 
                while (it != old_binding_if.end()) 
                { 
                    // Accessing the key 
                    std::string ident = it->first; 
                    // Accessing the value 
                    if (!NamedVal[ident])
                    {

                    }

                    else
                    {   
                        if (old_binding_if[ident] == NamedVal[ident] )
                        {
                            
                        }

                        else 
                        {
                            if (ThenV== nullptr)
                            {
                                PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, ident.c_str());
                                PN->addIncoming(old_binding_if[ident], ThenBB);
                                PN->addIncoming(NamedVal[ident], ElseBB);
                            
                                NamedVal[ident]=PN;

                            }
                            

                           
                        }

                       
                    }
                    // iterator incremented to point next item 
                    it++; 
                } 
                
                
                Value *ElseV= nullptr;
                
                if (ThenV)
                {

                ElseV= check_block_cgen(ElseBB, TheFunction, else_block, alloted_variables, ValuesAllot, NamedVal,flag_loop);
                Builder->CreateBr(MergeBB);
                // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
                ElseBB = Builder->GetInsertBlock();
                Builder->SetInsertPoint(MergeBB);

                }

                else
                {
                    ElseV= check_block_cgen(MergeBB, TheFunction, else_block, alloted_variables, ValuesAllot, NamedVal,flag_loop);
                    Builder->SetInsertPoint(MergeBB);
                }
                
                //Builder->SetInsertPoint(MergeBB);
                // Emit merge block.
                
                                
                if (ThenV!=nullptr)
                            {   
                 

                                if ( ElseV != nullptr)
                                {   
                                    PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "iftmp");
                                    PN->addIncoming(ThenV, ThenBB);
                                    PN->addIncoming(ElseV, ElseBB);
                                    Builder->CreateRet(PN);
                                    flag_loop=1;
                
                                    break;
                                }  


                                else
                                {
                                    PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "iftmp");
                                    PN->addIncoming(ThenV, ThenBB);
                                    PN->addIncoming(ConstantInt::get(*TheContext, APInt(32,0)), ElseBB);
                                    Builder->CreateRet(PN);
                                    flag_loop=1;
                
                                    break;
                                }  


                                
                            }

                else
                    { 
                        
                        if ( ElseV != nullptr)
                                {   
                                    Builder->CreateRet(ElseV);
                                    
                                    
                                }  


                        else
                                {
                                   
                                    Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                                    
                                }  
                   
                    }


                BB=MergeBB;
                flag_loop=1;
                break;
                
            }

            else if (id=="ifelse")
            {   
                
                std::map<std::string, Value*> old_binding_if;
                old_binding_if=NamedVal;

                std::map<std::string, Value*> old_binding_else;
                old_binding_else=NamedVal;
                
                std::list<TreeNode*> cond_children= blk_node->children;
                
                TreeNode* condition=cond_children.front();
                cond_children.pop_front();
                TreeNode* block=cond_children.front();
                cond_children.pop_front();
                TreeNode* else_cond=cond_children.front();
                cond_children.pop_front();
                
                
                TreeNode *else_block= new TreeNode("block_item_list");
                else_block->children=(child);
                //else_block->children.push_front(else_cond);
                //

                

                Value *CondV= check_blk_cgen(condition, NamedVal, ValuesAllot,alloted_variables);
                
                // Convert condition to a bool by comparing non-equal to 0.0.
                CondV = Builder->CreateICmpNE(CondV, ConstantInt::get(*TheContext, APInt(1,0)), "ifcond");

                //Function *TheFunction = Builder->GetInsertBlock()->getParent();

                // Create blocks for the then and else cases.  Insert the 'then' block at the
                // end of the function.
                // Emit then value.
                BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
                BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else", TheFunction);
                BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont", TheFunction);

                Builder->CreateCondBr(CondV, ThenBB, ElseBB);

                // Emit then value.
                Builder->SetInsertPoint(ThenBB);

                Value* ThenV= check_block_cgen(ThenBB,TheFunction,block, alloted_variables,ValuesAllot, old_binding_if,flag_loop);
                
                Builder->SetInsertPoint(ElseBB);

                Value* ElseV_temp= check_block_cgen(ElseBB,TheFunction,else_cond, alloted_variables,ValuesAllot, old_binding_else,flag_loop);
                
                if (ThenV==nullptr and ElseV_temp!=nullptr)
                {   
                    Builder->SetInsertPoint(ThenBB);

                    ThenV=check_block_cgen(ThenBB, TheFunction, else_block, alloted_variables, ValuesAllot, old_binding_if,flag_loop);
                }
                
                Builder->SetInsertPoint(ThenBB);

                Builder->CreateBr(MergeBB);

               
                 // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
                ThenBB=Builder->GetInsertBlock();

                // Emit else block.

                Builder->SetInsertPoint(ElseBB);

                if (ThenV==nullptr)
                {
                    Builder->CreateBr(MergeBB);
                    Builder->SetInsertPoint(MergeBB);
                }
  
                std::map<std::string, Value*>::iterator it = old_binding_if.begin(); 
  
                // Iterating over the map using Iterator till map end. 
                while (it != old_binding_if.end()) 
                { 
                    // Accessing the key 
                    std::string ident = it->first; 
                    // Accessing the value 
                    if (!old_binding_else[ident])
                    {

                    }

                    else
                    {   
                        if (old_binding_if[ident] == old_binding_else[ident] )
                        {
                            
                        }

                        else 
                        {
                            if (ThenV== nullptr and ElseV_temp==nullptr)
                            {
                                PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, ident.c_str());
                                PN->addIncoming(old_binding_if[ident], ThenBB);
                                PN->addIncoming(old_binding_else[ident], ElseBB);
                            
                                NamedVal[ident]=PN;

                            }
                            

                           
                        }

                       
                    }
                    // iterator incremented to point next item 
                    it++; 
                } 

                
                Value *ElseV= nullptr;
                
                if (ThenV)
                {
                
                if (!ElseV_temp)
                ElseV= check_block_cgen(ElseBB, TheFunction, else_block, alloted_variables, ValuesAllot, old_binding_else,flag_loop);
                else
                ElseV=ElseV_temp;

                Builder->CreateBr(MergeBB);
                // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
                ElseBB = Builder->GetInsertBlock();
                Builder->SetInsertPoint(MergeBB);

                }

                else
                {
                
                if (!ElseV_temp)
                ElseV= check_block_cgen(MergeBB, TheFunction, else_block, alloted_variables, ValuesAllot, NamedVal,flag_loop);
                else
                {
                    ElseV=ElseV_temp;
                    
                }

                
                    Builder->SetInsertPoint(MergeBB);
                }
                
                // Emit merge block.
                
                if (ThenV!=nullptr)
                            {   
                                if ( ElseV != nullptr)
                                {   
                                    PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "iftmp");
                                    PN->addIncoming(ThenV, ThenBB);
                                    PN->addIncoming(ElseV, ElseBB);
                                    Builder->CreateRet(PN);
                                    flag_loop=1;
                                    break;
                                }  


                                else
                                {
                                    PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "iftmp");
                                    PN->addIncoming(ThenV, ThenBB);
                                    PN->addIncoming(ConstantInt::get(*TheContext, APInt(32,0)), ElseBB);
                                    Builder->CreateRet(PN);
                                    flag_loop=1;
                                    break;
                                }  
                                
                            }

                else
                    {
                        if ( ElseV != nullptr)
                                {   
                                    if (ElseV_temp!=nullptr)
                                    {
                                        PHINode *PN = Builder->CreatePHI(Type::getInt32Ty(*TheContext), 2, "iftmp");
                                        PN->addIncoming(ConstantInt::get(*TheContext, APInt(32,0)), ThenBB);
                                        PN->addIncoming(ElseV, ElseBB);
                                        Builder->CreateRet(PN);
                                        flag_loop=1;
                                        break;
                                    }
                                    else
                                    Builder->CreateRet(ElseV);
                                    
                                    
                                }  


                        else
                                {
                                   
                                    Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                                
                                    
                                }  


                    }


                BB=MergeBB;
                
                flag_loop=1;
                break;
                             
            }

            else if (id=="WHILE")
            {   //
                flag_loop=1;

                //
                //BasicBlock *_BB = BasicBlock::Create(*TheContext, "whilecondition", TheFunction);
                //Builder->SetInsertPoint(_BB);
                
                std::list<TreeNode*> cond_children= blk_node->children;
                
                TreeNode* condition=cond_children.front();
                TreeNode* block=cond_children.back();
                
                std::string VarName=condition->children.front()->children.front()->name;
                // Create an alloca for the variable in the entry block.
                AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
                
                Builder->CreateStore(NamedVal[VarName], Alloca);
                
     
                // Store the value into the alloca.
                
                
                //Function *TheFunction = Builder->GetInsertBlock()->getParent();

                // Create blocks for the then and else cases.  Insert the 'then' block at the
                // end of the function.
                // Emit then value.
        
                BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "loop_body", TheFunction);

                BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "after_loop",TheFunction);

                std::map<std::string, Value*>::iterator it = NamedVal.begin(); 

                // Iterating over the map using Iterator till map end. 
                while (it != NamedVal.end()) { 
                    // Accessing the key 
                    std::string ident = it->first; 

                    if (ident!=VarName )
                    {   
                        if (!ValuesAllot[ident])
                        {
                             AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, ident);
                             Builder->CreateStore(NamedVal[ident], Alloca);
                             ValuesAllot[ident]=Alloca;
                            
                        }
                       

                    }
                    it++;

                    // Accessing the value 
                }

                Builder->CreateBr(ThenBB);

                // Emit then value.
                Builder->SetInsertPoint(ThenBB);

                ValuesAllot[VarName]= Alloca;

            
                //Value *CondV= check_blk_cgen(condition, NamedVal, ValuesAllot,alloted_variables);


                check_block_cgen(ThenBB, TheFunction,block, alloted_variables,ValuesAllot, NamedVal,flag_loop);

                 Value *CondV= check_blk_cgen(condition, NamedVal, ValuesAllot,alloted_variables);

                // Create the "after loop" block and insert it.
                
                 // Insert the conditional branch into the end of LoopEndBB.
                 // Convert condition to a bool by comparing non-equal to 0.0.
                 
                CondV = Builder->CreateICmpNE(CondV, ConstantInt::get(*TheContext, APInt(1,0)), "loopcond");

                Builder->CreateCondBr(CondV, ThenBB, MergeBB);

                // Any new code will be inserted in AfterBB.
                Builder->SetInsertPoint(MergeBB);

                BB=MergeBB;
                
                flag_loop=0;
                
            }

            else if (id=="expression")
            {  
               
               check_block_cgen(BB, TheFunction,blk_node->children.front(), alloted_variables,ValuesAllot, NamedVal,flag_loop);
               
            
            }

            else if (id==":")

            {   
                
                std::string goto_lb= blk_node->children.front()->name;

                BasicBlock *gotoBB = BasicBlock::Create(*TheContext, "goto_blk", TheFunction);
                BasicBlock *gotoBB_after = BasicBlock::Create(*TheContext, "post_goto", TheFunction);

                std::map<std::string, Value*>::iterator it = NamedVal.begin(); 

                // Iterating over the map using Iterator till map end. 
                while (it != NamedVal.end()) { 
                    // Accessing the key 
                    std::string ident = it->first; 

                
                    
                        if (!ValuesAllot[ident])
                        {
                             AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, ident);
                             Builder->CreateStore(NamedVal[ident], Alloca);
                             ValuesAllot[ident]=Alloca;
                            
                        }
                       

                    
                    it++;

                    // Accessing the value 
                }
                //Builder->GetInsertBlock();
                check_block_cgen(BB, TheFunction,blk_node->children.back(), alloted_variables,ValuesAllot, NamedVal,flag_loop);
                
                Builder->CreateBr(gotoBB_after);
                Builder->SetInsertPoint(gotoBB);

                // Emit then value.
                
                check_block_cgen(BB, TheFunction,blk_node->children.back(), alloted_variables,ValuesAllot, NamedVal,flag_loop);
                
                Builder->CreateBr(gotoBB_after);
                // Emit then value.
                Builder->SetInsertPoint(gotoBB_after);
                
                goto_blk[goto_lb]=gotoBB;

            }

            else if (goto_blk[id])
            {   
            
                BasicBlock *temp= goto_blk[id];
                Builder->CreateBr(temp);
                Builder->SetInsertPoint(temp);
            
            }

            else if (id=="=") //assignment operator
            {   
                
                std::list<TreeNode*> assign_children= blk_node->children;
                
                TreeNode* variable=assign_children.front();
                TreeNode* value=assign_children.back();

                std::string VarName= variable->name;

                if (flag_loop==1)
                {
                    // Create an alloca for the variable in the entry block.

                    if (ValuesAllot[VarName])
                    {
                        
                        AllocaInst* Alloca= ValuesAllot[VarName];

                        Value *var_assign=check_blk_cgen(value,NamedVal,ValuesAllot,alloted_variables);
                        alloted_variables.push_back(variable->name);
                        
                        Builder->CreateStore(var_assign, Alloca);
                        NamedVal[VarName]=var_assign;

                    }

                    else
                    {   
                        

                        Value *var_assign=check_blk_cgen(value,NamedVal,ValuesAllot,alloted_variables);
                        alloted_variables.push_back(variable->name);
                      
                        NamedVal[VarName]=var_assign;



                    }
                    
                    
                }
                
                
                else
                {   
                    if (ValuesAllot[VarName])
                    {
                        
                        AllocaInst* Alloca= ValuesAllot[VarName];

                        Value *var_assign=check_blk_cgen(value,NamedVal,ValuesAllot,alloted_variables);
                        alloted_variables.push_back(variable->name);
                        
                        Builder->CreateStore(var_assign, Alloca);
                        NamedVal[VarName]=var_assign;

                    }

                    else
                    {
                        Value *var_assign=check_blk_cgen(value,NamedVal,ValuesAllot,alloted_variables);
                        NamedVal[variable->name]=var_assign;
                        alloted_variables.push_back(variable->name);


                    }
                    

                    
                    
                        
                    
                    
                }

                


            }
            
            else
            {
                //BasicBlock *BB = BasicBlock::Create(*TheContext, "block", TheFunction);
                Builder->SetInsertPoint(BB);
                check_blk_cgen(blk_node,NamedVal,ValuesAllot,alloted_variables);

            }
      

            
            

            }
        }
    
    
    return V;
}

void check_function_cgen(struct TreeNode *ast_node)
{
    Function *TheFunction;

    TreeNode *temp= ast_node;

    std:: string category=temp->category;
         
    std::list<TreeNode*> child= temp->children;

    std::list<std::string> alloted_variables;
    std::map<std::string, AllocaInst*> ValuesAllot;
    std::map<std::string, Value *> NamedVal;

    std::list<std::string> args;
    std::string Name;

    BasicBlock *BB;

    int i=0;
    int flag=0;
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

                declared_functions_cgen.push_back(fn->name);
                Name=fn->name;

                

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
                                
                                int track=0;
                                while (it.size()!=0)
                                {    
                                    TreeNode* it_child=it.front();
                                    it.pop_front();

                                    if (((it_child)->name==("declarator")))
                                    {   
                                        num=num-1;
                                        //declared_var.push_back((it_child)->children.back()->name);
                                        //NamedVal[(it_child)->children.back()->name]=codegen_var((it_child)->children.back()->name);
                                        //args.push_back((it_child)->children.back()->name);
                                        alloted_variables.push_back((it_child)->children.back()->name);
                                    }

                                    else if ((it_child)->children.size()==0 and track==1)
                                    {
                                        //declared_var.push_back((it_child)->name);
                                        //NamedVal[(it_child)->name]=codegen_var((it_child)->name);
                                        args.push_back((it_child)->name);
                                        alloted_variables.push_back((it_child)->name);
                                    }

                                    track=track+1;
                                }

                        
                            }
                            else if ((func_temp->name==("ELLIPSIS")))
                            {
                                isellipsis=true;
                            }
                        }

                        if (isellipsis)
                        {
                            func_param_cgen.insert({fn->name,-1});
                            
                        }
                        else
                        {
                            func_param_cgen.insert({fn->name,num});
                            
                            
                        }
                }
                else
                {   
                    func_param_cgen.insert({fn->name,0});
                    
                }

            TheFunction = generate_function(args, Name);

            BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
            Builder->SetInsertPoint(BB);

            for (auto &Arg : TheFunction->args())
               NamedVal[std::string(Arg.getName())] = &Arg;
            
            

            }

            else if ((fn->name==("block_item_list"))) //block
            {   
                int flag_loop=0;
                Value *return_val= check_block_cgen(BB, TheFunction, fn, alloted_variables, ValuesAllot, NamedVal,flag_loop);

                if (flag==1)
                {
                    Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                }
                
                // Run the optimizer on the function.

               
                else if (return_val != nullptr)
                {
                    Builder->CreateRet(return_val);
                }

                else if (flag_loop==0 and return_val==nullptr)
                {
                    Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
                }
                
                
            }

            else if (fn->name=="{}")
            {
                Builder->CreateRet(ConstantInt::get(*TheContext, APInt(32,0)));
            }
            
        }

        else
        {
            if (fn->name == "VOID")
            {
                flag=1;
            }
        }

        i=i+1;
    }

    // optimisation support

    //support already provided in respective ASTs

}

void code_generate (struct TreeNode *ast_node)
{       
    used_op.push_back("inclusive_or");
    used_op.push_back("Sub");
    used_op.push_back("DIV");
    used_op.push_back("Add");
    used_op.push_back("And");
    used_op.push_back("Mult");
    used_op.push_back("Logical_OR");
    used_op.push_back("GreaterThan");
    used_op.push_back("Logical_And");
    used_op.push_back("GE_OP");
    used_op.push_back("LE_OP");
    used_op.push_back("LEFT_OP");
    used_op.push_back("RIGHT_OP");
    used_op.push_back("LessThan");
    used_op.push_back("=");

   
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
                        
                        code_generate(it);
                        
                        
                    }

                }

                else if ((id==("declaration")))
                {
                    
                    check_declaration_cgen(ast_node, alloted_var, NamedValuesAllot, NamedValues);
                }
                
                else if ((id==("function_definition")))
                {   

                    check_function_cgen(ast_node);
                    
                }
                
            }
        

        
}

int final(struct TreeNode *ast_node)
{   
    TheContext = make_unique<LLVMContext>();
  TheModule = make_unique<Module>("Generating IR bitcode", *TheContext);

  // Create a new builder for the module.
  Builder = make_unique<IRBuilder<>>(*TheContext);

  // optimisation support

  // code generation

    code_generate(ast_node);
    
    // Print out all of the generated code.
    std::error_code EC;
llvm::raw_fd_ostream OS("output.ll", EC, llvm::sys::fs::F_None);



    if (EC)
      printf("Reported Errors : %s\n", EC.message().c_str());
    
    else 
      //TheModule->print(errs(), nullptr);
      TheModule->print(OS, nullptr);
    

    return 0;
}
