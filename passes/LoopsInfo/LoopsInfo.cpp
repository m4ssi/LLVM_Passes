// Pass
#include "llvm/Pass.h"

// Analysis
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"

// IR
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"

// ADT
#include "llvm/ADT/APInt.h"

// Support
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

// Transforms
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#define DEBUG_TYPE "loops-info"

using namespace llvm;

static cl::opt<bool> OutputFile("of", cl::init(false), cl::desc("Specify if the pass should module.txt instead of stdout"));

typedef struct
{
	Loop * LL;

	int id,
		loads,
		stores,
		others,
		paths,
		kind,
		n_subloops;
	bool has_ret;

	unsigned    start,
                end;

    StringRef parent;
}module_loop_t;

void print_innermort ( raw_ostream & os, module_loop_t &self)
{
/*
 * InnerMost
 */
	os << "loop_" << self.id << ";";
/*
	os << self.start << ";";
	os << self.end << ";";
*/
	os << 	"  innerost;";
	
	os << self.loads  << " loads;";
	os << self.stores << " stores;";
	os << self.others << " others;";
	os << self.paths  << " paths;";
	os << "from " << self.parent << ";\n";
	if ( self.has_ret)
	{
		os << "WARNING : your loop has a RETURN instruction before reaching the exit(s) bloc\n";				
	}	
}

void print_not_innermort ( raw_ostream & os, module_loop_t &self)
{
/*
 * OuterMost/Inbetween
 */
	os << "loop_" << self.id << ";";
/*
	os << self.start << ";";
	os << self.end << ";";
*/
	if ( self.kind == 1) os << 	"outermost" << ";";
	if ( self.kind == 2) os << 	" inbetween" << ";";
	os << self.n_subloops	<<" sub_loops;";
	
	os << "from " << self.parent << ";\n";

}

void print_stats ( raw_ostream &os, std::vector<module_loop_t> & loops, Module &M)
{
	os << "============================  Loops Info  ============================\n";
	os << "Module : " << M.getSourceFileName() << "\n";
	os << "loop_id;type;n subloops;from func_name\n";
	os << "loop_id;innermost;n loads;n stores;n others;n paths;from func_name\n";
	os << "\n";

	for ( auto self : loops){
		if ( self.kind == 3)
			print_innermort ( os, self);
		else
			print_not_innermort ( os, self);
	}	
}

int DFS ( Loop * L, BasicBlock * B)
{
	if ( L->isLoopLatch ( B)){
		return 1;
	}
	else if ( !L->contains ( B)){
		return 0;
	}
	// Instruction terminal
	Instruction * term = B->getTerminator();
		
	if (dyn_cast<ReturnInst>(term)) return 1;
	
	// Nombre de successeurs du blocs
	unsigned n = term->getNumSuccessors();
	int acc = 0;

	// Pour chaque BB successeur
	for (unsigned i = 0; i < n; i++)
	{
		// i-eme successeur
		acc += DFS ( L, term->getSuccessor ( i));
	}
	return acc;
}

int paths_innermost ( Loop * L)
{
	BasicBlock * header = L->getHeader();
	return DFS ( L, header);
}

int inner_most( std::vector<module_loop_t>& loops, Loop * LL, Function& F, int loop_id)
{
	// Count acces
	bool has_ret = false;
	int loads = 0, stores = 0, others = 0;
	for ( auto BB : LL->getBlocks())
	{
		for (BasicBlock::iterator BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
			// Load/Store
			if ( isa<LoadInst>(BI))
			{
				loads++;
			}
			else if ( isa<StoreInst>(BI))
			{
				stores++;
			}
			else
			{
				others++;
			}

		}
	}
	module_loop_t self;
	self.LL = LL;
	self.id = loop_id;
	// if debug infos
/*
	self.start = LL->getLocRange().getStart().getLine();
	self.end = LL->getLocRange().getEnd().getLine();
*/
	// else set to -1
	
	self.kind = 3;
	self.loads = loads;
	self.stores = stores;
	self.others = others;
	//~ self.paths = paths;
	self.paths = paths_innermost ( LL);
	self.n_subloops = 0;
	self.has_ret = has_ret;
	self.parent = LL->getHeader()->getParent()->getName();
	loops.push_back(self);
		
	return loop_id + 1;	
}

int not_inner_most ( std::vector<module_loop_t>& loops, Loop * LL, Function &F, int loop_id)
{
	if ( LL->isInnermost())
	{
		return inner_most ( loops,LL, F, loop_id);
	}
	
	int acc = 1, tmp = 0;
	std::vector< Loop * > SLV = LL->getSubLoops();

	module_loop_t self;
	self.LL = LL;
	self.id = loop_id;
	// if debug
/*
	self.start = LL->getLocRange().getStart().getLine();
	self.end = LL->getLocRange().getEnd().getLine();
*/
	// else -1

	self.n_subloops = SLV.size();
	self.kind = LL->isOutermost() ? 1 : 2;
	self.loads = 0;
	self.stores = 0;
	self.others = 0;
	self.paths = 0;
	self.parent = LL->getHeader()->getParent()->getName();
	self.has_ret = false;

	loops.push_back (self);
	
	for (auto SL : SLV)
	{
		tmp = not_inner_most ( loops, SL, F, loop_id+acc);
		acc++;
	}
	return tmp;
}

namespace {
struct LoopsInfo : public ModulePass {
	static char ID;
	raw_ostream * out;
    std::vector<module_loop_t> loops;
	LoopsInfo() : ModulePass(ID) {}

bool runOnModule(Module &M) override {
	int i = 0;
	for (auto & F : M)
	{
		if ( F.isDeclaration())
		continue;

		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();

		for (auto &LL : LI)
		{
			i = not_inner_most ( this->loops, LL, F, i);
		}
		
	}
	
	print_stats ( errs(), loops, M);
	return false;
}


// S'assure que la passe LoopSimplify est exécuté avant celle si
void getAnalysisUsage ( AnalysisUsage &AU) const	{
	AU.addRequiredID(LoopSimplifyID);
}


std::vector<module_loop_t>& getLoops()	{
	return loops;
}
}; // end of struct LoopsInfo
}  // end of anonymous namespace

char LoopsInfo::ID = 0;
static RegisterPass<LoopsInfo> X("loops-info", "LoopsInfo Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new LoopsInfo()); });

