#include "thorin/lambda.h"
#include "thorin/world.h"
#include "thorin/type.h"
#include "thorin/analyses/scope.h"
#include "thorin/analyses/verify.h"
#include "thorin/transform/mangle.h"
#include "thorin/transform/merge_lambdas.h"

namespace thorin {

class CFFLowering {
public:
    CFFLowering(World& world) {
        Scope::for_each(world, [&] (const Scope& scope) { top_.insert(scope.entry()); });
    }

    void transform(Lambda* lambda);
    size_t process();

private:
    LambdaSet top_;
};

void CFFLowering::transform(Lambda* lambda) {
    Scope scope(lambda);
    HashMap<Array<Def>, Lambda*> args2lambda;

    for (auto use : lambda->uses()) {
        if (use.index() == 0) {
            if (auto ulambda = use->isa_lambda()) {
                if (!scope.contains(ulambda)) {
                    Type2Type map;
                    bool res = lambda->type()->infer_with(map, ulambda->arg_fn_type());
                    assert(res);

                    size_t num_args = lambda->num_params();
                    //bool ret = false;
                    bool ret = true; // HACK
                    Array<Def> args(num_args);
                    for (size_t i = num_args; i-- != 0;) {
                        // don't drop the "return" of a top-level function
                        if (!ret &&  top_.find(lambda) != top_.end() && lambda->param(i)->type()->specialize(map)->order() == 1) {
                            ret = true;
                            args[i] = nullptr;
                        } else
                            args[i] = (lambda->param(i)->order() >= 1) ? ulambda->arg(i) : nullptr;
                    }

                    // check whether we can reuse an existing version
                    auto i = args2lambda.find(args);
                    Lambda* target;
                    if (i != args2lambda.end())
                        target = i->second; // use already dropped version as target
                    else
                        args2lambda[args] = target = drop(scope, args, map);

                    std::vector<Def> nargs;
                    for (size_t i = 0, e = num_args; i != e; ++i) {
                        if (args[i] == nullptr)
                            nargs.push_back(ulambda->arg(i));
                    }
                    ulambda->jump(target, nargs);
                }
            }
        }
    }
}

size_t CFFLowering::process() {
    std::vector<Lambda*> todo;
    for (auto top : top_) {
        Scope scope(top);
        for (auto i = scope.rbegin(), e = scope.rend(); i != e; ++i) {
            auto lambda = *i;
            if (!lambda->is_intrinsic() && !lambda->is_passed_to_accelerator()) {
                if (lambda->num_params() != 0                           // is there sth to drop?
                    && (lambda->type()->is_polymorphic()                // drop polymorphic functions
                        || (!lambda->is_basicblock()                    // don't drop basic blocks
                            && (!lambda->is_returning()                 // drop non-returning lambdas
                                || top_.find(lambda) == top_.end()))))  // lift/drop returning non top-level lambdas
                    todo.push_back(lambda);
            }
        }
    }

    for (auto lambda : todo)
        transform(lambda);

    return todo.size();
}

void lower2cff(World& world) {
    size_t todo;
    do {
        CFFLowering lowering(world);
        todo = lowering.process();
        debug_verify(world);
        merge_lambdas(world);
        world.cleanup();
    } while (todo);
    verify_calls(world);
}

}
