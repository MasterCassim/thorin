#include "thorin/primop.h"
#include "thorin/lambda.h"
#include "thorin/world.h"
#include "thorin/util/hash.h"
#include "thorin/util/queue.h"

#include <iostream>

namespace thorin {

struct Edge {
    Lambda* src;
    Lambda* dst;
    bool operator == (const Edge& other) const { 
        return this->src == other.src && this->dst == other.dst; 
    }
};

struct EdgeHash {
    size_t operator () (const Edge& edge) {
        return hash_combine(hash_value(edge.src), edge.dst);
    }
};

typedef HashSet<Edge, EdgeHash> EdgeSet;

void sccp(World& world) {
    EdgeSet edges;
    std::queue<Def> ssa_queue;
    std::queue<Lambda*> flow_queue;

    for (auto lambda : world.externals())
        flow_queue.push(lambda);

    while (!flow_queue.empty() || !ssa_queue.empty()) {
        while (!flow_queue.empty()) {
            auto lambda = pop(flow_queue);
            lambda->dump();
        }

        while (!ssa_queue.empty()) {
            auto def = pop(ssa_queue);
            def->dump();
        }
    }

}

}
