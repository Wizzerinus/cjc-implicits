// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

/**
 * @file
 *
 * This file implements the Desugar functions used after cjo exporting step.
 */

#include "DesugarInTypeCheck.h"

#include <vector>

#include "ImplicitCalls.h"
#include "TypeCheckUtil.h"
#include "TypeCheckerImpl.h"

#include "cangjie/AST/Node.h"
#include "cangjie/Basic/Match.h"

using namespace Cangjie;
using namespace AST;
using namespace TypeCheckUtil;
using namespace Meta;

// Perform desugar after cjo exporting.
void TypeChecker::PerformDesugarAfterCjo(ASTContext& ctx, Package& pkg) const
{
    impl->PerformDesugarAfterCjo(ctx, pkg);
}

void TypeChecker::TypeCheckerImpl::PerformDesugarAfterCjo([[maybe_unused]] ASTContext& ctx, Package& pkg)
{
    if (pkg.files.empty()) {
        return;
    }
    ImplicitCalls imps(typeManager);
    imps.DesugarImplicitCalls(pkg);
}

VisitAction ImplicitCalls::RewriteFunctionType(FuncType& ft)
{
    if (ft.usingType.has_value()) {
        std::vector<OwnedPtr<Type>> newTypes;
        auto& ut = ft.usingType.value();
        for (auto& o : ut->paramTypes) {
            newTypes.emplace_back(std::move(o));
        }
        for (auto& o : ft.paramTypes) {
            newTypes.emplace_back(std::move(o));
        }
        ft.usingType = {};
        ft.paramTypes = std::move(newTypes);
    }
    return RewriteAnyType(ft);
}

VisitAction ImplicitCalls::RewriteAnyType(Type& t)
{
    RewriteType(t.aliasTy);
    return RewriteAnyNode(t);
}

VisitAction ImplicitCalls::RewriteAnyNode(Node& n)
{
    RewriteType(n.ty);
    return VisitAction::WALK_CHILDREN;
}

VisitAction ImplicitCalls::RewriteNameReferenceExpr(NameReferenceExpr& nre)
{
    for (auto& it : nre.instTys) {
        RewriteType(it);
    }
    RewriteType(nre.matchedParentTy);
    return RewriteAnyNode(nre);
}

VisitAction ImplicitCalls::RewriteMemberAccess(MemberAccess& ma)
{
    // This is not necessary as it is only used in Generic Instantiation and below
    /*
    for (auto& it : ma.foundUpperBoundMap) {
        for (auto& ptr : it.second) {
            RewriteType(ptr);
        }
    }
    */
    return RewriteNameReferenceExpr(ma);
}

void ImplicitCalls::RewriteType(Ptr<Ty>& ty) {
    if (ty == nullptr || rewrittenTypes.find(ty) != rewrittenTypes.end()) {
        return;
    }
    rewrittenTypes.insert(ty);
    if (auto funcTy = DynamicCast<FuncTy>(ty)) {
        if (!funcTy->implicitParamTys.empty()) {
            ty = typeManager.GetFunctionTy(funcTy->typeArgs, {}, funcTy->retTy, {funcTy->isC, funcTy->isClosureTy, funcTy->hasVariableLenArg, funcTy->noCast});
        }
    }
    for (auto& it : ty->typeArgs) {
        RewriteType(it);
    }
}

/**
 * Option Box happens before type check finished with no errors.
 * All nodes and sema types should be valid.
 */
void ImplicitCalls::DesugarImplicitCalls(Package& pkg)
{
    std::function<VisitAction(Ptr<Node>)> preVisit = [this](Ptr<Node> node) -> VisitAction {
        // Matcher can only call one branch of the functor. So the more specific calls must be on top
        // and must call the less specific branches as well.
        return match(*node)(
            [this](FuncType& ft) { return RewriteFunctionType(ft); },
            [this](Type& t) { return RewriteAnyType(t); },
            [this](MemberAccess& ma) { return RewriteMemberAccess(ma); },
            [this](NameReferenceExpr& nre) { return RewriteNameReferenceExpr(nre); },
            [this](Node& n) { return RewriteAnyNode(n); },
            []() { return VisitAction::WALK_CHILDREN; });
    };
    Walker walker(&pkg, preVisit);
    walker.Walk();
}
