
**Ordering is DAG order, not just parallel.** `CompilerInstance::_sema`
runs each fid's pipeline in reverse import-tree order. I needs it (source
overlay complete before re-export folds). T needs it (an imported file's
nodes are settled before the importer READS them). Do not change the loop.

### 2.1 P parse [DONE]

POST: every DC has a frozen table; `out_of_line` collected.
INVARIANT: frozen tables are never mutated again by ANY later phase.

Statement-scope named decls (`class Local`, `fn inner` inside a body) are
linked into the enclosing executable DC's `dc_decls`
(`StmtParse::_attach_local_decl`, never for the TU). `_build_symbols`
indexes their BODIES through `_index_local_scopes` so members have tables;
their NAMES go in no table (N's lexical stack / T's `ltypes` own them).
`Self` in expression and pattern position lexes as `BiSelf` and parses as
an ordinary `NamedIdentExpr` head; `Self<...>` never takes generic args.

### 2.2 I ImportResolution [DONE, IMPORTS.md §4]

path->fid is the PP's; keys re-intern once at the boundary; overlay cells
are thin and multi-target; plain imports bind ONE name (the ModuleDecl);
import everything, carry the visibility fact, diagnose at use; two walls
against accidental transitivity; NO unfold pass, NO fwd-decl synthesis.

Selective and symbol-path imports of a name the source RE-EXPORTS
(`import foo::{bar}` where foo `pub import`s bar) probe the source's
overlay on a table miss (`_reexported_cell`). Wildcards fold re-exports.

### 2.3 E Expand [PARTIAL]

`RequiresDesugar` runs. Macro expansion and eval lowering do not.

### 2.4 V Verify [DONE]

Six independent structural checks. Errors hard-stop before N.

### 2.5 N NameResolution [DONE]

`Sema/Resolve/NameResolution.k`. Two halves, one pass.

(a) Decl validation over frozen cells: link `Redeclarable` chains for the
five type kinds + modules, retarget canonical at the definition, diagnose
redefinition and conflicting kinds, attach `out_of_line` defs, populate
`sc.well_known`. **Specializations are skipped**: their own chains link in
T on canonical spec args (#12).

    [MISSING] FUNCTION redecl chains: need signatures. OverloadResolution.

(b) Binding: every `NamedIdentExpr` head gets `resolved_decl` or
`candidate_cell`. Order: lexical locals stack (innermost first) ->
`sc->lookup.unqualified(cur_dc)` -> miss. Redecl chains collapse to the
representative; specializations never compete for a name; a genuine
overload set goes through whole.

    [DONE] Lexical stack holds: params, generic params of the enclosing
    function/type, statement binders (`for`, `catch`, destructuring,
    `case var n`, context bindings), body `var`s (declared AFTER their
    initializer), and statement-scope named decls (`fn inner`, `class
    Local`, `type X` -- declared BEFORE their body, so recursion binds).
    C-style `for var i = ...` scopes `i` to the loop.
    [DONE] `Self` (BiSelf) in expression position binds to the enclosing
    type body's decl -- the class/struct/... or the ExtensionDecl.
    "Outside a type body" is N's error in expression position. (parser: BiSelf
    is in EXPR_IDENT_TBL)
    [DONE] Bare-ffi miss gate (TEMPORARY, IMPORTS.md §5).
    [DONE, by design] N does NOT bind: chain steps (ChainBinding);
    ConstructorPattern heads and bare `case n` (pattern checking);
    named-initializer field names (inference); attribute ARGUMENTS.

`NameBindingVerifier` is N's exit test: no reachable `NamedIdentExpr`
survives with both slots null unless poisoned or foreign.

### 2.6 T TypeResolution [DONE]

`Sema/Resolve/TypeResolve.k`. Every type node gets `canonical`,
`type_flags`, per-segment `resolved_*`. Poisons on error.

**T never rewrites a node.** Slots only.

