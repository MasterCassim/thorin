#include "thorin/analyses/bta.h"

#include <iostream>

#include <thorin/util/printer.h>

#include "thorin/analyses/bb_schedule.h"
#include "thorin/analyses/schedule.h"
#include "thorin/be/thorin.h"
#include "thorin/primop.h"
#include "thorin/type.h"
#include "thorin/world.h"


namespace thorin {

LV const LV::Top(LV::Dynamic);
LV const LV::Bot(LV::Static);

/** Computes the join of this lattice value with another. */
LV LV::join(LV const other) const {
    return LV(Type(type | other.type));
}

std::string to_string(LV const lv) {
    switch (lv.type) {
        case LV::Static:  return "Static";
        case LV::Dynamic: return "Dynamic";
        default:          THORIN_UNREACHABLE;
    }
}

class CodeGenCopy : public Printer {
private:
    BTA bta;
public:
    CodeGenCopy(BTA bta, bool fancy, bool colored = false)
            : Printer(std::cout, fancy, colored)
    {
        CodeGenCopy::bta = bta;
    }

    std::ostream& emit_type_vars(Type);
    std::ostream& emit_type_args(Type);
    std::ostream& emit_type_elems(Type);
    std::ostream& emit_type(Type);
    std::ostream& emit_name(Def);
    std::ostream& emit_def(Def);
    std::ostream& emit_primop(const PrimOp*);
    std::ostream& emit_assignment(const PrimOp*);
    std::ostream& emit_head(const Lambda*);
    std::ostream& emit_jump(const Lambda*);

