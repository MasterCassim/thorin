#include "thorin/world.h"
#include "thorin/analyses/free_vars.h"
#include "thorin/analyses/scope.h"
#include "thorin/transform/mangle.h"
#include "thorin/util/queue.h"
#include "thorin/be/thorin.h"

#include <iostream>

namespace thorin {

void lift_builtins(World& world) {
    std::cout << "********************************************************************************" << std::endl;
    emit_thorin(world);
    std::vector<Lambda*> todo;
    for (auto cur : world.copy_lambdas()) {
        if (cur->is_passed_to_accelerator() && !cur->is_basicblock())
            todo.push_back(cur);
    }

    for (auto cur : todo) {
        Scope scope(cur);
        std::vector<Def> vars;
        for (auto param : free_params(scope)) {
            assert(param->order() == 0 && "creating a higher-order function");

            //if (param->type().isa<MemType>()) {
                //std::queue<Def> queue;
                //DefSet done;
                //auto enqueue = [&] (Def def) {
                    //if (!visit(done, def)) {
                        //queue.insert(def);
                    //}
                //};

                //for (auto use : param->uses()) {
                    //vars.push_back(use);
                //}
            //} else
            //assert(!param->type().isa<MemType>());
            vars.push_back(param);
        }

        auto lifted = lift(scope, vars);

        for (auto use : cur->uses()) {
            if (auto ulambda = use->isa_lambda()) {
                if (auto to = ulambda->to()->isa_lambda()) {
                    if (to->is_intrinsic()) {
                        auto oops = ulambda->ops();
                        Array<Def> nops(oops.size() + vars.size());
                        std::copy(oops.begin(), oops.end(), nops.begin());               // copy over old ops
                        assert(oops[use.index()] == cur);
                        nops[use.index()] = world.global(lifted, false, lifted->name);   // update to new lifted lambda
                        std::copy(vars.begin(), vars.end(), nops.begin() + oops.size()); // append former free vars
                        ulambda->jump(cur, nops.slice_from_begin(1));                    // set new args
                        // jump to new top-level dummy function
                        ulambda->update_to(world.lambda(ulambda->arg_fn_type(), to->cc(), to->intrinsic(), to->name));
                    }
                }
            }
        }

        assert(free_params(Scope(lifted)).empty());
    }

    std::cout << "********************************************************************************" << std::endl;
    emit_thorin(world);
}

}