**Post-order, demand-driven.** `dispatch_type` resolves children first.
Aliases expand on demand (`_expand_alias`), memoized on ResolutionState;
`type A = B; type B = A` is one error with every link as a note.

**T never dispatches a node it does not own.** An imported alias body or a
default on an imported primary is READ through its slots (`_foreign`);
DAG order guarantees they are filled. Re-dispatching would consult a
per-TU memo that says "unresolved", re-run lookups from the wrong TU, and
report foreign errors into this sink.

**Unqualified lookup uses the overlay of the TU the walk started in**
(`NameLookup::unqualified`), never the pass's TU. Same key-space rule as
#2, applied to overlays.

What T decides:
- **Builtins** by token kind, before any lookup. `i32::x`: error.
- **`self`/`Self` in type position** (KwSelf, BiSelf): the innermost type
  scope's record with its own params as args.
  `Self::Inner` is a HEAD like any other type decl and walks the enclosing
  body's table; inside `extend Box<i32>` it walks the pattern, not the
  unfilled instance. `Self<T>` in type position is an error.
- **Heads**: generic frames (innermost first) -> local-type stack
  (`ltypes`: statement-scope type decls, one frame per block) ->
  `unqualified(cur_dc)`. Redecl chains collapse to the representative;
  `_pick_type_candidate` picks the primary and never a specialization; a
  name that has only specializations and no primary is an error here.
