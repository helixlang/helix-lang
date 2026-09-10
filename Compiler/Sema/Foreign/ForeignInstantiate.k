/// --- The Kairo Project ------------ Sema/Foreign/ForeignInstantiate.k --- ///
///
///   \brief Fill an implicit instance of an IMPORTED class template from
///          clang: complete the specialization in the primary's phase-A AST,
///          then import its members into the registry shell.
///
///   \detail
///     An instance of a C++ template is clang's, never M2's
///     (MemberLookup::filled_by_clang). Substituting into the pattern Kairo
///     imported would have to re-derive what clang's instantiation already
///     knows -- dependent member typedefs through `allocator_traits`,
///     SFINAE'd members, partial-specialization selection -- and get some of
///     it wrong. So the shell is filled by asking clang for the
///     ClassTemplateSpecializationDecl and running DeclImport over it exactly
///     as HeaderIndex runs it over a header. Afterwards the shell is an
///     ordinary filled instance (`instantiated`) and nothing walks the
///     pattern for it again.
///
///     Demand-driven: MemberLookup::_walk_record calls fill() the first time
///     anything looks into the shell. A failure is sticky
///     (`foreign_fill_failed`) and leaves lookup on the pattern -- exactly
///     the behaviour before fills existed, so a fill can only add answers.
///
///   \par Threading
///     `_mu` is ONE lock for every fill in the build, on purpose. A fill
///     allocates in the primary's header TU (tables, frozen lists, tokens),
///     and two HeaderSets can each hold their own sema_lock() while
///     instantiating two instances of one primary, so the set lock alone does
///     not cover the Kairo side. Fills are rare and cheap next to a parse.
///     Lock order: `_mu`, then the set's sema_lock(); nothing takes them the
///     other way round.
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

ffi "c++" import "Native/Types.hh";
ffi "c++" import "clang/AST/ASTContext.h";
ffi "c++" import "clang/AST/Decl.h";
ffi "c++" import "clang/AST/DeclCXX.h";
ffi "c++" import "clang/AST/DeclTemplate.h";
ffi "c++" import "clang/Basic/DiagnosticSema.h";
ffi "c++" import "clang/Sema/Sema.h";
ffi "c++" import "llvm/ADT/SmallVector.h";

import AST::Context::ASTContext::*;
import AST::Context::DeclContext::*;
import AST::Context::SymbolTable::*;
import AST::Context::InstantiationRegistry::*;
import AST::Node::Base::*;
import AST::Node::Declarations::*;
import AST::Node::Casting::*;
import Sema::Resolve::NameLookup::*;
import Sema::Foreign::ForeignRegistry::*;
import Sema::Foreign::TypeImport::*;
import Sema::Foreign::TypeExport::*;
import Sema::Foreign::DeclImport::*;
import Sema::Foreign::HeaderIndex::*;
import Interop::Clang::HeaderSet::*;

class ForeignInstantiate {
    priv var _mu:        std::Mutex;               ///< ONE lock for all fills; see the file header.
    priv var _hindex:    obs::<HeaderIndex>;
    priv var _reg:       obs::<ForeignRegistry>;
    priv var _instances: obs::<InstantiationRegistry>;
    priv var _why:       string = "";              ///< the last failure's reason, for the log line.

    pub var n_filled: usize = 0;
    pub var n_failed: usize = 0;

    fn ForeignInstantiate(self, hindex: *HeaderIndex, reg: *ForeignRegistry, instances: *InstantiationRegistry) {
        self._hindex    = hindex;
        self._reg       = reg;
        self._instances = instances;
    }

