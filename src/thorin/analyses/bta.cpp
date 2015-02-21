#include <iostream>
#include <thorin/be/thorin.h>
#include <thorin/tables/nodetable.h>
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
		bool changed = false;

		std::cout << "Starting with iteration " << i << std::endl;

		// do fixpoint iteration here
		for(auto lambda : world.lambdas()) {
			std::cout << "\t" << "looking at lambda: " << lambda->unique_name() << std::endl;

			auto to = lambda->to();
			if (auto primop = to->isa<PrimOp>()) {
				std::cout << "\t\t" << "we have a primop here!" << std::endl;
				emit_def(primop);
			} else {
				// just forward information
				to->join_lattice(lambda->get_lattice());

				if(auto to_lambda = to->isa_lambda()) {
					std::cout << "\t\t" << "we have a lambda here!" << std::endl;

					if(lambda->num_args() != to_lambda->num_params()) {
						std::cerr << "Number of arguments does not match number of params" << std::endl;
					} else {
						for(int i = 0; i < lambda->num_args(); i++) {
							to_lambda->param(i)->join_lattice(lambda->arg(i)->get_lattice());
						}
					}
				}	 else {
					std::cerr << "Definition was not a lambda" << std::endl;
				}

			}
		}
	} while(changed);

	std::cout << "Resulting annotated world:" << std::endl;
	debug(world, true);
}

}
