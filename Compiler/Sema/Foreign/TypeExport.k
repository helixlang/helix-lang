/// --- The Kairo Project -------------------- Sema/Foreign/TypeExport.k --- ///
///
///   \brief Kairo CANONICAL type -> clang::QualType, against ONE HeaderSet's
///          ASTContext. The inverse of TypeImport::_map.
///
///   \detail
///     Exists for ForeignInstantiate: to ask clang for `cont<i32>` it has to
///     spell `i32` as a clang type in the AST that owns `cont`. Null QualType
///     for anything it cannot express, and the caller reads null as "cannot
///     instantiate in phase A" -- never as an error of the user's.
///
///     What cannot be expressed, and why:
///       generic params, opaque, anything dependent   no concrete clang type.
///       a Kairo-native record    phase A's AST has never heard of it. Needs
///                                sema to emit a C++ forward shape for the
///                                Kairo type into that AST first.
///                                TODO(ffi): `std::vector<KairoStruct>`.
///       a decl from ANOTHER set  its clang_decl points into a different
///                                ASTContext. v1 instantiates only through
///                                the set the primary came from, so a decl
///                                of any other set is refused, not re-found.
///       nullables, Kairo vectors/maps/slices   no C++ counterpart.
///
///   \par Threading
///     A nested instance (`cont<cont<i32>>`) is spelled by specializing the
///     inner one, which is a clang::Sema call: callers hold the set's
///     sema_lock() across to_clang, not just across the outer instantiation.
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
ffi "c++" import "clang/AST/TemplateBase.h";
ffi "c++" import "clang/AST/TemplateName.h";
ffi "c++" import "clang/AST/Type.h";
ffi "c++" import "clang/Sema/Sema.h";
ffi "c++" import "llvm/ADT/APInt.h";
ffi "c++" import "llvm/ADT/SmallVector.h";

import AST::Node::Base::*;
import AST::Node::Declarations::*;
import AST::Node::Types::*;
import AST::Node::CanonicalNodes::*;
import AST::Node::Casting::*;

class TypeExport {
    pub var cctx: obs::<clang::ASTContext>;
    pub var sema: obs::<clang::Sema>;        ///< for nested specializations.
    pub var set:  obs::<void>;               ///< the HeaderSet cctx belongs to (FFILinkage::origin_set's type).

    fn TypeExport(self, cctx: *clang::ASTContext, sema: *clang::Sema, set: *void) {
        self.cctx = cctx;
        self.sema = sema;
        self.set  = set;
    }

    /// The clang type for canonical \p t, or a null QualType.
    fn to_clang(self, t: *Type) -> clang::QualType {
        if t == &null { return clang::QualType(); }
        if t->is_dependent() || t->is_instantiation_dependent() { return clang::QualType(); }

        if isa::<BuiltinType>(t) { return self._builtin(cast::<BuiltinType>(t)); }

        if isa::<PointerType>(t) {
            var p = self.to_clang(cast::<PointerType>(t)->pointee);
            if p.isNull() { return p; }
            return self.cctx->getPointerType(p);
        }
        if isa::<CXXLValueRefType>(t) {
            var lr = cast::<CXXLValueRefType>(t);
            var r  = self.to_clang(lr->referee);
            if r.isNull() { return r; }
            if lr->is_const { r = r.withConst(); }
            return self.cctx->getLValueReferenceType(r);
        }
        if isa::<CXXRValueRefType>(t) {
            var r = self.to_clang(cast::<CXXRValueRefType>(t)->referee);
            if r.isNull() { return r; }
            return self.cctx->getRValueReferenceType(r);
        }

        if isa::<FixedArrayType>(t) {
            var at = cast::<FixedArrayType>(t);
            var el = self.to_clang(at->element);
            if el.isNull() { return el; }
            // The store keeps a canonical array's extent in size_bits (it has
            // no size_expr); 0 is how TypeImport spells `T[]`.
            var extent = static_cast::<u64>(at->size_bits);
            if at->is_incomplete || (extent == 0) {
                return self.cctx->getIncompleteArrayType(el, clang::ArraySizeModifier::Normal, 0);
            }
            return self.cctx->getConstantArrayType(el, llvm::APInt(64, extent), &null,
                                                   clang::ArraySizeModifier::Normal, 0);
        }

        // Kairo `fn(...)` IS the pointer (TypeImport::_map folds `R(*)(A)`
        // into it), so the way back out wraps the function type in one.
        if isa::<FunctionPointerType>(t) {
            var ft  = cast::<FunctionPointerType>(t);
            var ret = self.cctx->VoidTy;
            var rq  = clang::QualType(ret);
            if ft->return_type != &null {
                rq = self.to_clang(ft->return_type);
                if rq.isNull() { return rq; }
            }
            if ft->is_unprototyped {
                return self.cctx->getPointerType(self.cctx->getFunctionNoProtoType(rq));
            }
            var ps: llvm::SmallVector::<clang::QualType, 8>;
            for var i: usize = 0; i < ft->params.size(); ++i {
                var q = self.to_clang(*(ft->params.get(i)));
                if q.isNull() { return q; }
                ps.push_back(q);
            }
            var fn_ = self.cctx->getFunctionType(rq, ps, clang::FunctionProtoType::ExtProtoInfo());
            return self.cctx->getPointerType(fn_);
        }

        if isa::<RecordType>(t) { return self._record(cast::<RecordType>(t)); }

        // GenericParamType and OpaqueFFIType were caught as dependent / have
        // no shape; nullable, vector, map, slice, tuple have no C++ twin.
        return clang::QualType();
    }

