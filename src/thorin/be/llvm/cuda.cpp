#include <llvm/IR/Function.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include "thorin/primop.h"
#include "thorin/world.h"
#include "thorin/be/c.h"
#include "thorin/be/llvm/cuda.h"

#include <iostream>
#include <fstream>

namespace thorin {

CUDACodeGen::CUDACodeGen(World& world)
    : CodeGen(world, llvm::CallingConv::C, llvm::CallingConv::C, llvm::CallingConv::C)
{}

void CUDACodeGen::emit() {
    auto name = get_output_name(world_.name());
    std::ofstream file(name);
    if (!file.is_open())
        throw std::runtime_error("cannot write '" + name + "': " + strerror(errno));
    thorin::emit_c(world_, file, CUDA);
    file.close();
}

}
