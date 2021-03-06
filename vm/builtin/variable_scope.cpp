#include "vm.hpp"
#include "objectmemory.hpp"
#include "call_frame.hpp"
#include "gc/gc.hpp"

#include "builtin/object.hpp"
#include "builtin/variable_scope.hpp"
#include "builtin/class.hpp"
#include "builtin/system.hpp"
#include "builtin/tuple.hpp"

namespace rubinius {
  void VariableScope::init(STATE) {
    GO(variable_scope).set(state->new_class("VariableScope", G(object), G(rubinius)));
    G(variable_scope)->set_object_type(state, VariableScopeType);
    G(variable_scope)->name(state, state->symbol("Rubinius::VariableScope"));
  }

  void VariableScope::bootstrap_methods(STATE) {
    System::attach_primitive(state,
                             G(variable_scope), false,
                             state->symbol("method_visibility"),
                             state->symbol("variable_scope_method_visibility"));
  }

  VariableScope* VariableScope::of_sender(STATE, CallFrame* call_frame) {
    CallFrame* dest = static_cast<CallFrame*>(call_frame->previous);
    return dest->promote_scope(state);
  }

  VariableScope* VariableScope::current(STATE, CallFrame* call_frame) {
    return call_frame->promote_scope(state);
  }

  Tuple* VariableScope::locals(STATE) {
    Tuple* tup = Tuple::create(state, number_of_locals_);
    for(int i = 0; i < number_of_locals_; i++) {
      tup->put(state, i, get_local(state, i));
    }

    return tup;
  }

  // bootstrap method, replaced with an attr_accessor in kernel.
  Object* VariableScope::method_visibility(STATE) {
    return Qnil;
  }

  void VariableScope::Info::mark(Object* obj, ObjectMark& mark) {
    auto_mark(obj, mark);

    VariableScope* vs = as<VariableScope>(obj);

    vs->fixup();

    if(!vs->isolated()) {
      Object** ary = vs->stack_locals();
      size_t locals = vs->number_of_locals();

      for(size_t i = 0; i < locals; i++) {
        Object* tmp = mark.call(ary[i]);
        if(tmp) { ary[i] = tmp; }
      }
    }
  }

  void VariableScope::Info::visit(Object* obj, ObjectVisitor& visit) {
    auto_visit(obj, visit);

    VariableScope* vs = as<VariableScope>(obj);

    if(!vs->isolated()) {
      Object** ary = vs->stack_locals();
      size_t locals = vs->number_of_locals();

      for(size_t i = 0; i < locals; i++) {
        visit.call(ary[i]);
      }
    }
  }
}
