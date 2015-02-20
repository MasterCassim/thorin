#include <iostream>
#include "thorin/analyses/bta.h"
#include "thorin/primop.h"
#include "thorin/lambda.h"
#include "thorin/world.h"

namespace thorin {

LV LV::join(LV other) const {
    return LV(Type(type | other.type));
}

void debug(World& world) {
	for(auto lambda : world.lambdas()) {
		/*std::cout << lambda->unique_name() << " (";
		for(auto param : lambda->params()) {
			std::cout << param->unique_name() << ",";
		}
		std::cout << ")" << std::endl;*/
		lambda->dump();
	}
}

void bta(World& world) {
	std::cout << "Running bta on the following world:" << std::endl;
	debug(world);
}

}
