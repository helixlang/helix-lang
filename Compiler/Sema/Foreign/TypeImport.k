/// --- The Kairo Project -------------------- Sema/Foreign/TypeImport.k --- ///
///
///   \brief clang::QualType -> Kairo CANONICAL type.
///
///   \detail
///     Always through `getCanonicalType()` first: canonical identity on the
///     Kairo side is clang's canonical identity, so two spellings of one
///     C++ type are one Kairo canonical. The sugared spelling is kept
///     separately (spelling()) for EmitIR, which writes C++ back and wants
///     the name the header used.
///
///     Rules that are decisions, not translations:
///       T*            -> *T           (safe pointer; foreign-pointer safety is a lint)
///       const T&      -> cxx_lref(T, const=true)   const IS in the key for refs only
///       T&&           -> cxx_rref(T)
///       builtins      by (size, sign, float format) against the target
///       Foo<int>      -> instances->instance_for(Foo, [int])   (registry shell)
///       T (tparam)    -> generic_param(owner, index)
///       unmappable    -> opaque(spelling)   usable as a value, never looked into
///
///     Nothing here allocates a syntax node. Every consumer of a foreign
///     decl reads `->canonical`, and a canonical is self-canonical, so the
///     canonical node itself goes into the decl's type slot.
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

/// FIXME: _BitInt(N) comes under clang::BitIntType, not a BuiltinType, add a dyn_cast<BitIntType> arm mapping 128/256/512 by getNumBits()

ffi "c++" import "Native/Types.hh";
ffi "c++" import "clang/AST/ASTContext.h";
ffi "c++" import "clang/AST/Decl.h";
ffi "c++" import "clang/AST/DeclCXX.h";
ffi "c++" import "clang/AST/DeclTemplate.h";
ffi "c++" import "clang/AST/PrettyPrinter.h";
ffi "c++" import "clang/AST/QualTypeNames.h";
ffi "c++" import "clang/AST/Type.h";
ffi "c++" import "clang/Index/USRGeneration.h";
ffi "c++" import "llvm/ADT/SmallString.h";

import AST::Context::ASTContext::*;
import AST::Context::CanonicalTypes::*;
import AST::Context::InstantiationRegistry::*;
import AST::Node::Base::*;
import AST::Node::Declarations::*;
import AST::Node::CanonicalNodes::*;
import AST::Node::Casting::*;
import Sema::Resolve::NameLookup::*;
import Sema::Foreign::ForeignRegistry::*;

class TypeImport {
    pub var store:     obs::<CanonicalTypeStore>;
    pub var instances: obs::<InstantiationRegistry>;
    pub var reg:       obs::<ForeignRegistry>;
    pub var cctx:      obs::<clang::ASTContext>;       ///< the phase-A context.
    pub var kctx:      obs::<ASTContext>;              ///< for opaque-name interning (any header ctx of the set).
    pub var fid:       u32 = 0;                        ///< home fid for registry entries.
    pub var n_opaque:  usize = 0;

    fn TypeImport(self, store: *CanonicalTypeStore, instances: *InstantiationRegistry,
                  reg: *ForeignRegistry, cctx: *clang::ASTContext, kctx: *ASTContext, fid: u32) {
        self.store     = store;
        self.instances = instances;
        self.reg       = reg;
        self.cctx      = cctx;
        self.kctx      = kctx;
        self.fid       = fid;
    }

    // === spelling ===========================================================

    /// The fully qualified C++ spelling of a (sugared) type, for EmitIR.
    fn spelling(self, qt: clang::QualType) -> string {
        var pol = clang::PrintingPolicy(self.cctx->getLangOpts());
        pol.FullyQualifiedName   = true;
        pol.SuppressScope        = false;
        pol.SuppressUnwrittenScope = true;
        pol.AnonymousTagLocations = false;
        var s = clang::TypeName::getFullyQualifiedName(qt, *(self.cctx), pol, true);
        return std::nstring_to_string(s);
    }

