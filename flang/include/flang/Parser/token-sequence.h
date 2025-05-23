//===-- lib/Parser/token-sequence.h -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef FORTRAN_PARSER_TOKEN_SEQUENCE_H_
#define FORTRAN_PARSER_TOKEN_SEQUENCE_H_

// A buffer class capable of holding a contiguous sequence of characters
// and a partitioning thereof into preprocessing tokens, along with their
// associated provenances.

#include "flang/Parser/char-block.h"
#include "flang/Parser/provenance.h"
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace llvm {
class raw_ostream;
}

namespace Fortran::parser {

class Messages;
class Prescanner;

// Buffers a contiguous sequence of characters that has been partitioned into
// a sequence of preprocessing tokens with provenances.
class TokenSequence {
public:
  TokenSequence() {}
  TokenSequence(const TokenSequence &that) { CopyAll(that); }
  TokenSequence(
      const TokenSequence &that, std::size_t at, std::size_t count = 1) {
    AppendRange(that, at, count);
  }
  TokenSequence(TokenSequence &&that)
      : start_{std::move(that.start_)}, nextStart_{that.nextStart_},
        char_{std::move(that.char_)},
        provenances_{std::move(that.provenances_)} {}
  TokenSequence(const std::string &s, Provenance p) { Put(s, p); }

  TokenSequence &operator=(const TokenSequence &that) {
    clear();
    CopyAll(that);
    return *this;
  }
  TokenSequence &operator=(TokenSequence &&that);
  bool empty() const { return start_.empty(); }
  void clear();
  void pop_back();
  void shrink_to_fit();
  void swap(TokenSequence &);

  std::size_t SizeInTokens() const { return start_.size(); }
  std::size_t SizeInChars() const { return char_.size(); }

  CharBlock ToCharBlock() const {
    return char_.empty() ? CharBlock{} : CharBlock{&char_[0], char_.size()};
  }
  std::string ToString() const { return ToCharBlock().ToString(); }

  CharBlock TokenAt(std::size_t token) const {
    if (auto bytes{TokenBytes(token)}) {
      return {&char_[start_.at(token)], bytes};
    } else { // char_ could be empty
      return {};
    }
  }
  char CharAt(std::size_t j) const { return char_.at(j); }
  CharBlock CurrentOpenToken() const {
    return {&char_[nextStart_], char_.size() - nextStart_};
  }

  std::size_t SkipBlanks(std::size_t) const;
  std::optional<std::size_t> SkipBlanksBackwards(std::size_t) const;

  // True if anything remains in the sequence at & after the given offset
  // except blanks and line-ending C++ and Fortran free-form comments.
  bool IsAnythingLeft(std::size_t) const;

  void PutNextTokenChar(char ch, Provenance provenance) {
    char_.emplace_back(ch);
    provenances_.Put({provenance, 1});
  }

  void CloseToken() {
    start_.emplace_back(nextStart_);
    nextStart_ = char_.size();
  }

  void ReopenLastToken() {
    nextStart_ = start_.back();
    start_.pop_back();
  }

  // These append characters with provenance and then close the token.
  // When the last token of this sequence remains open beforehand,
  // the new characters are appended to it.
  void Put(const char *, std::size_t, Provenance);
  void Put(const CharBlock &, Provenance);
  void Put(const std::string &, Provenance);
  void Put(llvm::raw_string_ostream &, Provenance);

  // Appends a full copy of another sequence.  When the last token of this
  // sequence remains open beforehand, it is closed before the new text
  // is appended.
  void CopyAll(const TokenSequence &);
  // Copies a range of tokens from another sequence.  If the last token of
  // this sequence remains open, the first token of the copied range will be
  // appended to it.
  void AppendRange(
      const TokenSequence &, std::size_t at, std::size_t tokens = 1);
  // Copies tokens (via Put above) with new provenance.
  void CopyWithProvenance(const TokenSequence &, ProvenanceRange);

  Provenance GetCharProvenance(std::size_t) const;
  Provenance GetTokenProvenance(
      std::size_t token, std::size_t offset = 0) const;
  ProvenanceRange GetTokenProvenanceRange(
      std::size_t token, std::size_t offset = 0) const;
  ProvenanceRange GetIntervalProvenanceRange(
      std::size_t token, std::size_t tokens = 1) const;
  ProvenanceRange GetProvenanceRange() const;

  char *GetMutableCharData() { return &char_[0]; }
  TokenSequence &ToLowerCase();
  bool HasBlanks(std::size_t firstChar = 0) const;
  bool HasRedundantBlanks(std::size_t firstChar = 0) const;
  TokenSequence &RemoveBlanks(std::size_t firstChar = 0);
  TokenSequence &RemoveRedundantBlanks(std::size_t firstChar = 0);
  TokenSequence &ClipComment(const Prescanner &, bool skipFirst = false);
  const TokenSequence &CheckBadFortranCharacters(
      Messages &, const Prescanner &, bool allowAmpersand) const;
  bool BadlyNestedParentheses() const;
  const TokenSequence &CheckBadParentheses(Messages &) const;
  void Emit(CookedSource &) const;
  llvm::raw_ostream &Dump(llvm::raw_ostream &) const;

private:
  std::size_t TokenBytes(std::size_t token) const {
    return (token + 1 >= start_.size() ? char_.size() : start_[token + 1]) -
        start_[token];
  }

  std::vector<std::size_t> start_;
  std::size_t nextStart_{0};
  std::vector<char> char_;
  OffsetToProvenanceMappings provenances_;
};
} // namespace Fortran::parser
#endif // FORTRAN_PARSER_TOKEN_SEQUENCE_H_
