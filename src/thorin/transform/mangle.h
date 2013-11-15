#ifndef ANYDSL2_TRANSFORM_MANGLE_H
#define ANYDSL2_TRANSFORM_MANGLE_H

#include "anydsl2/type.h"

namespace anydsl2 {

class Scope;

Lambda* mangle(const Scope& scope, 
               ArrayRef<size_t> to_drop, 
               ArrayRef<Def> drop_with, 
               ArrayRef<Def> to_lift, 
               const GenericMap& generic_map = GenericMap());

Lambda* drop(const Scope& scope, ArrayRef<Def> with);
inline Lambda* clone(const Scope& scope, const GenericMap& generic_map = GenericMap()) { 
    return mangle(scope, Array<size_t>(), Array<Def>(), Array<Def>(), generic_map);
}
inline Lambda* drop(const Scope& scope, ArrayRef<size_t> to_drop, ArrayRef<Def> drop_with, const GenericMap& generic_map = GenericMap()) {
    return mangle(scope, to_drop, drop_with, Array<Def>(), generic_map);
}
inline Lambda* lift(const Scope& scope, ArrayRef<Def> to_lift, const GenericMap& generic_map = GenericMap()) {
    return mangle(scope, Array<size_t>(), Array<Def>(), to_lift, generic_map);
}

}

#endif