    /// Fill \p shell (an unfilled instance whose primary is imported). True
    /// if it is filled afterwards, by this call or an earlier one.
    fn fill(self, shell: *Decl) -> bool {
        if shell == &null { return false; }
        // Unlocked fast path. Both flags only ever go false -> true, each is
        // written LAST by a fill holding `_mu`, and today every caller is the
        // driver thread; the re-check under the lock is what is load-bearing.
        if ForeignInstantiate::_filled(shell) { return true; }
        if ForeignInstantiate::_failed(shell) { return false; }

        var lock = std::LockGuard(self._mu);
        if ForeignInstantiate::_filled(shell) { return true; }
        if ForeignInstantiate::_failed(shell) { return false; }

        if self._fill_locked(shell) {
            self.n_filled += 1;
            return true;
        }
        ForeignInstantiate::_set_failed(shell);
        self.n_failed += 1;
        // TODO(diag): no Kairo diagnostic in v1; the use site's member typing
        // reports `unknown` as it did before fills. When a header set's clang
        // diagnostics are routed through the engine (IMPORTS.md §5,
        // [MISSING]), a failed fill goes with them, pointed at the use that
        // demanded it.
        var prim = TypeExport::pattern_of(shell);
        Logger::warn(Logger::Stage::Driver,
            f"ForeignInstantiate: instance of {ForeignInstantiate::_spelling(prim)} not filled: {self._why}");
        return false;
    }

    priv fn _fill_locked(self, shell: *Decl) -> bool {
        // 1. who owns the pattern.
        var prim = TypeExport::pattern_of(shell);
        if prim == &null { self._why = "no pattern"; return false; }
        var hs = reinterpret_cast::<*HeaderSet>(TypeExport::origin_set_of(prim));
        if hs == &null { self._why = "primary has no header set"; return false; }
        // A set's clang instance lives until HeaderSetCache::release_all,
        // after every TU's sema. A fill past that is a pass looking into an
        // instance after Checked -- a schedule bug, never the user's.
        ICE_IF((!hs->alive()), "ForeignInstantiate: fill after the primary's header set was released");
        var cctx = hs->ast();
        var csema = hs->sema();
        if (cctx == &null) || (csema == &null) { self._why = "header set has no AST / Sema"; return false; }

        var ct = TypeExport::class_template_of(reinterpret_cast::<*clang::Decl>(TypeExport::clang_decl_of(prim)));
        if ct == &null { self._why = "primary is not a class template"; return false; }
        var t = self._hindex->target_for_fid(TypeExport::origin_fid_of(prim));
        if t == &null { self._why = "primary's header has no import target"; return false; }
        // The shell's own spec_args are empty by design; its args are the
        // registry entry's (canonical, parameter order, defaults filled).
        var e = self._instances->entry_of(shell);
        if e == &null { self._why = "not a registry instance"; return false; }

        // Everything from here asks clang::Sema, including spelling the args:
        // a nested instance argument is itself a CheckTemplateIdType.
        var slock = std::LockGuard(*(hs->sema_lock()));

        // 2. the args, spelled in THIS set's AST.
        var tx = TypeExport(cctx, csema, reinterpret_cast::<*void>(hs));
        var qs: llvm::SmallVector::<clang::QualType, 8>;
        for var i: usize = 0; i < e->args.size(); ++i {
            var q = tx.to_clang(e->args[i]);
            if q.isNull() { self._why = f"template argument {i} has no clang spelling in phase A"; return false; }
            qs.push_back(q);
        }

        // 3. declare + complete. RequireCompleteType is what instantiates the
        //    class definition: member DECLARATIONS, which is all sema needs.
        //    Member function bodies stay pending; phase B instantiates those
        //    itself from the PCH.
        var ty = tx.template_id(ct, qs);
        if ty.isNull() { self._why = "clang rejected the template-id"; return false; }
        var diags  = &(csema->getDiagnostics());
        var before = diags->getNumErrors();
        var incomplete = csema->RequireCompleteType(ct->getLocation(), ty, clang::diag::err_incomplete_type);
        if incomplete || (diags->getNumErrors() > before) {
            self._why = "clang could not complete the specialization";
            return false;
        }
        var spec = llvm::dyn_cast_or_null::<clang::ClassTemplateSpecializationDecl>(ty->getAsCXXRecordDecl());
        if spec == &null { self._why = "completed type is not a class template specialization"; return false; }

        // 4. import, in fill mode: every decl lands in the primary's header,
        //    the specialization itself maps to the shell.
        var di = self._hindex->importer_for(hs);
        di.fill_target = t;
        di.fill_scope  = NameLookup::context_of(shell);
        di.fill_spec   = static_cast::<*clang::Decl>(spec);
        var ti = self._hindex->type_importer_for(hs, t->ctx, t->fid);

        var pairs = vec::<(*void, *Decl)>();
        self._hindex->walk_declare(spec, &di, pairs);
        for var i: usize = 0; i < pairs.size(); ++i {
            di.define(reinterpret_cast::<*clang::Decl>(libcxx::get::<0>(pairs[i])), libcxx::get::<1>(pairs[i]), &ti);
        }
        di.define_instance(spec, shell, &ti);

        // 5. the shell becomes an imported decl: EmitIR spells it by the
        //    specialization's full name, and the registry maps the spec's
        //    USR to it so a later import naming the spec finds this node.
        var usr = TypeImport::usr_of(spec);
        var sp  = ti.spelling(clang::QualType(cctx->getCanonicalTagType(spec)));
        ForeignInstantiate::_stamp(shell, reinterpret_cast::<*void>(spec), std::nstring_to_string(usr),
                                   sp, reinterpret_cast::<*void>(hs), t->fid);
        if !usr.empty() { self._reg->put(usr, shell); }

        // 6. freeze what the fill created, THEN publish. `instantiated` is
        //    the fast path's flag and must be the last write.
        for var i: usize = 0; i < di.fill_scopes.size(); ++i {
            if di.fill_scopes[i]->dc_symbols != &null { di.fill_scopes[i]->dc_symbols->freeze(); }
        }
        ForeignInstantiate::_set_filled(shell);
        Logger::info(Logger::Stage::Driver,
            f"ForeignInstantiate: filled {sp}: {di.n_declared} decl(s) declared, {di.n_defined} defined, {ti.n_opaque} opaque type(s)");
        return true;
    }