    static fn usr_of(d: const_obs::<clang::Decl>) -> libcxx::string {
        var buf: llvm::SmallString::<128>;
        if clang::index::generateUSRForDecl(d, buf) { return libcxx::string(); }
        return buf.str().str();
    }

    // === the mapping ========================================================

    /// Canonical for \p qt, never null: anything unmappable is opaque.
    fn _import(self, qt_in: clang::QualType) -> *Type {
        var qt = qt_in.getCanonicalType();
        var t  = self._map(qt);
        if t != &null { return t; }
        self.n_opaque += 1;
        return self.opaque(qt_in);
    }

    /// Canonical or null (no opaque fallback). For callers that want to
    /// decide themselves what an unmappable type means.
    fn try_import(self, qt_in: clang::QualType) -> *Type {
        return self._map(qt_in.getCanonicalType());
    }

    fn opaque(self, qt: clang::QualType) -> *Type {
        var sp  = self.spelling(qt);
        var imm = self.kctx->get_imm_table()->intern_ident_copy(sp);
        return self.store->opaque(imm);
    }

    priv fn _map(self, qt: clang::QualType) -> *Type {
        if qt.isNull() { return &null; }
        var ty = qt.getTypePtr();

        // --- builtins ---------------------------------------------------------
        var bt = llvm::dyn_cast::<clang::BuiltinType>(ty);
        if bt != &null { return self._builtin(bt); }

        // --- pointers / references --------------------------------------------
        var pt = llvm::dyn_cast::<clang::PointerType>(ty);
        if pt != &null {
            var pointee = pt->getPointeeType();
            if pointee->isFunctionType() {
                var f = self._map(pointee.getCanonicalType());
                return f;                                 // Kairo `fn(...)` IS the pointer.
            }
            var inner = self._map_or_opaque(pointee);
            return self.store->pointer(inner, false);
        }
        var lr = llvm::dyn_cast::<clang::LValueReferenceType>(ty);
        if lr != &null {
            var ref_ = lr->getPointeeType();
            return self.store->cxx_lref(self._map_or_opaque(ref_), ref_.isConstQualified());
        }
        var rr = llvm::dyn_cast::<clang::RValueReferenceType>(ty);
        if rr != &null {
            return self.store->cxx_rref(self._map_or_opaque(rr->getPointeeType()));
        }

        // --- arrays -------------------------------------------------------------
        var ca = llvm::dyn_cast::<clang::ConstantArrayType>(ty);
        if ca != &null {
            return self.store->array(self._map_or_opaque(ca->getElementType()),
                                     ca->getSize().getZExtValue());
        }
        var ia = llvm::dyn_cast::<clang::IncompleteArrayType>(ty);
        if ia != &null {
            return self.store->array(self._map_or_opaque(ia->getElementType()), 0);
        }

        // --- functions ------------------------------------------------------------
        var fp = llvm::dyn_cast::<clang::FunctionProtoType>(ty);
        if fp != &null {
            var params = vec::<*Type>();
            var n = fp->getNumParams();
            for var i: u32 = 0; i < n; ++i {
                params.push_back(self._map_or_opaque(fp->getParamType(i)));
            }
            return self.store->func(&params, self._map_or_opaque(fp->getReturnType()));
        }
        var fnp = llvm::dyn_cast::<clang::FunctionNoProtoType>(ty);
        if fnp != &null {
            var params = vec::<*Type>();
            return self.store->func(&params, self._map_or_opaque(fnp->getReturnType()));
        }

        // --- template parameters -----------------------------------------------
        var tp = llvm::dyn_cast::<clang::TemplateTypeParmType>(ty);
        if tp != &null {
            var pd = tp->getDecl();
            if pd == &null { return &null; }
            var owner = self._owner_of_param(pd);
            if owner == &null { return &null; }
            return self.store->generic_param(owner, tp->getIndex(), pd->isParameterPack());
        }
        var st = llvm::dyn_cast::<clang::SubstTemplateTypeParmType>(ty);
        if st != &null { return self._map(st->getReplacementType().getCanonicalType()); }

        // --- records / enums ------------------------------------------------------
        var rt = llvm::dyn_cast::<clang::RecordType>(ty);
        if rt != &null { return self._record(rt->getDecl()); }
        var et = llvm::dyn_cast::<clang::EnumType>(ty);
        if et != &null {
            var kd = self._decl_for(et->getDecl());
            if kd == &null { return &null; }
            var args = vec::<*Type>();
            return self.store->record(NameLookup::representative(kd), &args);
        }

        // A dependent specialization (`std::vector<T>` in a template body):
        // the primary with dependent args, a pattern (#5).
        var tst = llvm::dyn_cast::<clang::TemplateSpecializationType>(ty);
        if tst != &null {
            var td = tst->getTemplateName().getAsTemplateDecl();
            if td == &null { return &null; }
            var prim = self._decl_for(td);
            if prim == &null { return &null; }
            var args = vec::<*Type>();
            if !self._args(tst->template_arguments(), args) { return &null; }
            return self.store->record(NameLookup::representative(prim), &args);
        }

        // Everything else (member pointers, vectors, atomics, decltype that
        // stayed dependent, `nullptr_t`, ...): the caller decides.
        return &null;
    }

