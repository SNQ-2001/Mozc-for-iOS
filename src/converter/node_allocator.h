// Copyright 2010-2014, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MOZC_CONVERTER_NODE_ALLOCATOR_H_
#define MOZC_CONVERTER_NODE_ALLOCATOR_H_

#include "base/port.h"
#include "base/freelist.h"
#include "base/logging.h"
#include "converter/node.h"

namespace mozc {

class NodeAllocator : public NodeAllocatorInterface {
 public:
  NodeAllocator() : node_freelist_(1024), node_count_(0) {}
  virtual ~NodeAllocator() {}

  virtual Node *NewNode() {
    Node *node = node_freelist_.Alloc();
    DCHECK(node);
    node->Init();
    ++node_count_;
    return node;
  }

  // Free all nodes allocateed by NewNode()
  void Free() {
    node_freelist_.Free();
    node_count_ = 0;
  }

  size_t node_count() const {
    return node_count_;
  }

 private:
  FreeList<Node> node_freelist_;
  size_t node_count_;

  DISALLOW_COPY_AND_ASSIGN(NodeAllocator);
};

}  // namespace mozc

#endif  // MOZC_CONVERTER_NODE_ALLOCATOR_H_