    /// `ct<args...>` as a canonical clang type, or null. Declares the
    /// specialization (CheckTemplateIdType) but does NOT complete it: a
    /// nested argument only has to be nameable. PRE: sema_lock() held.
    fn template_id(self, ct: *clang::ClassTemplateDecl, args: ref!(llvm::SmallVector::<clang::QualType, 8>)) -> clang::QualType {
        if (ct == &null) || (self.sema == &null) { return clang::QualType(); }
        var loc = ct->getLocation();                     // any valid loc; the pattern's is honest.
        var ali = clang::TemplateArgumentListInfo(loc, loc);
        for var i: usize = 0; i < args.size(); ++i {
            var qt = args[i];
            ali.addArgument(clang::TemplateArgumentLoc(clang::TemplateArgument(qt),
                                                       self.cctx->getTrivialTypeSourceInfo(qt, loc)));
        }
        var diags  = &(self.sema->getDiagnostics());
        var before = diags->getNumErrors();
        // 22.x signature (VERIFIED against /usr/include/clang/Sema/Sema.h,
        // 22.1.8): the elaborated-type refactor put the keyword FIRST and
        // added (Scope*, ForNestedNameSpecifier) at the end.
        var tn = clang::TemplateName(static_cast::<*clang::TemplateDecl>(ct));
        var ty = self.sema->CheckTemplateIdType(clang::ElaboratedTypeKeyword::None, tn, loc, ali, &null, false);
        if ty.isNull() || (diags->getNumErrors() > before) { return clang::QualType(); }
        return ty.getCanonicalType();
    }

    /// The ClassTemplateDecl behind a Kairo primary, or null. DeclImport
    /// stamps the decl it WALKED, which for a primary is the ClassTemplateDecl
    /// -- unless the first sighting was the templated record itself.
    static fn class_template_of(cd: *clang::Decl) -> *clang::ClassTemplateDecl {
        if cd == &null { return &null; }
        var ct = llvm::dyn_cast::<clang::ClassTemplateDecl>(cd);
        if ct != &null { return ct; }
        // A partial specialization M2-style repointing never happens for a
        // clang instance, but a pattern's stamp can still be one.
        var ps = llvm::dyn_cast::<clang::ClassTemplatePartialSpecializationDecl>(cd);
        if ps != &null { return ps->getSpecializedTemplate(); }
        var rd = llvm::dyn_cast::<clang::CXXRecordDecl>(cd);
        if rd != &null { return rd->getDescribedClassTemplate(); }
        return &null;
    }

    // records

    priv fn _record(self, rt: *RecordType) -> clang::QualType {
        var d = rt->decl;
        if d == &null { return clang::QualType(); }

        // Anything with a live clang decl in THIS set: a plain imported
        // record or enum, an explicit specialization, or an instance a fill
        // already stamped with its ClassTemplateSpecializationDecl.
        var cd = TypeExport::clang_decl_of(d);
        if (cd != &null) && (TypeExport::origin_set_of(d) == self.set) {
            var tag = llvm::dyn_cast::<clang::TagDecl>(reinterpret_cast::<*clang::Decl>(cd));
            if tag != &null { return clang::QualType(self.cctx->getCanonicalTagType(tag)); }
        }

        // An unfilled instance of an imported template: spell the template-id
        // from its args. Recursing here is what makes `cont<cont<i32>>` work.
        var prim = TypeExport::pattern_of(d);
        if (prim == &null) || (!TypeExport::is_instance(d)) { return clang::QualType(); }
        if TypeExport::origin_set_of(prim) != self.set { return clang::QualType(); }
        var ct = TypeExport::class_template_of(reinterpret_cast::<*clang::Decl>(TypeExport::clang_decl_of(prim)));
        if ct == &null { return clang::QualType(); }
        var qs: llvm::SmallVector::<clang::QualType, 8>;
        for var i: usize = 0; i < rt->args.size(); ++i {
            var q = self.to_clang(*(rt->args.get(i)));
            if q.isNull() { return q; }
            qs.push_back(q);
        }
        return self.template_id(ct, qs);
    }