    // === per-kind flag access ===============================================
    //
    // An imported class template is minted as a Class, Struct or Union
    // (DeclImport::_mint_record), and its shells share the primary's kind.

    static fn _filled(d: *Decl) -> bool {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->instantiated;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->instantiated; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->instantiated;  }
        return false;
    }

    static fn _failed(d: *Decl) -> bool {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->foreign_fill_failed;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->foreign_fill_failed; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->foreign_fill_failed;  }
        return true;                                    // no kind we can fill: fail once, fast.
    }

    static fn _set_filled(d: *Decl) {
        if isa::<ClassDecl>(d)  { cast::<ClassDecl>(d)->instantiated  = true; return; }
        if isa::<StructDecl>(d) { cast::<StructDecl>(d)->instantiated = true; return; }
        if isa::<UnionDecl>(d)  { cast::<UnionDecl>(d)->instantiated  = true; return; }
    }

    static fn _set_failed(d: *Decl) {
        if isa::<ClassDecl>(d)  { cast::<ClassDecl>(d)->foreign_fill_failed  = true; return; }
        if isa::<StructDecl>(d) { cast::<StructDecl>(d)->foreign_fill_failed = true; return; }
        if isa::<UnionDecl>(d)  { cast::<UnionDecl>(d)->foreign_fill_failed  = true; return; }
    }

    static fn _stamp(d: *Decl, cd: *void, usr: string, sp: string, hs: *void, fid: u32) {
        if isa::<ClassDecl>(d)  { cast::<ClassDecl>(d)->set_imported(cd, usr, sp, hs, fid);  return; }
        if isa::<StructDecl>(d) { cast::<StructDecl>(d)->set_imported(cd, usr, sp, hs, fid); return; }
        if isa::<UnionDecl>(d)  { cast::<UnionDecl>(d)->set_imported(cd, usr, sp, hs, fid);  return; }
    }

    static fn _spelling(d: *Decl) -> string {
        if d == &null { return "<no pattern>"; }
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->cxx_spelling;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->cxx_spelling; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->cxx_spelling;  }
        return "<?>";
    }
}
