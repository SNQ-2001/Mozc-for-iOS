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

#ifndef MOZC_CHROME_NACL_URL_LOADER_UTIL_H_
#define MOZC_CHROME_NACL_URL_LOADER_UTIL_H_

#include <ppapi/cpp/completion_callback.h>

#include <string>

#include "base/port.h"

namespace pp {
class Instance;
}  // namespace pp

namespace mozc {
namespace chrome {
namespace nacl {

// Utility class to handle pp::URLLoader.
class URLLoaderUtil {
 public:
  // Downloads the file from url to file_name on HTML5 filesystem.
  static void StartDownloadToFile(pp::Instance *instance,
                                  const string &url,
                                  const string &file_name,
                                  pp::CompletionCallback callback);
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(URLLoaderUtil);
};

}  // namespace nacl
}  // namespace chrome
}  // namespace mozc

#endif  // MOZC_CHROME_NACL_URL_LOADER_UTIL_H_
