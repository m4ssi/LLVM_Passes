CC=/home/m4ssi/Documents/M2_CHPS/IHPS/COA/LLVM/llvm-env/bin/clang
OPT=/home/m4ssi/Documents/M2_CHPS/IHPS/COA/LLVM/llvm-env/bin/opt

LOOPSINFOPASS=/home/m4ssi/Documents/M2_CHPS/IHPS/COA/LLVM/llvm-env/lib/LLVMLoopsInfo.so
LOOPSRDTSCPASS=/home/m4ssi/Documents/M2_CHPS/IHPS/COA/LLVM/llvm-env/lib/LLVMLoopsRDTSC.so

all: loops dotprod_loops basic_loop simple_loop
	./basic_loop_rdtsc
	./simple_loop_rdtsc
	./dotprod_loops_rdtsc
	./loops_rdtsc

loops: loops.c
	$(CC) -emit-llvm -c -o loops.bc loops.c
	$(OPT) -load $(LOOPSINFOPASS) -enable-new-pm=0 -loops-info <loops.bc> /dev/null
	$(OPT) -load $(LOOPSRDTSCPASS) -enable-new-pm=0 -loops-rdtsc <loops.bc> loops_rdtsc.bc
	$(CC) -o loops_rdtsc loops_rdtsc.bc -latomic

dotprod_loops: dotprod_loops.c
	$(CC) -emit-llvm -c -o dotprod_loops.bc dotprod_loops.c
	$(OPT) -load $(LOOPSINFOPASS) -enable-new-pm=0 -loops-info <dotprod_loops.bc> /dev/null
	$(OPT) -load $(LOOPSRDTSCPASS) -enable-new-pm=0 -loops-rdtsc <dotprod_loops.bc> dotprod_loops_rdtsc.bc
	$(CC) -o dotprod_loops_rdtsc dotprod_loops_rdtsc.bc -latomic

basic_loop: basic_loop.c
	$(CC) -emit-llvm -c -o basic_loop.bc basic_loop.c
	$(OPT) -load $(LOOPSINFOPASS) -enable-new-pm=0 -loops-info <basic_loop.bc> /dev/null
	$(OPT) -load $(LOOPSRDTSCPASS) -enable-new-pm=0 -loops-rdtsc <basic_loop.bc> basic_loop_rdtsc.bc
	$(CC) -o basic_loop_rdtsc basic_loop_rdtsc.bc -latomic

simple_loop: simple_loop.c
	$(CC) -emit-llvm -c -o simple_loop.bc simple_loop.c
	$(OPT) -load $(LOOPSINFOPASS) -enable-new-pm=0 -loops-info <simple_loop.bc> /dev/null
	$(OPT) -load $(LOOPSRDTSCPASS) -enable-new-pm=0 -loops-rdtsc <simple_loop.bc> simple_loop_rdtsc.bc
	$(CC) -o simple_loop_rdtsc simple_loop_rdtsc.bc -latomic
	
clean:
	rm -f *.bc *_rdtsc 	./basic_loop ./simple_loop ./dotprod_loops ./loops