- **`::` segments** step through `context_of(decl)`, alias expanded first,
  spelled for cross-TU probes. `T::Item` and `Foo<T>::Inner` are marked
  dependent and stopped (#5); M2 owns member-of-instantiation.
- **Final decl -> canonical**: GenericParamDecl -> `generic_param(owner,
  index)` where owner is the REPRESENTATIVE of the declaring decl (frames
  are pushed with the representative, so a fwd decl's `T` and the
  definition's `T` are one identity). Alias -> expand. Nominal -> arity ->
  args by position and name -> defaults resolved in the PRIMARY's scope
  with the primary's params in frame (`_demand_default`) -> #12: dependent
  args -> `record(primary, args)`; concrete args -> `record(instance,
  args)` via the registry.
- **Specialization registration** (#12): a pre-pass over the TU's decl
  tree registers every Explicit/Partial type spec BEFORE any use resolves
  (`_register_specs_in`): find the primary (same name, same kind, not a
  spec, visible from the spec's scope -- "no primary" error home), resolve
  `spec_args` in the spec's own scope, register Explicit with concrete
  args in the instance table and everything else as a partial. A Primary
  the parser could not classify (#68) registers late, after
  `_refine_spec_kind`. Second explicit spec for the same args in the same
  file: fwd+def chain (`_link_spec`), or "redefinition of specialization".
  Different file: error. Already-instantiated: error.
- **Structural kinds** ask the store. `[T; N]` canonicalizes only for a
  literal N. Unprototyped `fn()` is zero-param.
  Dependence always flows up (`_component_ok`): `[T]`, `*T`, `(T, i32)`
  are dependent because a component is, canonical or not.
- **Receiver synthesis.** `ParamDecl::create_self` leaves `type_` null; T
  writes a `SelfType` with the enclosing record as canonical.
- **#68 refinement**; **enum underlying** must be a builtin integer;
  **`extend` target** resolved first; a record target becomes Self for the
  body. Non-record targets (`extend i32`, `extend <T> [T]`) are accepted --
  extendability is ExtensionLowering's question, not T's -- but `Self`
  inside such a body is "outside a type body" until `selfs` can hold a
  non-decl.

Error homes carry real diag-table codes. The name/type domain owns
`R020`-`R048` (`Resolution.diag.toml`); the Verify passes own `SC003`-`SC016`
(`Semantic.diag.toml`); an invariant violation reports `I003E`. That sweep is
complete for `Sema/` as of this writing, with one exception: `ImportResolution`
still shares `R015E` across three distinct errors.

`R001`/`SC001`/`U001` remain the per-category placeholder codes. Keep using them
for a new diagnostic rather than inventing a number inline -- a placeholder is
greppable and a wrong number is not. A later manual sweep triages the batch into
real codes, the same way this one did. Anything still on a placeholder must not
ship in a stable release.

    [PARTIAL] `Foo<i32>::Inner` (concrete args) is marked IsDependent, not
    merely instantiation-dependent; ChainBinding then refuses
    `Foo<i32>::Inner::make()`. Mark inst-dependent only when the previous
    segment's args are all concrete.

    [PARTIAL] `_register_specs_in` descends type and module scopes only;
    an explicit specialization written inside a function body is not
    registered.

### 2.7 ChainBinding [DONE]

`Sema/Resolve/ChainBinding.k`, after TypeResolution in the T stage. Walks
every `ChainExpr` left to right from an ANCHOR and binds each step:

    Module   -> `::` does table lookup (every reopened scope, unioned;
                re-exports via the module's overlay); `.` is an error
    Type     -> `::` does member lookup; `.` is an error
    Value    -> `.`/`->`/`?.`/`?->` peel the wrapper then member lookup on
                the canonical; `::` is an error
    NeedsInference / Dependent / Foreign / Errored -> record why, stop

Anchors: param/field/typed var -> Value; class/struct/enum/interface/
alias -> Type; `Self` bound to an ExtensionDecl -> Type (its target);
module -> Module; generic param -> Dependent; call / inferred var /
overload set / operator or tuple-index step -> NeedsInference; ffi
anchor -> Foreign.

    [DONE] A dependent RECORD is still looked up by name (`self.x` in
    `class <T> Foo` binds to the field); only a dependent NON-record
    (`T`, `*T`, `[T]`) stops as Dependent.
    [DONE] `Box<i32>::make()`: a type head with explicit args in
    expression position anchors as the registry instance -- positional,
    every param supplied, no packs, no names. Anything else is
    NeedsInference (inference owns defaults and named args, as for calls).
    [DONE] Cross-TU probes are spelled, not imm-keyed.

Commit rule: one decl -> `resolved_decl`; all functions ->
`candidate_cell`; distinct non-function entities under one name ->
ambiguity error. Outcomes recorded per step in `ResolutionTrace`.

### 2.8 C checks [PARTIAL]

`ConstraintExtraction` and `TypeCycleCheck` run. `OperatorSignatureCheck`,
`ConformanceChecking`, `ConstChecking`, `AccessCheck` are stubs.

    [DECIDED] TypeCycleCheck / LayoutPass treat a `RecordType::decl` that
    is `is_instance() && !instantiated` as "size unknown until M2", never
    as empty.

### 2.9 L, M1, M2 [MISSING]

Mono model: "Kairo enumerates and checks; C++ instantiates explicitly".
M1 walks `InstantiationRegistry::collect`; it creates nothing (T did). M2
is the sync point: fills `Instantiated` nodes from their pattern, selects
partials, runs dependent conformance, resolves `T::Item`.

---

## 2b. Member lookup & OOP

**MERGE, not hide** [DONE]. `MemberLookup::lookup(canonical, name, out)`
walks own table -> extensions -> bases breadth-first, deduped, UNION.

**Extensions** [DONE]: indexed once per run over every parsed TU.
Records keyed by `RecordType::decl` (the primary for `extend <T> Vec<T>`);
an instance decl also consults its `instantiated_from`'s extensions, so
the primary's extensions apply to `Vec<i32>` whether implicit or an
explicit spec. Generic extensions on STRUCTURAL types (`extend <T> [T]`)
are keyed by TypeKind (shape); exact non-generic targets (`extend i32`) by
canonical pointer.

**Instances** [DONE]: an explicit spec walks its own body. An unfilled
implicit instance walks its pattern (`instantiated_from`).

    [OPEN] Where do `[T]` / `{K:V}` / `string` members live? Today: only
    extensions. If `push` is a corelib record method, lang items must map
    the structural canonical to that record's decl before ChainBinding.

**`needs_using`** [MISSING]. **Virtual dispatch** [MISSING]. **Access
control** is a late filter [MISSING].

---

## 3. Hard invariants

1. **Frozen means frozen.**
2. **One key space per TU.** Cross-TU goes through the spelling shim; the
   overlay consulted is the one of the TU the walk started in.
3. **One error, one home.** Import existence/ambiguity: I. Import access:
   N(b). Redefinition / conflicting kinds: N(a). Unresolved head, `Self`
   outside a type body (expression): N(b). Unknown type / arity / alias
   cycle / primitive misuse / no primary for a spec / spec redefinition /
   spec after instantiation: T. No member / wrong separator / member
   ambiguity: ChainBinding. Dependent conformance: M2. Access: AccessCheck.
4. **Parallelism boundary.** P parallel. I, T DAG-ordered. N, M1 per-TU
   parallel. M2 sync. Store and registry are the shared mutable
   structures and both are locked.
5. **Dependent = deferred, not failed.**
6. **Tracing never changes behavior.**
7. **One writer per fact.** Heads: N. Steps: ChainBinding. Promotion
   cell->decl: inference. `canonical`, `type_flags`, segment slots, `self`
   receiver type, `spec_kind` refinement, spec chain links,
   `instantiated_from` on specs: T. `instantiated_from` on implicit
   nodes: registry (then M2). `needs_using`: MemberLookup.
8. **Canonical identity is build-wide.**
9. **T never rewrites nodes.**
10. **Builtins are not names.**
11. **Imports are erased at N/CB.**
12. **`RecordType::decl` is the record whose body defines the instance.**
    Dependent args -> the primary's representative (a pattern). Concrete
    args -> the instance decl from the registry: an explicit full
    specialization with equal canonical `spec_args`, else an implicit
    `Instantiated` node minted on first mention and filled by M2. Partial
    specializations are patterns: never a `RecordType::decl`. Spec redecl
    chains link in T on canonical `spec_args`, not in N(a). A spec's
    forward declaration and definition must be in one file. Specializing
    after instantiation is an error. Non-generic types never enter the
    registry.
13. **T never dispatches a foreign node.** Read the slot; DAG order fills it.
14. **The representative is the only identity.** Every frame owner, every
    `record()` decl, every `generic_param()` owner, every chain collapse
    goes through `NameLookup::representative`. No local copies.

---

## 4. What remains, in the order it should be done

Name/type domain -- residue:

    a. Split `ImportResolution`'s shared R015E into per-error codes    diag table
    b. `Foo<i32>::Inner` dependence flag (§2.6 PARTIAL)              small
    c. `NameLookup::qualified_step` spelled overload or delete       small
    d. decide the `[T]`/`string` member model (§2b OPEN)             design
    e. `_register_specs_in` into executable scopes (§2.6 PARTIAL)      small
    f. `Self` for non-record `extend` targets (§2.6)                  small
    g. closure bodies push a null DC (`_parse_closure_expr`); decls
       inside a closure get no parent and are not attached             small

Type domain (each unblocks the next):

    e. OverloadResolution     `Candidates` steps; function redecl chains;
                                ctor selection; operators (ADL); UFCS
    f. TypeInference          `NeedsInference` steps, inferred vars, tuple
                                index, initializer field names, generic
                                heads with defaults/named args
    g. Pattern checking       ctor-pattern heads, bare `case n`, `.Variant`
    h. AccessCheck
    i. ConformanceChecking
    j. ExtensionLowering      `semantic_dc` rewrite

Mono / codegen:

    k. M1 enumeration over the registry; M2 fill + partial selection +
       dependent resolution; EmitPlan reads `InstanceEntry::home_fid`.

Everything in (a)-(d) is name-domain residue. Everything from (e) on needs
a type on an EXPRESSION before a name can be picked, and is not resolution.