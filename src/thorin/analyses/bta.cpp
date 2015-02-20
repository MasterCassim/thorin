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

void debug(World& world) {
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
	emit_thorin(world, true, false);
}

void bta(World& world) {
	std::cout << "Running bta on the following world:" << std::endl;
	debug(world);
}

}
