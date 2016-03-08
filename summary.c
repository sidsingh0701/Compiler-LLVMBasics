/*
 * File: summary.c
 *
 * Description:
 *   This is where you implement your project 3 support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LLVM Header Files */
#include "llvm-c/Core.h"
#include "dominance.h"


//#include "uthash.h"

/* Header file global to this project */
#include "summary.h"



/*struct TmpMap{
  LLVMBasicBlockRef bblock;
  struct TmpMap *next;            
  UT_hash_handle hh;         
};


struct TmpMap *head = NULL; 

struct TmpMap *list;

void add_tmp(LLVMBasicBlockRef BB) { 
  struct TmpMap *s; 
  s = malloc(sizeof(struct TmpMap)); 
  s->bblock = BB;
}
*/



typedef struct Stats_def {
  int functions;
  int globals;
  int bbs;
  int insns;
  int insns_bb_deps;
  int insns_g_deps;
  int branches;
  int loads;
  int stores;
  int calls;
  int loops; //approximated by backedges
} Stats;

void pretty_print_stats(FILE *f, Stats s, int spaces)
{
  char spc[128];
  int i;

  // insert spaces before each line
  for(i=0; i<spaces; i++)
    spc[i] = ' ';
  spc[i] = '\0';
    
  fprintf(f,"%sFunctions.......................%d\n",spc,s.functions);
  fprintf(f,"%sGlobal Vars.....................%d\n",spc,s.globals);
  fprintf(f,"%sBasic Blocks....................%d\n",spc,s.bbs);
  fprintf(f,"%sInstructions....................%d\n",spc,s.insns);
  fprintf(f,"%sInstructions (bb deps)..........%d\n",spc,s.insns_bb_deps);
  fprintf(f,"%sInstructions (g/c deps).........%d\n",spc,s.insns_g_deps);
  fprintf(f,"%sInstructions - Branches.........%d\n",spc,s.branches);
  fprintf(f,"%sInstructions - Loads............%d\n",spc,s.loads);
  fprintf(f,"%sInstructions - Stores...........%d\n",spc,s.stores);
  fprintf(f,"%sInstructions - Calls............%d\n",spc,s.calls);
  fprintf(f,"%sInstructions - Other............%d\n",spc,
	  s.insns-s.branches-s.loads-s.stores);
  fprintf(f,"%sLoops...........................%d\n",spc,s.loops);
}

void print_csv_file(const char *filename, Stats s, const char *id)
{
  FILE *f = fopen(filename,"w");
  fprintf(f,"id,%s\n",id);
  fprintf(f,"functions,%d\n",s.functions);
  fprintf(f,"globals,%d\n",s.globals);
  fprintf(f,"bbs,%d\n",s.bbs);
  fprintf(f,"insns,%d\n",s.insns);
  fprintf(f,"insns_bb_deps,%d\n",s.insns_bb_deps);
  fprintf(f,"insns_g_deps,%d\n",s.insns_g_deps);
  fprintf(f,"branches,%d\n",s.branches);
  fprintf(f,"loads,%d\n",s.loads);
  fprintf(f,"stores,%d\n",s.stores);
  fprintf(f,"calls,%d\n",s.calls);
  fprintf(f,"loops,%d\n",s.loops);
  fclose(f);
}

Stats MyStats;

void
Summarize(LLVMModuleRef Module, const char *id, const char* filename)
{
	LLVMValueRef i;	
	for (i = LLVMGetFirstGlobal(Module);
           i!= NULL;
       	   i = LLVMGetNextGlobal(i))
    	{
      		if (LLVMGetInitializer(i))
			MyStats.globals++;
    	}
	LLVMValueRef  fn_iter; // iterator 
	for (fn_iter = LLVMGetFirstFunction(Module); fn_iter!=NULL; 
  	   fn_iter = LLVMGetNextFunction(fn_iter))
	{
	  if(LLVMCountBasicBlocks(fn_iter)){
	// fn_iter points to a function
	  MyStats.functions++;
   	  LLVMBasicBlockRef bb_iter; /* points to each basic block
                              one at a time */
   	  	for (bb_iter = LLVMGetFirstBasicBlock(fn_iter);
			bb_iter != NULL; bb_iter = LLVMGetNextBasicBlock(bb_iter))
     		  {
			MyStats.bbs++;
			LLVMValueRef inst_iter; /* points to each instruction */
       			 for(inst_iter = LLVMGetFirstInstruction(bb_iter);
        	  	  inst_iter != NULL; 
 				inst_iter = LLVMGetNextInstruction(inst_iter)) 
        		{	
				   MyStats.insns++;
				   if(LLVMIsACallInst(inst_iter)) MyStats.calls++;
				   if(LLVMIsALoadInst(inst_iter)) MyStats.loads++;
				   if(LLVMIsAStoreInst(inst_iter)) MyStats.stores++;
				   int number_operands = LLVMGetNumOperands (inst_iter);
				   int k,count = 0;
				   LLVMValueRef inst_observe;
				   for(k=0;k<number_operands;k++){
						inst_observe = LLVMGetOperand(inst_iter,k);
						if(LLVMIsAGlobalValue(inst_observe) || LLVMIsAConstant(inst_observe))	count++;
				   }
				   if((count == number_operands) && (number_operands != 0) && (count != 0))	MyStats.insns_g_deps++;
				   int j,count_bb=0;
				   LLVMValueRef bb_observe;
				   for(j=0;j<number_operands;j++){
						bb_observe = LLVMGetOperand(inst_iter,j);
						if(LLVMIsAInstruction(bb_observe)){
							if(LLVMGetInstructionParent(bb_observe) == bb_iter){
								count_bb++;
							}
						}
				   }
				   if(count_bb > 0)	MyStats.insns_bb_deps++; 
				   LLVMOpcode opcode = LLVMGetInstructionOpcode(inst_iter);
				   if (opcode == LLVMBr){
					number_operands = LLVMGetNumOperands (inst_iter);
					if(number_operands == 3){
					//	if (LLVMIsATerminatorInst(inst_iter) && !LLVMIsAUnreachableInst(inst_iter)){	
							MyStats.branches++;
					//	}
					}
				  }
				

				

				if (LLVMIsABranchInst(inst_iter))
		      		{
		     			int index,count_head = 0;
	
		      			LLVMBasicBlockRef BB_TERM;
					if (LLVMIsATerminatorInst(inst_iter) && !LLVMIsAUnreachableInst(inst_iter))
		  			{
		      				number_operands = LLVMGetNumOperands (inst_iter);
						for(index=0;index<number_operands;index++)
		 				{
						  LLVMValueRef Header_check;
						  Header_check = LLVMGetOperand(inst_iter,index);
			 			  if (LLVMIsABasicBlock(Header_check))
			  			  {
  			   				  BB_TERM = (LLVMBasicBlockRef)(Header_check);
			    				  if (LLVMDominates(fn_iter,BB_TERM,bb_iter))
			    				  {
								//list = head;
				      				count_head = 0;
   			        				if (count_head == 0)
								{
								  count_head++;
								//  add_tmp(BB_TERM);
				 				  MyStats.loops++; // Counting number of loops
							        } 
			      			 	}
			  			}
					       }
		      		}  
		   }	
        		}
     	  	}
	  }
	}

	print_csv_file(filename,MyStats,id);
}

