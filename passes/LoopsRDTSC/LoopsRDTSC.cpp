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
#include "llvm/ADT/APFloat.h"

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

	unsigned id;
	int	loads,
		stores,
		others,
		paths,
		kind,
		n_subloops;
	bool has_ret;

	unsigned    start,
                end;

    StringRef	parent,
				* name;
}module_loop_t;

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
	self.kind = 3;
	self.loads = loads;
	self.stores = stores;
	self.others = others;
	self.paths = 0;
	self.n_subloops = 0;
	self.has_ret = has_ret;
	self.parent = LL->getHeader()->getParent()->getName();
	self.name = new StringRef ( LL->getHeader()->getName().data());
	loops.push_back(self);

	LL->getHeader()->setName( Twine(std::string("loop_")+std::to_string(loop_id)));
	errs() << "Loop name : " << LL->getName() << " from function " << self.parent <<"\n";
	
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

int innermost_insertion (LLVMContext &CTX, Loop * LL, FunctionCallee &RDTSCCa, std::map< std::string, Value *>& loop_tac)
{
	// Insertion innermost
	
	// Entree de la boucle
	// A l'aide de la passe LoopSimplifyForm, on devrait avoir un unique Predecessor avant le header
		// Equivalent au Preheader

	BasicBlock * pred = LL->getLoopPredecessor();
//	BasicBlock * preh = LL->getLoopPreheader(); // Equivalent

	if ( !pred) {
		errs() << "Loop has multiple predecessors\n";
		return 1;
	}

	BasicBlock * header = LL->getHeader();

	// Construction d'un BasicBlock entre le Preheader et le Header

	BasicBlock * bb_tic = BasicBlock::Create (CTX, "tic_block", header->getParent(), header);
	IRBuilder<> tic_builder (bb_tic);
	tic_builder.SetInsertPoint(bb_tic);
	
	// Premier appel a rdtsc
	Value * tic = tic_builder.CreateCall ( RDTSCCa, None, "tic");
	
	
	// Le Preheader se branche maintenant vers le block 'tic'
	Instruction * term_B = pred->getTerminator();
	BranchInst * br_B = dyn_cast<BranchInst>(term_B);
	if ( br_B)
	{
		for ( unsigned i = 0; i < br_B->getNumSuccessors(); i++)
		{
			if ( br_B->getSuccessor(i) == header)
			{
				br_B->setSuccessor(i, bb_tic);
			}
		}
	}

	// Le block 'toc' se branch vers le Header
	/*BranchInst * tic_end = */tic_builder.CreateBr (header);
	
	
	// Loop exits
	SmallVector< BasicBlock *, 8 >  ExitBlocks, ExitingBlocks;
	LL->getExitBlocks (ExitBlocks);			// Blocks appartenant a la boucle
	LL->getExitingBlocks (ExitingBlocks);	// Blocks hors de la boucle

	for ( auto *BB : ExitBlocks)
	{
		BasicBlock * bb_tac = BasicBlock::Create (CTX, "tac_block", header->getParent(), BB);
		
		//~ Instruction * ip_tac = BB->getFirstNonPHI();
		
		IRBuilder<> tac_builder (bb_tac);
		tac_builder.SetInsertPoint(bb_tac);
		
		// Deuxieme appel a rdtsc
		Value * tac = tac_builder.CreateCall ( RDTSCCa, None, "tac");
	
		// Injection d'une soustraction pour avoir le temps ecoule
		Value * top = tac_builder.CreateNSWSub ( tac, tic, "top");

		// Mise a jour de la variable globale affectee a cette boucle
		/*AtomicRMWInst *armwi = */tac_builder.CreateAtomicRMW (
			 AtomicRMWInst::Add,
			 loop_tac[LL->getName().data()],
			 top,
			 Align(),
			 AtomicOrdering::SequentiallyConsistent,
			 SyncScope::System);


		// Insertion du block 'tac' entre les Exitings et les Exti blocks
		for ( auto *B : ExitingBlocks ) 
		{
			Instruction * term_B = B->getTerminator();
			BranchInst * br_B = dyn_cast<BranchInst>(term_B);
			if ( br_B)
			{

				for ( unsigned i = 0; i < br_B->getNumSuccessors(); i++)
				{
					if ( br_B->getSuccessor(i) == BB)
					{
						br_B->setSuccessor(i, bb_tac);
					}
				}
			}

		}
		/*BranchInst * tac_end = */tac_builder.CreateBr (BB);
	}
	return 1;
}


