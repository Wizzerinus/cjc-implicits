// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

#include "TypeCheckerImpl.h"

#include "Diags.h"
#include "TypeCheckUtil.h"
#include "cangjie/AST/Create.h"

using namespace Cangjie;
using namespace Sema;
using namespace TypeCheckUtil;

void TypeChecker::TypeCheckerImpl::EnsureImplicitDeclarations(ASTContext& ctx, ImplicitWithExpr& iwe) {
    if (iwe.synthesizedDecls.empty()) {
        std::vector<OwnedPtr<Decl>> decls;
        for (size_t i = 0; i < iwe.children.size(); i++) {
            auto& child = iwe.children[i];
            auto ty = Synthesize(CheckerContext{ctx, SynPos::EXPR_ARG}, child);
            typeManager.ReplaceIdealTy(ty);
            child->SetTy(ty);
            auto withType = MakeOwned<Type>();
            withType->SetTy(ty);
            auto withDecl = CreateVarDecl("with$" + std::to_string(i), std::move(child), withType);
            withDecl->SetTy(ty);
            decls.push_back(std::move(withDecl));
        }
        iwe.children.clear();
        iwe.synthesizedDecls = std::move(decls);
    }

    std::vector<ImplicitValue> impTys;
    for (auto& decl : iwe.synthesizedDecls) {
        impTys.push_back(ImplicitValue{decl->GetTy(), decl.get()});
    }
    scopeManager.EnterImplicitScope(ctx, ImplicitScope{std::move(impTys)});
}

bool TypeChecker::TypeCheckerImpl::ChkImplicitWithExpr(ASTContext& ctx, Ty& target, ImplicitWithExpr& iwe)
{
    scopeManager.InitializeScope(ctx);
    EnsureImplicitDeclarations(ctx, iwe);
    auto result = Check(ctx, &target, iwe.body.get());
    scopeManager.ExitImplicitScope(ctx);
    scopeManager.FinalizeScope(ctx);
    return result;
}

Ptr<Ty> TypeChecker::TypeCheckerImpl::SynImplicitWithExpr(ASTContext& ctx, ImplicitWithExpr& iwe)
{
    scopeManager.InitializeScope(ctx);
    EnsureImplicitDeclarations(ctx, iwe);
    iwe.SetTy(Synthesize(CheckerContext{ctx, SynPos::IMPLICIT_RETURN}, iwe.body.get()));
    iwe.body->SetTy(iwe.GetTy());
    scopeManager.ExitImplicitScope(ctx);
    scopeManager.FinalizeScope(ctx);
    return iwe.GetTy();
}
