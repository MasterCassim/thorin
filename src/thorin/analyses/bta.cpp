#include <iostream>
#include <thorin/be/thorin.h>
#include "thorin/analyses/bta.h"
#include "thorin/primop.h"
#include "thorin/lambda.h"
#include "thorin/world.h"

namespace thorin {

LV LV::join(LV other) const {
    return LV(Type(type | other.type));
}

std::string LV::dump() const {
	if(type == Static) {
		return "Static";
	}

	return "Dynamic";
}

void debug(World& world, bool bta) {
	/*for(auto lambda : world.lambdas()) {
		std::cout << lambda->unique_name() << " (";
		for(auto param : lambda->params()) {
			std::cout << param->unique_name() << ",";
		}
		std::cout << ")" << std::endl;
		std::cout << "\t";
		lambda->dump_jump();
		std::cout << "\t";
		lambda->to().dump();
	}*/
	if(bta) {
		emit_bta(world, true, true);
	} else {
		emit_thorin(world, true, true);
	}
}

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
		std::cerr << "Unknown / not handled definition found!" << std::endl;
	}

	return changed;
}

bool forward(Lambda& from, const Def& to) {
	bool changed = false;

	if(auto to_lambda = to->isa_lambda()) {
		std::cout << "\t\t" << "we have a lambda here!" << std::endl;

		if(from.num_args() != to_lambda->num_params()) {
			std::cerr << "Number of arguments does not match number of params" << std::endl;
		} else {
			for(unsigned int i = 0; i < from.num_args(); i++) {
				auto arg = from.arg(i);

				// we have to take the lambda joined argument
				auto out = arg->get_lattice().join(from.get_lattice());

				changed |= handle(arg);
				changed |= to_lambda->param(i)->join_lattice(out);
			}
		}
	} else if(to->isa<Param>()) {
		// TODO: do we want to handle calling functions?
		//param->join_lattice(LV(LV::Dynamic));
	} else {
		std::cerr << "Definition was not a lambda" << std::endl;
	}

	return changed;
}

void bta(World& world) {
	std::cout << "Running bta on the following world:" << std::endl;
	debug(world, false);

	LV DYNAMIC = LV(LV::Dynamic);

	// set all global variables to dynamic
	for (auto primop : world.primops()) {
		if (auto global = primop->isa<Global>())
			global->join_lattice(DYNAMIC);
	}

	// every extern function is dynamic with all parameters
	for(auto lambda : world.lambdas()) {
		if(lambda->is_external() || lambda->cc() == CC::Device) {
			//lambda->join_lattice(DYNAMIC);

			for(auto param : lambda->params()) {
				param->join_lattice(DYNAMIC);
			}
		}
	}

	int i = 1;
	bool changed;
	do {
		changed = false;

		std::cout << "Starting with iteration " << i << std::endl;

		// do fixpoint iteration here
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

					// TODO: think about doing this in handle?!
					changed |= lhs->join_lattice(out);
					changed |= rhs->join_lattice(out);

					// we still have to forward arguments
					changed |= forward(*lambda, lhs);
					changed |= forward(*lambda, rhs);
				}
			} else {
				// just forward information
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
