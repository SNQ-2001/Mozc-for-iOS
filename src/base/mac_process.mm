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

#import "base/mac_process.h"

#import <Foundation/Foundation.h>

#include "base/const.h"
#include "base/logging.h"
#include "base/mac_util.h"
#include "base/util.h"

namespace mozc {
namespace {
const char kFileSchema[] = "file://";
}  // namespace

bool MacProcess::OpenBrowserForMac(const string &url) {
  return false;
}

bool MacProcess::OpenApplication(const string &path) {
  return false;
}

namespace {
bool LaunchMozcToolInternal(const string &tool_name, const string &error_type) {
  return false;
}
}  // namespace

bool MacProcess::LaunchMozcTool(const string &tool_name) {
  return LaunchMozcToolInternal(tool_name, "");
}

bool MacProcess::LaunchErrorMessageDialog(const string &error_type) {
  return LaunchMozcToolInternal("error_message_dialog", error_type);
}

}  // namespace mozc
