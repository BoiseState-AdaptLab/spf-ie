#ifndef SPFIE_DRIVER_HPP
#define SPFIE_DRIVER_HPP

#include "clang/AST/ASTContext.h"

namespace spf_ie {

//! Globally-accessible pointer to the ASTContext, initialized before the
//! tool runs.
extern const clang::ASTContext* Context;

}  // namespace spf_ie

#endif