// Recherche des Innermost pour insertion
void search_innermost (LLVMContext &CTX, Loop * LL, FunctionCallee &RDTSCCa, std::map< std::string, Value *>& loop_tac)
{

	if ( LL->isInnermost())
	{		
		innermost_insertion ( CTX, LL, RDTSCCa, loop_tac);
	}
	std::vector< Loop * > SLV = LL->getSubLoops();	
	for (auto SL : SLV)
	{
		search_innermost ( CTX, SL, RDTSCCa, loop_tac);
	}
}

namespace {
struct LoopsRDTSC : public ModulePass {
	static char ID;
    std::vector<module_loop_t> loops;
	std::map<std::string, Value *> loop_tac;
	LoopsRDTSC() : ModulePass(ID) {}

bool runOnModule(Module &M) override {

	// Iterate at each insertion
	int i = 0;
	auto &CTX = M.getContext();

	
	
	// Injection des declarations de fonction
	
	PointerType * CharPtrTy  = PointerType::getUnqual ( Type::getInt8Ty ( CTX));

	// Injection de la declaration de printf
	FunctionType * PrintfTy = FunctionType::get ( IntegerType::getInt32Ty ( CTX), CharPtrTy, true);
	FunctionCallee PrintfCa = M.getOrInsertFunction ( "printf", PrintfTy);
	Function *     Printf   = dyn_cast<Function> ( PrintfCa.getCallee());

	Printf->setDoesNotThrow();
	Printf->addParamAttr(0, Attribute::NoCapture);
	Printf->addParamAttr(0, Attribute::ReadOnly);
	
	// Injection de la declaration de RDTSC
	FunctionType * RDTSCTy = FunctionType::get ( IntegerType::getInt64Ty ( CTX), false);
	FunctionCallee RDTSCCa = M.getOrInsertFunction ( "rdtsc", RDTSCTy);
	Function *     RDTSC   = dyn_cast<Function> ( RDTSCCa.getCallee());

	RDTSC->setDoesNotThrow();


	// Injection de la variable globale contenant le format pour la fonction printf
	// et de l'en-tete contenant les information sur la passe a affichier 
	Constant * LoopFmtStr = ConstantDataArray::getString ( CTX, "==loop-rdtsc== %16s  %16ld cycles\n");
	Constant * LoopFmtStrVar = M.getOrInsertGlobal( "LoopFmtStr", LoopFmtStr->getType());
	
	GlobalVariable * GlobalFmtVar = dyn_cast<GlobalVariable>(LoopFmtStrVar);
	GlobalFmtVar->setInitializer(LoopFmtStr);
	
	Constant * HeaderPassStr = ConstantDataArray::getString ( CTX,
		"============================== Loops RDTSC passe ==============================\n"
		"==loop-rdtsc==\n"
		"==loop-rdtsc==                    loop_id         number of cycles\n"
		"==loop-rdtsc==\n");

	Constant * HeaderPassStrVar = M.getOrInsertGlobal ( "HeaderPassStr", HeaderPassStr->getType());

	GlobalVariable * GlobalHeaderVar = dyn_cast<GlobalVariable>(HeaderPassStrVar);
	GlobalHeaderVar->setInitializer(HeaderPassStr);

	

	// Premiere phase d'analyse afin de separer les innermost des outermost
	for (auto & F : M)
	{
		if ( F.isDeclaration())
		continue;
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();


		for (auto &LL : LI)
		{
			// Reconstruction des sous boucle et affectation d'un nom "loop_id" en fonction de l'ordre de passage
			i = not_inner_most ( this->loops, LL, F, i);
		}
	}

	// Constante a zero pour initialiser les variables globlales
	Constant * zero = ConstantInt::get ( IntegerType::getInt64Ty ( CTX), 0, true);
	for (auto &l : loops)
	{
		if ( l.kind == 3) // Type 3 : Innermost
		{
			// Creation d'une variable globale pour chaque innermost
			loop_tac.insert( std::pair<std::string, Value *>( Twine(std::string("loop_")+std::to_string(l.id)).str(), new GlobalVariable(M, IntegerType::getInt64Ty(CTX),false, GlobalValue::CommonLinkage, 0, "loopCount")));
			dyn_cast<GlobalVariable>(loop_tac[Twine(std::string("loop_")+std::to_string(l.id)).str()])->setInitializer(zero);			
		}
		//~ errs() << "Laste test : " << loop_tac[LL->getName().data()] << "\n";
	}	

	// Insertion des appels a RDTSC dans un BasicBlock
	// Un BasicBlock est ajouté entre le Preheader et le Header 
	// Un BasicBlock est ajouté entre chaque coupe Exiting-Exit Block
	for (auto & F : M)
	{
		if ( F.isDeclaration())
		continue;
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();	
		for (auto &LL : LI)
		{
			search_innermost ( CTX, LL, RDTSCCa, loop_tac);
		}		
	}
	
	// Creation des appels a printf dans le main
	Function * Main = M.getFunction ( "main");
	
	Instruction * ip_beg = Main->getEntryBlock().getFirstNonPHI();
	
	IRBuilder <> MainEntryBuilder ( ip_beg);
	Value *MainFormatStrPtr = MainEntryBuilder.CreatePointerCast ( HeaderPassStrVar,
															  CharPtrTy,
															  "formatStr");

	Value * LoopInfoStrPtr = MainEntryBuilder.CreatePointerCast ( LoopFmtStrVar, CharPtrTy, "loopStrCount");

	for ( auto &BB : *Main)
	{
		for ( auto &II : BB)
		{
			// On chercher 'return' afin d'appeler printf avant ce dernier
			ReturnInst * RI = dyn_cast<ReturnInst>(&II);
			if ( RI)
			{
				IRBuilder <> MainTerminatorBuilder (RI);
				MainTerminatorBuilder.CreateCall (PrintfCa, {MainFormatStrPtr});
				// Pour chaque boucle innermost disposant d'un compteur
				for ( auto &tac : loop_tac)
				{
					auto LoopId = MainEntryBuilder.CreateGlobalStringPtr(tac.first);
					// Load global variable
					LoadInst *gvar_rdtsc_count =
					new LoadInst(IntegerType::getInt64Ty(CTX),
							   tac.second,
							   "load_gvar",
							   RI);

					// Injection de printf avec le loop_id et le temps mesure
					MainTerminatorBuilder.CreateCall(PrintfCa,
										  {LoopInfoStrPtr,
										   LoopId,
										   gvar_rdtsc_count
										  });
							   
				}
			}
		}
	}

	return true;
}


// S'assure que la passe LoopSimplify est exécuté avant celle si
void getAnalysisUsage ( AnalysisUsage &AU) const	{
	AU.addRequiredID(LoopSimplifyID);
}

void print (llvm::raw_ostream &O, const Module *M) const override	{
	//~ O << "LoopsRDTSC: " << L->LoopBase<BasicBlock,Loop>::getHeader()->getParent()->getName() << ":" << L->getName() << ": ";	  
	O << "LoopsRDTSC: \n";// << L->LoopBase<BasicBlock,Loop>::getHeader()->getParent()->getName() << ":" << L->getName() << ": ";	  
}

std::vector<module_loop_t>& getLoops()	{
	return loops;
}
}; // end of struct LoopsRDTSC
}  // end of anonymous namespace

char LoopsRDTSC::ID = 0;
static RegisterPass<LoopsRDTSC> X("loops-rdtsc", "LoopsRDTSC Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new LoopsRDTSC()); });

