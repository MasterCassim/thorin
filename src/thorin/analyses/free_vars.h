#ifndef THORIN_ANALYSES_FREE_VARS_H
#define THORIN_ANALYSES_FREE_VARS_H

#include <vector>

#include "thorin/def.h"

namespace thorin {

class Scope;

std::vector<const Param*> free_params(const Scope&);

}

#endif
