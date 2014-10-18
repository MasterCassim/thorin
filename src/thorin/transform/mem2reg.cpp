#include "thorin/primop.h"
#include "thorin/world.h"
#include "thorin/analyses/scope.h"
#include "thorin/analyses/schedule.h"
#include "thorin/analyses/verify.h"
#include "thorin/transform/critical_edge_elimination.h"

namespace thorin {

class Mem2Reg {
public:
    Mem2Reg(const Scope& scope)
        : scope_(scope)
        , schedule_(schedule_late(scope))
    {}

    const Scope& scope() const { return scope_; }
    World& world() const { return scope_.world(); }
    void run();
    Def read(Lambda* lambda, Def ptr);
    void write(Lambda* lambda, Def ptr, Def val);
    size_t ptr2handle(Def ptr);
    bool escapes(Def ptr);
    bool is_escaping(Def ptr);

private:
    const Scope& scope_;
    Schedule schedule_;
    DefMap<size_t> slot2handle_;
    DefMap<bool> escaping_;
    LambdaMap<size_t> lambda2num_;
    size_t cur_handle_ = 0;
};

bool Mem2Reg::escapes(Def ptr) {
    assert(ptr->type().isa<PtrType>());
    auto i = escaping_.find(ptr);
    if (i != escaping_.end())
        return i->second;

    for (auto use : ptr->uses()) {
        if (auto lea = use->isa<LEA>()) {
            if (escapes(lea))
                return escaping_[ptr] = true;
        } else if (!use->isa<Load>() && !use->isa<Store>())
            return escaping_[ptr] = true;
    }
    return escaping_[ptr] = false;
}

bool Mem2Reg::is_escaping(Def ptr) {
    assert(ptr->type().isa<PtrType>());
    auto i = escaping_.find(ptr);
    if (i != escaping_.end())
        return i->second;
    return escaping_[ptr] = true;
}

Def Mem2Reg::read(Lambda* lambda, Def ptr) {
    if (auto slot = ptr->isa<Slot>()) {
        auto type = slot->type().as<PtrType>()->referenced_type();
        assert(slot2handle_.contains(slot));
        return lambda->get_value(slot2handle_[slot], type, slot->name.c_str());
    }
    auto lea = ptr->as<LEA>();
    return lambda->world().extract(read(lambda, lea->ptr()), lea->index());
}

void Mem2Reg::write(Lambda* lambda, Def ptr, Def val) {
    if (auto slot = ptr->isa<Slot>()) {
        assert(slot2handle_.contains(slot));
        lambda->set_value(slot2handle_[slot], val);
    } else {
        auto lea = ptr->as<LEA>();
        write(lambda, lea->ptr(), world().insert(read(lambda, lea->ptr()), lea->index(), val));
    }
}

void Mem2Reg::run() {
    for (auto lambda : scope())
        lambda->clear_value_numbering_table();

    // unseal all lambdas ...
    for (auto lambda : scope()) {
        lambda->set_parent(lambda);
        lambda->unseal();
        assert(lambda->is_cleared());
    }

    // ... except top-level lambdas
    scope().entry()->set_parent(0);
    scope().entry()->seal();

    for (auto lambda : scope()) {
        // search for loads/stores from top to bottom and use set_value/get_value to install parameters if pointer does not escape
        for (auto primop : schedule_[lambda]) {
            auto def = Def(primop);
            if (auto slot = def->isa<Slot>()) {
                if (!escapes(slot))
                    slot2handle_[slot] = cur_handle_++;
            } else if (auto load = def->isa<Load>()) {
                if (!is_escaping(load->ptr()))
                    load->replace(read(lambda, load->ptr()));
            } else if (auto store = def->isa<Store>()) {
                if (!is_escaping(store->ptr())) {
                    write(lambda, store->ptr(), store->val());
                    store->replace(store->mem());
                }
            }
        }

        // seal successors of last lambda if applicable
        for (auto succ : scope().succs(lambda)) {
            if (succ->parent() != nullptr) {
                auto i = lambda2num_.find(succ);
                if (i == lambda2num_.end())
                    i = lambda2num_.insert(std::make_pair(succ, scope().num_preds(succ))).first;
                if (--i->second == 0)
                    succ->seal();
            }
        }
    }
}

void mem2reg(World& world) {
    critical_edge_elimination(world);
    Scope::for_each(world, [] (const Scope& scope) { Mem2Reg(scope).run(); });
    world.cleanup();
    debug_verify(world);
}

}
