#include <exception>
#include <thorin/be/thorin.h>
#include "thorin/analyses/bta.h"
#include "thorin/primop.h"
#include "thorin/lambda.h"
#include "thorin/world.h"

namespace thorin {

namespace {
LV LV_STATIC  = LV(LV::Static);
LV LV_DYNAMIC = LV(LV::Dynamic);
}

/** Computes the join of this lattice value with another. */
LV LV::join(LV other) const {
    return LV(Type(type | other.type));
}

std::string to_string(LV const lv) {
    switch (lv.type) {
        case LV::Static:  return "Static";
        case LV::Dynamic: return "Dynamic";
        default:          THORIN_UNREACHABLE;
    }
}

std::ostream & operator<<(std::ostream &os, LV const lv) {
    return os << to_string(lv);
}

void LV::dump(std::ostream &os) const {
    os << *this << std::endl;
}

void LV::dump() const {
    dump(std::cerr);
}

/*
 *  Print the thorin program with or without bta information.
 */
void debug(World& world, bool bta) {
    if(bta) {
        emit_bta(world, true, true);
    } else {
        emit_thorin(world, true, true);
    }
}

/*
 *  Method handling definitions, updating lattice values accordingly.
 *  Returns true if something has changed.
 */
bool handle(const Def& def) {
    bool changed = false;

    if (auto primop = def->isa<PrimOp>()) {
        for (auto op : primop->ops()) {
            changed |= handle(op);
            changed |= def->join_lattice(op->get_lattice());
        }
    } else if(def->isa<Param>() || def->isa_lambda()) {
        // nothing to do in this case
    } else {
        // TODO: anything other to consider here?
        std::cerr << "Unknown / not handled definition found!" << std::endl;

        auto def2 = def.deref();

        // just overapproximate with dynamic
        changed |= def2->join_lattice(LV_DYNAMIC);

        for(auto op : def2->ops()) {
            changed |= handle(op);
        }
    }

    return changed;
}

/*
 *  Method forwarding information from one lambda to another.
 */
bool forward(Lambda& from, const Def& to) {
    bool changed = false;

    for(unsigned int i = 0; i < from.num_args(); i++) {
        changed |= handle(from.arg(i));
    }

    if(auto to_lambda = to->isa_lambda()) {
        std::cout << "\t\t" << "we have a lambda here!" << std::endl;

        if(from.num_args() != to_lambda->num_params()) {
            std::cerr << "Number of arguments does not match number of params" << std::endl;
        } else {
            for(unsigned int i = 0; i < from.num_args(); i++) {
                // the lattice value of the lambda is joined (control flow)
                auto out = from.arg(i)->get_lattice().join(from.get_lattice());

                changed |= to_lambda->param(i)->join_lattice(out);
            }
        }
    } else if(to->isa<Param>()) {
        // TODO: do we want to / can / should handle calling functions?
        //param->join_lattice(LV(LV::Dynamic));
    } else {
        std::cerr << "Definition has not handled type" << std::endl;

        // overapproximate in this case
        changed |= to.deref()->join_lattice(LV_DYNAMIC);
        for(auto op : to.deref()->ops()) {
            changed |= op->join_lattice(LV_DYNAMIC);
        }
    }

    return changed;
}

void bta(World& world) {
    std::cout << "Running bta on the following world:" << std::endl;
    debug(world, false);

    // all global variables are dynamic by default
    for (auto primop : world.primops()) {
        if (auto global = primop->isa<Global>())
            global->join_lattice(LV_DYNAMIC);
    }

    // functions and arguments called from outside are dynamic
    for(auto lambda : world.lambdas()) {
        if(lambda->is_external() || lambda->cc() == CC::Device) {
            //lambda->join_lattice(DYNAMIC);

            for(auto param : lambda->params()) {
                param->join_lattice(LV_DYNAMIC);
            }
        }
    }

    int i = 1;
    bool changed;
    do {
        changed = false;

        std::cout << "Starting with iteration " << i << std::endl;

        // just look at every lambda on its own
        for(auto lambda : world.lambdas()) {
            std::cout << "\t" << "looking at lambda: " << lambda->unique_name() << std::endl;

            auto to = lambda->to();
            if (auto primop = to->isa<PrimOp>()) {
                changed |= handle(primop);

                std::cout << "\t\t" << "we have a primop here! " << std::endl;
                if(auto select = primop->isa<Select>()) {
                    std::cout << "\t\t" << "we have a select here!" << std::endl;
                    auto cond = select->op(0).deref();
                    auto lhs = select->op(1).deref();
                    auto rhs = select->op(2).deref();

                    auto out = cond->get_lattice().join(lambda->get_lattice());

                    // TODO: think about doing this in handle?! can we have something more complex here (not a lambda)?
                    changed |= lhs->join_lattice(out);
                    changed |= rhs->join_lattice(out);

                    // we still have to forward argument lattice values
                    changed |= forward(*lambda, lhs);
                    changed |= forward(*lambda, rhs);
                }
            } else {
                // just forward the information one to one
                changed |= to->join_lattice(lambda->get_lattice());
                changed |= forward(*lambda, to);
            }
        }

        i++;

        std::cout << std::endl;
    } while(changed);

    std::cout << "Resulting annotated world:" << std::endl;
    debug(world, true);
}

}
