#include <vector>

#include "thorin/world.h"
#include "thorin/analyses/scope.h"
#include "thorin/util/queue.h"

namespace thorin {

std::vector<const Param*> free_params(const Scope& scope) {
    DefSet set;
    std::vector<const Param*> params;
    std::queue<Def> queue;

    // now find all params not in scope
    auto enqueue = [&] (Def def) {
        if (!visit(set, def)) {
            if (auto param = def->isa<Param>()) {
                if (!scope.contains(param))
                    params.push_back(param);
            } else if (auto primop = def->isa<PrimOp>()) {
                for (auto op : primop->ops())
                    queue.push(op);
            }
        }
    };

    for (auto lambda : scope) {
        for (auto op : lambda->ops())
            enqueue(op);

        while (!queue.empty())
            enqueue(pop(queue));
    }

    return params;
}

}