    priv fn _map_or_opaque(self, qt: clang::QualType) -> *Type {
        var t = self._map(qt.getCanonicalType());
        if t != &null { return t; }
        self.n_opaque += 1;
        return self.opaque(qt);
    }

    /// A record: concrete specialization -> registry instance; template
    /// pattern -> the primary; plain -> its decl.
    priv fn _record(self, rd: *clang::RecordDecl) -> *Type {
        if rd == &null { return &null; }
        var spec = llvm::dyn_cast::<clang::ClassTemplateSpecializationDecl>(rd);
        if (spec != &null) && (!llvm::isa::<clang::ClassTemplatePartialSpecializationDecl>(rd)) {
            var prim = self._decl_for(spec->getSpecializedTemplate());
            if prim == &null { return &null; }
            var args = vec::<*Type>();
            if !self._args(spec->getTemplateArgs().asArray(), args) { return &null; }
            var dep = false;
            for var i: usize = 0; i < args.size(); ++i {
                if args[i]->is_dependent() || args[i]->is_instantiation_dependent() { dep = true; }
            }
            var rep = NameLookup::representative(prim);
            if dep { return self.store->record(rep, &args); }
            var e = self.instances->instance_for(rep, &args, self.fid, NameLookup::ctx_of(rep));
            var inst = (e->decl) if (e != &null) else (rep);
            return self.store->record(inst, &args);
        }
        var kd = self._decl_for(rd);
        if kd == &null { return &null; }
        var rep = NameLookup::representative(kd);
        // A template's own record (the injected-class-name / pattern) is the
        // primary with its own params as args, dependent by construction.
        var args = vec::<*Type>();
        var cxx = llvm::dyn_cast::<clang::CXXRecordDecl>(rd);
        if (cxx != &null) && (cxx->getDescribedClassTemplate() != &null) {
            var tpl = cxx->getDescribedClassTemplate()->getTemplateParameters();
            for var i: u32 = 0; i < tpl->size(); ++i {
                var p = tpl->getParam(i);
                args.push_back(self.store->generic_param(rep, i, p->isParameterPack()));
            }
        }
        return self.store->record(rep, &args);
    }

    /// Template argument list -> canonicals. False if any argument is not a
    /// type (Kairo's registry keys on types only today; a value argument
    /// makes the whole use opaque).
    priv fn _args(self, list: llvm::ArrayRef::<clang::TemplateArgument>, out: ref!(vec::<*Type>)) -> bool {
        for var i: usize = 0; i < list.size(); ++i {
            var a = list[i];
            if a.getKind() == clang::TemplateArgument::Type {
                out.push_back(self._map_or_opaque(a.getAsType()));
                continue;
            }
            if a.getKind() == clang::TemplateArgument::Pack {
                if !self._args(a.pack_elements(), out) { return false; }
                continue;
            }
            return false;   // Integral / Declaration / Template / Expression: no canonical shape yet.
        }
        return true;
    }

