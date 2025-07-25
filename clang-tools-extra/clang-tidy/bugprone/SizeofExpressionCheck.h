//===--- SizeofExpressionCheck.h - clang-tidy--------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BUGPRONE_SIZEOFEXPRESSIONCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BUGPRONE_SIZEOFEXPRESSIONCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::bugprone {

/// Find suspicious usages of sizeof expressions.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/bugprone/sizeof-expression.html
class SizeofExpressionCheck : public ClangTidyCheck {
public:
  SizeofExpressionCheck(StringRef Name, ClangTidyContext *Context);
  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  const bool WarnOnSizeOfConstant;
  const bool WarnOnSizeOfIntegerExpression;
  const bool WarnOnSizeOfThis;
  const bool WarnOnSizeOfCompareToConstant;
  const bool WarnOnSizeOfPointerToAggregate;
  const bool WarnOnSizeOfPointer;
  const bool WarnOnOffsetDividedBySizeOf;
  const bool WarnOnSizeOfInLoopTermination;
};

} // namespace clang::tidy::bugprone

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_BUGPRONE_SIZEOFEXPRESSIONCHECK_H
