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

// Handler of mozc configuration.

#include "config/config_handler.h"

#include <algorithm>

#include "base/config_file_stream.h"
#include "base/logging.h"
#include "base/number_util.h"
#include "base/port.h"
#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "base/system_util.h"
#include "base/util.h"
#include "base/version.h"
#include "config/config.pb.h"

namespace mozc {
namespace config {
namespace {

const char kFileNamePrefix[] = "user://config";

void AddCharacterFormRule(const char *group,
                          const Config::CharacterForm preedit_form,
                          const Config::CharacterForm conversion_form,
                          Config *config) {
  Config::CharacterFormRule *rule =
      config->add_character_form_rules();
  rule->set_group(group);
  rule->set_preedit_character_form(preedit_form);
  rule->set_conversion_character_form(conversion_form);
}

bool GetPlatformSpecificDefaultEmojiSetting() {
  // Disable Unicode emoji conversion by default on specific platforms.
  bool use_emoji_conversion_default = true;
#if defined(OS_WIN)
  if (!SystemUtil::IsWindows8OrLater()) {
    use_emoji_conversion_default = false;
  }
#elif defined(OS_ANDROID)
  use_emoji_conversion_default = false;
#endif
  return use_emoji_conversion_default;
}

class ConfigHandlerImpl {
 public:
  ConfigHandlerImpl() {
    // <user_profile>/config1.db
    filename_ = kFileNamePrefix;
    filename_ += NumberUtil::SimpleItoa(CONFIG_VERSION);
    filename_ += ".db";
    Reload();
  }
  virtual ~ConfigHandlerImpl() {}
  const Config &GetConfig() const;
  bool GetConfig(Config *config) const;
  const Config &GetStoredConfig() const;
  bool GetStoredConfig(Config *config) const;
  bool SetConfig(const Config &config);
  void SetImposedConfig(const Config &config);
  bool Reload();
  void SetConfigFileName(const string &filename);
  string GetConfigFileName();

 private:
  // copy config to config_ and do some
  // platform dependent hooks/rewrites
  bool SetConfigInternal(const Config &config);
  void UpdateMergedConfig();