    /// The Kairo decl a clang decl was imported as. Through the canonical
    /// decl's USR, so redeclarations and instances of one entity agree.
    fn _decl_for(self, d: const_obs::<clang::Decl>) -> *Decl {
        if d == &null { return &null; }
        var usr = TypeImport::usr_of(d->getCanonicalDecl());
        if usr.empty() { return &null; }
        return self.reg->find(usr);
    }

    /// The Kairo decl that DECLARES a template parameter: the templated
    /// decl (class/function/alias) that owns the parameter list.
    priv fn _owner_of_param(self, pd: const_obs::<clang::TemplateTypeParmDecl>) -> *Decl {
        var dc = pd->getDeclContext();
        // DeclContext is not in the Decl isa hierarchy; clang's explicit
        // cross-cast is the only correct one.
        var owner = clang::Decl::castFromDeclContext(dc);
        if owner == &null { return &null; }
        // The context is the TEMPLATED decl (the CXXRecordDecl / FunctionDecl);
        // its USR is what DeclImport registered.
        var kd = self._decl_for(owner);
        if kd == &null { return &null; }
        return NameLookup::representative(kd);
    }

    // === builtins ===========================================================

    priv fn _builtin(self, bt: const_obs::<clang::BuiltinType>) -> *Type {
        var k = bt->getKind();
        if k == clang::BuiltinType::Void     { return self.store->builtin(BuiltinKind::Void); }
        if k == clang::BuiltinType::Bool     { return self.store->builtin(BuiltinKind::Bool); }
        if k == clang::BuiltinType::NullPtr  { return &null; }                   // opaque: `nullptr_t`
        if k == clang::BuiltinType::Char32   { return self.store->builtin(BuiltinKind::Char); }
        if k == clang::BuiltinType::Float    { return self.store->builtin(BuiltinKind::F32); }
        if k == clang::BuiltinType::Double   { return self.store->builtin(BuiltinKind::F64); }
        if k == clang::BuiltinType::Half     { return self.store->builtin(BuiltinKind::F16); }
        if k == clang::BuiltinType::Float16  { return self.store->builtin(BuiltinKind::F16); }
        if k == clang::BuiltinType::BFloat16 { return self.store->builtin(BuiltinKind::BF16); }
        if k == clang::BuiltinType::Float128 { return self.store->builtin(BuiltinKind::F128); }
        if k == clang::BuiltinType::LongDouble {
            var bits = self.cctx->getTypeSize(clang::QualType(bt, 0));
            if bits == 64  { return self.store->builtin(BuiltinKind::F64); }
            if bits == 128 {
                // x86: 80-bit extended padded to 16 bytes. PPC double-double
                // and true binary128 also report 128; ask the format.
                var fmt = &(self.cctx->getFloatTypeSemantics(clang::QualType(bt, 0)));
                if fmt == &(llvm::APFloat::x87DoubleExtended()) { return self.store->builtin(BuiltinKind::F80); }
                if fmt == &(llvm::APFloat::IEEEquad())          { return self.store->builtin(BuiltinKind::F128); }
                return &null;                                  // PPC double-double: opaque.
            }
            return &null;
        }
        if bt->isInteger() {
            var bits = self.cctx->getTypeSize(clang::QualType(bt, 0));
            var sgn  = bt->isSignedInteger();
            if bits == 8   { return self.store->builtin((BuiltinKind::I8)   if (sgn) else (BuiltinKind::U8));   }
            if bits == 16  { return self.store->builtin((BuiltinKind::I16)  if (sgn) else (BuiltinKind::U16));  }
            if bits == 32  { return self.store->builtin((BuiltinKind::I32)  if (sgn) else (BuiltinKind::U32));  }
            if bits == 64  { return self.store->builtin((BuiltinKind::I64)  if (sgn) else (BuiltinKind::U64));  }
            if bits == 128 { return self.store->builtin((BuiltinKind::I128) if (sgn) else (BuiltinKind::U128)); }
            return &null;
        }
        return &null;   // vector/OpenCL/ObjC kinds: opaque.
    }
}