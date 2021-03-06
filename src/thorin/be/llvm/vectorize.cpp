#ifdef WFV2_SUPPORT
#include "thorin/be/llvm/llvm.h"

#include <llvm/PassManager.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Scalar.h>

#include "thorin/primop.h"
#include "thorin/world.h"

#include <wfvInterface.h>

namespace thorin {

llvm::Function* CodeGen::get_vectorize_tid() {
    const char* vectorize_intrinsic_name = "wfv_get_tid";
    return module_->getFunction(vectorize_intrinsic_name);
}

enum {
    VEC_ARG_MEM,
    VEC_ARG_LENGTH,
    VEC_ARG_LOWER,
    VEC_ARG_UPPER,
    VEC_ARG_BODY,
    VEC_ARG_RETURN,
    VEC_NUM_ARGS
};

Lambda* CodeGen::emit_vectorize_continuation(Lambda* lambda) {
    auto target = lambda->to()->as_lambda();
    assert(target->intrinsic() == Intrinsic::Vectorize);
    assert(lambda->num_args() >= VEC_NUM_ARGS && "required arguments are missing");

    // arguments
    auto vector_length = lookup(lambda->arg(VEC_ARG_LENGTH));
    auto lower = lookup(lambda->arg(VEC_ARG_LOWER));
    auto upper = lookup(lambda->arg(VEC_ARG_UPPER));
    auto kernel = lambda->arg(VEC_ARG_BODY)->as<Global>()->init()->as_lambda();

    const size_t num_kernel_args = lambda->num_args() - VEC_NUM_ARGS;

    // build simd-function signature
    Array<llvm::Type*> simd_args(num_kernel_args);
    for (size_t i = 0; i < num_kernel_args; ++i) {
        Type type = lambda->arg(i + VEC_NUM_ARGS)->type();
        simd_args[i] = convert(type);
    }

    auto simd_type = llvm::FunctionType::get(builder_.getVoidTy(), llvm_ref(simd_args), false);
    auto kernel_simd_func = (llvm::Function*)module_->getOrInsertFunction(kernel->unique_name() + "_vectorize", simd_type);

    // build iteration loop and wire the calls
    llvm::CallInst* simd_kernel_call;
    auto llvm_entry = emit_function_decl(entry_);
    auto loop_counter = create_loop(lower, upper, vector_length, llvm_entry, [&](llvm::Value* counter) {
        Array<llvm::Value*> args(num_kernel_args);
        for (size_t i = 0; i < num_kernel_args; ++i) {
            // check target type
            Def arg = lambda->arg(i + VEC_NUM_ARGS);
            auto llvm_arg = lookup(arg);
            if (arg->type().isa<PtrType>())
                llvm_arg = builder_.CreateBitCast(llvm_arg, simd_args[i]);
            args[i] = llvm_arg;
        }
        // call new function
        simd_kernel_call = builder_.CreateCall(kernel_simd_func, llvm_ref(args));
    });

    u32 vector_length_constant = lambda->arg(VEC_ARG_LENGTH)->as<PrimLit>()->qu32_value();
    wfv_todo_.emplace_back(vector_length_constant, loop_counter, emit_function_decl(kernel), simd_kernel_call);

    return lambda->arg(VEC_ARG_RETURN)->as_lambda();
}

void CodeGen::emit_vectorize(u32 vector_length, llvm::Value* loop_counter, llvm::Function* kernel_func, llvm::CallInst* simd_kernel_call) {
    auto cur_func = simd_kernel_call->getParent()->getParent();
    auto kernel_simd_func = simd_kernel_call->getCalledFunction();
    // ensure proper loop forms
    FunctionPassManager pm(module_);
    pm.add(llvm::createLICMPass());
    pm.add(llvm::createLCSSAPass());
    pm.run(*kernel_func);

    // vectorize function
    auto vector_tid_getter = get_vectorize_tid();
    WFVInterface::WFVInterface wfv(module_, &context_, kernel_func, kernel_simd_func, vector_length);
    wfv.addCommonMappings(true, true, true, true, false);
    if (vector_tid_getter) {
        bool b_simd = wfv.addSIMDSemantics(*vector_tid_getter, false, true, false, false, false, true, false, true, false, true);
        assert(b_simd && "simd semantics for vectorization failed");
    }
    bool b = wfv.run();
    assert(b && "vectorization failed");

    // inline kernel
    llvm::InlineFunctionInfo info;
    llvm::InlineFunction(simd_kernel_call, info);

    // wire loop counter
    if (vector_tid_getter) {
        std::vector<llvm::CallInst*> calls;
        for (auto it = vector_tid_getter->use_begin(), e = vector_tid_getter->use_end(); it != e; ++it) {
            if (auto call = llvm::dyn_cast<llvm::CallInst>(*it))
                if (call->getParent()->getParent() == cur_func)
                    calls.push_back(call);
        }
        for (auto it = calls.rbegin(), e = calls.rend(); it != e; ++it) {
            BasicBlock::iterator ii(*it);
            ReplaceInstWithValue((*it)->getParent()->getInstList(), ii, loop_counter);
        }
    }

    // remove functions
    kernel_func->removeFromParent();
    kernel_func->deleteBody();
    kernel_simd_func->removeFromParent();
    kernel_simd_func->deleteBody();
}

}

#endif
