#ifndef PDFG_PDFGDRIVER_HPP
#define PDFG_PDFGDRIVER_HPP

#include "clang/AST/ASTContext.h"

namespace pdfg_c {

//! Globally-accessible pointer to the ASTContext, initialized before the
//! tool runs.
extern clang::ASTContext* Context;

}

#endif
