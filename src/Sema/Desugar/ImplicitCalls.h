// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

/**
 * @file
 *
 * This file declares class used to resolve implicit calls and prepare their CHIR conversion.
 */
#ifndef CANGJIE_SEMA_IMPLICIT_CALLS_H
#define CANGJIE_SEMA_IMPLICIT_CALLS_H

#include "cangjie/AST/Node.h"
#include "cangjie/AST/Walker.h"
#include "cangjie/Sema/TypeManager.h"

namespace Cangjie {
class ImplicitCalls {
public:
    explicit ImplicitCalls(TypeManager& typeManager) : typeManager(typeManager)
    {
    }
    ~ImplicitCalls() = default;

    void DesugarImplicitCalls(AST::Package& pkg);

private:
    AST::VisitAction RewriteFunctionType(AST::FuncType& ft);
    AST::VisitAction RewriteAnyType(AST::Type& t);
    AST::VisitAction RewriteAnyNode(AST::Node& n);
    AST::VisitAction RewriteNameReferenceExpr(AST::NameReferenceExpr& nre);
    AST::VisitAction RewriteMemberAccess(AST::MemberAccess& ma);
    void RewriteType(Ptr<AST::Ty>& ty);

    TypeManager& typeManager;
    std::unordered_set<AST::Ty*> rewrittenTypes;
};
} // namespace Cangjie

#endif
