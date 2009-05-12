//===- UnifyFunctionExitNodes.cpp - Make all functions have a single exit -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass is used to ensure that functions have at most one return
// instruction in them.  Additionally, it keeps track of which node is the new
// exit node of the CFG.  If there are no exit nodes in the CFG, the getExitNode
// method will return a null pointer.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/UnifyFunctionExitNodes.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/BasicBlock.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Type.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
using namespace llvm;

char UnifyFunctionExitNodes::ID = 0;
static RegisterPass<UnifyFunctionExitNodes>
X("mergereturn", "Unify function exit nodes");

Pass *llvm::createUnifyFunctionExitNodesPass() {
  return new UnifyFunctionExitNodes();
}

void UnifyFunctionExitNodes::getAnalysisUsage(AnalysisUsage &AU) const{
  // We preserve the non-critical-edgeness property
  AU.addPreservedID(BreakCriticalEdgesID);
  // This is a cluster of orthogonal Transforms
  AU.addPreservedID(PromoteMemoryToRegisterID);
  AU.addPreservedID(LowerSwitchID);
}

// UnifyAllExitNodes - Unify all exit nodes of the CFG by creating a new
// BasicBlock, and converting all returns to unconditional branches to this
// new basic block.  The singular exit node is returned.
//
// If there are no return stmts in the Function, a null pointer is returned.
//
bool UnifyFunctionExitNodes::runOnFunction(Function &F) {
  // Loop over all of the blocks in a function, tracking all of the blocks that
  // return.
  //
  std::vector<BasicBlock*> ReturningBlocks;
  std::vector<BasicBlock*> UnwindingBlocks;
  std::vector<BasicBlock*> UnreachableBlocks;
  for(Function::iterator I = F.begin(), E = F.end(); I != E; ++I)
    if (isa<ReturnInst>(I->getTerminator()))
      ReturningBlocks.push_back(I);
    else if (isa<UnwindInst>(I->getTerminator()))
      UnwindingBlocks.push_back(I);
    else if (isa<UnreachableInst>(I->getTerminator()))
      UnreachableBlocks.push_back(I);

  // Handle unwinding blocks first.
  if (UnwindingBlocks.empty()) {
    UnwindBlock = 0;
  } else if (UnwindingBlocks.size() == 1) {
    UnwindBlock = UnwindingBlocks.front();
  } else {
    UnwindBlock = BasicBlock::Create("UnifiedUnwindBlock", &F);
    new UnwindInst(UnwindBlock);

    for (std::vector<BasicBlock*>::iterator I = UnwindingBlocks.begin(),
           E = UnwindingBlocks.end(); I != E; ++I) {
      BasicBlock *BB = *I;
      BB->getInstList().pop_back();  // Remove the unwind insn
      BranchInst::Create(UnwindBlock, BB);
    }
  }

  // Then unreachable blocks.
  if (UnreachableBlocks.empty()) {
    UnreachableBlock = 0;
  } else if (UnreachableBlocks.size() == 1) {
    UnreachableBlock = UnreachableBlocks.front();
  } else {
    UnreachableBlock = BasicBlock::Create("UnifiedUnreachableBlock", &F);
    new UnreachableInst(UnreachableBlock);

    for (std::vector<BasicBlock*>::iterator I = UnreachableBlocks.begin(),
           E = UnreachableBlocks.end(); I != E; ++I) {
      BasicBlock *BB = *I;
      BB->getInstList().pop_back();  // Remove the unreachable inst.
      BranchInst::Create(UnreachableBlock, BB);
    }
  }

  // Now handle return blocks.
  if (ReturningBlocks.empty()) {
    ReturnBlock = 0;
    return false;                          // No blocks return
  } else if (ReturningBlocks.size() == 1) {
    ReturnBlock = ReturningBlocks.front(); // Already has a single return block
    return false;
  }

  // Otherwise, we need to insert a new basic block into the function, add a PHI
  // nodes (if the function returns values), and convert all of the return
  // instructions into unconditional branches.
  //
  BasicBlock *NewRetBlock = BasicBlock::Create("UnifiedReturnBlock", &F);

  SmallVector<Value *, 4> Phis;
  unsigned NumRetVals = ReturningBlocks[0]->getTerminator()->getNumOperands();
  if (NumRetVals == 0)
    ReturnInst::Create(NULL, NewRetBlock);
  else if (const StructType *STy = dyn_cast<StructType>(F.getReturnType())) {
    Instruction *InsertPt = NULL;
    if (NumRetVals == 0)
      InsertPt = NewRetBlock->getFirstNonPHI();
    PHINode *PN = NULL;
    for (unsigned i = 0; i < NumRetVals; ++i) {
      if (InsertPt)
        PN = PHINode::Create(STy->getElementType(i), "UnifiedRetVal." 
                         + utostr(i), InsertPt);
      else
        PN = PHINode::Create(STy->getElementType(i), "UnifiedRetVal." 
                         + utostr(i), NewRetBlock);
      Phis.push_back(PN);
      InsertPt = PN;
    }
    ReturnInst::Create(&Phis[0], NumRetVals, NewRetBlock);
  }
  else {
    // If the function doesn't return void... add a PHI node to the block...
    PHINode *PN = PHINode::Create(F.getReturnType(), "UnifiedRetVal");
    NewRetBlock->getInstList().push_back(PN);
    Phis.push_back(PN);
    ReturnInst::Create(PN, NewRetBlock);
  }

  // Loop over all of the blocks, replacing the return instruction with an
  // unconditional branch.
  //
  for (std::vector<BasicBlock*>::iterator I = ReturningBlocks.begin(),
         E = ReturningBlocks.end(); I != E; ++I) {
    BasicBlock *BB = *I;

    // Add an incoming element to the PHI node for every return instruction that
    // is merging into this new block...
    if (!Phis.empty()) {
      for (unsigned i = 0; i < NumRetVals; ++i) 
        cast<PHINode>(Phis[i])->addIncoming(BB->getTerminator()->getOperand(i), 
                                            BB);
    }

    BB->getInstList().pop_back();  // Remove the return insn
    BranchInst::Create(NewRetBlock, BB);
  }
  ReturnBlock = NewRetBlock;
  return true;
}