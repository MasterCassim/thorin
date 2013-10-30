CUDA_DIR ?= /usr/local/cuda
INC_DIRS = -I $(CUDA_DIR)/include \
           -I $(CUDA_DIR)/nvvm/include \
           -I $(CUDA_DIR)/nvvm/libnvvm-samples/common/include
LIB_DIRS = -L $(CUDA_DIR)/nvvm/lib64
OPT_FLAGS := -O3
DBG_FLAGS := 

all: main.out
	@echo "Running main ..."
	@./main.out

%.llo: %.cpp gpu_runtime.h
	clang++ $(DBG_FLAGS) $(INC_DIRS) -std=c++11 -o $@ -c -emit-llvm $<

%.bc: $(COMMON_OBJ) %.llo
	llvm-link $^ -o $*-unop.bc
	opt $*-unop.bc $(OPT_FLAGS) -o $@
	llvm-link $@ -o $@
	llvm-dis $@ -o $*.ll

%.s: %.bc
	llc $(OPT_FLAGS) $< -o $@

%.out: %.s
	clang++ $< $(LIB_DIRS) -lcuda -lnvvm -o $@

clean:
	@rm -f main $(MAIN_FILES) *.ll *.llo *.o *.bc *.s *.out

.PHONY: all