    // builtins

    /// Mirrors TypeImport::_builtin, so a round trip is the identity on every
    /// canonical import can produce. Integers go by (width, sign) through the
    /// target, exactly as they came in.
    priv fn _builtin(self, bt: *BuiltinType) -> clang::QualType {
        var k = bt->builtin;
        if builtin_is_integer(k) {
            var bits = builtin_fixed_bits(k);
            if bits == 0 { bits = bt->size_bits; }       // usize/isize: the store's target width.
            var sgn: u32 = (1) if (builtin_is_signed(k)) else (0);
            return self.cctx->getIntTypeForBitwidth(static_cast::<u32>(bits), sgn);   // null past 128.
        }
        switch k {
            case BuiltinKind::Void { return clang::QualType(self.cctx->VoidTy); }
            case BuiltinKind::Bool { return clang::QualType(self.cctx->BoolTy); }
            case BuiltinKind::Char { return clang::QualType(self.cctx->Char32Ty); }
            case BuiltinKind::F16  { return clang::QualType(self.cctx->Float16Ty); }
            case BuiltinKind::BF16 { return clang::QualType(self.cctx->BFloat16Ty); }
            case BuiltinKind::F32  { return clang::QualType(self.cctx->FloatTy); }
            case BuiltinKind::F64  { return clang::QualType(self.cctx->DoubleTy); }
            // The format through a `var` first, as TypeImport does: Stage 0
            // drops a leading `&(...)` inside a comparison and compares the
            // fltSemantics by value.
            case BuiltinKind::F80 {
                var ld  = clang::QualType(self.cctx->LongDoubleTy);
                var fmt = &(self.cctx->getFloatTypeSemantics(ld));
                if fmt == &(llvm::APFloat::x87DoubleExtended()) { return ld; }
                return clang::QualType();
            }
            case BuiltinKind::F128 {
                // _builtin maps a binary128 `long double` AND `__float128` to
                // f128; `long double` is the one headers actually spell.
                var ld  = clang::QualType(self.cctx->LongDoubleTy);
                var fmt = &(self.cctx->getFloatTypeSemantics(ld));
                if fmt == &(llvm::APFloat::IEEEquad()) { return ld; }
                return clang::QualType(self.cctx->Float128Ty);
            }
            default { return clang::QualType(); }       // never, byte, f8*, f256+: no import produces them.
        }
    }

    // FFILinkage, per kind
    //
    // A CRTP mixin has no common base to read through; these are the kinds
    // that derive it AND can back a RecordType.

    static fn clang_decl_of(d: *Decl) -> *void {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->clang_decl;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->clang_decl; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->clang_decl;  }
        if isa::<EnumDecl>(d)   { return cast::<EnumDecl>(d)->clang_decl;   }   // not stamped today: null.
        return &null;
    }

    static fn origin_set_of(d: *Decl) -> *void {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->origin_set;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->origin_set; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->origin_set;  }
        if isa::<EnumDecl>(d)   { return cast::<EnumDecl>(d)->origin_set;   }
        return &null;
    }

    static fn origin_fid_of(d: *Decl) -> u32 {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->origin_fid;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->origin_fid; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->origin_fid;  }
        return 0;
    }

    static fn pattern_of(d: *Decl) -> *Decl {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->instantiated_from;  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->instantiated_from; }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->instantiated_from;  }
        return &null;
    }

    static fn is_instance(d: *Decl) -> bool {
        if isa::<ClassDecl>(d)  { return cast::<ClassDecl>(d)->is_instance();  }
        if isa::<StructDecl>(d) { return cast::<StructDecl>(d)->is_instance(); }
        if isa::<UnionDecl>(d)  { return cast::<UnionDecl>(d)->is_instance();  }
        return false;
    }
}
