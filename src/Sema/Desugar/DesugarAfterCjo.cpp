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
void TypeChecker::PerformDesugarAfterCjo(Package& pkg) const
{
    impl->PerformDesugarAfterCjo(pkg);
}

void TypeChecker::TypeCheckerImpl::PerformDesugarAfterCjo(Package& pkg)
{
    if (pkg.files.empty()) {
        return;
    }
    ImplicitCalls imps;
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

VisitAction ImplicitCalls::RewriteCallExpr(CallExpr& ce)
{
    if (ce.desugarArgs.has_value()) {
        std::vector<Ptr<FuncArg>> newDesugarArgs;
        for (auto& arg : ce.implicitlyAssignedArgs) {
            CJC_NULLPTR_CHECK(arg);
            newDesugarArgs.emplace_back(arg.get());
        }
        for (auto& arg : ce.desugarArgs.value()) {
            newDesugarArgs.emplace_back(arg.get());
        }
        ce.desugarArgs = std::move(newDesugarArgs);
    }

    std::vector<OwnedPtr<FuncArg>> newArgs;
    for (auto& arg : ce.implicitlyAssignedArgs) {
        CJC_NULLPTR_CHECK(arg);
        newArgs.emplace_back(std::move(arg));
    }
    for (auto& arg : ce.args) {
        newArgs.emplace_back(std::move(arg));
    }
    ce.args = std::move(newArgs);

    ce.implicitlyAssignedArgs.clear();
    return RewriteAnyNode(ce);
}

VisitAction ImplicitCalls::RewriteFuncBody(FuncBody& fb)
{
    if (fb.implicitParamList.has_value()) {
        CJC_ASSERT(fb.paramLists.size() == 1);
        std::vector<OwnedPtr<FuncParam>> newParams;
        for (auto& arg : fb.implicitParamList.value()->params) {
            newParams.emplace_back(std::move(arg));
        }
        for (auto& arg : fb.paramLists[0]->params) {
            newParams.emplace_back(std::move(arg));
        }
        fb.paramLists[0]->params = std::move(newParams);
        fb.implicitParamList = {};
    }
    return RewriteAnyNode(fb);
}

void ImplicitCalls::RewriteType(Ptr<Ty> ty) {
    if (ty == nullptr) {
        return;
    }
    if (auto funcTy = DynamicCast<FuncTy>(ty)) {
        if (!funcTy->implicitParamTys.empty()) {
            std::vector<Ptr<Ty>> typeArgs(funcTy->typeArgs);
            typeArgs.pop_back();  // remove the return value
            funcTy->paramTys = typeArgs;
            funcTy->implicitParamTys.clear();
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
            [this](CallExpr& ce) { return RewriteCallExpr(ce); },
            [this](FuncBody& fb) { return RewriteFuncBody(fb); },
            [this](Node& n) { return RewriteAnyNode(n); },
            []() { return VisitAction::WALK_CHILDREN; });
    };
    Walker walker(&pkg, preVisit);
    walker.Walk();
}
