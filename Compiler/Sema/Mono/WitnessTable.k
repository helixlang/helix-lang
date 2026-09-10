Right. Here's the whole problem and the decision, because it determines what ConformanceChecking has to *produce*, and that's the next thing on your list.

## The problem, precisely

A generic body emits **once**, as a C++ template. Inside `fn <T impl VectorI> f(x: T) { x.push(1) }`, the typer bound `push` to VectorI's requirement. The emitted C++ has to reach the *concrete* implementation for every instantiation:

- Conformance by an own method (`class Foo impl VectorI { fn push }`): `x.push(1)` works, C++ duck typing resolves it per instantiation.
- Conformance by an extension (`extend Foo impl VectorI { fn push }`): the implementation is a free function `std::Vec::push(Foo*, i32)`. Inside the template, `push(&x, 1)` finds nothing at definition and ADL at instantiation searches `Foo`'s namespace (`builtins`), not `std::Vec`. Qualifying it hardcodes one conformer. There is no single C++ spelling that reaches a different free function per `T`.

And since builtins are empty structs and *every* container method is an extension, this isn't an edge case it's `push` on a `[T]` in a generic function.

## The decision: witness structs

Every interface gets a witness template; every conformance gets a specialization; generic bodies call requirements through it:

```cpp
// emitted in VectorI's home TU (part of the interface's exported surface)
template <class Self, class T> struct VectorI_witness;

// emitted in the TU that declares the conformance (extend Vector impl VectorI, in std/Vec.k)
template <class T> struct VectorI_witness<builtins::Vector<T>, T> {
    static void push(builtins::Vector<T>* self, T x) { std::Vec::push(self, x); }
};

// emitted for an own-method conformance
template <> struct VectorI_witness<Foo, int> {
    static void push(Foo* self, int x) { self->push(x); }
};

// the generic body
template <class T> void f(T x) { VectorI_witness<T, /*iface args*/>::push(&x, 1); }
```

Uniform for both conformance kinds, zero cost after inlining, and it covers everything an interface can require: methods, operators (`op_plus` forwarder), constructors (`construct(...)`), statics. A generic conformance `extend <T> Vector<T> impl VectorI<T>` is a partial specialization; C++ does that natively. Friends are untouched: the forwarder calls the extension function, which is the friend.

Non-generic code never touches it: `v.push(1)` on a concrete type stays a fully qualified `std::Vec::push(&v, 1)` from `resolved_decl`.

## The `: type` rule that falls out

Under `<T: type>` there is no interface to key a witness on. A body emitted once can't reach an extension member per-`T` unless you generate a name-keyed dispatcher per extension member (`__ext_push<T>::call`) plus a `requires`-expression fallback to own members. That works in C++20 but it's a whole second dispatch system for one regime.

Don't. **`: type` means C++ duck typing: the body sees what C++ can see own members only. Extension members are unreachable under `: type`; use an interface bound.** That gives `: type` a precise meaning ("T is a C++ type, clang checks the body"), and the M2 re-check enforces it: a duck-body member that resolves to an extension member is an error naming the interface to bound with. Honest, one dispatch system, and it nudges people toward the regime that gives definition-side errors.

## Four homes

1. **ConformanceChecking** (`Check/`, stub today): builds the conformance table `(type decl, interface instance) → witness map: requirement decl → implementing decl` (own method or extension member), checking signatures as it goes. This table is the *only* input the emission needs, so its shape is the deliverable, not the checks. Also the visibility rule: a conformance must be in the instantiating TU's import set, otherwise the instance is an error same rule as extension visibility.
2. **ConformanceLowering** → new emitter `Codegen/Emit/EmitWitness.k`: the primary in the interface's TU (rides with the interface, invariant 11), one specialization per row of the table in the conformance's TU. Pure forwarders, no logic.
3. **EmitPlan**: new closure edge an instance `f<Foo>` whose bounds include `I` needs `I_witness<Foo>` **Complete** in the instance's home TU (§7: instantiation happens in exactly one TU, so only the home needs it). Tier 1, after `Foo` and the extension's function declarations it forwards to.
4. **EmitIR**: in a generic body, a call whose callee step bound to an interface requirement (that's `f->semantic_dc` owner being an `InterfaceDecl`, the same test `call_result_type` uses) emits `I_witness<T, args>::name(&recv, ...)` instead of a qualified call. That's one branch in `_call`.

Nothing here touches the typer. It already binds bound-members to the interface requirement, which is exactly the key the witness needs.