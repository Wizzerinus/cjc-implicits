// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

#include "Desugar/AfterTypeCheck.h"

#include "TypeCheckUtil.h"

#include "cangjie/AST/Create.h"
#include "cangjie/AST/Utils.h"

using namespace Cangjie;
using namespace TypeCheckUtil;

namespace Cangjie::Sema::Desugar::AfterTypeCheck {
/**
 * Desugar ImplicitWithExpr to block.
 * *************** before desugar ****************
 * with (a, 123) { code }
 * *************** after desugar ****************
 * {
 *  let new_var = 123
 *  code
 * }
 * */
void DesugarImplicitWithExpr(ImplicitWithExpr& iwe)
{
    if (iwe.desugarExpr) {
        return;
    }
    CJC_NULLPTR_CHECK(iwe.body);
    std::vector<OwnedPtr<Node>> nodes;
    for (auto& decl : iwe.synthesizedDecls) {
        nodes.emplace_back(std::move(decl));
    }
    iwe.synthesizedDecls.clear();
    auto ty = iwe.body->ty;
    nodes.emplace_back(std::move(iwe.body));
    iwe.desugarExpr = CreateBlock(std::move(nodes), ty);
    AddCurFile(*iwe.desugarExpr, iwe.curFile);
}
} // namespace Cangjie::Sema::Desugar::AfterTypeCheck