  string filename_;
  Config stored_config_;
  Config imposed_config_;
  // equals to config_.MergeFrom(imposed_config_)
  Config merged_config_;
};

ConfigHandlerImpl *GetConfigHandlerImpl() {
  return Singleton<ConfigHandlerImpl>::get();
}

const Config &ConfigHandlerImpl::GetConfig() const {
  return merged_config_;
}

// return current Config
bool ConfigHandlerImpl::GetConfig(Config *config) const {
  config->CopyFrom(merged_config_);
  return true;
}

const Config &ConfigHandlerImpl::GetStoredConfig() const {
  return stored_config_;
}

// return stored Config
bool ConfigHandlerImpl::GetStoredConfig(Config *config) const {
  config->CopyFrom(stored_config_);
  return true;
}

// set config and rewirte internal data
bool ConfigHandlerImpl::SetConfigInternal(const Config &config) {
  stored_config_.CopyFrom(config);

#ifdef NO_LOGGING
  // Delete the optional field from the config.
  stored_config_.clear_verbose_level();
  // Fall back if the default value is not the expected value.
  if (stored_config_.verbose_level() != 0) {
    stored_config_.set_verbose_level(0);
  }
#endif

  Logging::SetConfigVerboseLevel(stored_config_.verbose_level());

  // Initialize platform specific configuration.
  if (stored_config_.session_keymap() == Config::NONE) {
    stored_config_.set_session_keymap(ConfigHandler::GetDefaultKeyMap());
  }

#if defined(OS_ANDROID) && defined(CHANNEL_DEV)
  stored_config_.mutable_general_config()->set_upload_usage_stats(true);
#endif  // CHANNEL_DEV && OS_ANDROID

  if (GetPlatformSpecificDefaultEmojiSetting() &&
      !stored_config_.has_use_emoji_conversion()) {
    stored_config_.set_use_emoji_conversion(true);
  }

  UpdateMergedConfig();

  return true;
}

void ConfigHandlerImpl::UpdateMergedConfig() {
  merged_config_.CopyFrom(stored_config_);
  merged_config_.MergeFrom(imposed_config_);
}

bool ConfigHandlerImpl::SetConfig(const Config &config) {
  Config output_config;
  output_config.CopyFrom(config);

  ConfigHandler::SetMetaData(&output_config);

  VLOG(1) << "Setting new config: " << filename_;
  ConfigFileStream::AtomicUpdate(filename_, output_config.SerializeAsString());

#ifdef DEBUG
  string debug_content(
      "# This is a text-based config file for debugging.\n"
      "# Nothing happens when you edit this file manually.\n");
  debug_content += output_config.DebugString();
  ConfigFileStream::AtomicUpdate(filename_ + ".txt", debug_content);
#endif  // DEBUG

  return SetConfigInternal(output_config);
}

void ConfigHandlerImpl::SetImposedConfig(const Config &config) {
  VLOG(1) << "Setting new overriding config";
  imposed_config_.CopyFrom(config);

#ifdef DEBUG
  string debug_content(
      "# This is a text-based config file for debugging.\n"
      "# Nothing happens when you edit this file manually.\n");
  debug_content += config.DebugString();
  ConfigFileStream::AtomicUpdate(filename_ + ".overriding.txt", debug_content);
#endif  // DEBUG
  UpdateMergedConfig();
}

// Reload from file
bool ConfigHandlerImpl::Reload() {
  VLOG(1) << "Reloading config file: " << filename_;
  scoped_ptr<istream> is(ConfigFileStream::OpenReadBinary(filename_));
  Config input_proto;
  bool ret_code = true;

  if (is.get() == NULL) {
    LOG(ERROR) << filename_ << " is not found";
    ret_code = false;
  } else if (!input_proto.ParseFromIstream(is.get())) {
    LOG(ERROR) << filename_ << " is broken";
    input_proto.Clear();   // revert to default setting
    ret_code = false;
  }

  // we set default config when file is broekn
  ret_code |= SetConfigInternal(input_proto);

  return ret_code;
}

void ConfigHandlerImpl::SetConfigFileName(const string &filename) {
  VLOG(1) << "set new config file name: " << filename;
  filename_ = filename;
  Reload();
}

string ConfigHandlerImpl::GetConfigFileName() {
#ifdef __native_client__
  // Copies filename_ string here to prevent Copy-On-Write issues in
  // multi-thread environment.
  // See: http://stackoverflow.com/questions/1661154/c-stdstring-in-a-multi-threaded-program/
  // TODO(hsumita): Remove this hack if not necessary.
  return string(filename_.data(), filename_.size());
#else
  return filename_;
#endif  // __native_client__
}
}  // namespace

const Config &ConfigHandler::GetConfig() {
  return GetConfigHandlerImpl()->GetConfig();
}

// Returns current Config
bool ConfigHandler::GetConfig(Config *config) {
  return GetConfigHandlerImpl()->GetConfig(config);
}

const Config &ConfigHandler::GetStoredConfig() {
  return GetConfigHandlerImpl()->GetStoredConfig();
}

// Returns Stored Config
bool ConfigHandler::GetStoredConfig(Config *config) {
  return GetConfigHandlerImpl()->GetStoredConfig(config);
}

bool ConfigHandler::SetConfig(const Config &config) {
  return GetConfigHandlerImpl()->SetConfig(config);
}

// Sets overriding config
void ConfigHandler::SetImposedConfig(const Config &config) {
  GetConfigHandlerImpl()->SetImposedConfig(config);
}

void ConfigHandler::GetDefaultConfig(Config *config) {
  config->Clear();
  config->set_session_keymap(ConfigHandler::GetDefaultKeyMap());

  const Config::CharacterForm kFullWidth = Config::FULL_WIDTH;
  const Config::CharacterForm kLastForm = Config::LAST_FORM;
  // "ア"
  AddCharacterFormRule("\xE3\x82\xA2", kFullWidth, kFullWidth, config);
  AddCharacterFormRule("A", kFullWidth, kLastForm, config);
  AddCharacterFormRule("0", kFullWidth, kLastForm, config);
  AddCharacterFormRule("(){}[]", kFullWidth, kLastForm, config);
  AddCharacterFormRule(".,", kFullWidth, kLastForm, config);
  // "。、",
  AddCharacterFormRule("\xE3\x80\x82\xE3\x80\x81", kFullWidth, kFullWidth,
                       config);
  // "・「」"
  AddCharacterFormRule("\xE3\x83\xBB\xE3\x80\x8C\xE3\x80\x8D",
                       kFullWidth, kFullWidth, config);
  AddCharacterFormRule("\"'", kFullWidth, kLastForm, config);
  AddCharacterFormRule(":;", kFullWidth, kLastForm, config);
  AddCharacterFormRule("#%&@$^_|`\\", kFullWidth, kLastForm, config);
  AddCharacterFormRule("~", kFullWidth, kLastForm, config);
  AddCharacterFormRule("<>=+-/*", kFullWidth, kLastForm, config);
  AddCharacterFormRule("?!", kFullWidth, kLastForm, config);

#if defined(OS_ANDROID) && defined(CHANNEL_DEV)
  config->mutable_general_config()->set_upload_usage_stats(true);
#endif  // OS_ANDROID && CHANNEL_DEV

  if (GetPlatformSpecificDefaultEmojiSetting()) {
    config->set_use_emoji_conversion(true);
  }
}

// Reload from file
bool ConfigHandler::Reload() {
  return GetConfigHandlerImpl()->Reload();
}

void ConfigHandler::SetConfigFileName(const string &filename) {
  GetConfigHandlerImpl()->SetConfigFileName(filename);
}

string ConfigHandler::GetConfigFileName() {
  return GetConfigHandlerImpl()->GetConfigFileName();
}

// static
void ConfigHandler::SetMetaData(Config *config) {
  GeneralConfig *general_config = config->mutable_general_config();
  general_config->set_config_version(CONFIG_VERSION);
  general_config->set_last_modified_time(Util::GetTime());
  general_config->set_last_modified_product_version(Version::GetMozcVersion());
  general_config->set_platform(SystemUtil::GetOSVersionString());
}

Config::SessionKeymap ConfigHandler::GetDefaultKeyMap() {
#if defined(OS_MACOSX)
  return config::Config::KOTOERI;
#elif defined(__native_client__)  // OS_MACOSX
  return config::Config::CHROMEOS;
#else  // OS_MACOSX or __native_client__
  return config::Config::MSIME;
#endif  // OS_MACOSX or __native_client__
}

}  // namespace config
}  // namespace mozc
