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
			lambda->join_lattice(DYNAMIC);

			for(auto param : lambda->params()) {
				param->join_lattice(DYNAMIC);
			}
		}
	}

	bool changed = false;
	do {
		// do fixpoint iteration here
	} while(changed);

	std::cout << "Resulting annotated world:" << std::endl;
	debug(world, true);
}

}