    // BTA - stuff
    std::ostream& emit_jump_bta(const Lambda*);
    std::ostream& emit_head_bta(const Lambda*);
    std::ostream& emit_def_bta(Def);
    std::ostream& emit_name_bta(Def);
    std::ostream& emit_assignment_bta(const PrimOp*);
};

//------------------------------------------------------------------------------

std::ostream& CodeGenCopy::emit_type_vars(Type type) {
    if (type->num_type_vars() != 0)
        return dump_list([&](TypeVar type_var) { emit_type(type_var); }, type->type_vars(), "[", "]");
    return stream();
}

std::ostream& CodeGenCopy::emit_type_args(Type type) {
    return dump_list([&](Type type) { emit_type(type); }, type->args(), "(", ")");
}

std::ostream& CodeGenCopy::emit_type_elems(Type type) {
    if (auto struct_app = type.isa<StructAppType>())
        return dump_list([&](Type type) { emit_type(type); }, struct_app->elems(), "{", "}");
    return emit_type_args(type);
}

std::ostream& CodeGenCopy::emit_type(Type type) {
    if (type.empty()) {
        return stream() << "<NULL>";
    } else if (type.isa<FrameType>()) {
        return stream() << "frame";
    } else if (type.isa<MemType>()) {
        return stream() << "mem";
    } else if (auto fn = type.isa<FnType>()) {
        stream() << "fn";
        emit_type_vars(fn);
        return emit_type_args(fn);
    } else if (auto tuple = type.isa<TupleType>()) {
        emit_type_vars(tuple);
        return emit_type_args(tuple);
    } else if (auto struct_abs = type.isa<StructAbsType>()) {
        stream() << struct_abs->name();
        return emit_type_vars(struct_abs);
        // TODO emit args - but don't do this inline: structs may be recursive
        //return emit_type_args(struct_abs);
    } else if (auto struct_app = type.isa<StructAppType>()) {
        stream() << struct_app->struct_abs_type()->name();
        return emit_type_elems(struct_app);
    } else if (auto type_var = type.isa<TypeVar>()) {
        return stream() << '<' << type_var->gid() << '>';
    } else if (auto array = type.isa<IndefiniteArrayType>()) {
        stream() << '[';
        emit_type(array->elem_type());
        return stream() << ']';
    } else if (auto array = type.isa<DefiniteArrayType>()) {
        stream() << '[' << array->dim() << " x ";
        emit_type(array->elem_type());
        return stream() << ']';
    } else if (auto ptr = type.isa<PtrType>()) {
        if (ptr->is_vector())
            stream() << '<' << ptr->length() << " x ";
        emit_type(ptr->referenced_type());
        stream() << '*';
        if (ptr->is_vector())
            stream() << '>';
        auto device = ptr->device();
        if (device != -1)
            stream() << '[' << device << ']';
        switch (ptr->addr_space()) {
            case AddressSpace::Global:   stream() << "[Global]";   break;
            case AddressSpace::Texture:  stream() << "[Tex]";      break;
            case AddressSpace::Shared:   stream() << "[Shared]";   break;
            case AddressSpace::Constant: stream() << "[Constant]"; break;
            default: /* ignore unknown address space */            break;
        }
        return stream();
    } else if (auto primtype = type.isa<PrimType>()) {
        if (primtype->is_vector())
            stream() << "<" << primtype->length() << " x ";
        switch (primtype->primtype_kind()) {
#define THORIN_ALL_TYPE(T, M) case Node_PrimType_##T: stream() << #T; break;
#include "thorin/tables/primtypetable.h"
            default: THORIN_UNREACHABLE;
        }
        if (primtype->is_vector())
            stream() << ">";
        return stream();
    }
    THORIN_UNREACHABLE;
}

std::ostream& CodeGenCopy::emit_def(Def def) {
    if (auto primop = def->isa<PrimOp>())
        return emit_primop(primop);
    return emit_name(def);
}



std::ostream& CodeGenCopy::emit_name(Def def) {
    if (is_fancy()) // elide white = 0 and black = 7
        color(def->gid() % 6 + 30 + 1);

    stream() << def->unique_name();

    if (is_fancy())
        reset_color();

    return stream();
}

std::ostream& CodeGenCopy::emit_primop(const PrimOp* primop) {
    if (primop->is_proxy())
        stream() << "<proxy>";
    else if (auto primlit = primop->isa<PrimLit>()) {
        emit_type(primop->type()) << ' ';
        auto kind = primlit->primtype_kind();

        // print i8 as ints
        if (kind == PrimType_qs8)
            stream() << (int) primlit->qs8_value();
        else if (kind == PrimType_ps8)
            stream() << (int) primlit->ps8_value();
        else if (kind == PrimType_qu8)
            stream() << (unsigned) primlit->qu8_value();
        else if (kind == PrimType_pu8)
            stream() << (unsigned) primlit->pu8_value();
        else {
            switch (kind) {
#define THORIN_ALL_TYPE(T, M) case PrimType_##T: stream() << primlit->T##_value(); break;
#include "thorin/tables/primtypetable.h"
                default: THORIN_UNREACHABLE;
            }
        }
    } else if (primop->isa<Global>()) {
        emit_name(primop);
    } else if (primop->is_const()) {
        if (primop->empty()) {
            stream() << primop->op_name() << ' ';
            emit_type(primop->type());
        } else {
            stream() << '(';
            if (primop->isa<PrimLit>())
                emit_type(primop->type()) << ' ';
            stream() << primop->op_name();
            dump_list([&](Def def) { emit_def(def); }, primop->ops(), " ", ")");
        }
    } else
        emit_name(primop);

    return stream();
}

std::ostream& CodeGenCopy::emit_assignment(const PrimOp* primop) {
    emit_type(primop->type()) << " ";
    emit_name(primop) << " = ";

    auto ops = primop->ops();
    if (auto vectorop = primop->isa<VectorOp>()) {
        if (!vectorop->cond()->is_allset()) {
            stream() << "@ ";
            emit_name(vectorop->cond()) << " ";
        }
        ops = ops.slice_from_begin(1);
    }

    stream() << primop->op_name() << " ";
    dump_list([&](Def def) { emit_def(def); }, ops);
    return newline();
}

std::ostream& CodeGenCopy::emit_head(const Lambda* lambda) {
    emit_name(lambda);
    emit_type_vars(lambda->type());
    dump_list([&](const Param* param) { emit_type(param->type()) << " "; emit_name(param); }, lambda->params(), "(", ")");

    if (lambda->is_external())
        stream() << " extern ";
    if (lambda->cc() == CC::Device)
        stream() << " device ";

    return up();
}

std::ostream& CodeGenCopy::emit_jump(const Lambda* lambda) {
    if (!lambda->empty()) {
        emit_def(lambda->to());
        dump_list([&](Def def) { emit_def(def); }, lambda->args(), " ", "");
    }
    return down();
}

// BTA - stuff
std::ostream& CodeGenCopy::emit_jump_bta(const Lambda* lambda) {
    if (!lambda->empty()) {
        emit_def(lambda->to());
        dump_list([&](Def def) { emit_def(def); }, lambda->args(), " ", "");
    }
    return down();
}

std::ostream& CodeGenCopy::emit_def_bta(Def def) {
    if (auto primop = def->isa<PrimOp>())
        return emit_primop(primop);
    return emit_name_bta(def);
}

std::ostream& CodeGenCopy::emit_assignment_bta(const PrimOp* primop) {
    emit_type(primop->type()) << " ";
    emit_name_bta(primop) << " = ";

    auto ops = primop->ops();
    if (auto vectorop = primop->isa<VectorOp>()) {
        if (!vectorop->cond()->is_allset()) {
            stream() << "@ ";
            emit_name_bta(vectorop->cond()) << " ";
        }
        ops = ops.slice_from_begin(1);
    }

    stream() << primop->op_name() << " ";
    dump_list([&](Def def) { emit_def(def); }, ops);
    return newline();
}

std::ostream& CodeGenCopy::emit_name_bta(Def def) {
    if (is_fancy()) // elide white = 0 and black = 7
        color(def->gid() % 6 + 30 + 1);

    stream() << def->unique_name();

    if (is_fancy())
        reset_color();

    //stream() << " : ";
    stream() << " [" << bta.get(def) << "] ";

    return stream();
}

std::ostream& CodeGenCopy::emit_head_bta(const Lambda* lambda) {
    emit_name_bta(lambda);
    emit_type_vars(lambda->type());
    dump_list([&](const Param* param) { emit_type(param->type()) << " "; emit_name_bta(param); }, lambda->params(), "(", ")");

    if (lambda->is_external())
        stream() << " extern ";
    if (lambda->cc() == CC::Device)
        stream() << " device ";

    return up();
}

//------------------------------------------------------------------------------

void BTA::emit_bta(const Scope& scope, bool fancy, bool nocolor) {
    CodeGenCopy cg(*this, fancy, nocolor);
    auto schedule = schedule_smart(scope);
    auto bbs = bb_schedule(scope);
    for (auto lambda : bbs) {
        int depth = lambda == scope.entry() ? 0 : 1;
        cg.indent += depth;
        cg.newline();
        cg.emit_head_bta(lambda);

        for (auto op : schedule[lambda])
            cg.emit_assignment_bta(op);

        cg.emit_jump_bta(lambda);
        cg.indent -= depth;
    }
    cg.newline();
}

void BTA::emit_bta(const World& world, bool fancy, bool nocolor) {
    CodeGenCopy cg(*this, fancy, nocolor);
    cg.stream() << "module '" << world.name() << "'\n\n";

    for (auto primop : world.primops()) {
        if (auto global = primop->isa<Global>())
            cg.emit_assignment_bta(global);
    }

    Scope::for_each<false>(world, [&] (const Scope& scope) { emit_bta(scope, fancy, nocolor); });
}

void BTA::emit(World &world) {
    emit_bta(world, true, true);
}

void BTA::run(World &world) {
    LatticeValues.clear();
    worklist.clear();

    // all global variables are dynamic by default
    for (auto primop : world.primops())
        if (auto global = primop->isa<Global>())
            propagate(global, LV::Top);

    // functions and arguments called from outside are dynamic
    for(auto lambda : world.lambdas())
        if(lambda->is_external() || lambda->cc() == CC::Device)
            for(auto param : lambda->params())
                propagate(param, LV::Top);

    while (not worklist.empty()) {
        auto const def = worklist.back();
        worklist.pop_back();
        visit(def);
    }
}

LV BTA::get(DefNode const *def) {
    /* implicitly invokes the default constructor if no entry is present */
    return LatticeValues[def];
}

/// Updates the analysis information by joining the value of key `def` with `lv`.
/// \return true if the information changed
bool BTA::update(DefNode const *def, LV const lv) {
    auto it = LatticeValues.find(def);
    if (LatticeValues.end() == it) {
        LatticeValues.emplace(def, lv);
        return true;
    }

    LV const Old = it->second;
    LV const New = Old.join(lv);
    LatticeValues[def] = New;
    return New != Old;
}

void BTA::propagate(DefNode const *def, LV const lv) {
    if (not update(def, lv))
        return; // nothing changed
    for (auto use : def->uses())
        worklist.push_back(use);
}

void BTA::visit(DefNode const *def) {
    if (get(def).isTop())
        return;

    if (auto select = def->isa<Select>())
        return visit(select);
    if (auto primOp = def->isa<PrimOp>())
        return visit(primOp);
    if (auto param = def->isa<Param>())
        return visit(param);
    if (auto lambda = def->isa<Lambda>())
        return visit(lambda);

    THORIN_UNREACHABLE;
}

void BTA::visit(Lambda const *lambda) {
    /* The Binding Type of a lambda is defined by the binding type of its TO. */
    auto const to = lambda->to();
    return propagate(lambda, get(to));
}

void BTA::visit(Param const *param) {
    LV lv;
    for (auto arg : param->peek())
        lv = lv.join(get(arg.def()));
    propagate(param, lv);
}

void BTA::visit(PrimOp const *primOp) {
    LV lv;
    for (auto op : primOp->ops())
        lv = lv.join(get(op));
    propagate(primOp, lv);
}

void BTA::visit(Select const *select) {
    auto const ops  = select->ops();
    auto const cond = ops[0];
    auto const lhs  = ops[1];
    auto const rhs  = ops[2];

    if (not update(select, get(cond)))
        return; // nothing changed

    /* Add all uses of the select.  This includes the lambda "owning" the select. */
    for (auto use : select->uses())
        worklist.push_back(use);
    /* Add the successors of this lambda. */
    worklist.push_back(lhs);
    worklist.push_back(rhs);
}

//------------------------------------------------------------------------------

void bta(World& world) {
    BTA bta;
    bta.run(world);
    bta.emit(world);
    // TODO export results
}

}
