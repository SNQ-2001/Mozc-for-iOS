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

#include "session/session.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/system_util.h"
#include "base/util.h"
#include "composer/composer.h"
#include "composer/table.h"
#include "config/config.pb.h"
#include "config/config_handler.h"
#include "converter/conversion_request.h"
#include "converter/converter_mock.h"
#include "converter/segments.h"
#include "data_manager/user_pos_manager.h"
#include "engine/engine_interface.h"
#include "engine/mock_converter_engine.h"
#include "engine/mock_data_engine_factory.h"
#include "rewriter/transliteration_rewriter.h"
#include "session/candidates.pb.h"
#include "session/commands.pb.h"
#include "session/internal/ime_context.h"
#include "session/internal/keymap.h"
#include "session/key_parser.h"
#include "session/request_test_util.h"
#include "session/session_converter_interface.h"
#include "testing/base/public/googletest.h"
#include "testing/base/public/gunit.h"
#include "usage_stats/usage_stats.h"
#include "usage_stats/usage_stats_testing_util.h"

using ::mozc::commands::Request;
using ::mozc::usage_stats::UsageStats;

DECLARE_string(test_tmpdir);

namespace mozc {

class ConverterInterface;
class PredictorInterface;
class SuppressionDictionary;

namespace session {

namespace {
// "あいうえお"
const char kAiueo[] =
    "\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A";
// "　"
const char kFullWidthSpace[] = "\xE3\x80\x80";
// "アイウエオ"
const char kKatakanaAiueo[] =
    "\xe3\x82\xa2\xe3\x82\xa4\xe3\x82\xa6\xe3\x82\xa8\xe3\x82\xaa";
// "あ"
const char kHiraganaA[] = "\xE3\x81\x82";
// "ａ"
const char kFullWidthSmallA[] = "\xEF\xBD\x81";

void SetSendKeyCommandWithKeyString(const string &key_string,
                                    commands::Command *command) {
  command->Clear();
  command->mutable_input()->set_type(commands::Input::SEND_KEY);
  commands::KeyEvent *key = command->mutable_input()->mutable_key();
  key->set_key_string(key_string);
}

bool SetSendKeyCommand(const string &key, commands::Command *command) {
  command->Clear();
  command->mutable_input()->set_type(commands::Input::SEND_KEY);
  return KeyParser::ParseKey(key, command->mutable_input()->mutable_key());
}

bool SendKey(const string &key,
             Session *session,
             commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  return session->SendKey(command);
}

bool SendKeyWithMode(const string &key,
                     commands::CompositionMode mode,
                     Session *session,
                     commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  command->mutable_input()->mutable_key()->set_mode(mode);
  return session->SendKey(command);
}

bool SendKeyWithModeAndActivated(const string &key,
                                 bool activated,
                                 commands::CompositionMode mode,
                                 Session *session,
                                 commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  command->mutable_input()->mutable_key()->set_activated(activated);
  command->mutable_input()->mutable_key()->set_mode(mode);
  return session->SendKey(command);
}

bool TestSendKey(const string &key,
                 Session *session,
                 commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  return session->TestSendKey(command);
}

bool TestSendKeyWithMode(const string &key,
                         commands::CompositionMode mode,
                         Session *session,
                         commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  command->mutable_input()->mutable_key()->set_mode(mode);
  return session->TestSendKey(command);
}

bool TestSendKeyWithModeAndActivated(const string &key,
                                     bool activated,
                                     commands::CompositionMode mode,
                                     Session *session,
                                     commands::Command *command) {
  if (!SetSendKeyCommand(key, command)) {
    return false;
  }
  command->mutable_input()->mutable_key()->set_activated(activated);
  command->mutable_input()->mutable_key()->set_mode(mode);
  return session->TestSendKey(command);
}

bool SendSpecialKey(commands::KeyEvent::SpecialKey special_key,
                    Session* session,
                    commands::Command* command) {
  command->Clear();
  command->mutable_input()->set_type(commands::Input::SEND_KEY);
  command->mutable_input()->mutable_key()->set_special_key(special_key);
  return session->SendKey(command);
}


void SetSendCommandCommand(commands::SessionCommand::CommandType type,
                           commands::Command *command) {
  command->Clear();
  command->mutable_input()->set_type(commands::Input::SEND_COMMAND);
  command->mutable_input()->mutable_command()->set_type(type);
}

bool SendCommand(commands::SessionCommand::CommandType type,
                 Session *session,
                 commands::Command *command) {
  SetSendCommandCommand(type, command);
  return session->SendCommand(command);
}

bool InsertCharacterCodeAndString(const char key_code,
                                  const string &key_string,
                                  Session *session,
                                  commands::Command *command) {
  command->Clear();
  commands::KeyEvent *key_event = command->mutable_input()->mutable_key();
  key_event->set_key_code(key_code);
  key_event->set_key_string(key_string);
  return session->InsertCharacter(command);
}

Segment::Candidate *AddCandidate(const string &key, const string &value,
                                 Segment *segment) {
  Segment::Candidate *candidate = segment->add_candidate();
  candidate->key = key;
  candidate->content_key = key;
  candidate->value = value;
  return candidate;
}

Segment::Candidate *AddMetaCandidate(const string &key, const string &value,
                                     Segment *segment) {
  Segment::Candidate *candidate = segment->add_meta_candidate();
  candidate->key = key;
  candidate->content_key = key;
  candidate->value = value;
  return candidate;
}

string GetComposition(const commands::Command &command) {
  if (!command.output().has_preedit()) {
    return "";
  }

  string preedit;
  for (size_t i = 0; i < command.output().preedit().segment_size(); ++i) {
    preedit.append(command.output().preedit().segment(i).value());
  }
  return preedit;
}

::testing::AssertionResult EnsurePreedit(const string &expected,
                                         const commands::Command &command) {
  if (!command.output().has_preedit()) {
    return ::testing::AssertionFailure() << "No preedit.";
  }
  string actual;
  for (size_t i = 0; i < command.output().preedit().segment_size(); ++i) {
    actual.append(command.output().preedit().segment(i).value());
  }
  if (expected == actual) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
      << "expected: " << expected << ", actual: " << actual;
}

::testing::AssertionResult EnsureSingleSegment(
    const string &expected, const commands::Command &command) {
  if (!command.output().has_preedit()) {
    return ::testing::AssertionFailure() << "No preedit.";
  }
  if (command.output().preedit().segment_size() != 1) {
    return ::testing::AssertionFailure()
        << "Not single segment. segment size: "
        << command.output().preedit().segment_size();
  }
  const commands::Preedit::Segment &segment =
      command.output().preedit().segment(0);
  if (!segment.has_value()) {
    return ::testing::AssertionFailure() << "No segment value.";
  }
  const string &actual = segment.value();
  if (expected == actual) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
      << "expected: " << expected << ", actual: " << actual;
}

::testing::AssertionResult EnsureSingleSegmentAndKey(
    const string &expected_value,
    const string &expected_key,
    const commands::Command &command) {
  if (!command.output().has_preedit()) {
    return ::testing::AssertionFailure() << "No preedit.";
  }
  if (command.output().preedit().segment_size() != 1) {
    return ::testing::AssertionFailure()
        << "Not single segment. segment size: "
        << command.output().preedit().segment_size();
  }
  const commands::Preedit::Segment &segment =
      command.output().preedit().segment(0);
  if (!segment.has_value()) {
    return ::testing::AssertionFailure() << "No segment value.";
  }
  if (!segment.has_key()) {
    return ::testing::AssertionFailure() << "No segment key.";
  }
  const string &actual_value = segment.value();
  const string &actual_key = segment.key();
  if (expected_value == actual_value && expected_key == actual_key) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
      << "expected_value: " << expected_value
      << ", actual_value: " << actual_value
      << ", expected_key: " << expected_key
      << ", actual_key: " << actual_key;
}

::testing::AssertionResult EnsureResult(const string &expected,
                                        const commands::Command &command) {
  if (!command.output().has_result()) {
    return ::testing::AssertionFailure() << "No result.";
  }
  if (!command.output().result().has_value()) {
    return ::testing::AssertionFailure() << "No result value.";
  }
  const string &actual = command.output().result().value();
  if (expected == actual) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
      << "expected: " << expected << ", actual: " << actual;
}

::testing::AssertionResult EnsureResultAndKey(
    const string &expected_value,
    const string &expected_key,
    const commands::Command &command) {
  if (!command.output().has_result()) {
    return ::testing::AssertionFailure() << "No result.";
  }
  if (!command.output().result().has_value()) {
    return ::testing::AssertionFailure() << "No result value.";
  }
  if (!command.output().result().has_key()) {
    return ::testing::AssertionFailure() << "No result value.";
  }
  const string &actual_value = command.output().result().value();
  const string &actual_key = command.output().result().key();
  if (expected_value == actual_value && expected_key == actual_key) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
      << "expected_value: " << expected_value
      << ", actual_value: " << actual_value
      << ", expected_key: " << expected_key
      << ", actual_key: " << actual_key;
}

::testing::AssertionResult TryUndoAndAssertSuccess(Session *session) {
  commands::Command command;
  session->RequestUndo(&command);
  if (!command.output().consumed()) {
    return ::testing::AssertionFailure() << "Not consumed.";
  }
  if (!command.output().has_callback()) {
    return ::testing::AssertionFailure() << "No callback.";
  }
  if (command.output().callback().session_command().type() !=
      commands::SessionCommand::UNDO) {
    return ::testing::AssertionFailure() <<
        "Callback type is not Undo. Actual type: " <<
        command.output().callback().session_command().type();
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult TryUndoAndAssertDoNothing(Session *session) {
  commands::Command command;
  session->RequestUndo(&command);
  if (command.output().consumed()) {
    return ::testing::AssertionFailure()
        << "Key event is consumed against expectation.";
  }
  return ::testing::AssertionSuccess();
}

#define EXPECT_PREEDIT(expected, command)  \
    EXPECT_TRUE(EnsurePreedit(expected, command))
#define EXPECT_SINGLE_SEGMENT(expected, command)  \
    EXPECT_TRUE(EnsureSingleSegment(expected, command))
#define EXPECT_SINGLE_SEGMENT_AND_KEY(expected_value, expected_key, command)  \
    EXPECT_TRUE(EnsureSingleSegmentAndKey(expected_value,                     \
                                          expected_key, command))
#define EXPECT_RESULT(expected, command)  \
    EXPECT_TRUE(EnsureResult(expected, command))
#define EXPECT_RESULT_AND_KEY(expected_value, expected_key, command)  \
    EXPECT_TRUE(EnsureResultAndKey(expected_value, expected_key, command))

void SetCaretLocation(const commands::Rectangle rectangle, Session *session) {
  commands::Command command;
  SetSendCommandCommand(commands::SessionCommand::SEND_CARET_LOCATION,
                        &command);
  command.mutable_input()->mutable_command()->mutable_caret_rectangle()->
      CopyFrom(rectangle);
  EXPECT_TRUE(session->SendCommand(&command));
}

void SwitchInputFieldType(commands::Context::InputFieldType type,
                          Session *session) {
  commands::Command command;
  SetSendCommandCommand(commands::SessionCommand::SWITCH_INPUT_FIELD_TYPE,
                        &command);
  command.mutable_input()->mutable_context()->set_input_field_type(type);
  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_EQ(type, session->context().composer().GetInputFieldType());
}

void SwitchInputMode(commands::CompositionMode mode, Session *session) {
  commands::Command command;
  SetSendCommandCommand(commands::SessionCommand::SWITCH_INPUT_MODE, &command);
  command.mutable_input()->mutable_command()->set_composition_mode(mode);
  EXPECT_TRUE(session->SendCommand(&command));
}

// since History segments are almost hidden from
class ConverterMockForReset : public ConverterMock {
 public:
  virtual bool ResetConversion(Segments *segments) const {
    reset_conversion_called_ = true;
    return true;
  }

  bool reset_conversion_called() const {
    return reset_conversion_called_;
  }

  void Reset() {
    reset_conversion_called_ = false;
  }

  ConverterMockForReset() : reset_conversion_called_(false) {}
 private:
  mutable bool reset_conversion_called_;
};

class MockConverterEngineForReset : public EngineInterface {
 public:
  MockConverterEngineForReset() : converter_mock_(new ConverterMockForReset) {}
  virtual ~MockConverterEngineForReset() {}

  virtual ConverterInterface *GetConverter() const {
    return converter_mock_.get();
  }

  virtual PredictorInterface *GetPredictor() const {
    return NULL;
  }

  virtual SuppressionDictionary *GetSuppressionDictionary() {
    return NULL;
  }

  virtual bool Reload() {
    return true;
  }

  virtual UserDataManagerInterface *GetUserDataManager() {
    return NULL;
  }

  const ConverterMockForReset &converter_mock() const {
    return *converter_mock_;
  }

  ConverterMockForReset *mutable_converter_mock() {
    return converter_mock_.get();
  }

 private:
  scoped_ptr<ConverterMockForReset> converter_mock_;
};

class ConverterMockForRevert : public ConverterMock {
 public:
  virtual bool RevertConversion(Segments *segments) const {
    revert_conversion_called_ = true;
    return true;
  }

  bool revert_conversion_called() const {
    return revert_conversion_called_;
  }

  void Reset() {
    revert_conversion_called_ = false;
  }

  ConverterMockForRevert() : revert_conversion_called_(false) {}
 private:
  mutable bool revert_conversion_called_;
};

class MockConverterEngineForRevert : public EngineInterface {
 public:
  MockConverterEngineForRevert()
      : converter_mock_(new ConverterMockForRevert) {}
  virtual ~MockConverterEngineForRevert() {}

  virtual ConverterInterface *GetConverter() const {
    return converter_mock_.get();
  }

  virtual PredictorInterface *GetPredictor() const {
    return NULL;
  }

  virtual SuppressionDictionary *GetSuppressionDictionary() {
    return NULL;
  }

  virtual bool Reload() {
    return true;
  }

  virtual UserDataManagerInterface *GetUserDataManager() {
    return NULL;
  }

  const ConverterMockForRevert &converter_mock() const {
    return *converter_mock_;
  }

  ConverterMockForRevert *mutable_converter_mock() {
    return converter_mock_.get();
  }

 private:
  scoped_ptr<ConverterMockForRevert> converter_mock_;
};

}  // namespace

class SessionTest : public testing::Test {
 protected:
  virtual void SetUp() {
    SystemUtil::SetUserProfileDirectory(FLAGS_test_tmpdir);

    config::Config config;
    config::ConfigHandler::GetDefaultConfig(&config);
    config::ConfigHandler::SetConfig(config);

    UsageStats::ClearAllStatsForTest();

    mobile_request_.reset(new Request);
    commands::RequestForUnitTest::FillMobileRequest(mobile_request_.get());

    mock_data_engine_.reset(MockDataEngineFactory::Create());
    engine_.reset(new MockConverterEngine);

    t13n_rewriter_.reset(
        new TransliterationRewriter(
            *UserPosManager::GetUserPosManager()->GetPOSMatcher()));
  }

  virtual void TearDown() {
    UsageStats::ClearAllStatsForTest();

    // just in case, reset the config in test_tmpdir
    config::Config config;
    config::ConfigHandler::GetDefaultConfig(&config);
    config::ConfigHandler::SetConfig(config);
  }

  void InsertCharacterChars(const string &chars,
                            Session *session,
                            commands::Command *command) const {
    const uint32 kNoModifiers = 0;
    for (int i = 0; i < chars.size(); ++i) {
      command->Clear();
      commands::KeyEvent *key_event = command->mutable_input()->mutable_key();
      key_event->set_key_code(chars[i]);
      key_event->set_modifiers(kNoModifiers);
      session->InsertCharacter(command);
    }
  }

  void InsertCharacterCharsWithContext(const string &chars,
                                       const commands::Context &context,
                                       Session *session,
                                       commands::Command *command) const {
    const uint32 kNoModifiers = 0;
    for (size_t i = 0; i < chars.size(); ++i) {
      command->Clear();
      command->mutable_input()->mutable_context()->CopyFrom(context);
      commands::KeyEvent *key_event = command->mutable_input()->mutable_key();
      key_event->set_key_code(chars[i]);
      key_event->set_modifiers(kNoModifiers);
      session->InsertCharacter(command);
    }
  }

  void InsertCharacterString(const string &key_strings,
                             const string &chars,
                             Session *session,
                             commands::Command *command) const {
    const uint32 kNoModifiers = 0;
    vector<string> inputs;
    const char *begin = key_strings.data();
    const char *end = key_strings.data() + key_strings.size();
    while (begin < end) {
      const size_t mblen = Util::OneCharLen(begin);
      inputs.push_back(string(begin, mblen));
      begin += mblen;
    }
    CHECK_EQ(inputs.size(), chars.size());
    for (int i = 0; i < chars.size(); ++i) {
      command->Clear();
      commands::KeyEvent *key_event = command->mutable_input()->mutable_key();
      key_event->set_key_code(chars[i]);
      key_event->set_modifiers(kNoModifiers);
      key_event->set_key_string(inputs[i]);
      session->InsertCharacter(command);
    }
  }

  // set result for "あいうえお"
  void SetAiueo(Segments *segments) {
    segments->Clear();
    Segment *segment;
    Segment::Candidate *candidate;

    segment = segments->add_segment();
    // "あいうえお"
    segment->set_key(kAiueo);
    candidate = segment->add_candidate();
    // "あいうえお"
    candidate->key = kAiueo;
    candidate->content_key = kAiueo;
    candidate->value = kAiueo;
    candidate = segment->add_candidate();
    // "アイウエオ"
    candidate->key = kAiueo;
    candidate->content_key = kAiueo;
    candidate->value = kKatakanaAiueo;
  }

  void InitSessionToDirect(Session* session) {
    InitSessionToPrecomposition(session);
    commands::Command command;
    session->IMEOff(&command);
  }

  void InitSessionToConversionWithAiueo(Session *session) {
    InitSessionToPrecomposition(session);

    commands::Command command;
    InsertCharacterChars("aiueo", session, &command);
    ConversionRequest request;
    Segments segments;
    SetComposer(session, &request);
    SetAiueo(&segments);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    EXPECT_TRUE(session->Convert(&command));
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());
  }

  void InitSessionToPrecomposition(Session* session) {
#ifdef OS_WIN
    // Session is created with direct mode on Windows
    // Direct status
    commands::Command command;
    session->IMEOn(&command);
#endif  // OS_WIN
    InitSessionWithRequest(session, commands::Request::default_instance());
  }

  void InitSessionToPrecomposition(
      Session* session,
      const commands::Request &request) {
#ifdef OS_WIN
    // Session is created with direct mode on Windows
    // Direct status
    commands::Command command;
    session->IMEOn(&command);
#endif  // OS_WIN
    InitSessionWithRequest(session, request);
  }

  void InitSessionWithRequest(
      Session* session,
      const commands::Request &request) {
    session->SetRequest(&request);
    table_.reset(new composer::Table());
    table_.get()->InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session->SetTable(table_.get());
  }

  // set result for "like"
  void SetLike(Segments *segments) {
    Segment *segment;
    Segment::Candidate *candidate;

    segments->Clear();
    segment = segments->add_segment();

    // "ぃ"
    segment->set_key("\xE3\x81\x83");
    candidate = segment->add_candidate();
    // "ぃ"
    candidate->value = "\xE3\x81\x83";

    candidate = segment->add_candidate();
    // "ィ"
    candidate->value = "\xE3\x82\xA3";

    segment = segments->add_segment();
    // "け"
    segment->set_key("\xE3\x81\x91");
    candidate = segment->add_candidate();
    // "家"
    candidate->value = "\xE5\xAE\xB6";
    candidate = segment->add_candidate();
    // "け"
    candidate->value = "\xE3\x81\x91";
  }

  void FillT13Ns(const ConversionRequest &request, Segments *segments) {
    t13n_rewriter_->Rewrite(request, segments);
  }

  void SetComposer(Session *session, ConversionRequest *request) {
    DCHECK(request);
    request->set_composer(session->get_internal_composer_only_for_unittest());
  }

  void SetupMockForReverseConversion(const string &kanji,
                                     const string &hiragana) {
    // Set up Segments for reverse conversion.
    Segments reverse_segments;
    Segment *segment;
    segment = reverse_segments.add_segment();
    segment->set_key(kanji);
    Segment::Candidate *candidate;
    candidate = segment->add_candidate();
    // For reverse conversion, key is the original kanji string.
    candidate->key = kanji;
    candidate->value = hiragana;
    GetConverterMock()->SetStartReverseConversion(&reverse_segments, true);
    // Set up Segments for forward conversion.
    Segments segments;
    segment = segments.add_segment();
    segment->set_key(hiragana);
    candidate = segment->add_candidate();
    candidate->key = hiragana;
    candidate->value = kanji;
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  void SetupCommandForReverseConversion(const string &text,
                                        commands::Input *input) {
    input->Clear();
    input->set_type(commands::Input::SEND_COMMAND);
    input->mutable_command()->set_type(
        commands::SessionCommand::CONVERT_REVERSE);
    input->mutable_command()->set_text(text);
  }

  void SetupZeroQuerySuggestionReady(bool enable,
                                     Session *session,
                                     commands::Request *request) {
    InitSessionToPrecomposition(session);

    // Enable zero query suggest.
    request->set_zero_query_suggestion(enable);
    session->SetRequest(request);

    // Type "google".
    commands::Command command;
    InsertCharacterChars("google", session, &command);

    {
      // Set up a mock conversion result.
      Segments segments;
      segments.set_request_type(Segments::CONVERSION);
      Segment *segment;
      segment = segments.add_segment();
      segment->set_key("google");
      segment->add_candidate()->value = "GOOGLE";
      GetConverterMock()->SetStartConversionForRequest(&segments, true);
    }
    command.Clear();
    session->Convert(&command);

    {
      // Set up a mock suggestion result.
      Segments segments;
      segments.set_request_type(Segments::SUGGESTION);
      Segment *segment;
      segment = segments.add_segment();
      segment->set_key("");
      AddCandidate("search", "search", segment);
      AddCandidate("input", "input", segment);
      GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
    }
  }

  void SetupZeroQuerySuggestion(Session *session,
                                commands::Request *request,
                                commands::Command *command) {
    SetupZeroQuerySuggestionReady(true, session, request);
    command->Clear();
    session->Commit(command);
  }

  void SetUndoContext(Session *session) {
    commands::Command command;
    Segments segments;

    {  // Create segments
      InsertCharacterChars("aiueo", session, &command);
      SetAiueo(&segments);
      // Don't use FillT13Ns(). It makes platform dependent segments.
      // TODO(hsumita): Makes FillT13Ns() independent from platforms.
      Segment::Candidate *candidate;
      candidate = segments.mutable_segment(0)->add_candidate();
      candidate->value = "aiueo";
      candidate = segments.mutable_segment(0)->add_candidate();
      candidate->value = "AIUEO";
    }

    {  // Commit the composition to make an undo context.
      GetConverterMock()->SetStartConversionForRequest(&segments, true);
      command.Clear();
      session->Convert(&command);
      EXPECT_FALSE(command.output().has_result());
      // "あいうえお"
      EXPECT_PREEDIT(kAiueo, command);

      GetConverterMock()->SetCommitSegmentValue(&segments, true);
      command.Clear();
      session->Commit(&command);
      EXPECT_FALSE(command.output().has_preedit());
      // "あいうえお"
      EXPECT_RESULT(kAiueo, command);
    }
  }

  ConverterMock *GetConverterMock() {
    return engine_->mutable_converter_mock();
  }

  // IMPORTANT: Use scoped_ptr and instanciate an object in SetUp() method
  //    if the target object should be initialized *AFTER* global settings
  //    such as user profile dir or global config are set up for unit test.
  //    If you directly define a variable here without scoped_ptr, its
  //    constructor will be called *BEFORE* SetUp() is called.
  scoped_ptr<MockConverterEngine> engine_;
  scoped_ptr<EngineInterface> mock_data_engine_;
  scoped_ptr<TransliterationRewriter> t13n_rewriter_;
  scoped_ptr<composer::Table> table_;
  scoped_ptr<Request> mobile_request_;
  mozc::usage_stats::scoped_usage_stats_enabler usage_stats_enabler_;
};

// This test is intentionally defined at this location so that this
// test can ensure that the first SetUp() initialized global
// config, and table object to the default state.
// Please do not define another test before this.
// FYI, each TEST_F will be eventually expanded into a global variable
// and global variables in a single translation unit (source file) are
// always initialized in the order in which they are defined.
TEST_F(SessionTest, TestOfTestForSetup) {
  config::Config config;
  config::ConfigHandler::GetConfig(&config);
  EXPECT_FALSE(config.has_use_auto_conversion())
      << "Global config should be initialized for each text fixture.";

  // Make sure that the default roman table is initialized.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    SendKey("a", session.get(), &command);
    // "あ"
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command)
        << "Global Romaji table should be initialized for each text fixture.";
  }

  // intentionally leave non-default value so that |TestOfTestForTearDown|
  // can test it later.
  config.set_use_auto_conversion(true);
  config::ConfigHandler::SetConfig(config);
}

// This test ensures that the TearDown() against |TestOfTestForSetup|
// restored global config, and table object to the default state
// Please do not define another test between |TestOfTestForSetup| and
// this test.
// FYI, each TEST_F will be eventually expanded into a global variable
// and global variables in a single translation unit (source file) are
// always initialized in the order in which they are defined.
TEST_F(SessionTest, TestOfTestForTearDown) {
  // Make sure that the initial global config has default value.
  config::Config config;
  config::ConfigHandler::GetConfig(&config);
  EXPECT_FALSE(config.has_use_auto_conversion())
      << "Global config should be initialized for each text fixture.";

  // Make sure that the initial roman table has default value.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    SendKey("a", session.get(), &command);
    // "あ"
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command)
        << "Global Romaji table should be initialized for each text fixture.";
  }
}

TEST_F(SessionTest, TestSendKey) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;

  // Precomposition status
  TestSendKey("Up", session.get(), &command);
  EXPECT_FALSE(command.output().consumed());

  SendKey("Up", session.get(), &command);
  EXPECT_FALSE(command.output().consumed());

  // InsertSpace on Precomposition status
  // TODO(komatsu): Test both cases of GET_CONFIG(ascii_character_form) is
  // FULL_WIDTH and HALF_WIDTH after dependency injection of GET_CONFIG.
  TestSendKey("Space", session.get(), &command);
  const bool consumed_on_testsendkey = command.output().consumed();
  SendKey("Space", session.get(), &command);
  const bool consumed_on_sendkey = command.output().consumed();
  EXPECT_EQ(consumed_on_sendkey, consumed_on_testsendkey);

  // Precomposition status
  TestSendKey("G", session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  SendKey("G", session.get(), &command);
  EXPECT_TRUE(command.output().consumed());

  // Composition status
  TestSendKey("Up", session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  SendKey("Up", session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
}

TEST_F(SessionTest, SendCommand) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
  InsertCharacterChars("kanji", session.get(), &command);

  // REVERT
  SendCommand(commands::SessionCommand::REVERT, session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_candidates());

  // SUBMIT
  InsertCharacterChars("k", session.get(), &command);
  SendCommand(commands::SessionCommand::SUBMIT, session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  // "ｋ"
  EXPECT_RESULT("\xef\xbd\x8b", command);
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_candidates());

  // SWITCH_INPUT_MODE
  SendKey("a", session.get(), &command);
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  SwitchInputMode(commands::FULL_ASCII, session.get());

  SendKey("a", session.get(), &command);
  // "あａ"
  EXPECT_SINGLE_SEGMENT("\xE3\x81\x82\xEF\xBD\x81", command);

  // GET_STATUS
  SendCommand(commands::SessionCommand::GET_STATUS, session.get(), &command);
  // FULL_ASCII was set at the SWITCH_INPUT_MODE testcase.
  SwitchInputMode(commands::FULL_ASCII, session.get());

  // RESET_CONTEXT
  // test of reverting composition
  InsertCharacterChars("kanji", session.get(), &command);
  SendCommand(commands::SessionCommand::RESET_CONTEXT, session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_candidates());
  // test of reseting the history segements
  scoped_ptr<MockConverterEngineForReset> engine(
      new MockConverterEngineForReset);
  session.reset(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());
  SendCommand(commands::SessionCommand::RESET_CONTEXT, session.get(), &command);
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(engine->converter_mock().reset_conversion_called());

  // USAGE_STATS_EVENT
  SendCommand(commands::SessionCommand::USAGE_STATS_EVENT, session.get(),
              &command);
  EXPECT_TRUE(command.output().has_consumed());
  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, SwitchInputMode) {
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // SWITCH_INPUT_MODE
    SendKey("a", session.get(), &command);
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

    SwitchInputMode(commands::FULL_ASCII, session.get());

    SendKey("a", session.get(), &command);
    // "あａ"
    EXPECT_SINGLE_SEGMENT("\xE3\x81\x82\xEF\xBD\x81", command);

    // GET_STATUS
    SendCommand(commands::SessionCommand::GET_STATUS, session.get(), &command);
    // FULL_ASCII was set at the SWITCH_INPUT_MODE testcase.
    EXPECT_EQ(commands::FULL_ASCII, command.output().mode());
  }

  {
    // Confirm that we can change the mode from DIRECT
    // to other modes directly (without IMEOn command).
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;

    // GET_STATUS
    SendCommand(commands::SessionCommand::GET_STATUS, session.get(), &command);
    // FULL_ASCII was set at the SWITCH_INPUT_MODE testcase.
    EXPECT_EQ(commands::DIRECT, command.output().mode());

    // SWITCH_INPUT_MODE
    SwitchInputMode(commands::HIRAGANA, session.get());

    // GET_STATUS
    SendCommand(commands::SessionCommand::GET_STATUS, session.get(), &command);
    // FULL_ASCII was set at the SWITCH_INPUT_MODE testcase.
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    SendKey("a", session.get(), &command);
    // "あ"
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

    // GET_STATUS
    SendCommand(commands::SessionCommand::GET_STATUS, session.get(), &command);
    // FULL_ASCII was set at the SWITCH_INPUT_MODE testcase.
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }
}

TEST_F(SessionTest, RevertComposition) {
  // Issue#2237323
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  // REVERT
  SendCommand(commands::SessionCommand::REVERT, session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_candidates());

  SendKey("a", session.get(), &command);
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);
}

TEST_F(SessionTest, InputMode) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());

  SendKey("a", session.get(), &command);
  EXPECT_EQ("a", command.output().preedit().segment(0).key());

  command.Clear();
  session->Commit(&command);

  // Input mode remains even after submission.
  command.Clear();
  session->GetStatus(&command);
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());
}

TEST_F(SessionTest, SelectCandidate) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->ConvertNext(&command);

  SetSendCommandCommand(commands::SessionCommand::SELECT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(
      -(transliteration::HALF_KATAKANA + 1));
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_result());
  // "ｱｲｳｴｵ"
  EXPECT_PREEDIT(
      "\xEF\xBD\xB1\xEF\xBD\xB2\xEF\xBD\xB3\xEF\xBD\xB4\xEF\xBD\xB5", command);
  EXPECT_FALSE(command.output().has_candidates());
}

TEST_F(SessionTest, HighlightCandidate) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->ConvertNext(&command);
  // "アイウエオ"
  EXPECT_SINGLE_SEGMENT(
      "\xE3\x82\xA2\xE3\x82\xA4\xE3\x82\xA6\xE3\x82\xA8\xE3\x82\xAA", command);

  SetSendCommandCommand(commands::SessionCommand::HIGHLIGHT_CANDIDATE,
                        &command);
  command.mutable_input()->mutable_command()->set_id(
      -(transliteration::HALF_KATAKANA + 1));
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_result());
  // "ｱｲｳｴｵ"
  EXPECT_SINGLE_SEGMENT(
      "\xEF\xBD\xB1\xEF\xBD\xB2\xEF\xBD\xB3\xEF\xBD\xB4\xEF\xBD\xB5", command);
  EXPECT_TRUE(command.output().has_candidates());
}

TEST_F(SessionTest, Conversion) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  // "あいうえお"
  EXPECT_SINGLE_SEGMENT_AND_KEY(kAiueo, kAiueo, command);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->ConvertNext(&command);

  string key;
  for (int i = 0; i < command.output().preedit().segment_size(); ++i) {
    EXPECT_TRUE(command.output().preedit().segment(i).has_value());
    EXPECT_TRUE(command.output().preedit().segment(i).has_key());
    key += command.output().preedit().segment(i).key();
  }
  // "あいうえお"
  EXPECT_EQ(kAiueo, key);
}

TEST_F(SessionTest, SegmentWidthShrink) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->SegmentWidthShrink(&command);

  command.Clear();
  session->SegmentWidthShrink(&command);
}

TEST_F(SessionTest, ConvertPrev) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  Segments segments;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->ConvertNext(&command);

  command.Clear();
  session->ConvertPrev(&command);

  command.Clear();
  session->ConvertPrev(&command);
}

TEST_F(SessionTest, ResetFocusedSegmentAfterCommit) {
  ConversionRequest request;
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("watasinonamaehanakanodesu", session.get(), &command);
  // "わたしのなまえはなかのです[]"

  segment = segments.add_segment();
  // "わたしの"
  segment->set_key("\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae");
  candidate = segment->add_candidate();
  // "私の"
  candidate->value = "\xe7\xa7\x81\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "わたしの"
  candidate->value = "\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "渡しの"
  candidate->value = "\xe6\xb8\xa1\xe3\x81\x97\xe3\x81\xae";

  segment = segments.add_segment();
  // "なまえは"
  segment->set_key("\xe3\x81\xaa\xe3\x81\xbe\xe3\x81\x88\xe3\x81\xaf");
  candidate = segment->add_candidate();
  // "名前は"
  candidate->value = "\xe5\x90\x8d\xe5\x89\x8d\xe3\x81\xaf";
  candidate = segment->add_candidate();
  // "ナマエは"
  candidate->value = "\xe3\x83\x8a\xe3\x83\x9e\xe3\x82\xa8\xe3\x81\xaf";

  segment = segments.add_segment();
  // "なかのです"
  segment->set_key(
      "\xe3\x81\xaa\xe3\x81\x8b\xe3\x81\xae\xe3\x81\xa7\xe3\x81\x99");
  candidate = segment->add_candidate();
  // "中野です"
  candidate->value = "\xe4\xb8\xad\xe9\x87\x8e\xe3\x81\xa7\xe3\x81\x99";
  candidate = segment->add_candidate();
  // "なかのです"
  candidate->value
      = "\xe3\x81\xaa\xe3\x81\x8b\xe3\x81\xae\xe3\x81\xa7\xe3\x81\x99";
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  EXPECT_TRUE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  // "[私の]名前は中野です"
  command.Clear();
  session->SegmentFocusRight(&command);
  EXPECT_TRUE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  // "私の[名前は]中野です"
  command.Clear();
  session->SegmentFocusRight(&command);
  EXPECT_TRUE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  // "私の名前は[中野です]"

  command.Clear();
  session->ConvertNext(&command);
  EXPECT_EQ(1, command.output().candidates().focused_index());
  EXPECT_TRUE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  // "私の名前は[中のです]"

  command.Clear();
  session->ConvertNext(&command);
  EXPECT_EQ(2, command.output().candidates().focused_index());
  EXPECT_TRUE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  // "私の名前は[なかのです]"

  command.Clear();
  session->Commit(&command);
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_TRUE(command.output().has_result());
  // "私の名前はなかのです[]"

  InsertCharacterChars("a", session.get(), &command);

  segments.Clear();
  segment = segments.add_segment();
  // "あ"
  segment->set_key(kHiraganaA);
  candidate = segment->add_candidate();
  // "阿"
  candidate->value = "\xe9\x98\xbf";
  candidate = segment->add_candidate();
  // "亜"
  candidate->value = "\xe4\xba\x9c";

  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  // "あ[]"

  command.Clear();
  session->Convert(&command);
  // "[阿]"

  command.Clear();
  // If the forcused_segment_ was not reset, this raises segmentation fault.
  session->ConvertNext(&command);
  // "[亜]"
}

TEST_F(SessionTest, ResetFocusedSegmentAfterCancel) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("ai", session.get(), &command);

  segment = segments.add_segment();
  // "あい"
  segment->set_key("\xe3\x81\x82\xe3\x81\x84");
  candidate = segment->add_candidate();
  // "愛"
  candidate->value = "\xe6\x84\x9b";
  candidate = segment->add_candidate();
  // "相"
  candidate->value = "\xe7\x9b\xb8";
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);
  // "あい[]"

  command.Clear();
  session->Convert(&command);
  // "[愛]"

  segments.Clear();
  segment = segments.add_segment();
  // "あ"
  segment->set_key(kHiraganaA);
  candidate = segment->add_candidate();
  // "あ"
  candidate->value = kHiraganaA;
  segment = segments.add_segment();
  // "い"
  segment->set_key("\xe3\x81\x84");
  candidate = segment->add_candidate();
  // "い"
  candidate->value = "\xe3\x81\x84";
  candidate = segment->add_candidate();
  // "位"
  candidate->value = "\xe4\xbd\x8d";
  GetConverterMock()->SetResizeSegment1(&segments, true);

  command.Clear();
  session->SegmentWidthShrink(&command);
  // "[あ]い"

  segment = segments.mutable_segment(0);
  segment->set_segment_type(Segment::FIXED_VALUE);
  GetConverterMock()->SetCommitSegmentValue(&segments, true);

  command.Clear();
  session->SegmentFocusRight(&command);
  // "あ[い]"

  command.Clear();
  session->ConvertNext(&command);
  // "あ[位]"

  command.Clear();
  session->ConvertCancel(&command);
  // "あい[]"

  segments.Clear();
  segment = segments.add_segment();
  // "あい"
  segment->set_key("\xe3\x81\x82\xe3\x81\x84");
  candidate = segment->add_candidate();
  // "愛"
  candidate->value = "\xe6\x84\x9b";
  candidate = segment->add_candidate();
  // "相"
  candidate->value = "\xe7\x9b\xb8";
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  // "[愛]"

  command.Clear();
  // If the forcused_segment_ was not reset, this raises segmentation fault.
  session->Convert(&command);
  // "[相]"
}


TEST_F(SessionTest, KeepFixedCandidateAfterSegmentWidthExpand) {
  // Issue#1271099
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("bariniryokouniitta", session.get(), &command);
  // "ばりにりょこうにいった[]"

  segment = segments.add_segment();
  // "ばりに"
  segment->set_key("\xe3\x81\xb0\xe3\x82\x8a\xe3\x81\xab");
  candidate = segment->add_candidate();
  // "バリに"
  candidate->value = "\xe3\x83\x90\xe3\x83\xaa\xe3\x81\xab";
  candidate = segment->add_candidate();
  // "針に"
  candidate->value = "\xe9\x87\x9d\xe3\x81\xab";

  segment = segments.add_segment();
  // "りょこうに"
  segment->set_key(
      "\xe3\x82\x8a\xe3\x82\x87\xe3\x81\x93\xe3\x81\x86\xe3\x81\xab");
  candidate = segment->add_candidate();
  // "旅行に"
  candidate->value = "\xe6\x97\x85\xe8\xa1\x8c\xe3\x81\xab";

  segment = segments.add_segment();
  // "いった"
  segment->set_key("\xe3\x81\x84\xe3\x81\xa3\xe3\x81\x9f");
  candidate = segment->add_candidate();
  // "行った"
  candidate->value = "\xe8\xa1\x8c\xe3\x81\xa3\xe3\x81\x9f";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  // ex. "[バリに]旅行に行った"
  EXPECT_EQ("\xE3\x83\x90\xE3\x83\xAA\xE3\x81\xAB\xE6\x97\x85\xE8\xA1\x8C\xE3"
      "\x81\xAB\xE8\xA1\x8C\xE3\x81\xA3\xE3\x81\x9F", GetComposition(command));
  command.Clear();
  session->ConvertNext(&command);
  // ex. "[針に]旅行に行った"
  const string first_segment = command.output().preedit().segment(0).value();

  segment = segments.mutable_segment(0);
  segment->set_segment_type(Segment::FIXED_VALUE);
  segment->move_candidate(1, 0);
  GetConverterMock()->SetCommitSegmentValue(&segments, true);

  command.Clear();
  session->SegmentFocusRight(&command);
  // ex. "針に[旅行に]行った"
  // Make sure the first segment (i.e. "針に" in the above case) remains
  // after moving the focused segment right.
  EXPECT_EQ(first_segment, command.output().preedit().segment(0).value());

  segment = segments.mutable_segment(1);
  // "りょこうにい"
  segment->set_key("\xe3\x82\x8a\xe3\x82\x87\xe3\x81\x93"
                   "\xe3\x81\x86\xe3\x81\xab\xe3\x81\x84");
  candidate = segment->mutable_candidate(0);
  // "旅行に行"
  candidate->value = "\xe6\x97\x85\xe8\xa1\x8c\xe3\x81\xab\xe8\xa1\x8c";

  segment = segments.mutable_segment(2);
  // "った"
  segment->set_key("\xe3\x81\xa3\xe3\x81\x9f");
  candidate = segment->mutable_candidate(0);
  // "った"
  candidate->value = "\xe3\x81\xa3\xe3\x81\x9f";

  GetConverterMock()->SetResizeSegment1(&segments, true);

  command.Clear();
  session->SegmentWidthExpand(&command);
  // ex. "針に[旅行に行]った"

  // Make sure the first segment (i.e. "針に" in the above case) remains
  // after expanding the focused segment.
  EXPECT_EQ(first_segment, command.output().preedit().segment(0).value());
}

TEST_F(SessionTest, CommitSegment) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;

  // Issue#1560608
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("watasinonamae", session.get(), &command);
  // "わたしのなまえ[]"

  segment = segments.add_segment();
  // "わたしの"
  segment->set_key("\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae");
  candidate = segment->add_candidate();
  // "私の"
  candidate->value = "\xe7\xa7\x81\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "わたしの"
  candidate->value = "\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "渡しの"
  candidate->value = "\xe6\xb8\xa1\xe3\x81\x97\xe3\x81\xae";

  segment = segments.add_segment();
  // "なまえ"
  segment->set_key("\xe3\x81\xaa\xe3\x81\xbe\xe3\x81\x88");
  candidate = segment->add_candidate();
  // "名前"
  candidate->value = "\xe5\x90\x8d\xe5\x89\x8d";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  // "[私の]名前"
  EXPECT_EQ(0, command.output().candidates().focused_index());

  command.Clear();
  session->ConvertNext(&command);
  // "[わたしの]名前"
  EXPECT_EQ(1, command.output().candidates().focused_index());

  command.Clear();
  session->ConvertNext(&command);
  // "[渡しの]名前" showing a candidate window
  EXPECT_EQ(2, command.output().candidates().focused_index());

  segment = segments.mutable_segment(0);
  segment->set_segment_type(Segment::FIXED_VALUE);
  segment->move_candidate(2, 0);

  GetConverterMock()->SetCommitSegments(&segments, true);

  command.Clear();
  session->CommitSegment(&command);
  // "渡しの" + "[名前]"
  EXPECT_EQ(0, command.output().candidates().focused_index());
}

TEST_F(SessionTest, CommitSegmentAt2ndSegment) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("watasinohaha", session.get(), &command);
  // "わたしのはは[]"

  segment = segments.add_segment();
  // "わたしの"
  segment->set_key("\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae");
  candidate = segment->add_candidate();
  // "私の"
  candidate->value = "\xe7\xa7\x81\xe3\x81\xae";
  segment = segments.add_segment();
  // "はは"
  segment->set_key("\xe3\x81\xaf\xe3\x81\xaf");
  candidate = segment->add_candidate();
  // "母"
  candidate->value = "\xe6\xaf\x8d";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  // "[私の]母"

  command.Clear();
  session->SegmentFocusRight(&command);
  // "私の[母]"

  segment->set_segment_type(Segment::FIXED_VALUE);
  segment->move_candidate(1, 0);
  GetConverterMock()->SetCommitSegments(&segments, true);

  command.Clear();
  session->CommitSegment(&command);
  // "私の" + "[母]"

  // "は"
  segment->set_key("\xe3\x81\xaf");
  // "葉"
  candidate->value = "\xe8\x91\x89";
  segment = segments.add_segment();
  // "は"
  segment->set_key("\xe3\x81\xaf");
  candidate = segment->add_candidate();
  // "は"
  candidate->value = "\xe3\x81\xaf";
  segments.pop_front_segment();
  GetConverterMock()->SetResizeSegment1(&segments, true);

  command.Clear();
  session->SegmentWidthShrink(&command);
  // "私の" + "[葉]は"
  EXPECT_EQ(2, command.output().preedit().segment_size());
}

TEST_F(SessionTest, Transliterations) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("jishin", session.get(), &command);

  segment = segments.add_segment();
  // "じしん"
  segment->set_key("\xe3\x81\x98\xe3\x81\x97\xe3\x82\x93");
  candidate = segment->add_candidate();
  // "自信"
  candidate->value = "\xe8\x87\xaa\xe4\xbf\xa1";
  candidate = segment->add_candidate();
  // "自身"
  candidate->value = "\xe8\x87\xaa\xe8\xba\xab";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  session->ConvertNext(&command);

  command.Clear();
  session->TranslateHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("jishin", command);

  command.Clear();
  session->TranslateHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("JISHIN", command);

  command.Clear();
  session->TranslateHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("Jishin", command);

  command.Clear();
  session->TranslateHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("jishin", command);
}

TEST_F(SessionTest, ConvertToTransliteration) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("jishin", session.get(), &command);

  segment = segments.add_segment();
  // "じしん"
  segment->set_key("\xe3\x81\x98\xe3\x81\x97\xe3\x82\x93");
  candidate = segment->add_candidate();
  // "自信"
  candidate->value = "\xe8\x87\xaa\xe4\xbf\xa1";
  candidate = segment->add_candidate();
  // "自身"
  candidate->value = "\xe8\x87\xaa\xe8\xba\xab";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("jishin", command);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("JISHIN", command);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("Jishin", command);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("jishin", command);
}

TEST_F(SessionTest, ConvertToTransliterationWithMultipleSegments) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("like", session.get(), &command);

  Segments segments;
  SetLike(&segments);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  // Convert
  command.Clear();
  session->Convert(&command);
  {  // Check the conversion #1
    const commands::Output &output = command.output();
    EXPECT_FALSE(output.has_result());
    EXPECT_TRUE(output.has_preedit());
    EXPECT_FALSE(output.has_candidates());

    const commands::Preedit &conversion = output.preedit();
    EXPECT_EQ(2, conversion.segment_size());
    // "ぃ"
    EXPECT_EQ("\xE3\x81\x83", conversion.segment(0).value());
    // "家"
    EXPECT_EQ("\xE5\xAE\xB6", conversion.segment(1).value());
  }

  // TranslateHalfASCII
  command.Clear();
  session->TranslateHalfASCII(&command);
  {  // Check the conversion #2
    const commands::Output &output = command.output();
    EXPECT_FALSE(output.has_result());
    EXPECT_TRUE(output.has_preedit());
    EXPECT_FALSE(output.has_candidates());

    const commands::Preedit &conversion = output.preedit();
    EXPECT_EQ(2, conversion.segment_size());
    EXPECT_EQ("li", conversion.segment(0).value());
  }
}

TEST_F(SessionTest, ConvertToHalfWidth) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("abc", session.get(), &command);

  Segments segments;
  {  // Initialize segments.
    Segment *segment = segments.add_segment();
    // "あｂｃ"
    segment->set_key("\xE3\x81\x82\xEF\xBD\x82\xEF\xBD\x83");
    // "あべし"
    segment->add_candidate()->value = "\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97";
  }
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->ConvertToHalfWidth(&command);
  // "ｱbc"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\xB1\x62\x63", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // The output is "ａｂｃ".

  command.Clear();
  session->ConvertToHalfWidth(&command);
  EXPECT_SINGLE_SEGMENT("abc", command);
}

TEST_F(SessionTest, ConvertConsonantsToFullAlphanumeric) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("dvd", session.get(), &command);

  segment = segments.add_segment();
  // "ｄｖｄ"
  segment->set_key("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84");
  candidate = segment->add_candidate();
  candidate->value = "DVD";
  candidate = segment->add_candidate();
  candidate->value = "dvd";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ＤＶＤ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBC\xA4\xEF\xBC\xB6\xEF\xBC\xA4", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "Ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBC\xA4\xEF\xBD\x96\xEF\xBD\x84", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84", command);
}

TEST_F(SessionTest, ConvertConsonantsToFullAlphanumericWithoutCascadingWindow) {
  config::Config config;
  config.set_use_cascading_window(false);
  config::ConfigHandler::SetConfig(config);

  commands::Command command;
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  InsertCharacterChars("dvd", session.get(), &command);

  segment = segments.add_segment();
  // "ｄｖｄ"
  segment->set_key("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84");
  candidate = segment->add_candidate();
  candidate->value = "DVD";
  candidate = segment->add_candidate();
  candidate->value = "dvd";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ＤＶＤ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBC\xA4\xEF\xBC\xB6\xEF\xBC\xA4", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "Ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBC\xA4\xEF\xBD\x96\xEF\xBD\x84", command);

  command.Clear();
  session->ConvertToFullASCII(&command);
  // "ｄｖｄ"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\x84\xEF\xBD\x96\xEF\xBD\x84", command);
}

// Convert input string to Hiragana, Katakana, and Half Katakana
TEST_F(SessionTest, SwitchKanaType) {
  {  // From composition mode.
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    InsertCharacterChars("abc", session.get(), &command);

    Segments segments;
    {  // Initialize segments.
      Segment *segment = segments.add_segment();
      // "あｂｃ"
      segment->set_key("\xE3\x81\x82\xEF\xBD\x82\xEF\xBD\x83");
      // "あべし"
      segment->add_candidate()->value = "\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97";
    }

    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    session->SwitchKanaType(&command);
    // "アｂｃ"
    EXPECT_SINGLE_SEGMENT("\xE3\x82\xA2\xEF\xBD\x82\xEF\xBD\x83", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "ｱbc"
    EXPECT_SINGLE_SEGMENT("\xEF\xBD\xB1\x62\x63", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "あｂｃ"
    EXPECT_SINGLE_SEGMENT("\xE3\x81\x82\xEF\xBD\x82\xEF\xBD\x83", command);

    command.Clear();
    session->SwitchKanaType(&command);
      // "アｂｃ"
    EXPECT_SINGLE_SEGMENT("\xE3\x82\xA2\xEF\xBD\x82\xEF\xBD\x83", command);
  }

  {  // From conversion mode.
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    InsertCharacterChars("kanji", session.get(), &command);

    Segments segments;
    {  // Initialize segments.
      Segment *segment = segments.add_segment();
      // "かんじ"
      segment->set_key("\xE3\x81\x8B\xE3\x82\x93\xE3\x81\x98");
      // "漢字"
      segment->add_candidate()->value = "\xE6\xBC\xA2\xE5\xAD\x97";
    }

    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    session->Convert(&command);
    // "漢字"
    EXPECT_SINGLE_SEGMENT("\xE6\xBC\xA2\xE5\xAD\x97", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "かんじ"
    EXPECT_SINGLE_SEGMENT("\xE3\x81\x8B\xE3\x82\x93\xE3\x81\x98", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "カンジ"
    EXPECT_SINGLE_SEGMENT("\xE3\x82\xAB\xE3\x83\xB3\xE3\x82\xB8", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "ｶﾝｼﾞ"
    EXPECT_SINGLE_SEGMENT(
        "\xEF\xBD\xB6\xEF\xBE\x9D\xEF\xBD\xBC\xEF\xBE\x9E", command);

    command.Clear();
    session->SwitchKanaType(&command);
    // "かんじ"
    EXPECT_SINGLE_SEGMENT("\xE3\x81\x8B\xE3\x82\x93\xE3\x81\x98", command);
  }
}

// Rotate input mode among Hiragana, Katakana, and Half Katakana
TEST_F(SessionTest, InputModeSwitchKanaType) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // HIRAGANA
  InsertCharacterChars("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());

  // HIRAGANA to FULL_KATAKANA
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeSwitchKanaType(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "ア"
  EXPECT_EQ("\xE3\x82\xA2", GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::FULL_KATAKANA, command.output().mode());

  // FULL_KATRAKANA to HALF_KATAKANA
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeSwitchKanaType(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "ｱ"
  EXPECT_EQ("\xEF\xBD\xB1",
            GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::HALF_KATAKANA, command.output().mode());

  // HALF_KATAKANA to HIRAGANA
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeSwitchKanaType(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());

  // To Half ASCII mode.
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeHalfASCII(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "a"
  EXPECT_EQ("a", GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  // HALF_ASCII to HALF_ASCII
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeSwitchKanaType(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "a"
  EXPECT_EQ("a", GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  // To Full ASCII mode.
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeFullASCII(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "ａ"
  EXPECT_EQ(kFullWidthSmallA, GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::FULL_ASCII, command.output().mode());

  // FULL_ASCII to FULL_ASCII
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->InputModeSwitchKanaType(&command);
  InsertCharacterChars("a", session.get(), &command);
  // "ａ"
  EXPECT_EQ(kFullWidthSmallA, GetComposition(command));
  EXPECT_TRUE(command.output().has_mode());
  EXPECT_EQ(commands::FULL_ASCII, command.output().mode());
}

TEST_F(SessionTest, TranslateHalfWidth) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("abc", session.get(), &command);

  command.Clear();
  session->TranslateHalfWidth(&command);
  // "ｱbc"
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\xB1\x62\x63", command);

  command.Clear();
  session->TranslateFullASCII(&command);
  // "ａｂｃ".
  EXPECT_SINGLE_SEGMENT("\xEF\xBD\x81\xEF\xBD\x82\xEF\xBD\x83", command);

  command.Clear();
  session->TranslateHalfWidth(&command);
  EXPECT_SINGLE_SEGMENT("abc", command);
}

TEST_F(SessionTest, UpdatePreferences) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);
  Segments segments;
  SetAiueo(&segments);

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);

  SetSendKeyCommand("SPACE", &command);
  command.mutable_input()->mutable_config()->set_use_cascading_window(false);
  session->SendKey(&command);
  const size_t no_cascading_cand_size =
      command.output().candidates().candidate_size();

  command.Clear();
  session->ConvertCancel(&command);

  SetSendKeyCommand("SPACE", &command);
  command.mutable_input()->mutable_config()->set_use_cascading_window(true);
  session->SendKey(&command);
  const size_t cascading_cand_size =
      command.output().candidates().candidate_size();

  EXPECT_GT(no_cascading_cand_size, cascading_cand_size);

  command.Clear();
  session->ConvertCancel(&command);

  // On MS-IME keymap, EISU key does nothing.
  SetSendKeyCommand("EISU", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MSIME);
  session->SendKey(&command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
  EXPECT_EQ(commands::HALF_ASCII, command.output().status().comeback_mode());

  // On KOTOERI keymap, EISU key does "ToggleAlphanumericMode".
  SetSendKeyCommand("EISU", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::KOTOERI);
  session->SendKey(&command);
  EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
  EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());
}

TEST_F(SessionTest, RomajiInput) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  composer::Table table;
  // "ぱ"
  table.AddRule("pa", "\xe3\x81\xb1", "");
  // "ん"
  table.AddRule("n", "\xe3\x82\x93", "");
  // "な"
  table.AddRule("na", "\xe3\x81\xaa", "");
  // This rule makes the "n" rule ambiguous.

  scoped_ptr<Session> session(new Session(engine_.get()));
  session->get_internal_composer_only_for_unittest()->SetTable(&table);
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("pan", session.get(), &command);

  // "ぱｎ"
  EXPECT_EQ("\xe3\x81\xb1\xef\xbd\x8e",
            command.output().preedit().segment(0).value());

  command.Clear();

  segment = segments.add_segment();
  // "ぱん"
  segment->set_key("\xe3\x81\xb1\xe3\x82\x93");
  candidate = segment->add_candidate();
  // "パン"
  candidate->value = "\xe3\x83\x91\xe3\x83\xb3";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  session->ConvertToHiragana(&command);
  // "ぱん"
  EXPECT_SINGLE_SEGMENT("\xe3\x81\xb1\xe3\x82\x93", command);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("pan", command);
}


TEST_F(SessionTest, KanaInput) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  composer::Table table;
  // "す゛", "ず"
  table.AddRule("\xe3\x81\x99\xe3\x82\x9b", "\xe3\x81\x9a", "");

  scoped_ptr<Session> session(new Session(engine_.get()));
  session->get_internal_composer_only_for_unittest()->SetTable(&table);
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  SetSendKeyCommand("m", &command);
  // "も"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x82\x82");
  session->SendKey(&command);

  SetSendKeyCommand("r", &command);
  // "す"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x81\x99");
  session->SendKey(&command);

  SetSendKeyCommand("@", &command);
  // "゛"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x82\x9b");
  session->SendKey(&command);

  SetSendKeyCommand("h", &command);
  // "く"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x81\x8f");
  session->SendKey(&command);

  SetSendKeyCommand("!", &command);
  command.mutable_input()->mutable_key()->set_key_string("!");
  session->SendKey(&command);

  // "もずく！"
  EXPECT_EQ("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f\xef\xbc\x81",
            command.output().preedit().segment(0).value());

  segment = segments.add_segment();
  // "もずく!"
  segment->set_key("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f!");
  candidate = segment->add_candidate();
  // "もずく！"
  candidate->value = "\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f\xef\xbc\x81";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->ConvertToHalfASCII(&command);
  EXPECT_SINGLE_SEGMENT("mr@h!", command);
}

TEST_F(SessionTest, ExceededComposition) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  const string exceeded_preedit(500, 'a');
  ASSERT_EQ(500, exceeded_preedit.size());
  InsertCharacterChars(exceeded_preedit, session.get(), &command);

  string long_a;
  for (int i = 0; i < 500; ++i) {
    // "あ"
    long_a += kHiraganaA;
  }
  segment = segments.add_segment();
  segment->set_key(long_a);
  candidate = segment->add_candidate();
  candidate->value = long_a;

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  EXPECT_FALSE(command.output().has_candidates());

  // The status should remain the preedit status, although the
  // previous command was convert.  The next command makes sure that
  // the preedit will disappear by canceling the preedit status.
  command.Clear();
  command.mutable_input()->mutable_key()->set_special_key(
      commands::KeyEvent::ESCAPE);
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, OutputAllCandidateWords) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  Segments segments;
  SetAiueo(&segments);
  InsertCharacterChars("aiueo", session.get(), &command);

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  {
    const commands::Output &output = command.output();
    EXPECT_TRUE(output.has_all_candidate_words());

    EXPECT_EQ(0, output.all_candidate_words().focused_index());
    EXPECT_EQ(commands::CONVERSION, output.all_candidate_words().category());
#ifdef OS_LINUX
    // Cascading window is not supported on Linux, so the size of
    // candidate words is different from other platform.
    // TODO(komatsu): Modify the client for Linux to explicitly change
    // the preference rather than relying on the exceptional default value.
    // [ "あいうえお", "アイウエオ",
    //   "aiueo" (t13n), "AIUEO" (t13n), "Aieuo" (t13n),
    //   "ａｉｕｅｏ"  (t13n), "ＡＩＵＥＯ" (t13n), "Ａｉｅｕｏ" (t13n),
    //   "ｱｲｳｴｵ" (t13n) ]
    EXPECT_EQ(9, output.all_candidate_words().candidates_size());
#else
    // [ "あいうえお", "アイウエオ", "アイウエオ" (t13n), "あいうえお" (t13n),
    //   "aiueo" (t13n), "AIUEO" (t13n), "Aieuo" (t13n),
    //   "ａｉｕｅｏ"  (t13n), "ＡＩＵＥＯ" (t13n), "Ａｉｅｕｏ" (t13n),
    //   "ｱｲｳｴｵ" (t13n) ]
    EXPECT_EQ(11, output.all_candidate_words().candidates_size());
#endif  // OS_LINUX
  }

  command.Clear();
  session->ConvertNext(&command);
  {
    const commands::Output &output = command.output();

    EXPECT_TRUE(output.has_all_candidate_words());

    EXPECT_EQ(1, output.all_candidate_words().focused_index());
    EXPECT_EQ(commands::CONVERSION, output.all_candidate_words().category());
#ifdef OS_LINUX
    // Cascading window is not supported on Linux, so the size of
    // candidate words is different from other platform.
    // TODO(komatsu): Modify the client for Linux to explicitly change
    // the preference rather than relying on the exceptional default value.
    // [ "あいうえお", "アイウエオ", "アイウエオ" (t13n), "あいうえお" (t13n),
    //   "aiueo" (t13n), "AIUEO" (t13n), "Aieuo" (t13n),
    //   "ａｉｕｅｏ"  (t13n), "ＡＩＵＥＯ" (t13n), "Ａｉｅｕｏ" (t13n),
    //   "ｱｲｳｴｵ" (t13n) ]
    EXPECT_EQ(9, output.all_candidate_words().candidates_size());
#else
    // [ "あいうえお", "アイウエオ",
    //   "aiueo" (t13n), "AIUEO" (t13n), "Aieuo" (t13n),
    //   "ａｉｕｅｏ"  (t13n), "ＡＩＵＥＯ" (t13n), "Ａｉｅｕｏ" (t13n),
    //   "ｱｲｳｴｵ" (t13n) ]
    EXPECT_EQ(11, output.all_candidate_words().candidates_size());
#endif  // OS_LINUX
  }
}

TEST_F(SessionTest, UndoForComposition) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  // Enable zero query suggest.
  commands::Request request;
  SetupZeroQuerySuggestionReady(true, &session, &request);

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session.set_client_capability(capability);

  commands::Command command;
  Segments segments;
  Segments empty_segments;

  {  // Undo for CommitFirstSuggestion
    SetAiueo(&segments);
    GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
    InsertCharacterChars("ai", &session, &command);
    ConversionRequest request;
    SetComposer(&session, &request);
    // "あい"
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84", GetComposition(command));

    command.Clear();
    GetConverterMock()->SetFinishConversion(&empty_segments, true);
    session.CommitFirstSuggestion(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());

    command.Clear();
    session.Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "あい"
    EXPECT_SINGLE_SEGMENT("\xE3\x81\x82\xE3\x81\x84", command);
    EXPECT_EQ(2, command.output().candidates().size());
    EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());
  }
}

TEST_F(SessionTest, RequestUndo) {
  scoped_ptr<Session> session(new Session(engine_.get()));

  // It is OK not to check ImeContext::DIRECT because you cannot
  // assign any key event to Undo command in DIRECT mode.
  // See "session/internal/keymap_interface.h".

  InitSessionToPrecomposition(session.get());
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()))
      << "When the UNDO context is empty and the context state is "
         "ImeContext::PRECOMPOSITION, UNDO command should be "
         "ignored. See b/5553298.";

  InitSessionToPrecomposition(session.get());
  SetUndoContext(session.get());
  EXPECT_TRUE(TryUndoAndAssertSuccess(session.get()));

  InitSessionToPrecomposition(session.get());
  SetUndoContext(session.get());
  session->context_->set_state(ImeContext::COMPOSITION);
  EXPECT_TRUE(TryUndoAndAssertSuccess(session.get()));

  InitSessionToPrecomposition(session.get());
  SetUndoContext(session.get());
  session->context_->set_state(ImeContext::CONVERSION);
  EXPECT_TRUE(TryUndoAndAssertSuccess(session.get()));
}

TEST_F(SessionTest, UndoForSingleSegment) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  commands::Command command;
  Segments segments;

  {  // Create segments
    InsertCharacterChars("aiueo", session.get(), &command);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    SetAiueo(&segments);
    // Don't use FillT13Ns(). It makes platform dependent segments.
    // TODO(hsumita): Makes FillT13Ns() independent from platforms.
    Segment::Candidate *candidate;
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "aiueo";
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "AIUEO";
  }

  {  // Undo after commitment of composition
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);

    // Undo twice - do nothing and keep the previous status.
    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_deletion_range());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);
  }

  {  // Undo after commitment of conversion
    command.Clear();
    session->ConvertNext(&command);
    EXPECT_FALSE(command.output().has_result());
    // "アイウエオ"
    EXPECT_PREEDIT(kKatakanaAiueo, command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "アイウエオ"
    EXPECT_RESULT(kKatakanaAiueo, command);

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "アイウエオ"
    EXPECT_PREEDIT(kKatakanaAiueo, command);

    // Undo twice - do nothing and keep the previous status.
    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_deletion_range());
    // "アイウエオ"
    EXPECT_PREEDIT(kKatakanaAiueo, command);
  }

  {  // Undo after commitment of conversion with Ctrl-Backspace.
    command.Clear();
    session->ConvertNext(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_PREEDIT("aiueo", command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_RESULT("aiueo", command);

    config::Config config;
    config.set_session_keymap(config::Config::MSIME);
    config::ConfigHandler::SetConfig(config);

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    EXPECT_PREEDIT("aiueo", command);
  }

  {
    // If capability does not support DELETE_PRECEDIGN_TEXT, Undo is not
    // performed.
    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_RESULT("aiueo", command);

    // Reset capability
    capability.Clear();
    session->set_client_capability(capability);

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_deletion_range());
    EXPECT_FALSE(command.output().has_preedit());
  }
}

TEST_F(SessionTest, ClearUndoContextByKeyEvent_Issue5529702) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  SetUndoContext(session.get());

  commands::Command command;

  // Modifier key event does not clear undo context.
  SendKey("Shift", session.get(), &command);

  // Ctrl+BS should be consumed as UNDO.
  SetSendKeyCommand("Ctrl Backspace", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MSIME);
  session->TestSendKey(&command);
  EXPECT_TRUE(command.output().consumed());

  // Any other (test) send key event clears undo context.
  TestSendKey("LEFT", session.get(), &command);
  EXPECT_FALSE(command.output().consumed());

  // Undo context is just cleared. Ctrl+BS should not be consumed b/5553298.
  SetSendKeyCommand("Ctrl Backspace", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MSIME);
  session->TestSendKey(&command);
  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, UndoForMultipleSegments) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  commands::Command command;
  Segments segments;

  {  // Create segments
    InsertCharacterChars("key1key2key3", session.get(), &command);
    ConversionRequest request;
    SetComposer(session.get(), &request);

    Segment::Candidate *candidate;
    Segment *segment;

    segment = segments.add_segment();
    segment->set_key("key1");
    candidate = segment->add_candidate();
    candidate->value = "cand1-1";
    candidate = segment->add_candidate();
    candidate->value = "cand1-2";

    segment = segments.add_segment();
    segment->set_key("key2");
    candidate = segment->add_candidate();
    candidate->value = "cand2-1";
    candidate = segment->add_candidate();
    candidate->value = "cand2-2";

    segment = segments.add_segment();
    segment->set_key("key3");
    candidate = segment->add_candidate();
    candidate->value = "cand3-1";
    candidate = segment->add_candidate();
    candidate->value = "cand3-2";
  }

  {  // Undo for CommitCandidate
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    command.mutable_input()->mutable_command()->set_id(1);
    session->CommitCandidate(&command);
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_RESULT("cand1-2", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-7, command.output().deletion_range().offset());
    EXPECT_EQ(7, command.output().deletion_range().length());
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    // Move to second segment and do the same thing.
    command.Clear();
    session->SegmentFocusRight(&command);
    command.Clear();
    command.mutable_input()->mutable_command()->set_id(1);
    session->CommitCandidate(&command);
    // "cand2-2" is focused
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_RESULT("cand1-1cand2-2", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-14, command.output().deletion_range().offset());
    EXPECT_EQ(14, command.output().deletion_range().length());
    // "cand2-1" is focused
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());
  }
  {  // Undo for CommitSegment
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->ConvertNext(&command);
    EXPECT_EQ("cand1-2cand2-1cand3-1", GetComposition(command));
    command.Clear();
    session->CommitSegment(&command);
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_RESULT("cand1-2", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-7, command.output().deletion_range().offset());
    EXPECT_EQ(7, command.output().deletion_range().length());
    EXPECT_PREEDIT("cand1-2cand2-1cand3-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    // Move to third segment and do the same thing.
    command.Clear();
    session->SegmentFocusRight(&command);
    command.Clear();
    session->SegmentFocusRight(&command);
    command.Clear();
    session->ConvertNext(&command);
    EXPECT_PREEDIT("cand1-1cand2-1cand3-2", command);
    command.Clear();
    session->CommitSegment(&command);
    // "cand3-2" is focused
    EXPECT_PREEDIT("cand1-1cand2-1cand3-1", command);
    EXPECT_RESULT("cand1-1", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-7, command.output().deletion_range().offset());
    EXPECT_EQ(7, command.output().deletion_range().length());
    // "cand3-2" is focused
    EXPECT_PREEDIT("cand1-1cand2-1cand3-2", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());
  }
}

TEST_F(SessionTest, UndoOrRewind_undo) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);


  // Commit twice.
  for (size_t i = 0; i < 2; ++i) {
    commands::Command command;
    Segments segments;
    {  // Create segments
      InsertCharacterChars("aiueo", session.get(), &command);
      ConversionRequest request;
      SetComposer(session.get(), &request);
      SetAiueo(&segments);
      Segment::Candidate *candidate;
      candidate = segments.mutable_segment(0)->add_candidate();
      candidate->value = "aiueo";
      candidate = segments.mutable_segment(0)->add_candidate();
      candidate->value = "AIUEO";
    }
    {
      GetConverterMock()->SetStartConversionForRequest(&segments, true);
      command.Clear();
      session->Convert(&command);
      EXPECT_FALSE(command.output().has_result());
      // "あいうえお"
      EXPECT_PREEDIT(kAiueo, command);

      GetConverterMock()->SetCommitSegmentValue(&segments, true);
      command.Clear();
      session->Commit(&command);
      EXPECT_FALSE(command.output().has_preedit());
      // "あいうえお"
      EXPECT_RESULT(kAiueo, command);
    }
  }
  // Try UndoOrRewind twice.
  // Second trial should not return deletation_range.
  commands::Command command;
  command.Clear();
  session->UndoOrRewind(&command);
  EXPECT_FALSE(command.output().has_result());
  // "あいうえお"
  EXPECT_PREEDIT(kAiueo, command);
  EXPECT_TRUE(command.output().has_deletion_range());
  command.Clear();
  session->UndoOrRewind(&command);
  EXPECT_FALSE(command.output().has_result());
  // "あいうえお"
  EXPECT_PREEDIT(kAiueo, command);
  EXPECT_FALSE(command.output().has_deletion_range());
}

TEST_F(SessionTest, UndoOrRewind_rewind) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get(), *mobile_request_);

  Segments segments;
  {
    segments.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments.add_segment();
    AddCandidate("e", "e", segment);
    AddCandidate("e", "E", segment);
  }
  GetConverterMock()->SetStartSuggestionForRequest(&segments, true);

  commands::Command command;
  InsertCharacterChars("11111", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  // "お"
  EXPECT_PREEDIT("\xE3\x81\x8A", command);
  EXPECT_FALSE(command.output().has_deletion_range());
  EXPECT_TRUE(command.output().has_all_candidate_words());

  command.Clear();
  session->UndoOrRewind(&command);
  EXPECT_FALSE(command.output().has_result());
  // "え"
  EXPECT_PREEDIT("\xE3\x81\x88", command);
  EXPECT_FALSE(command.output().has_deletion_range());
  EXPECT_TRUE(command.output().has_all_candidate_words());
}

TEST_F(SessionTest, CommitRawText) {
  {  // From composition mode.
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    InsertCharacterChars("abc", session.get(), &command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());

    Segments segments;
    {  // Initialize segments.
      Segment *segment = segments.add_segment();
      // "あｂｃ"
      segment->set_key("\xE3\x81\x82\xEF\xBD\x82\xEF\xBD\x83");
      // "あべし"
      segment->add_candidate()->value = "\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97";
    }

    command.Clear();
    SetSendCommandCommand(commands::SessionCommand::COMMIT_RAW_TEXT, &command);
    session->SendCommand(&command);
    // "abc"
    EXPECT_RESULT_AND_KEY("abc", "abc", command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
  }
  {  // From conversion mode.
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    InsertCharacterChars("abc", session.get(), &command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());

    Segments segments;
    {  // Initialize segments.
      Segment *segment = segments.add_segment();
      // "あｂｃ"
      segment->set_key("\xE3\x81\x82\xEF\xBD\x82\xEF\xBD\x83");
      // "あべし"
      segment->add_candidate()->value = "\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97";
    }

    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    // "あべし"
    EXPECT_PREEDIT("\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97", command);
    EXPECT_EQ(ImeContext::CONVERSION, session->context().state());

    command.Clear();
    SetSendCommandCommand(commands::SessionCommand::COMMIT_RAW_TEXT, &command);
    session->SendCommand(&command);
    // "abc"
    EXPECT_RESULT_AND_KEY("abc", "abc", command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
  }
}

TEST_F(SessionTest, CommitRawText_KanaInput) {
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  composer::Table table;
  // "す゛", "ず"
  table.AddRule("\xe3\x81\x99\xe3\x82\x9b", "\xe3\x81\x9a", "");

  scoped_ptr<Session> session(new Session(engine_.get()));
  session->get_internal_composer_only_for_unittest()->SetTable(&table);
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  SetSendKeyCommand("m", &command);
  // "も"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x82\x82");
  session->SendKey(&command);

  SetSendKeyCommand("r", &command);
  // "す"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x81\x99");
  session->SendKey(&command);

  SetSendKeyCommand("@", &command);
  // "゛"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x82\x9b");
  session->SendKey(&command);

  SetSendKeyCommand("h", &command);
  // "く"
  command.mutable_input()->mutable_key()->set_key_string("\xe3\x81\x8f");
  session->SendKey(&command);

  SetSendKeyCommand("!", &command);
  command.mutable_input()->mutable_key()->set_key_string("!");
  session->SendKey(&command);

  // "もずく！"
  EXPECT_EQ("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f\xef\xbc\x81",
            command.output().preedit().segment(0).value());

  segment = segments.add_segment();
  // "もずく!"
  segment->set_key("\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f!");
  candidate = segment->add_candidate();
  // "もずく！"
  candidate->value = "\xe3\x82\x82\xe3\x81\x9a\xe3\x81\x8f\xef\xbc\x81";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  SetSendCommandCommand(commands::SessionCommand::COMMIT_RAW_TEXT, &command);
  session->SendCommand(&command);
  // "abc"
  EXPECT_RESULT_AND_KEY("mr@h!", "mr@h!", command);
  EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
}

TEST_F(SessionTest, ConvertNextPage_PrevPage) {
  commands::Command command;
  scoped_ptr<Session> session(new Session(engine_.get()));

  InitSessionToPrecomposition(session.get());

  // Should be ignored in precomposition state.
  {
    command.Clear();
    command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
    command.mutable_input()->mutable_command()->set_type(
        commands::SessionCommand::CONVERT_NEXT_PAGE);
    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());

    command.Clear();
    command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
    command.mutable_input()->mutable_command()->set_type(
        commands::SessionCommand::CONVERT_PREV_PAGE);
    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
  }

  InsertCharacterChars("aiueo", session.get(), &command);
  EXPECT_PREEDIT(kAiueo, command);

  // Should be ignored in composition state.
  {
    command.Clear();
    command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
    command.mutable_input()->mutable_command()->set_type(
        commands::SessionCommand::CONVERT_NEXT_PAGE);
    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_PREEDIT(kAiueo, command) << "should do nothing";

    command.Clear();
    command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
    command.mutable_input()->mutable_command()->set_type(
        commands::SessionCommand::CONVERT_PREV_PAGE);
    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_PREEDIT(kAiueo, command) << "should do nothing";
  }

  // Generate sequential candidates as follows.
  //   "page0-cand0"
  //   "page0-cand1"
  //   ...
  //   "page0-cand8"
  //   "page1-cand0"
  //   ...
  //   "page1-cand8"
  //   "page2-cand0"
  //   ...
  //   "page2-cand8"
  {
    Segments segments;
    Segment *segment = NULL;
    segment = segments.add_segment();
    segment->set_key(kAiueo);
    for (int page_index = 0; page_index < 3; ++page_index) {
      for (int cand_index = 0; cand_index < 9; ++cand_index) {
        segment->add_candidate()->value = Util::StringPrintf(
            "page%d-cand%d", page_index, cand_index);
      }
    }
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  // Make sure the selected candidate changes as follows.
  //                              -> Convert
  //  -> "page0-cand0" -> SendCommand/CONVERT_NEXT_PAGE
  //  -> "page1-cand0" -> SendCommand/CONVERT_PREV_PAGE
  //  -> "page0-cand0" -> SendCommand/CONVERT_PREV_PAGE
  //  -> "page2-cand0"

  command.Clear();
  ASSERT_TRUE(session->Convert(&command));
  EXPECT_PREEDIT("page0-cand0", command);

  command.Clear();
  command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
  command.mutable_input()->mutable_command()->set_type(
      commands::SessionCommand::CONVERT_NEXT_PAGE);
  ASSERT_TRUE(session->SendCommand(&command));
  EXPECT_PREEDIT("page1-cand0", command);

  command.Clear();
  command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
  command.mutable_input()->mutable_command()->set_type(
      commands::SessionCommand::CONVERT_PREV_PAGE);
  ASSERT_TRUE(session->SendCommand(&command));
  EXPECT_PREEDIT("page0-cand0", command);

  command.Clear();
  command.mutable_input()->set_type(commands::Input::SEND_COMMAND);
  command.mutable_input()->mutable_command()->set_type(
      commands::SessionCommand::CONVERT_PREV_PAGE);
  ASSERT_TRUE(session->SendCommand(&command));
  EXPECT_PREEDIT("page2-cand0", command);
}

TEST_F(SessionTest, NeedlessClearUndoContext) {
  // This is a unittest against http://b/3423910.

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);
  commands::Command command;

  {  // Conversion -> Send Shift -> Undo
    Segments segments;
    InsertCharacterChars("aiueo", session.get(), &command);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    SetAiueo(&segments);
    FillT13Ns(request, &segments);

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);

    SendKey("Shift", session.get(), &command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_preedit());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);
  }

  {  // Type "aiueo" -> Convert -> Type "a" -> Escape -> Undo
    Segments segments;
    InsertCharacterChars("aiueo", session.get(), &command);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    SetAiueo(&segments);
    FillT13Ns(request, &segments);

    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);

    SendKey("a", session.get(), &command);
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);
    // "あ"
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

    SendKey("Escape", session.get(), &command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_preedit());

    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);
  }
}

TEST_F(SessionTest, ClearUndoContextAfterDirectInputAfterConversion) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Prepare Numpad
  config::Config config;
  config.set_numpad_character_form(config::Config::NUMPAD_DIRECT_INPUT);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::NUMPAD_DIRECT_INPUT,
            GET_CONFIG(numpad_character_form));
  // Update KeyEventTransformer
  session->ReloadConfig();

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);
  commands::Command command;

  // Cleate segments
  Segments segments;
  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);

  // Convert
  GetConverterMock()->SetStartConversionForRequest(&segments, true);
  command.Clear();
  session->Convert(&command);
  EXPECT_FALSE(command.output().has_result());
  // "あいうえお"
  EXPECT_PREEDIT(kAiueo, command);

  // Direct input
  SendKey("Numpad0", session.get(), &command);
  EXPECT_TRUE(GetComposition(command).empty());
  // "あいうえお0"
  EXPECT_RESULT(string(kAiueo) + "0", command);

  // Undo - Do NOT nothing
  command.Clear();
  session->Undo(&command);
  EXPECT_FALSE(command.output().has_result());
  EXPECT_FALSE(command.output().has_deletion_range());
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, TemporaryInputModeAfterUndo) {
  // This is a unittest against http://b/3423599.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);
  commands::Command command;

  // Shift + Ascii triggers temporary input mode switch.
  SendKey("A", session.get(), &command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  SendKey("Enter", session.get(), &command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());

  // Undo and keep temporary input mode correct
  command.Clear();
  session->Undo(&command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_PREEDIT("A", command);
  SendKey("Enter", session.get(), &command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());

  // Undo and input additional "A" with temporary input mode.
  command.Clear();
  session->Undo(&command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  SendKey("A", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  EXPECT_PREEDIT("AA", command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  // Input additional "a" with original input mode.
  SendKey("a", session.get(), &command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  EXPECT_FALSE(command.output().has_result());
  // "AAあ"
  EXPECT_PREEDIT("AA\xE3\x81\x82", command);

  // Submit and Undo
  SendKey("Enter", session.get(), &command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  command.Clear();
  session->Undo(&command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  EXPECT_FALSE(command.output().has_result());
  // "AAあ"
  EXPECT_PREEDIT("AA\xE3\x81\x82", command);

  // Input additional "Aa"
  SendKey("A", session.get(), &command);
  SendKey("a", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  // "AAあAa"
  EXPECT_PREEDIT("AA" + string(kHiraganaA) + "Aa", command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  // Submit and Undo
  SendKey("Enter", session.get(), &command);
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  command.Clear();
  session->Undo(&command);
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  EXPECT_FALSE(command.output().has_result());
  // "AAあAa"
  EXPECT_PREEDIT("AA" + string(kHiraganaA) + "Aa", command);
}

TEST_F(SessionTest, DCHECKFailureAfterUndo) {
  // This is a unittest against http://b/3437358.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);
  commands::Command command;

  InsertCharacterChars("abe", session.get(), &command);
  command.Clear();
  session->Commit(&command);
  command.Clear();
  session->Undo(&command);
  EXPECT_FALSE(command.output().has_result());
  // "あべ"
  EXPECT_PREEDIT("\xE3\x81\x82\xE3\x81\xB9", command);

  InsertCharacterChars("s", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  // "あべｓ"
  EXPECT_PREEDIT("\xE3\x81\x82\xE3\x81\xB9\xEF\xBD\x93", command);

  InsertCharacterChars("h", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  // "あべｓｈ"
  EXPECT_PREEDIT("\xE3\x81\x82\xE3\x81\xB9\xEF\xBD\x93\xEF\xBD\x88", command);

  InsertCharacterChars("i", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  // "あべし"
  EXPECT_PREEDIT("\xE3\x81\x82\xE3\x81\xB9\xE3\x81\x97", command);
}

TEST_F(SessionTest, ConvertToFullOrHalfAlphanumericAfterUndo) {
  // This is a unittest against http://b/3423592.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  Segments segments;
  SetAiueo(&segments);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);

  {  // ConvertToHalfASCII
    commands::Command command;
    InsertCharacterChars("aiueo", session.get(), &command);

    SendKey("Enter", session.get(), &command);
    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_EQ(kAiueo, GetComposition(command));

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->ConvertToHalfASCII(&command);
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_preedit());
    EXPECT_EQ("aiueo", GetComposition(command));
  }

  {  // ConvertToFullASCII
    commands::Command command;
    InsertCharacterChars("aiueo", session.get(), &command);

    SendKey("Enter", session.get(), &command);
    command.Clear();
    session->Undo(&command);
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_EQ(kAiueo, GetComposition(command));

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->ConvertToFullASCII(&command);
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_preedit());
    // "ａｉｕｅｏ"
    EXPECT_EQ("\xEF\xBD\x81\xEF\xBD\x89\xEF\xBD\x95\xEF\xBD\x85\xEF\xBD\x8F",
              GetComposition(command));
  }
}

TEST_F(SessionTest, ComposeVoicedSoundMarkAfterUndo_Issue5369632) {
  // This is a unittest against http://b/5369632.
  config::Config config;
  config.set_preedit_method(config::Config::KANA);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::KANA, GET_CONFIG(preedit_method));

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  commands::Command command;

  // "ち"
  InsertCharacterCodeAndString('a', "\xE3\x81\xA1", session.get(), &command);
  // "ち"
  EXPECT_EQ("\xE3\x81\xA1", GetComposition(command));

  SendKey("Enter", session.get(), &command);
  command.Clear();
  session->Undo(&command);

  EXPECT_FALSE(command.output().has_result());
  ASSERT_TRUE(command.output().has_preedit());
  // "ち"
  EXPECT_EQ("\xE3\x81\xA1", GetComposition(command));

  // "゛"
  InsertCharacterCodeAndString('@', "\xE3\x82\x9B", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
  ASSERT_TRUE(command.output().has_preedit());
  // "ぢ"
  EXPECT_EQ("\xE3\x81\xA2", GetComposition(command));
}

TEST_F(SessionTest, SpaceOnAlphanumeric) {
  commands::Request request;
  commands::Command command;

  {
    request.set_space_on_alphanumeric(commands::Request::COMMIT);

    Session session(engine_.get());
    InitSessionToPrecomposition(&session, request);

    SendKey("A", &session, &command);
    EXPECT_EQ("A", GetComposition(command));

    SendKey("Space", &session, &command);
    EXPECT_RESULT("A ", command);
  }

  {
    request.set_space_on_alphanumeric(
        commands::Request::SPACE_OR_CONVERT_COMMITING_COMPOSITION);

    Session session(engine_.get());
    InitSessionToPrecomposition(&session, request);

    SendKey("A", &session, &command);
    EXPECT_EQ("A", GetComposition(command));

    SendKey("Space", &session, &command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("A ", GetComposition(command));

    SendKey("a", &session, &command);
    EXPECT_RESULT("A ", command);
    // "あ"
    EXPECT_EQ(kHiraganaA, GetComposition(command));
  }

  {
    request.set_space_on_alphanumeric(
        commands::Request::SPACE_OR_CONVERT_KEEPING_COMPOSITION);

    Session session(engine_.get());
    InitSessionToPrecomposition(&session, request);

    SendKey("A", &session, &command);
    EXPECT_EQ("A", GetComposition(command));

    SendKey("Space", &session, &command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("A ", GetComposition(command));

    SendKey("a", &session, &command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("A a", GetComposition(command));
  }
}

TEST_F(SessionTest, Issue1805239) {
  // This is a unittest against http://b/1805239.
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("watasinonamae", session.get(), &command);

  segment = segments.add_segment();
  // "わたしの"
  segment->set_key("\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae");
  candidate = segment->add_candidate();
  // "私の"
  candidate->value = "\xe7\xa7\x81\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "渡しの"
  candidate->value = "\xe6\xb8\xa1\xe3\x81\x97\xe3\x81\xae";
  segment = segments.add_segment();
  // "名前"
  segment->set_key("\xe5\x90\x8d\xe5\x89\x8d");
  candidate = segment->add_candidate();
  // "なまえ"
  candidate->value = "\xe3\x81\xaa\xe3\x81\xbe\xe3\x81\x88";
  candidate = segment->add_candidate();
  // "ナマエ"
  candidate->value = "\xe3\x83\x8a\xe3\x83\x9e\xe3\x82\xa8";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  EXPECT_FALSE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  EXPECT_FALSE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());
}

TEST_F(SessionTest, Issue1816861) {
  // This is a unittest against http://b/1816861
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("kamabokonoinbou", session.get(), &command);
  segment = segments.add_segment();
  // "かまぼこの"
  segment->set_key(
      "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae");
  candidate = segment->add_candidate();
  // "かまぼこの"
  candidate->value
      = "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae";
  candidate = segment->add_candidate();
  // "カマボコの"
  candidate->value
      = "\xe3\x82\xab\xe3\x83\x9e\xe3\x83\x9c\xe3\x82\xb3\xe3\x81\xae";
  segment = segments.add_segment();
  // "いんぼう"
  segment->set_key("\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc\xe3\x81\x86");
  candidate = segment->add_candidate();
  // "陰謀"
  candidate->value = "\xe9\x99\xb0\xe8\xac\x80";
  candidate = segment->add_candidate();
  // "印房"
  candidate->value = "\xe5\x8d\xb0\xe6\x88\xbf";

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);
  SendSpecialKey(commands::KeyEvent::BACKSPACE, session.get(), &command);

  segments.Clear();
  segment = segments.add_segment();
  // "いんぼう"
  segment->set_key("\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc\xe3\x81\x86");
  candidate = segment->add_candidate();
  // "陰謀"
  candidate->value = "\xe9\x99\xb0\xe8\xac\x80";
  candidate = segment->add_candidate();
  // "陰謀論"
  candidate->value = "\xe9\x99\xb0\xe8\xac\x80\xe8\xab\x96";
  candidate = segment->add_candidate();
  // "陰謀説"
  candidate->value = "\xe9\x99\xb0\xe8\xac\x80\xe8\xaa\xac";

  GetConverterMock()->SetStartPredictionForRequest(&segments, true);

  SendSpecialKey(commands::KeyEvent::TAB, session.get(), &command);
}

TEST_F(SessionTest, T13NWithResegmentation) {
  // This is a unittest against http://b/3272827
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("kamabokonoinbou", session.get(), &command);

  {
    Segments segments;
    Segment *segment;
    segment = segments.add_segment();
    // "かまぼこの"
    segment->set_key(
        "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae");
    candidate = segment->add_candidate();
    // "かまぼこの"
    candidate->value
        = "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae";
    candidate = segment->add_candidate();
    // "カマボコの"
    candidate->value
        = "\xe3\x82\xab\xe3\x83\x9e\xe3\x83\x9c\xe3\x82\xb3\xe3\x81\xae";

    segment = segments.add_segment();
    // "いんぼう"
    segment->set_key("\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc\xe3\x81\x86");
    candidate = segment->add_candidate();
    // "陰謀"
    candidate->value = "\xe9\x99\xb0\xe8\xac\x80";
    candidate = segment->add_candidate();
    // "印房"
    candidate->value = "\xe5\x8d\xb0\xe6\x88\xbf";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }
  {
    Segments segments;
    Segment *segment;
    segment = segments.add_segment();
    // "かまぼこの"
    segment->set_key(
        "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae");
    candidate = segment->add_candidate();
    // "かまぼこの"
    candidate->value
        = "\xe3\x81\x8b\xe3\x81\xbe\xe3\x81\xbc\xe3\x81\x93\xe3\x81\xae";
    candidate = segment->add_candidate();
    // "カマボコの"
    candidate->value
        = "\xe3\x82\xab\xe3\x83\x9e\xe3\x83\x9c\xe3\x82\xb3\xe3\x81\xae";

    segment = segments.add_segment();
    // "いんぼ"
    segment->set_key("\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc");
    candidate = segment->add_candidate();
    // "いんぼ"
    candidate->value = "\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc";
    candidate = segment->add_candidate();
    // "インボ"
    candidate->value = "\xe3\x82\xa4\xe3\x83\xb3\xe3\x83\x9c";

    segment = segments.add_segment();
    // "う"
    segment->set_key("\xe3\x81\x86");
    candidate = segment->add_candidate();
    // "ウ"
    candidate->value = "\xe3\x82\xa6";
    candidate = segment->add_candidate();
    // "卯"
    candidate->value = "\xe5\x8d\xaf";

    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetResizeSegment1(&segments, true);
  }

  // Start conversion
  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  // Select second segment
  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  // Shrink segment
  SendKey("Shift left", session.get(), &command);
  // Convert to T13N (Half katakana)
  SendKey("F8", session.get(), &command);

  // "ｲﾝﾎﾞ"
  EXPECT_EQ("\xef\xbd\xb2\xef\xbe\x9d\xef\xbe\x8e\xef\xbe\x9e",
            command.output().preedit().segment(1).value());
}

TEST_F(SessionTest, Shortcut) {
  const config::Config::SelectionShortcut kDataShortcut[] = {
    config::Config::NO_SHORTCUT,
    config::Config::SHORTCUT_123456789,
    config::Config::SHORTCUT_ASDFGHJKL,
  };
  const string kDataExpected[][2] = {
    {"", ""},
    {"1", "2"},
    {"a", "s"},
  };
  for (size_t i = 0; i < arraysize(kDataShortcut); ++i) {
    config::Config::SelectionShortcut shortcut = kDataShortcut[i];
    const string *expected = kDataExpected[i];

    config::Config config;
    config.set_selection_shortcut(shortcut);
    config::ConfigHandler::SetConfig(config);
    ASSERT_EQ(shortcut, GET_CONFIG(selection_shortcut));

    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    Segments segments;
    SetAiueo(&segments);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    commands::Command command;
    InsertCharacterChars("aiueo", session.get(), &command);

    command.Clear();
    session->Convert(&command);

    command.Clear();
    // Convert next
    SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
    ASSERT_TRUE(command.output().has_candidates());
    const commands::Candidates &candidates = command.output().candidates();
    EXPECT_EQ(expected[0], candidates.candidate(0).annotation().shortcut());
    EXPECT_EQ(expected[1], candidates.candidate(1).annotation().shortcut());
  }
}

TEST_F(SessionTest, ShortcutWithCapsLock_Issue5655743) {
  config::Config config;
  config.set_selection_shortcut(config::Config::SHORTCUT_ASDFGHJKL);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::SHORTCUT_ASDFGHJKL,
            GET_CONFIG(selection_shortcut));

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  Segments segments;
  SetAiueo(&segments);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);

  command.Clear();
  session->Convert(&command);

  command.Clear();
  // Convert next
  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());

  const commands::Candidates &candidates = command.output().candidates();
  EXPECT_EQ("a", candidates.candidate(0).annotation().shortcut());
  EXPECT_EQ("s", candidates.candidate(1).annotation().shortcut());

  // Select the second candidate by 's' key when the CapsLock is enabled.
  // Note that "CAPS S" means that 's' key is pressed w/o shift key.
  // See the description in command.proto.
  EXPECT_TRUE(SendKey("CAPS S", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // "アイウエオ"
  EXPECT_EQ("\xE3\x82\xA2\xE3\x82\xA4\xE3\x82\xA6\xE3\x82\xA8\xE3\x82\xAA",
            GetComposition(command));
}

TEST_F(SessionTest, NumpadKey) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  config::Config config;
  config.set_numpad_character_form(config::Config::NUMPAD_DIRECT_INPUT);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::NUMPAD_DIRECT_INPUT,
            GET_CONFIG(numpad_character_form));
  // Update KeyEventTransformer
  session->ReloadConfig();

  // In the Precomposition state, numpad keys should not be consumed.
  EXPECT_TRUE(TestSendKey("Numpad1", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(SendKey("Numpad1", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());

  EXPECT_TRUE(TestSendKey("Add", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(SendKey("Add", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());

  EXPECT_TRUE(TestSendKey("Equals", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(SendKey("Equals", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());

  EXPECT_TRUE(TestSendKey("Separator", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(SendKey("Separator", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());

  EXPECT_TRUE(GetComposition(command).empty());

  config.set_numpad_character_form(config::Config::NUMPAD_HALF_WIDTH);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::NUMPAD_HALF_WIDTH,
            GET_CONFIG(numpad_character_form));
  // Update KeyEventTransformer
  session->ReloadConfig();

  // In the Precomposition state, numpad keys should not be consumed.
  EXPECT_TRUE(TestSendKey("Numpad1", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_TRUE(SendKey("Numpad1", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ("1", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Add", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_TRUE(SendKey("Add", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ("1+", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Equals", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_TRUE(SendKey("Equals", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ("1+=", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Separator", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_TRUE(SendKey("Separator", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  EXPECT_TRUE(GetComposition(command).empty());

  // "0" should be treated as full-width "０".
  EXPECT_TRUE(TestSendKey("0", session.get(), &command));
  EXPECT_TRUE(SendKey("0", session.get(), &command));

  // "０"
  EXPECT_SINGLE_SEGMENT_AND_KEY("\xEF\xBC\x90", "\xEF\xBC\x90", command);

  // In the Composition state, DIVIDE on the pre-edit should be treated as "/".
  EXPECT_TRUE(TestSendKey("Divide", session.get(), &command));
  EXPECT_TRUE(SendKey("Divide", session.get(), &command));

  // "０/"
  EXPECT_SINGLE_SEGMENT_AND_KEY(
      "\xEF\xBC\x90\x2F", "\xEF\xBC\x90\x2F", command);

  // In the Composition state, "Numpad0" should be treated as half-width "0".
  EXPECT_TRUE(SendKey("Numpad0", session.get(), &command));

  // "０/0"
  EXPECT_SINGLE_SEGMENT_AND_KEY(
      "\xEF\xBC\x90\x2F\x30", "\xEF\xBC\x90\x2F\x30", command);

  // Separator should be treated as Enter.
  EXPECT_TRUE(TestSendKey("Separator", session.get(), &command));
  EXPECT_TRUE(SendKey("Separator", session.get(), &command));

  EXPECT_FALSE(command.output().has_preedit());
  // "０/0"
  EXPECT_RESULT("\xEF\xBC\x90\x2F\x30", command);

  // http://b/2097087
  EXPECT_TRUE(SendKey("0", session.get(), &command));

  // "０"
  EXPECT_SINGLE_SEGMENT_AND_KEY("\xEF\xBC\x90", "\xEF\xBC\x90", command);

  EXPECT_TRUE(SendKey("Divide", session.get(), &command));
  // "０/"
  EXPECT_SINGLE_SEGMENT_AND_KEY(
      "\xEF\xBC\x90\x2F", "\xEF\xBC\x90\x2F", command);

  EXPECT_TRUE(SendKey("Divide", session.get(), &command));
  // "０//"
  EXPECT_SINGLE_SEGMENT_AND_KEY(
      "\xEF\xBC\x90\x2F\x2F", "\xEF\xBC\x90\x2F\x2F", command);

  EXPECT_TRUE(SendKey("Subtract", session.get(), &command));
  EXPECT_TRUE(SendKey("Subtract", session.get(), &command));
  EXPECT_TRUE(SendKey("Decimal", session.get(), &command));
  EXPECT_TRUE(SendKey("Decimal", session.get(), &command));
  // "０//--.."
  EXPECT_SINGLE_SEGMENT_AND_KEY(
      "\xEF\xBC\x90\x2F\x2F\x2D\x2D\x2E\x2E",
      "\xEF\xBC\x90\x2F\x2F\x2D\x2D\x2E\x2E", command);
}

TEST_F(SessionTest, KanaSymbols) {
  config::Config config;
  config.set_punctuation_method(config::Config::COMMA_PERIOD);
  config.set_symbol_method(config::Config::CORNER_BRACKET_SLASH);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::COMMA_PERIOD, GET_CONFIG(punctuation_method));
  ASSERT_EQ(config::Config::CORNER_BRACKET_SLASH, GET_CONFIG(symbol_method));

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  {
    commands::Command command;
    SetSendKeyCommand("<", &command);
    // "、"
    command.mutable_input()->mutable_key()->set_key_string("\xe3\x80\x81");
    EXPECT_TRUE(session->SendKey(&command));
    // "，"
    EXPECT_EQ(static_cast<uint32>(','), command.input().key().key_code());
    EXPECT_EQ("\xef\xbc\x8c", command.input().key().key_string());
    EXPECT_EQ("\xef\xbc\x8c", command.output().preedit().segment(0).value());
  }
  {
    commands::Command command;
    session->EditCancel(&command);
  }
  {
    commands::Command command;
    SetSendKeyCommand("?", &command);
    // "・"
    command.mutable_input()->mutable_key()->set_key_string("\xe3\x83\xbb");
    EXPECT_TRUE(session->SendKey(&command));
    // "／"
    EXPECT_EQ(static_cast<uint32>('/'), command.input().key().key_code());
    EXPECT_EQ("\xef\xbc\x8f", command.input().key().key_string());
    EXPECT_EQ("\xef\xbc\x8f", command.output().preedit().segment(0).value());
  }
}

TEST_F(SessionTest, InsertCharacterWithShiftKey) {
  {  // Basic behavior
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(SendKey("A", session.get(), &command));  // "あA"
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あAa"
    // Shift reverts the input mode to Hiragana.
    EXPECT_TRUE(SendKey("Shift", session.get(), &command));
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あAaあ"
    // Shift does nothing because the input mode has already been reverted.
    EXPECT_TRUE(SendKey("Shift", session.get(), &command));
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あAaああ"
    // "あAaああ"
    EXPECT_EQ("\xE3\x81\x82\x41\x61\xE3\x81\x82\xE3\x81\x82",
              GetComposition(command));
  }

  {  // Revert back to the previous input mode.
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    session->InputModeFullKatakana(&command);
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().mode());
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(SendKey("A", session.get(), &command));  // "アA"
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "アAa"
    // Shift reverts the input mode to Hiragana.
    EXPECT_TRUE(SendKey("Shift", session.get(), &command));
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "アAaア"
    // Shift does nothing because the input mode has already been reverted.
    EXPECT_TRUE(SendKey("Shift", session.get(), &command));
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "アAaアア"
    // "アAaアア"
    EXPECT_EQ("\xE3\x82\xA2\x41\x61\xE3\x82\xA2\xE3\x82\xA2",
              GetComposition(command));
  }
}

TEST_F(SessionTest, ExitTemporaryAlphanumModeAfterCommitingSugesstion) {
  // This is a unittest against http://b/2977131.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("N", session.get(), &command));
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    Segments segments;
    Segment *segment = segments.add_segment();
    segment->set_key("NFL");
    segment->add_candidate()->value = "NFL";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    EXPECT_TRUE(session->Convert(&command));
    EXPECT_FALSE(command.output().has_candidates());
    EXPECT_FALSE(command.output().candidates().has_focused_index());
    EXPECT_EQ(0, command.output().candidates().focused_index());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_FALSE(command.output().has_candidates());
    EXPECT_RESULT("NFL", command);
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("N", session.get(), &command));
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    Segments segments;
    Segment *segment = segments.add_segment();
    segment->set_key("NFL");
    segment->add_candidate()->value = "NFL";
    GetConverterMock()->SetStartPredictionForRequest(&segments, true);

    EXPECT_TRUE(session->PredictAndConvert(&command));
    ASSERT_TRUE(command.output().has_candidates());
    EXPECT_TRUE(command.output().candidates().has_focused_index());
    EXPECT_EQ(0, command.output().candidates().focused_index());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_FALSE(command.output().has_candidates());
    EXPECT_RESULT("NFL", command);

    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("N", session.get(), &command));
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    Segments segments;
    Segment *segment = segments.add_segment();
    segment->set_key("NFL");
    segment->add_candidate()->value = "NFL";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    EXPECT_TRUE(session->ConvertToHalfASCII(&command));
    EXPECT_FALSE(command.output().has_candidates());
    EXPECT_FALSE(command.output().candidates().has_focused_index());
    EXPECT_EQ(0, command.output().candidates().focused_index());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_FALSE(command.output().has_candidates());
    EXPECT_RESULT("NFL", command);
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());
  }
}

TEST_F(SessionTest, StatusOutput) {
  {  // Basic behavior
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あ"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    // command.output().mode() is going to be obsolete.
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("A", session.get(), &command));  // "あA"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あAa"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    // Shift reverts the input mode to Hiragana.
    EXPECT_TRUE(SendKey("Shift", session.get(), &command));
    EXPECT_TRUE(SendKey("a", session.get(), &command));  // "あAaあ"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("A", session.get(), &command));  // "あAaあA"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as HIRAGANA
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());

    // When the IME is deactivated, the temporary composition mode is reset.
    EXPECT_TRUE(SendKey("OFF", session.get(), &command));  // "あAaあA"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
    // command.output().mode() always returns DIRECT when IME is
    // deactivated.  This is the reason why command.output().mode() is
    // going to be obsolete.
    EXPECT_EQ(commands::DIRECT, command.output().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().comeback_mode());
  }

  {  // Katakana mode + Shift key
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    session->InputModeFullKatakana(&command);
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
    EXPECT_EQ(commands::FULL_KATAKANA,
              command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().mode());  // obsolete
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
    EXPECT_EQ(commands::FULL_KATAKANA,
              command.output().status().comeback_mode());

    EXPECT_TRUE(SendKey("A", session.get(), &command));  // "アA"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());  // obsolete
    EXPECT_EQ(commands::HALF_ASCII, command.output().status().mode());
    // Global mode should be kept as FULL_KATAKANA
    EXPECT_EQ(commands::FULL_KATAKANA,
              command.output().status().comeback_mode());

    // When the IME is deactivated, the temporary composition mode is reset.
    EXPECT_TRUE(SendKey("OFF", session.get(), &command));  // "アA"
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
    // command.output().mode() always returns DIRECT when IME is
    // deactivated.  This is the reason why command.output().mode() is
    // going to be obsolete.
    EXPECT_EQ(commands::DIRECT, command.output().mode());
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
    EXPECT_EQ(commands::FULL_KATAKANA,
              command.output().status().comeback_mode());
  }
}

TEST_F(SessionTest, Suggest) {
  Segments segments_m;
  {
    segments_m.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_m.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_moz;
  {
    segments_moz.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_moz.add_segment();
    segment->set_key("MOZ");
    segment->add_candidate()->value = "MOZUKU";
  }

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  SendKey("M", session.get(), &command);

  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // moz|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_moz, true);
  SendKey("Z", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(1, command.output().candidates().candidate_size());
  EXPECT_EQ("MOZUKU", command.output().candidates().candidate(0).value());

  // mo|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("Backspace", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // m|o
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorLeft(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // mo|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorToEnd(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // |mo
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorToBeginning(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // m|o
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorRight(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  // m|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);
  command.Clear();
  EXPECT_TRUE(session->Delete(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  Segments segments_m_conv;
  {
    segments_m_conv.set_request_type(Segments::CONVERSION);
    Segment *segment;
    segment = segments_m_conv.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "M";
    segment->add_candidate()->value = "m";
  }
  ConversionRequest request_m_conv;
  SetComposer(session.get(), &request_m_conv);
  FillT13Ns(request_m_conv, &segments_m_conv);
  GetConverterMock()->SetStartConversionForRequest(&segments_m_conv, true);
  command.Clear();
  EXPECT_TRUE(session->Convert(&command));

  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);
  command.Clear();
  EXPECT_TRUE(session->ConvertCancel(&command));
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());
}

TEST_F(SessionTest, ExpandSuggestion) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // Prepare suggestion candidates.
  Segments segments_m;
  {
    segments_m.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_m.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }
  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);

  SendKey("M", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());

  // Prepare expanded suggestion candidates.
  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOZUKU";
    segment->add_candidate()->value = "MOZUKUSU";
  }
  GetConverterMock()->SetStartPredictionForRequest(&segments_mo, true);

  command.Clear();
  EXPECT_TRUE(session->ExpandSuggestion(&command));
  ASSERT_TRUE(command.output().has_candidates());
  // 3 == MOCHA, MOZUKU and MOZUKUSU (duplicate MOZUKU is not counted).
  EXPECT_EQ(3, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());
}

TEST_F(SessionTest, ExpandSuggestionDirectMode) {
  // On direct mode, ExpandSuggestion() should do nothing.
  scoped_ptr<Session> session(new Session(engine_.get()));
  commands::Command command;

  session->IMEOff(&command);
  EXPECT_TRUE(session->ExpandSuggestion(&command));
  ASSERT_FALSE(command.output().has_candidates());

  // This test expects that ConverterInterface.StartPrediction() is not called
  // so SetStartPredictionForRequest() is not called.
}

TEST_F(SessionTest, ExpandSuggestionConversionMode) {
  // On conversion mode, ExpandSuggestion() should do nothing.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  InsertCharacterChars("aiueo", session.get(), &command);
  Segments segments;
  ConversionRequest request;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  command.Clear();
  session->ConvertNext(&command);

  EXPECT_TRUE(session->ExpandSuggestion(&command));

  // This test expects that ConverterInterface.StartPrediction() is not called
  // so SetStartPredictionForRequest() is not called.
}

TEST_F(SessionTest, CommitCandidate_TypingCorrection) {
  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(Request::QWERTY_MOBILE_TO_HIRAGANA);

  Segments segments_jueri;
  segments_jueri.set_request_type(Segments::PARTIAL_SUGGESTION);
  Segment *segment;
  segment = segments_jueri.add_segment();
  // "じゅえり"
  const char kJueri[] = "\xE3\x81\x98\xE3\x82\x85\xE3\x81\x88\xE3\x82\x8A";
  segment->set_key(kJueri);
  Segment::Candidate *candidate = segment->add_candidate();
  candidate->key = "\xE3\x81\x8F\xE3\x81\x88\xE3\x82\x8A";  // "くえり"
  candidate->content_key = candidate->key;
  candidate->value = "\xE3\x82\xAF\xE3\x82\xA8\xE3\x83\xAA";  // "クエリ"
  candidate->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
  candidate->consumed_key_size = Util::CharsLen(kJueri);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get(), request);

  commands::Command command;
  GetConverterMock()->SetStartSuggestionForRequest(&segments_jueri, true);
  InsertCharacterChars("jueri", session.get(), &command);

  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(1, command.output().preedit().segment_size());
  EXPECT_EQ(kJueri,
            command.output().preedit().segment(0).key());
  EXPECT_EQ(1, command.output().candidates().candidate_size());
  EXPECT_EQ("\xE3\x82\xAF\xE3\x82\xA8\xE3\x83\xAA",  // "クエリ"
            command.output().candidates().candidate(0).value());

  // commit partial suggestion
  Segments empty_segments;
  GetConverterMock()->SetFinishConversion(&empty_segments, true);
  SetSendCommandCommand(commands::SessionCommand::SUBMIT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(0);
  GetConverterMock()->SetStartSuggestionForRequest(&segments_jueri, true);
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  // "クエリ", "くえり"
  EXPECT_RESULT_AND_KEY(
      "\xE3\x82\xAF\xE3\x82\xA8\xE3\x83\xAA",
      "\xE3\x81\x8F\xE3\x81\x88\xE3\x82\x8A", command);
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, MobilePartialSuggestion) {
  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(
      commands::Request::QWERTY_MOBILE_TO_HIRAGANA);

  Segments segments_wata;
  {
    segments_wata.set_request_type(Segments::PARTIAL_SUGGESTION);
    Segment *segment;
    segment = segments_wata.add_segment();
    // "わた"
    const char kWata[] = "\xe3\x82\x8f\xe3\x81\x9f";
    segment->set_key(kWata);
    Segment::Candidate *cand1 = AddCandidate(kWata,
                                     "\xe7\xb6\xbf", segment);  // "綿"
    cand1->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
    cand1->consumed_key_size = Util::CharsLen(kWata);
    Segment::Candidate *cand2 = AddCandidate(kWata, kWata, segment);
    cand2->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
    cand2->consumed_key_size = Util::CharsLen(kWata);
  }

  Segments segments_watashino;
  {
    segments_watashino.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_watashino.add_segment();
    // "わたしの"
    const char kWatashino[] =
        "\xe3\x82\x8f\xe3\x81\x9f\xe3\x81\x97\xe3\x81\xae";
    segment->set_key(kWatashino);
    Segment::Candidate *cand1 = segment->add_candidate();
    cand1->value = "\xe7\xa7\x81\xe3\x81\xae";  //  "私の";
    cand1->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
    cand1->consumed_key_size = Util::CharsLen(kWatashino);
    Segment::Candidate *cand2 = segment->add_candidate();
    cand2->value = kWatashino;
    cand2->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
    cand2->consumed_key_size = Util::CharsLen(kWatashino);
  }

  Segments segments_shino;
  {
    segments_shino.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_shino.add_segment();
    // "しの"
    const char kShino[] = "\xe3\x81\x97\xe3\x81\xae";
    segment->set_key(kShino);
    Segment::Candidate *candidate;
    candidate = AddCandidate(
        "\xe3\x81\x97\xe3\x81\xae\xe3\x81\xbf\xe3\x82\x84",  // "しのみや"
        "\xe5\x9b\x9b\xe3\x83\x8e\xe5\xae\xae", segment);  // "四ノ宮"
    candidate->content_key = segment->key();
    candidate->attributes = Segment::Candidate::PARTIALLY_KEY_CONSUMED;
    candidate->consumed_key_size = Util::CharsLen(kShino);
    candidate = AddCandidate(kShino, "shino", segment);
  }

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get(), request);

  commands::Command command;
  GetConverterMock()->SetStartSuggestionForRequest(&segments_watashino, true);
  InsertCharacterChars("watashino", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  // "私の"
  EXPECT_EQ("\xe7\xa7\x81\xe3\x81\xae",
            command.output().candidates().candidate(0).value());

  // partial suggestion for "わた|しの"
  GetConverterMock()->SetStartPartialSuggestion(&segments_wata, false);
  GetConverterMock()->SetStartPartialSuggestionForRequest(&segments_wata, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorLeft(&command));
  command.Clear();
  EXPECT_TRUE(session->MoveCursorLeft(&command));
  // partial suggestion candidates
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  // "綿"
  EXPECT_EQ("\xe7\xb6\xbf", command.output().candidates().candidate(0).value());

  // commit partial suggestion
  SetSendCommandCommand(commands::SessionCommand::SUBMIT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(0);
  GetConverterMock()->SetStartSuggestionForRequest(&segments_shino, true);
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  // "綿", "わた"
  EXPECT_RESULT_AND_KEY("\xe7\xb6\xbf", "\xe3\x82\x8f\xe3\x81\x9f", command);

  // remaining text in preedit
  EXPECT_EQ(2, command.output().preedit().cursor());
  // "しの"
  EXPECT_SINGLE_SEGMENT("\xe3\x81\x97\xe3\x81\xae", command);

  // Suggestion for new text fills the candidates.
  EXPECT_TRUE(command.output().has_candidates());
  // "四ノ宮"
  EXPECT_EQ("\xe5\x9b\x9b\xe3\x83\x8e\xe5\xae\xae",
      command.output().candidates().candidate(0).value());
}

TEST_F(SessionTest, ToggleAlphanumericMode) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  {
    InsertCharacterChars("a", session.get(), &command);
    // "あ"
    EXPECT_EQ(kHiraganaA, GetComposition(command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    command.Clear();
    session->ToggleAlphanumericMode(&command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
    InsertCharacterChars("a", session.get(), &command);
    // "あa"
    EXPECT_EQ("\xE3\x81\x82\x61", GetComposition(command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    command.Clear();
    session->ToggleAlphanumericMode(&command);
    InsertCharacterChars("a", session.get(), &command);
    // "あaあ"
    EXPECT_EQ("\xE3\x81\x82\x61\xE3\x81\x82", GetComposition(command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }

  {
    // ToggleAlphanumericMode on Precomposition mode should work.
    command.Clear();
    session->EditCancel(&command);
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    session->ToggleAlphanumericMode(&command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
    InsertCharacterChars("a", session.get(), &command);
    EXPECT_EQ("a", GetComposition(command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  }

  {
    // A single "n" on Hiragana mode should not converted to "ん" for
    // the compatibility with MS-IME.
    command.Clear();
    session->EditCancel(&command);
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    session->ToggleAlphanumericMode(&command);
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
    InsertCharacterChars("n", session.get(), &command);  // on Hiragana mode
    // "ｎ"
    EXPECT_EQ("\xEF\xBD\x8E", GetComposition(command));

    command.Clear();
    session->ToggleAlphanumericMode(&command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
    InsertCharacterChars("a", session.get(), &command);  // on Half ascii mode.
    // "ｎa"
    EXPECT_EQ("\xEF\xBD\x8E\x61", GetComposition(command));
  }

  {
    // ToggleAlphanumericMode should work even when it is called in
    // the conversion state.
    command.Clear();
    session->EditCancel(&command);
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    session->InputModeHiragana(&command);
    InsertCharacterChars("a", session.get(), &command);  // on Hiragana mode
    // "あ"
    EXPECT_EQ(kHiraganaA, GetComposition(command));

    Segments segments;
    SetAiueo(&segments);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    session->Convert(&command);

    // "あいうえお"
    EXPECT_EQ(kAiueo, GetComposition(command));

    command.Clear();
    session->ToggleAlphanumericMode(&command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    command.Clear();
    session->Commit(&command);

    InsertCharacterChars("a", session.get(), &command);  // on Half ascii mode.
    EXPECT_EQ("a", GetComposition(command));
  }
}

TEST_F(SessionTest, InsertSpace) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  commands::KeyEvent space_key;
  space_key.set_special_key(commands::KeyEvent::SPACE);

  // Default should be FULL_WIDTH.
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpace(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  // "　" (full-width space)
  EXPECT_RESULT(kFullWidthSpace, command);

  // Change the setting to HALF_WIDTH.
  config::Config config;
  config.set_space_character_form(config::Config::FUNDAMENTAL_HALF_WIDTH);
  config::ConfigHandler::SetConfig(config);
  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpace(&command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  // Change the setting to FULL_WIDTH.
  config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
  config::ConfigHandler::SetConfig(config);
  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpace(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  // "　" (full-width space)
  EXPECT_RESULT(kFullWidthSpace, command);
}

TEST_F(SessionTest, InsertSpaceToggled) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  commands::KeyEvent space_key;
  space_key.set_special_key(commands::KeyEvent::SPACE);

  // Default should be FULL_WIDTH.  So the toggled space should be
  // half-width.
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceToggled(&command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  // Change the setting to HALF_WIDTH.
  config::Config config;
  config.set_space_character_form(config::Config::FUNDAMENTAL_HALF_WIDTH);
  config::ConfigHandler::SetConfig(config);
  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceToggled(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  // "　" (full-width space)
  EXPECT_RESULT(kFullWidthSpace, command);

  // Change the setting to FULL_WIDTH.
  config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
  config::ConfigHandler::SetConfig(config);
  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceToggled(&command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, InsertSpaceHalfWidth) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  commands::KeyEvent space_key;
  space_key.set_special_key(commands::KeyEvent::SPACE);

  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceHalfWidth(&command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  EXPECT_TRUE(SendKey("a", session.get(), &command));
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  command.Clear();
  EXPECT_TRUE(session->InsertSpaceHalfWidth(&command));
  // "あ "
  EXPECT_EQ("\xE3\x81\x82\x20", GetComposition(command));

  {  // Convert "あ " with dummy conversions.
    Segments segments;
    // "亜 "
    segments.add_segment()->add_candidate()->value = "\xE4\xBA\x9C\x20";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    EXPECT_TRUE(session->Convert(&command));
  }

  command.Clear();
  EXPECT_TRUE(session->InsertSpaceHalfWidth(&command));
  // "亜  "
  EXPECT_EQ("\xE4\xBA\x9C  ", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
}

TEST_F(SessionTest, InsertSpaceFullWidth) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  commands::KeyEvent space_key;
  space_key.set_special_key(commands::KeyEvent::SPACE);

  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceFullWidth(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_RESULT(kFullWidthSpace, command);

  EXPECT_TRUE(SendKey("a", session.get(), &command));
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceFullWidth(&command));
  // "あ　" (full-width space)
  EXPECT_EQ("\xE3\x81\x82\xE3\x80\x80", GetComposition(command));

  {  // Convert "あ　" with dummy conversions.
    Segments segments;
    // "亜　"
    segments.add_segment()->add_candidate()->value = "\xE4\xBA\x9C\xE3\x80\x80";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);

    command.Clear();
    EXPECT_TRUE(session->Convert(&command));
  }

  command.Clear();
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceFullWidth(&command));
  // "亜　　"
  EXPECT_EQ("\xE4\xBA\x9C\xE3\x80\x80\xE3\x80\x80",
            command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
}

TEST_F(SessionTest, InsertSpaceWithInputMode) {
  // First, test against http://b/6027559
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\tSpace\tInsertSpace\n"
        "Composition\tSpace\tInsertSpace\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);
    config::ConfigHandler::SetConfig(config);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_FALSE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    // In this case, space key event should not be consumed.
    EXPECT_FALSE(command.output().consumed());
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKey("a", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "あ"
    EXPECT_PREEDIT(kHiraganaA, command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());

    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "あ "
    EXPECT_PREEDIT(string(kHiraganaA) + " ", command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());
  }

  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\tSpace\tInsertAlternateSpace\n"
        "Composition\tSpace\tInsertAlternateSpace\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);
    config::ConfigHandler::SetConfig(config);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "　"
    EXPECT_RESULT(kFullWidthSpace, command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
    EXPECT_EQ(commands::HALF_KATAKANA, command.output().mode());
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKey("a", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "あ"
    EXPECT_PREEDIT(kHiraganaA, command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());

    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_KATAKANA, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "あ　"
    EXPECT_PREEDIT(string(kHiraganaA) + kFullWidthSpace, command);
    EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());
  }

  // Second, the 1st case filed in http://b/2936141
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\tSpace\tInsertSpace\n"
        "Composition\tSpace\tInsertSpace\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);

    config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
    config::ConfigHandler::SetConfig(config);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "　"
    EXPECT_RESULT(kFullWidthSpace, command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session->context().state());
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "a", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "a", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_PREEDIT("a", command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::HALF_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "a　"
    EXPECT_PREEDIT(string("a") + kFullWidthSpace, command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());
  }

  // Finally, the 2nd case filed in http://b/2936141
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\tSpace\tInsertSpace\n"
        "Composition\tSpace\tInsertSpace\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);

    config.set_space_character_form(config::Config::FUNDAMENTAL_HALF_WIDTH);
    config::ConfigHandler::SetConfig(config);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::FULL_ASCII, session.get(), &command));
    EXPECT_FALSE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::FULL_ASCII, session.get(), &command));
    EXPECT_FALSE(command.output().consumed());
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    EXPECT_TRUE(TestSendKeyWithMode(
        "a", commands::FULL_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "a", commands::FULL_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "ａ"
    EXPECT_PREEDIT(kFullWidthSmallA, command);
    EXPECT_EQ(commands::FULL_ASCII, command.output().mode());

    EXPECT_TRUE(TestSendKeyWithMode(
        "Space", commands::FULL_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKeyWithMode(
        "Space", commands::FULL_ASCII, session.get(), &command));
    EXPECT_TRUE(command.output().consumed());
    // "ａ "
    EXPECT_PREEDIT(kFullWidthSmallA + string(" "), command);
    EXPECT_EQ(commands::FULL_ASCII, command.output().mode());
  }
}

TEST_F(SessionTest, InsertSpaceWithCustomKeyBinding) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Precomposition\tSpace\tInsertSpace\n"
      "Precomposition\tShift Space\tInsertSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config.set_space_character_form(config::Config::FUNDAMENTAL_HALF_WIDTH);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // A plain space key event dispatched to InsertHalfSpace should be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  SetUndoContext(session.get());
  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  // A space key event with any modifier key dispatched to InsertHalfSpace
  // should be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // It is OK not to check |TryUndoAndAssertDoNothing| here because this
  // (test) send key event is actually *consumed*.

  EXPECT_TRUE(SendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_RESULT(" ", command);
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
}

TEST_F(SessionTest, InsertAlternateSpaceWithCustomKeyBinding) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Precomposition\tSpace\tInsertAlternateSpace\n"
      "Precomposition\tShift Space\tInsertAlternateSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // A plain space key event dispatched to InsertHalfSpace should be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  SetUndoContext(session.get());
  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  // A space key event with any modifier key dispatched to InsertHalfSpace
  // should be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // It is OK not to check |TryUndoAndAssertDoNothing| here because this
  // (test) send key event is actually *consumed*.

  EXPECT_TRUE(SendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_RESULT(" ", command);
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
}

TEST_F(SessionTest, InsertSpaceHalfWidthWithCustomKeyBinding) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Precomposition\tSpace\tInsertHalfSpace\n"
      "Precomposition\tShift Space\tInsertHalfSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // A plain space key event assigned to InsertHalfSpace should be echoed back.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  SetUndoContext(session.get());
  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());  // should not be consumed.
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  // A space key event with any modifier key assigned to InsertHalfSpace should
  // be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // It is OK not to check |TryUndoAndAssertDoNothing| here because this
  // (test) send key event is actually *consumed*.

  EXPECT_TRUE(SendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_RESULT(" ", command);
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
}

TEST_F(SessionTest, InsertSpaceFullWidthWithCustomKeyBinding) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Precomposition\tSpace\tInsertFullSpace\n"
      "Precomposition\tShift Space\tInsertFullSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToDirect(session.get());

  commands::Command command;

  // A plain space key event assigned to InsertFullSpace should be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // It is OK not to check |TryUndoAndAssertDoNothing| here because this
  // (test) send key event is actually *consumed*.

  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  // "　" (full-width space)
  EXPECT_RESULT(kFullWidthSpace, command);
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));

  // A space key event with any modifier key assigned to InsertFullSpace should
  // be consumed.
  SetUndoContext(session.get());
  EXPECT_TRUE(TestSendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  // It is OK not to check |TryUndoAndAssertDoNothing| here because this
  // (test) send key event is actually *consumed*.

  EXPECT_TRUE(SendKey("Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  // "　" (full-width space)
  EXPECT_RESULT(kFullWidthSpace, command);
  EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
}

TEST_F(SessionTest, InsertSpaceInDirectMode) {
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Direct\tCtrl a\tInsertSpace\n"
      "Direct\tCtrl b\tInsertAlternateSpace\n"
      "Direct\tCtrl c\tInsertHalfSpace\n"
      "Direct\tCtrl d\tInsertFullSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToDirect(session.get());

  commands::Command command;

  // [InsertSpace] should be echoes back in the direct mode.
  EXPECT_TRUE(TestSendKey("Ctrl a", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_TRUE(SendKey("Ctrl a", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  // [InsertAlternateSpace] should be echoes back in the direct mode.
  EXPECT_TRUE(TestSendKey("Ctrl b", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_TRUE(SendKey("Ctrl b", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  // [InsertHalfSpace] should be echoes back in the direct mode.
  EXPECT_TRUE(TestSendKey("Ctrl c", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_TRUE(SendKey("Ctrl c", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());

  // [InsertFullSpace] should be echoes back in the direct mode.
  EXPECT_TRUE(TestSendKey("Ctrl d", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
  EXPECT_TRUE(SendKey("Ctrl d", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, InsertSpaceInCompositionMode) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Composition\tCtrl a\tInsertSpace\n"
      "Composition\tCtrl b\tInsertAlternateSpace\n"
      "Composition\tCtrl c\tInsertHalfSpace\n"
      "Composition\tCtrl d\tInsertFullSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  SendKey("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));
  EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());

  EXPECT_TRUE(TestSendKey("Ctrl a", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  SendKey("Ctrl a", session.get(), &command);
  // "あ　"
  EXPECT_EQ("\xE3\x81\x82\xE3\x80\x80", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Ctrl b", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  SendKey("Ctrl b", session.get(), &command);
  // "あ　 "
  EXPECT_EQ("\xE3\x81\x82\xE3\x80\x80 ", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Ctrl c", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  SendKey("Ctrl c", session.get(), &command);
  // "あ　  "
  EXPECT_EQ("\xE3\x81\x82\xE3\x80\x80  ", GetComposition(command));

  EXPECT_TRUE(TestSendKey("Ctrl d", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  SendKey("Ctrl d", session.get(), &command);
  // "あ　  　"
  EXPECT_EQ("\xE3\x81\x82\xE3\x80\x80  \xE3\x80\x80", GetComposition(command));
}

TEST_F(SessionTest, InsertSpaceInConversionMode) {
  // This is a unittest against http://b/5872031
  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "Conversion\tCtrl a\tInsertSpace\n"
      "Conversion\tCtrl b\tInsertAlternateSpace\n"
      "Conversion\tCtrl c\tInsertHalfSpace\n"
      "Conversion\tCtrl d\tInsertFullSpace\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));

  {
    InitSessionToConversionWithAiueo(session.get());
    commands::Command command;

    EXPECT_TRUE(TestSendKey("Ctrl a", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("Ctrl a", session.get(), &command));
    EXPECT_TRUE(GetComposition(command).empty());
    ASSERT_TRUE(command.output().has_result());
    // "あいうえお　"
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88"
              "\xE3\x81\x8A\xE3\x80\x80",
              command.output().result().value());
    EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
  }

  {
    InitSessionToConversionWithAiueo(session.get());
    commands::Command command;

    EXPECT_TRUE(TestSendKey("Ctrl b", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("Ctrl b", session.get(), &command));
    EXPECT_TRUE(GetComposition(command).empty());
    ASSERT_TRUE(command.output().has_result());
    // "あいうえお "
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A ",
              command.output().result().value());
    EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
  }

  {
    InitSessionToConversionWithAiueo(session.get());
    commands::Command command;

    EXPECT_TRUE(TestSendKey("Ctrl c", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("Ctrl c", session.get(), &command));
    EXPECT_TRUE(GetComposition(command).empty());
    ASSERT_TRUE(command.output().has_result());
    // "あいうえお "
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88\xE3\x81\x8A ",
              command.output().result().value());
    EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
  }

  {
    InitSessionToConversionWithAiueo(session.get());
    commands::Command command;

    EXPECT_TRUE(TestSendKey("Ctrl d", session.get(), &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("Ctrl d", session.get(), &command));
    EXPECT_TRUE(GetComposition(command).empty());
    ASSERT_TRUE(command.output().has_result());
    // "あいうえお　"
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86\xE3\x81\x88"
              "\xE3\x81\x8A\xE3\x80\x80",
              command.output().result().value());
    EXPECT_TRUE(TryUndoAndAssertDoNothing(session.get()));
  }
}

TEST_F(SessionTest, InsertSpaceFullWidthOnHalfKanaInput) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  EXPECT_TRUE(session->InputModeHalfKatakana(&command));
  EXPECT_EQ(commands::HALF_KATAKANA, command.output().mode());
  InsertCharacterChars("a", session.get(), &command);
  // "ｱ"
  EXPECT_EQ("\xEF\xBD\xB1", GetComposition(command));

  command.Clear();
  commands::KeyEvent space_key;
  space_key.set_special_key(commands::KeyEvent::SPACE);
  command.mutable_input()->mutable_key()->CopyFrom(space_key);
  EXPECT_TRUE(session->InsertSpaceFullWidth(&command));
  // "ｱ　" (full-width space)
  EXPECT_EQ("\xEF\xBD\xB1\xE3\x80\x80", GetComposition(command));
}

TEST_F(SessionTest, IsFullWidthInsertSpace) {
  scoped_ptr<Session> session;
  config::Config config;

  { // When |empty_command| does not have |empty_command.key().input()| field,
    // the current input mode will be used.

    // Default config -- follow to the current mode.
    config.set_space_character_form(config::Config::FUNDAMENTAL_INPUT_MODE);
    config::ConfigHandler::SetConfig(config);
    session.reset(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Input empty_input;

    // Hiragana
    commands::Command command;
    session->InputModeHiragana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Full-Katakana
    command.Clear();
    session->InputModeFullKatakana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Half-Katakana
    command.Clear();
    session->InputModeHalfKatakana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Full-ASCII
    command.Clear();
    session->InputModeFullASCII(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Half-ASCII
    command.Clear();
    session->InputModeHalfASCII(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Direct
    command.Clear();
    session->IMEOff(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));

    // Set config to 'half' -- all mode has to emit half-width space.
    config.set_space_character_form(config::Config::FUNDAMENTAL_HALF_WIDTH);
    config::ConfigHandler::SetConfig(config);
    session.reset(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    // Hiragana
    command.Clear();
    session->InputModeHiragana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Full-Katakana
    command.Clear();
    session->InputModeFullKatakana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Half-Katakana
    command.Clear();
    session->InputModeHalfKatakana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Full-ASCII
    command.Clear();
    session->InputModeFullASCII(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Half-ASCII
    command.Clear();
    session->InputModeHalfASCII(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
    // Direct
    command.Clear();
    session->IMEOff(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));

    // Set config to 'FULL' -- all mode except for DIRECT emits
    // full-width space.
    config.set_space_character_form(config::Config::FUNDAMENTAL_FULL_WIDTH);
    config::ConfigHandler::SetConfig(config);
    session.reset(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    // Hiragana
    command.Clear();
    session->InputModeHiragana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Full-Katakana
    command.Clear();
    session->InputModeFullKatakana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(command.input()));
    // Half-Katakana
    command.Clear();
    session->InputModeHalfKatakana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Full-ASCII
    command.Clear();
    session->InputModeFullASCII(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Half-ASCII
    command.Clear();
    session->InputModeHalfASCII(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(empty_input));
    // Direct
    command.Clear();
    session->IMEOff(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(empty_input));
  }

  { // When |input| has |input.key().mode()| field,
    // the specified input mode by |input| will be used.

    // Default config -- follow to the current mode.
    config.set_space_character_form(config::Config::FUNDAMENTAL_INPUT_MODE);
    config::ConfigHandler::SetConfig(config);
    session.reset(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    // Use HALF_KATAKANA for the new input mode
    commands::Input input;
    input.mutable_key()->set_mode(commands::HALF_KATAKANA);

    // Hiragana
    commands::Command command;
    session->InputModeHiragana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
    // Full-Katakana
    command.Clear();
    session->InputModeFullKatakana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
    // Half-Katakana
    command.Clear();
    session->InputModeHalfKatakana(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
    // Full-ASCII
    command.Clear();
    session->InputModeFullASCII(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
    // Half-ASCII
    command.Clear();
    session->InputModeHalfASCII(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
    // Direct
    command.Clear();
    session->IMEOff(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));

    // Use FULL_ASCII for the new input mode
    input.mutable_key()->set_mode(commands::FULL_ASCII);

    // Hiragana
    command.Clear();
    session->InputModeHiragana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(input));
    // Full-Katakana
    command.Clear();
    session->InputModeFullKatakana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(input));
    // Half-Katakana
    command.Clear();
    session->InputModeHalfKatakana(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(input));
    // Full-ASCII
    command.Clear();
    session->InputModeFullASCII(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(input));
    // Half-ASCII
    command.Clear();
    session->InputModeHalfASCII(&command);
    EXPECT_TRUE(session->IsFullWidthInsertSpace(input));
    // Direct
    command.Clear();
    session->IMEOff(&command);
    EXPECT_FALSE(session->IsFullWidthInsertSpace(input));
  }
}

TEST_F(SessionTest, Issue1951385) {
  // This is a unittest against http://b/1951385
  Segments segments;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  const string exceeded_preedit(500, 'a');
  ASSERT_EQ(500, exceeded_preedit.size());
  InsertCharacterChars(exceeded_preedit, session.get(), &command);

  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, false);

  command.Clear();
  session->ConvertToFullASCII(&command);
  EXPECT_FALSE(command.output().has_candidates());

  // The status should remain the preedit status, although the
  // previous command was convert.  The next command makes sure that
  // the preedit will disappear by canceling the preedit status.
  command.Clear();
  command.mutable_input()->mutable_key()->set_special_key(
      commands::KeyEvent::ESCAPE);
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, Issue1978201) {
  // This is a unittest against http://b/1978201
  Segments segments;
  Segment *segment;
  segment = segments.add_segment();
  // "いんぼう"
  segment->set_key("\xe3\x81\x84\xe3\x82\x93\xe3\x81\xbc\xe3\x81\x86");
  // "陰謀"
  segment->add_candidate()->value = "\xe9\x99\xb0\xe8\xac\x80";
  // "陰謀論"
  segment->add_candidate()->value = "\xe9\x99\xb0\xe8\xac\x80\xe8\xab\x96";
  // "陰謀説"
  segment->add_candidate()->value = "\xe9\x99\xb0\xe8\xac\x80\xe8\xaa\xac";
  GetConverterMock()->SetStartPredictionForRequest(&segments, true);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  EXPECT_TRUE(session->SegmentWidthShrink(&command));

  command.Clear();
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);
  EXPECT_TRUE(session->Convert(&command));

  command.Clear();
  EXPECT_TRUE(session->CommitSegment(&command));
  // "陰謀"
  EXPECT_RESULT("\xE9\x99\xB0\xE8\xAC\x80", command);
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, Issue1975771) {
  // This is a unittest against http://b/1975771
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Trigger suggest by pressing "a".
  Segments segments;
  SetAiueo(&segments);
  GetConverterMock()->SetStartSuggestionForRequest(&segments, true);

  commands::Command command;
  commands::KeyEvent* key_event = command.mutable_input()->mutable_key();
  key_event->set_key_code('a');
  key_event->set_modifiers(0);  // No modifiers.
  EXPECT_TRUE(session->InsertCharacter(&command));

  // Click the first candidate.
  SetSendCommandCommand(commands::SessionCommand::SELECT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(0);
  EXPECT_TRUE(session->SendCommand(&command));

  // After select candidate session->status_ should be
  // SessionStatus::CONVERSION.

  SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
  EXPECT_TRUE(command.output().has_candidates());
  // The second candidate should be selected.
  EXPECT_EQ(1, command.output().candidates().focused_index());
}

TEST_F(SessionTest, Issue2029466) {
  // This is a unittest against http://b/2029466
  //
  // "a<tab><ctrl-N>a" raised an exception because CommitFirstSegment
  // did not check if the current status is in conversion or
  // precomposition.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // "a"
  commands::Command command;
  InsertCharacterChars("a", session.get(), &command);

  // <tab>
  Segments segments;
  SetAiueo(&segments);
  GetConverterMock()->SetStartPredictionForRequest(&segments, true);
  command.Clear();
  EXPECT_TRUE(session->PredictAndConvert(&command));

  // <ctrl-N>
  segments.Clear();
  // FinishConversion is expected to return empty Segments.
  GetConverterMock()->SetFinishConversion(&segments, true);
  command.Clear();
  EXPECT_TRUE(session->CommitSegment(&command));

  // "a"
  InsertCharacterChars("a", session.get(), &command);
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);
  EXPECT_FALSE(command.output().has_candidates());
}

TEST_F(SessionTest, Issue2034943) {
  // This is a unittest against http://b/2029466
  //
  // The composition should have been reset if CommitSegment submitted
  // the all segments (e.g. the size of segments is one).
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("mozu", session.get(), &command);

  {  // Initialize a suggest result triggered by "mozu".
    Segments segments;
    Segment *segment = segments.add_segment();
    segment->set_key("mozu");
    Segment::Candidate *candidate;
    candidate = segment->add_candidate();
    candidate->value = "MOZU";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }
  // Get conversion
  command.Clear();
  EXPECT_TRUE(session->Convert(&command));

  // submit segment
  command.Clear();
  EXPECT_TRUE(session->CommitSegment(&command));

  // The composition should have been reset.
  InsertCharacterChars("ku", session.get(), &command);
  // "く"
  EXPECT_EQ("\343\201\217", command.output().preedit().segment(0).value());
}

TEST_F(SessionTest, Issue2026354) {
  // This is a unittest against http://b/2026354
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);

  // Trigger suggest by pressing "a".
  Segments segments;
  SetAiueo(&segments);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  EXPECT_TRUE(session->Convert(&command));

  //  EXPECT_TRUE(session->ConvertNext(&command));
  TestSendKey("Space", session.get(), &command);
  EXPECT_PREEDIT(kAiueo, command);
  command.mutable_output()->clear_candidates();
  EXPECT_FALSE(command.output().has_candidates());
}

TEST_F(SessionTest, Issue2066906) {
  // This is a unittest against http://b/2066906
  Segments segments;
  Segment *segment;
  Segment::Candidate *candidate;
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  segment = segments.add_segment();
  segment->set_key("a");
  candidate = segment->add_candidate();
  candidate->value = "abc";
  candidate = segment->add_candidate();
  candidate->value = "abcdef";
  GetConverterMock()->SetStartPredictionForRequest(&segments, true);

  // Prediction with "a"
  commands::Command command;
  EXPECT_TRUE(session->PredictAndConvert(&command));
  EXPECT_FALSE(command.output().has_result());

  // Commit
  command.Clear();
  EXPECT_TRUE(session->Commit(&command));
  EXPECT_RESULT("abc", command);

  GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
  InsertCharacterChars("a", session.get(), &command);
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, Issue2187132) {
  // This is a unittest against http://b/2187132
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // Shift + Ascii triggers temporary input mode switch.
  SendKey("A", session.get(), &command);
  SendKey("Enter", session.get(), &command);

  // After submission, input mode should be reverted.
  SendKey("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  command.Clear();
  session->EditCancel(&command);
  EXPECT_TRUE(GetComposition(command).empty());

  // If a user intentionally switched an input mode, it should remain.
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  SendKey("A", session.get(), &command);
  SendKey("Enter", session.get(), &command);
  SendKey("a", session.get(), &command);
  EXPECT_EQ("a", GetComposition(command));
}

TEST_F(SessionTest, Issue2190364) {
  // This is a unittest against http://b/2190364
  config::Config config;
  config.set_preedit_method(config::Config::KANA);
  config::ConfigHandler::SetConfig(config);
  ASSERT_EQ(config::Config::KANA, GET_CONFIG(preedit_method));

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  session->ToggleAlphanumericMode(&command);

  // "ち"
  InsertCharacterCodeAndString('a', "\xE3\x81\xA1", session.get(), &command);
  EXPECT_EQ("a", GetComposition(command));

  command.Clear();
  session->ToggleAlphanumericMode(&command);
  EXPECT_EQ("a", GetComposition(command));

  // "に"
  InsertCharacterCodeAndString('i', "\xE3\x81\xAB", session.get(), &command);
  // "aに"
  EXPECT_EQ("a\xE3\x81\xAB", GetComposition(command));
}

TEST_F(SessionTest, Issue1556649) {
  // This is a unittest against http://b/1556649
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("kudoudesu", session.get(), &command);
  // "くどうです"
  EXPECT_EQ("\xE3\x81\x8F\xE3\x81\xA9\xE3\x81\x86\xE3\x81\xA7\xE3\x81\x99",
            GetComposition(command));
  EXPECT_EQ(5, command.output().preedit().cursor());

  command.Clear();
  EXPECT_TRUE(session->DisplayAsHalfKatakana(&command));
  // "ｸﾄﾞｳﾃﾞｽ"
  EXPECT_EQ("\xEF\xBD\xB8\xEF\xBE\x84\xEF\xBE\x9E\xEF\xBD\xB3\xEF\xBE\x83"
            "\xEF\xBE\x9E\xEF\xBD\xBD",
            GetComposition(command));
  EXPECT_EQ(7, command.output().preedit().cursor());

  for (size_t i = 0; i < 7; ++i) {
    const size_t expected_pos = 6 - i;
    EXPECT_TRUE(SendKey("Left", session.get(), &command));
    EXPECT_EQ(expected_pos, command.output().preedit().cursor());
  }
}

TEST_F(SessionTest, Issue1518994) {
  // This is a unittest against http://b/1518994.
  // - Can't input space in ascii mode.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    command.Clear();
    EXPECT_TRUE(session->ToggleAlphanumericMode(&command));
    EXPECT_TRUE(SendKey("i", session.get(), &command));
    // "あi"
    EXPECT_EQ("\xE3\x81\x82\x69", GetComposition(command));

    EXPECT_TRUE(SendKey("Space", session.get(), &command));
    // "あi "
    EXPECT_EQ("\xE3\x81\x82\x69\x20", GetComposition(command));
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;
    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(SendKey("I", session.get(), &command));
    // "あI"
    EXPECT_EQ("\xE3\x81\x82\x49", GetComposition(command));

    EXPECT_TRUE(SendKey("Space", session.get(), &command));
    // "あI "
    EXPECT_EQ("\xE3\x81\x82\x49\x20", GetComposition(command));
  }
}

TEST_F(SessionTest, Issue1571043) {
  // This is a unittest against http://b/1571043.
  // - Underline of composition is separated.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("aiu", session.get(), &command);
  // "あいう"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x84\xE3\x81\x86", GetComposition(command));

  for (size_t i = 0; i < 3; ++i) {
    const size_t expected_pos = 2 - i;
    EXPECT_TRUE(SendKey("Left", session.get(), &command));
    EXPECT_EQ(expected_pos, command.output().preedit().cursor());
    EXPECT_EQ(1, command.output().preedit().segment_size());
  }
}

TEST_F(SessionTest, Issue1799384) {
  // This is a unittest against http://b/1571043.
  // - ConvertToHiragana converts Vu to U+3094 "ヴ"
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("ravu", session.get(), &command);
  // TODO(komatsu) "ヴ" might be preferred on Mac.
  // "らヴ"
  EXPECT_EQ("\xE3\x82\x89\xE3\x83\xB4", GetComposition(command));

  {  // Initialize GetConverterMock() to generate t13n candidates.
    Segments segments;
    Segment *segment;
    segments.set_request_type(Segments::CONVERSION);
    segment = segments.add_segment();
    // "らヴ"
    segment->set_key("\xE3\x82\x89\xE3\x83\xB4");
    Segment::Candidate *candidate;
    candidate = segment->add_candidate();
    // "らぶ"
    candidate->value = "\xE3\x82\x89\xE3\x81\xB6";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  command.Clear();
  EXPECT_TRUE(session->ConvertToHiragana(&command));

  // "らヴ"
  EXPECT_EQ("\xE3\x82\x89\xE3\x83\xB4", GetComposition(command));
}

TEST_F(SessionTest, Issue2217250) {
  // This is a unittest against http://b/2217250.
  // Temporary direct input mode through a special sequence such as
  // www. continues even after committing them
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  InsertCharacterChars("www.", session.get(), &command);
  EXPECT_EQ("www.", GetComposition(command));
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  SendKey("Enter", session.get(), &command);
  EXPECT_EQ("www.", command.output().result().value());
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
}

TEST_F(SessionTest, Issue2223823) {
  // This is a unittest against http://b/2223823
  // Input mode does not recover like MS-IME by single shift key down
  // and up.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  SendKey("G", session.get(), &command);
  EXPECT_EQ("G", GetComposition(command));
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  SendKey("Shift", session.get(), &command);
  EXPECT_EQ("G", GetComposition(command));
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
}


TEST_F(SessionTest, Issue2223762) {
  // This is a unittest against http://b/2223762.
  // - The first space in half-width alphanumeric mode is full-width.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, Issue2223755) {
  // This is a unittest against http://b/2223755.
  // - F6 and F7 convert space to half-width.

  {  // DisplayAsFullKatakana
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(SendKey("Eisu", session.get(), &command));
    EXPECT_TRUE(SendKey("Space", session.get(), &command));
    EXPECT_TRUE(SendKey("Eisu", session.get(), &command));
    EXPECT_TRUE(SendKey("i", session.get(), &command));

    // "あ い"
    EXPECT_EQ("\xE3\x81\x82\x20\xE3\x81\x84", GetComposition(command));

    command.Clear();
    EXPECT_TRUE(session->DisplayAsFullKatakana(&command));

    // "ア　イ"(fullwidth space)
    EXPECT_EQ("\xE3\x82\xA2\xE3\x80\x80\xE3\x82\xA4", GetComposition(command));
  }

  {  // ConvertToFullKatakana
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    EXPECT_TRUE(SendKey("a", session.get(), &command));
    EXPECT_TRUE(SendKey("Eisu", session.get(), &command));
    EXPECT_TRUE(SendKey("Space", session.get(), &command));
    EXPECT_TRUE(SendKey("Eisu", session.get(), &command));
    EXPECT_TRUE(SendKey("i", session.get(), &command));

    // "あ い"
    EXPECT_EQ("\xE3\x81\x82\x20\xE3\x81\x84", GetComposition(command));

    {  // Initialize GetConverterMock() to generate t13n candidates.
      Segments segments;
      Segment *segment;
      segments.set_request_type(Segments::CONVERSION);
      segment = segments.add_segment();
      // "あ い"
      segment->set_key("\xE3\x81\x82\x20\xE3\x81\x84");
      Segment::Candidate *candidate;
      candidate = segment->add_candidate();
      // "あ い"
      candidate->value = "\xE3\x81\x82\x20\xE3\x81\x84";
      ConversionRequest request;
      SetComposer(session.get(), &request);
      FillT13Ns(request, &segments);
      GetConverterMock()->SetStartConversionForRequest(&segments, true);
    }

    command.Clear();
    EXPECT_TRUE(session->ConvertToFullKatakana(&command));

    // "ア　イ"(fullwidth space)
    EXPECT_EQ("\xE3\x82\xA2\xE3\x80\x80\xE3\x82\xA4", GetComposition(command));
  }
}

TEST_F(SessionTest, Issue2269058) {
  // This is a unittest against http://b/2269058.
  // - Temporary input mode should not be overridden by a permanent
  //   input mode change.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  EXPECT_TRUE(SendKey("G", session.get(), &command));
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  command.Clear();
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

  EXPECT_TRUE(SendKey("Shift", session.get(), &command));
  EXPECT_EQ(commands::HIRAGANA, command.output().mode());
}

TEST_F(SessionTest, Issue2272745) {
  // This is a unittest against http://b/2272745.
  // A temporary input mode remains when a composition is canceled.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    EXPECT_TRUE(SendKey("G", session.get(), &command));
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(SendKey("Backspace", session.get(), &command));
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    EXPECT_TRUE(SendKey("G", session.get(), &command));
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(SendKey("Escape", session.get(), &command));
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }
}

TEST_F(SessionTest, Issue2282319) {
  // This is a unittest against http://b/2282319.
  // InsertFullSpace is not working in half-width input mode.
  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  ASSERT_EQ(config::Config::MSIME, GET_CONFIG(session_keymap));

  commands::Command command;
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());

  EXPECT_TRUE(TestSendKey("a", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  EXPECT_TRUE(SendKey("a", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_PREEDIT("a", command);

  EXPECT_TRUE(TestSendKey("Ctrl Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());

  EXPECT_TRUE(SendKey("Ctrl Shift Space", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_PREEDIT(string("a") + kFullWidthSpace, command);
}

TEST_F(SessionTest, Issue2297060) {
  // This is a unittest against http://b/2297060.
  // Ctrl-Space is not working
  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  ASSERT_EQ(config::Config::MSIME, GET_CONFIG(session_keymap));

  commands::Command command;
  EXPECT_TRUE(SendKey("Ctrl Space", session.get(), &command));
  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, Issue2379374) {
  // This is a unittest against http://b/2379374.
  // Numpad ignores Direct input style when typing after conversion.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  {  // Set numpad_character_form with NUMPAD_DIRECT_INPUT
    config::Config config;
    config.set_numpad_character_form(config::Config::NUMPAD_DIRECT_INPUT);
    config::ConfigHandler::SetConfig(config);
    ASSERT_EQ(config::Config::NUMPAD_DIRECT_INPUT,
              GET_CONFIG(numpad_character_form));
    // Update KeyEventTransformer.
    session->ReloadConfig();
  }

  Segments segments;
  {  // Set mock conversion.
    Segment *segment;
    Segment::Candidate *candidate;

    segment = segments.add_segment();
    // "あ"
    segment->set_key(kHiraganaA);
    candidate = segment->add_candidate();
    // "亜"
    candidate->value = "\xE4\xBA\x9C";
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  EXPECT_TRUE(SendKey("a", session.get(), &command));
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  EXPECT_TRUE(SendKey("Space", session.get(), &command));
  // "亜"
  EXPECT_EQ("\xE4\xBA\x9C", GetComposition(command));

  EXPECT_TRUE(SendKey("Numpad0", session.get(), &command));
  EXPECT_TRUE(GetComposition(command).empty());
  // "亜0", "あ0"
  EXPECT_RESULT_AND_KEY("\xE4\xBA\x9C\x30", "\xE3\x81\x82\x30", command);

  // The previous Numpad0 must not affect the current composition.
  EXPECT_TRUE(SendKey("a", session.get(), &command));
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));
}

TEST_F(SessionTest, Issue2569789) {
  // This is a unittest against http://b/2379374.
  // After typing "google", the input mode does not come back to the
  // previous input mode.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    InsertCharacterChars("google", session.get(), &command);
    EXPECT_EQ("google", GetComposition(command));
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    EXPECT_TRUE(SendKey("enter", session.get(), &command));
    ASSERT_TRUE(command.output().has_result());
    EXPECT_EQ("google", command.output().result().value());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    InsertCharacterChars("Google", session.get(), &command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(SendKey("enter", session.get(), &command));
    ASSERT_TRUE(command.output().has_result());
    EXPECT_EQ("Google", command.output().result().value());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    InsertCharacterChars("Google", session.get(), &command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(SendKey("shift", session.get(), &command));
    EXPECT_EQ("Google", GetComposition(command));
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    InsertCharacterChars("aaa", session.get(), &command);
    // "Googleあああ"
    EXPECT_EQ("Google\xE3\x81\x82\xE3\x81\x82\xE3\x81\x82",
              GetComposition(command));
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    InsertCharacterChars("http", session.get(), &command);
    EXPECT_EQ(commands::HALF_ASCII, command.output().mode());

    EXPECT_TRUE(SendKey("enter", session.get(), &command));
    ASSERT_TRUE(command.output().has_result());
    EXPECT_EQ("http", command.output().result().value());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
  }
}

TEST_F(SessionTest, Issue2555503) {
  // This is a unittest against http://b/2555503.
  // Mode respects the previous character too much.

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  SendKey("a", session.get(), &command);

  command.Clear();
  session->InputModeFullKatakana(&command);

  SendKey("i", session.get(), &command);
  // "あイ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x82\xA4", GetComposition(command));

  SendKey("backspace", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));
  EXPECT_EQ(commands::FULL_KATAKANA, command.output().mode());
}

TEST_F(SessionTest, Issue2791640) {
  // This is a unittest against http://b/2791640.
  // Existing preedit should be committed when IME is turned off.

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  SendKey("a", session.get(), &command);
  SendKey("hankaku/zenkaku", session.get(), &command);

  ASSERT_TRUE(command.output().consumed());

  ASSERT_TRUE(command.output().has_result());
  // "あ"
  EXPECT_EQ(kHiraganaA, command.output().result().value());
  EXPECT_EQ(commands::DIRECT, command.output().mode());

  ASSERT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, CommitExistingPreeditWhenIMEIsTurnedOff) {
  // Existing preedit should be committed when IME is turned off.

  // Check "hankaku/zenkaku"
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    SendKey("a", session.get(), &command);
    SendKey("hankaku/zenkaku", session.get(), &command);

    ASSERT_TRUE(command.output().consumed());

    ASSERT_TRUE(command.output().has_result());
    // "あ"
    EXPECT_EQ(kHiraganaA, command.output().result().value());
    EXPECT_EQ(commands::DIRECT, command.output().mode());

    ASSERT_FALSE(command.output().has_preedit());
  }

  // Check "kanji"
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;
    SendKey("a", session.get(), &command);
    SendKey("kanji", session.get(), &command);

    ASSERT_TRUE(command.output().consumed());

    ASSERT_TRUE(command.output().has_result());
    // "あ"
    EXPECT_EQ(kHiraganaA, command.output().result().value());
    EXPECT_EQ(commands::DIRECT, command.output().mode());

    ASSERT_FALSE(command.output().has_preedit());
  }
}


TEST_F(SessionTest, SendKeyDirectInputStateTest) {
  // InputModeChange commands from direct mode are supported only for Windows
  // for now.
#ifdef OS_WIN
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToDirect(session.get());
  commands::Command command;

  config::Config config;
  const string custom_keymap_table =
      "status\tkey\tcommand\n"
      "DirectInput\tHiragana\tInputModeHiragana\n";
  config.set_session_keymap(config::Config::CUSTOM);
  config.set_custom_keymap_table(custom_keymap_table);
  config::ConfigHandler::SetConfig(config);

  EXPECT_TRUE(SendKey("Hiragana", session.get(), &command));
  EXPECT_TRUE(SendKey("a", session.get(), &command));
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);
#endif  // OS_WIN
}

TEST_F(SessionTest, HandlingDirectInputTableAttribute) {
  composer::Table table;
  // "か"
  table.AddRuleWithAttributes("ka", "\xE3\x81\x8B", "",
                              composer::DIRECT_INPUT);
  // "っ"
  table.AddRuleWithAttributes("tt", "\xE3\x81\xA3", "t",
                              composer::DIRECT_INPUT);
  // "た"
  table.AddRuleWithAttributes("ta", "\xE3\x81\x9F", "",
                              composer::NO_TABLE_ATTRIBUTE);

  Session session(engine_.get());
  InitSessionToPrecomposition(&session);
  session.get_internal_composer_only_for_unittest()->SetTable(&table);

  commands::Command command;
  SendKey("k", &session, &command);
  EXPECT_FALSE(command.output().has_result());

  SendKey("a", &session, &command);
  // "か"
  EXPECT_RESULT("\xE3\x81\x8B", command);

  SendKey("t", &session, &command);
  EXPECT_FALSE(command.output().has_result());

  SendKey("t", &session, &command);
  EXPECT_FALSE(command.output().has_result());

  SendKey("a", &session, &command);
  // "った"
  EXPECT_RESULT("\xE3\x81\xA3\xE3\x81\x9F", command);
}

TEST_F(SessionTest, IMEOnWithModeTest) {
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;
    command.mutable_input()->mutable_key()->set_mode(
        commands::HIRAGANA);
    EXPECT_TRUE(session->IMEOn(&command));
    EXPECT_TRUE(command.output().has_consumed());
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HIRAGANA,
              command.output().mode());
    SendKey("a", session.get(), &command);
    // "あ"
    EXPECT_SINGLE_SEGMENT(kHiraganaA, command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;
    command.mutable_input()->mutable_key()->set_mode(
        commands::FULL_KATAKANA);
    EXPECT_TRUE(session->IMEOn(&command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::FULL_KATAKANA,
              command.output().mode());
    SendKey("a", session.get(), &command);
    // "ア"
    EXPECT_SINGLE_SEGMENT("\xE3\x82\xA2", command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;
    command.mutable_input()->mutable_key()->set_mode(
        commands::HALF_KATAKANA);
    EXPECT_TRUE(session->IMEOn(&command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_KATAKANA,
              command.output().mode());
    SendKey("a", session.get(), &command);
    // "ｱ" (half-width Katakana)
    EXPECT_SINGLE_SEGMENT("\xEF\xBD\xB1", command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;
    command.mutable_input()->mutable_key()->set_mode(
        commands::FULL_ASCII);
    EXPECT_TRUE(session->IMEOn(&command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::FULL_ASCII,
              command.output().mode());
    SendKey("a", session.get(), &command);
    // "ａ"
    EXPECT_SINGLE_SEGMENT("\xEF\xBD\x81", command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToDirect(session.get());

    commands::Command command;
    command.mutable_input()->mutable_key()->set_mode(
        commands::HALF_ASCII);
    EXPECT_TRUE(session->IMEOn(&command));
    EXPECT_TRUE(command.output().has_mode());
    EXPECT_EQ(commands::HALF_ASCII,
              command.output().mode());
    SendKey("a", session.get(), &command);
    // "a"
    EXPECT_SINGLE_SEGMENT("a", command);
  }
}

TEST_F(SessionTest, InputModeConsumed) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  EXPECT_TRUE(session->InputModeHiragana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HIRAGANA, command.output().mode());
  command.Clear();
  EXPECT_TRUE(session->InputModeFullKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_KATAKANA, command.output().mode());
  command.Clear();
  EXPECT_TRUE(session->InputModeHalfKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_KATAKANA, command.output().mode());
  command.Clear();
  EXPECT_TRUE(session->InputModeFullASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_ASCII, command.output().mode());
  command.Clear();
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());
}

TEST_F(SessionTest, InputModeConsumedForTestSendKey) {
  // This test is only for Windows, because InputModeHiragana bound
  // with Hiragana key is only supported on Windows yet.
#ifdef OS_WIN
  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  ASSERT_EQ(config::Config::MSIME, GET_CONFIG(session_keymap));
  // In MSIME keymap, Hiragana is assigned for
  // ImputModeHiragana in Precomposition.

  commands::Command command;
  EXPECT_TRUE(TestSendKey("Hiragana", session.get(), &command));
  EXPECT_TRUE(command.output().consumed());
#endif  // OS_WIN
}

TEST_F(SessionTest, InputModeOutputHasComposition) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  SendKey("a", session.get(), &command);
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  command.Clear();
  EXPECT_TRUE(session->InputModeHiragana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HIRAGANA, command.output().mode());
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  command.Clear();
  EXPECT_TRUE(session->InputModeFullKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_KATAKANA, command.output().mode());
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  command.Clear();
  EXPECT_TRUE(session->InputModeHalfKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_KATAKANA, command.output().mode());
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  command.Clear();
  EXPECT_TRUE(session->InputModeFullASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_ASCII, command.output().mode());
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);

  command.Clear();
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());
  // "あ"
  EXPECT_SINGLE_SEGMENT(kHiraganaA, command);
}

TEST_F(SessionTest, InputModeOutputHasCandidates) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  Segments segments;
  SetAiueo(&segments);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  commands::Command command;
  InsertCharacterChars("aiueo", session.get(), &command);

  command.Clear();
  session->Convert(&command);
  session->ConvertNext(&command);
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());

  command.Clear();
  EXPECT_TRUE(session->InputModeHiragana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HIRAGANA, command.output().mode());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());

  command.Clear();
  EXPECT_TRUE(session->InputModeFullKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_KATAKANA, command.output().mode());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());

  command.Clear();
  EXPECT_TRUE(session->InputModeHalfKatakana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_KATAKANA, command.output().mode());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());

  command.Clear();
  EXPECT_TRUE(session->InputModeFullASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::FULL_ASCII, command.output().mode());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());

  command.Clear();
  EXPECT_TRUE(session->InputModeHalfASCII(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HALF_ASCII, command.output().mode());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_TRUE(command.output().has_preedit());
}

TEST_F(SessionTest, PerformedCommand) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  {
    commands::Command command;
    // IMEOff
    EXPECT_STATS_NOT_EXIST("Performed_Precomposition_IMEOff");
    SendSpecialKey(commands::KeyEvent::OFF, session.get(), &command);
    EXPECT_COUNT_STATS("Performed_Precomposition_IMEOff", 1);
  }
  {
    commands::Command command;
    // IMEOn
    EXPECT_STATS_NOT_EXIST("Performed_Direct_IMEOn");
    SendSpecialKey(commands::KeyEvent::ON, session.get(), &command);
    EXPECT_COUNT_STATS("Performed_Direct_IMEOn", 1);
  }
  {
    commands::Command command;
    // 'a'
    EXPECT_STATS_NOT_EXIST("Performed_Precomposition_InsertCharacter");
    SendKey("a", session.get(), &command);
    EXPECT_COUNT_STATS("Performed_Precomposition_InsertCharacter", 1);
  }
  {
    // SetStartConversion for changing state to Convert.
    Segments segments;
    SetAiueo(&segments);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    FillT13Ns(request, &segments);
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    commands::Command command;
    // SPACE
    EXPECT_STATS_NOT_EXIST("Performed_Composition_Convert");
    SendSpecialKey(commands::KeyEvent::SPACE, session.get(), &command);
    EXPECT_COUNT_STATS("Performed_Composition_Convert", 1);
  }
  {
    commands::Command command;
    // ENTER
    EXPECT_STATS_NOT_EXIST("Performed_Conversion_Commit");
    SendSpecialKey(commands::KeyEvent::ENTER, session.get(), &command);
    EXPECT_COUNT_STATS("Performed_Conversion_Commit", 1);
  }
}

TEST_F(SessionTest, ResetContext) {
  scoped_ptr<MockConverterEngineForReset> engine(
      new MockConverterEngineForReset);
  ConverterMockForReset *convertermock = engine->mutable_converter_mock();

  scoped_ptr<Session> session(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  session->ResetContext(&command);
  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(convertermock->reset_conversion_called());

  convertermock->Reset();
  EXPECT_TRUE(SendKey("A", session.get(), &command));
  command.Clear();
  session->ResetContext(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_TRUE(convertermock->reset_conversion_called());
}

TEST_F(SessionTest, ClearUndoOnResetContext) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  commands::Command command;
  Segments segments;

  {  // Create segments
    InsertCharacterChars("aiueo", session.get(), &command);
    ConversionRequest request;
    SetComposer(session.get(), &request);
    SetAiueo(&segments);
    // Don't use FillT13Ns(). It makes platform dependent segments.
    // TODO(hsumita): Makes FillT13Ns() independent from platforms.
    Segment::Candidate *candidate;
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "aiueo";
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "AIUEO";
  }

  {
    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session->Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    // "あいうえお"
    EXPECT_SINGLE_SEGMENT(kAiueo, command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session->Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);

    command.Clear();
    session->ResetContext(&command);

    command.Clear();
    session->Undo(&command);
    // After reset, undo shouldn't run.
    EXPECT_FALSE(command.output().has_preedit());
  }
}

TEST_F(SessionTest, IssueResetConversion) {
  scoped_ptr<MockConverterEngineForReset> engine(
      new MockConverterEngineForReset);
  ConverterMockForReset *convertermock = engine->mutable_converter_mock();

  scoped_ptr<Session> session(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // any meaneangless key calls ResetConversion
  EXPECT_FALSE(convertermock->reset_conversion_called());
  EXPECT_TRUE(SendKey("enter", session.get(), &command));
  EXPECT_TRUE(convertermock->reset_conversion_called());

  convertermock->Reset();
  EXPECT_FALSE(convertermock->reset_conversion_called());
  EXPECT_TRUE(SendKey("space", session.get(), &command));
  EXPECT_TRUE(convertermock->reset_conversion_called());
}

TEST_F(SessionTest, IssueRevert) {
  scoped_ptr<MockConverterEngineForRevert> engine(
      new MockConverterEngineForRevert);
  ConverterMockForRevert *convertermock = engine->mutable_converter_mock();

  scoped_ptr<Session> session(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  // changes the state to PRECOMPOSITION
  session->IMEOn(&command);

  session->Revert(&command);

  EXPECT_FALSE(command.output().consumed());
  EXPECT_TRUE(convertermock->revert_conversion_called());
}

// Undo command must call RervertConversion
TEST_F(SessionTest, Issue3428520) {
  scoped_ptr<MockConverterEngineForRevert> engine(
      new MockConverterEngineForRevert);
  ConverterMockForRevert *convertermock = engine->mutable_converter_mock();

  scoped_ptr<Session> session(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  commands::Command command;
  Segments segments;

  InsertCharacterChars("aiueo", session.get(), &command);
  ConversionRequest request;
  SetComposer(session.get(), &request);
  SetAiueo(&segments);
  FillT13Ns(request, &segments);
  convertermock->SetStartConversionForRequest(&segments, true);

  command.Clear();
  session->Convert(&command);
  EXPECT_FALSE(command.output().has_result());
  // "あいうえお"
  EXPECT_SINGLE_SEGMENT(kAiueo, command);

  convertermock->SetCommitSegmentValue(&segments, true);
  command.Clear();
  session->Commit(&command);
  EXPECT_FALSE(command.output().has_preedit());
  // "あいうえお"
  EXPECT_RESULT(kAiueo, command);

  command.Clear();
  session->Undo(&command);

  // After check the status of revert_conversion_called.
  EXPECT_TRUE(convertermock->revert_conversion_called());
}

// Revert command must clear the undo context.
TEST_F(SessionTest, Issue5742293) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session->set_client_capability(capability);

  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);

  SetUndoContext(session.get());

  commands::Command command;

  // BackSpace key event issues Revert command, which should clear the undo
  // context.
  EXPECT_TRUE(SendKey("Backspace", session.get(), &command));

  // Ctrl+BS should be consumed as UNDO.
  EXPECT_TRUE(TestSendKey("Ctrl Backspace", session.get(), &command));

  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, AutoConversion) {
  Segments segments;
  SetAiueo(&segments);
  ConversionRequest default_request;
  FillT13Ns(default_request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  // Auto Off
  config::Config config;
  config.set_use_auto_conversion(false);
  config::ConfigHandler::SetConfig(config);
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    InsertCharacterChars("tesuto.", session.get(), &command);

    // "てすと。",
    EXPECT_SINGLE_SEGMENT_AND_KEY(
        "\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82",
        "\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82", command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    // "てすと。"
    InsertCharacterString("\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82",
                          "wrs/", session.get(), &command);

    // "てすと。",
    EXPECT_SINGLE_SEGMENT_AND_KEY(
        "\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82",
        "\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82", command);
  }

  // Auto On
  config.set_use_auto_conversion(true);
  config::ConfigHandler::SetConfig(config);
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;

    // The last "." is a triggering key for auto conversion
    InsertCharacterChars("tesuto.", session.get(), &command);

    EXPECT_SINGLE_SEGMENT_AND_KEY(kAiueo, kAiueo, command);
  }
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    commands::Command command;

    // The last "." is a triggering key for auto conversion
    // "てすと。",
    InsertCharacterString("\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8\xE3\x80\x82",
                          "wrs/", session.get(), &command);

    EXPECT_SINGLE_SEGMENT_AND_KEY(kAiueo, kAiueo, command);
  }

  // Don't trigger auto conversion for the pattern number + "."
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    InsertCharacterChars("123.", session.get(), &command);

    // "１２３．"
    EXPECT_SINGLE_SEGMENT_AND_KEY(
        "\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\xEF\xBC\x8E",
        "\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\xEF\xBC\x8E", command);
  }

  // Don't trigger auto conversion for the ".."
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    InsertCharacterChars("..", session.get(), &command);

    // "。。"
    EXPECT_SINGLE_SEGMENT_AND_KEY(
        "\xE3\x80\x82\xE3\x80\x82", "\xE3\x80\x82\xE3\x80\x82", command);
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    // "１２３。"
    InsertCharacterString("\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\xE3\x80\x82",
                          "123.", session.get(), &command);

    // "１２３．"
    EXPECT_SINGLE_SEGMENT_AND_KEY(
        "\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\xEF\xBC\x8E",
        "\xEF\xBC\x91\xEF\xBC\x92\xEF\xBC\x93\xEF\xBC\x8E", command);
  }

  // Don't trigger auto conversion for "." only.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    InsertCharacterChars(".", session.get(), &command);

    // "。"
    EXPECT_SINGLE_SEGMENT_AND_KEY("\xE3\x80\x82", "\xE3\x80\x82", command);
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());
    commands::Command command;

    // The last "." is a triggering key for auto conversion
    // "。",
    InsertCharacterString("\xE3\x80\x82", "/", session.get(), &command);

    // "。"
    EXPECT_SINGLE_SEGMENT_AND_KEY("\xE3\x80\x82", "\xE3\x80\x82", command);
  }

  // Do auto conversion even if romanji-table is modified.
  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get());

    // Modify romanji-table to convert "zz" -> "。"
    composer::Table zz_table;
    // "てすと。"
    zz_table.AddRule("te", "\xE3\x81\xA6", "");
    zz_table.AddRule("su", "\xE3\x81\x99", "");
    zz_table.AddRule("to", "\xE3\x81\xA8", "");
    zz_table.AddRule("zz", "\xE3\x80\x82", "");
    session->get_internal_composer_only_for_unittest()->SetTable(&zz_table);

    // The last "zz" is converted to "." and triggering key for auto conversion
    commands::Command command;
    InsertCharacterChars("tesutozz", session.get(), &command);

    EXPECT_SINGLE_SEGMENT_AND_KEY(kAiueo, kAiueo, command);
  }

  {
    const char trigger_key[] = ".,?!";

    // try all possible patterns.
    for (int kana_mode = 0; kana_mode < 2; ++kana_mode) {
      for (int onoff = 0; onoff < 2; ++onoff) {
        for (int pattern = 0; pattern <= 16; ++pattern) {
          config.set_use_auto_conversion(onoff != 0);
          config.set_auto_conversion_key(pattern);
          config::ConfigHandler::SetConfig(config);

          int flag[4];
          flag[0] = static_cast<int>(
              config.auto_conversion_key() &
              config::Config::AUTO_CONVERSION_KUTEN);
          flag[1] = static_cast<int>(
              config.auto_conversion_key() &
              config::Config::AUTO_CONVERSION_TOUTEN);
          flag[2] = static_cast<int>(
              config.auto_conversion_key() &
              config::Config::AUTO_CONVERSION_QUESTION_MARK);
          flag[3] = static_cast<int>(
              config.auto_conversion_key() &
              config::Config::AUTO_CONVERSION_EXCLAMATION_MARK);

          for (int i = 0; i < 4; ++i) {
            scoped_ptr<Session> session(new Session(engine_.get()));
            InitSessionToPrecomposition(session.get());
            commands::Command command;

            if (kana_mode) {
              // "てすと"
              string key = "\xE3\x81\xA6\xE3\x81\x99\xE3\x81\xA8";
              key += trigger_key[i];
              InsertCharacterString(key, "wst/", session.get(), &command);
            } else {
              string key = "tesuto";
              key += trigger_key[i];
              InsertCharacterChars(key, session.get(), &command);
            }
            EXPECT_TRUE(command.output().has_preedit());
            EXPECT_EQ(1, command.output().preedit().segment_size());
            EXPECT_TRUE(command.output().preedit().segment(0).has_value());
            EXPECT_TRUE(command.output().preedit().segment(0).has_key());

            if (onoff > 0 && flag[i] > 0) {
              // "あいうえお"
              EXPECT_EQ("\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86"
                        "\xe3\x81\x88\xe3\x81\x8a",
                        command.output().preedit().segment(0).key());
            } else {
              // Not "あいうえお"
              EXPECT_NE("\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86"
                        "\xe3\x81\x88\xe3\x81\x8a",
                        command.output().preedit().segment(0).key());
            }
          }
        }
      }
    }
  }
}

TEST_F(SessionTest, InputSpaceWithKatakanaMode) {
  // This is a unittest against http://b/3203944.
  // Input mode should not be changed when a space key is typed.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  EXPECT_TRUE(session->InputModeHiragana(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(mozc::commands::HIRAGANA, command.output().mode());

  SetSendKeyCommand("Space", &command);
  command.mutable_input()->mutable_key()->set_mode(commands::FULL_KATAKANA);
  EXPECT_TRUE(session->SendKey(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_RESULT(kFullWidthSpace, command);
  EXPECT_EQ(mozc::commands::FULL_KATAKANA, command.output().mode());
}

TEST_F(SessionTest, AlphanumericOfSSH) {
  // This is a unittest against http://b/3199626
  // 'ssh' (っｓｈ) + F10 should be 'ssh'.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  InsertCharacterChars("ssh", session.get(), &command);
  // "っｓｈ"
  EXPECT_EQ("\xE3\x81\xA3\xEF\xBD\x93\xEF\xBD\x88", GetComposition(command));

  Segments segments;
  // Set a dummy segments for ConvertToHalfASCII.
  {
    Segment *segment;
    segment = segments.add_segment();
    //    // "っｓｈ"
    //    segment->set_key("\xE3\x81\xA3\xEF\xBD\x93\xEF\xBD\x88");
    // "っsh"
    segment->set_key("\xE3\x81\xA3sh");

    segment->add_candidate()->value = "[SSH]";
  }
  ConversionRequest request;
  SetComposer(session.get(), &request);
  FillT13Ns(request, &segments);
  GetConverterMock()->SetStartConversionForRequest(&segments, true);

  command.Clear();
  EXPECT_TRUE(session->ConvertToHalfASCII(&command));
  EXPECT_SINGLE_SEGMENT("ssh", command);
}

TEST_F(SessionTest, KeitaiInput_toggle) {
  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);
  scoped_ptr<Session> session(new Session(engine_.get()));

  InitSessionToPrecomposition(session.get(), *mobile_request_);
  commands::Command command;

  SendKey("1", session.get(), &command);
  // "あ|"
  EXPECT_EQ(kHiraganaA, command.output().preedit().segment(0).value());
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendKey("1", session.get(), &command);
  // "い|"
  EXPECT_EQ("\xE3\x81\x84", command.output().preedit().segment(0).value());
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  SendKey("1", session.get(), &command);
  // "あ|"
  EXPECT_EQ(kHiraganaA, command.output().preedit().segment(0).value());
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendKey("2", session.get(), &command);
  // "あか|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8B",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(2, command.output().preedit().cursor());

  SendKey("2", session.get(), &command);
  // "あき|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(2, command.output().preedit().cursor());

  SendKey("*", session.get(), &command);
  // "あぎ|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8E",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(2, command.output().preedit().cursor());

  SendKey("*", session.get(), &command);
  // "あき|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(2, command.output().preedit().cursor());

  SendKey("3", session.get(), &command);
  // "あきさ|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(3, command.output().preedit().cursor());

  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  // "あきさ|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(3, command.output().preedit().cursor());

  SendKey("3", session.get(), &command);
  // "あきささ|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(4, command.output().preedit().cursor());

  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  // "あきさ|さ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(3, command.output().preedit().cursor());

  SendKey("4", session.get(), &command);
  // "あきさた|さ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95\xE3\x81\x9F\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(4, command.output().preedit().cursor());

  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  // "あきさ|たさ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x95\xE3\x81\x9F\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(3, command.output().preedit().cursor());

  SendKey("*", session.get(), &command);
  // "あきざ|たさ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95",
            command.output().preedit().segment(0).value());
  EXPECT_EQ(3, command.output().preedit().cursor());

  // Test for End key
  SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
  SendKey("6", session.get(), &command);
  SendKey("6", session.get(), &command);
  SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
  SendKey("6", session.get(), &command);
  SendKey("*", session.get(), &command);
  // "あきざたさひば|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0",
      command.output().preedit().segment(0).value());
  EXPECT_EQ(7, command.output().preedit().cursor());

  // Test for Right key
  SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
  SendKey("6", session.get(), &command);
  SendKey("6", session.get(), &command);
  SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
  SendKey("6", session.get(), &command);
  SendKey("*", session.get(), &command);
  // "あきざたさひばひば|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0",
      command.output().preedit().segment(0).value());
  EXPECT_EQ(9, command.output().preedit().cursor());

  // Test for Left key
  SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
  SendKey("6", session.get(), &command);
  SendKey("6", session.get(), &command);
  // "あきざたさひばひばひ|"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2",
      command.output().preedit().segment(0).value());
  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
  SendKey("6", session.get(), &command);
  // "あきざたさひばひばは|ひ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xAF"
      "\xE3\x81\xB2",
      command.output().preedit().segment(0).value());
  SendKey("*", session.get(), &command);
  // "あきざたさひばひばば|ひ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB0"
      "\xE3\x81\xB2",
      command.output().preedit().segment(0).value());
  EXPECT_EQ(10, command.output().preedit().cursor());

  // Test for Home key
  SendSpecialKey(commands::KeyEvent::HOME, session.get(), &command);
  // "|あきざたさひばひばばひ"
  EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F\xE3\x81\x95"
      "\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB0"
      "\xE3\x81\xB2",
      command.output().preedit().segment(0).value());
  SendKey("6", session.get(), &command);
  SendKey("*", session.get(), &command);
  // "ば|あきざたさひばひばばひ"
  EXPECT_EQ("\xE3\x81\xB0\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F"
      "\xE3\x81\x95\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0"
      "\xE3\x81\xB0\xE3\x81\xB2",
      command.output().preedit().segment(0).value());
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
  SendKey("5", session.get(), &command);
  // "ばあきざたさひばひばばひな|"
  EXPECT_EQ("\xE3\x81\xB0\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F"
      "\xE3\x81\x95\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0"
      "\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xAA",
      command.output().preedit().segment(0).value());
  SendKey("*", session.get(), &command);  // no effect
  // "ばあきざたさひばひばばひな|"
  EXPECT_EQ("\xE3\x81\xB0\xE3\x81\x82\xE3\x81\x8D\xE3\x81\x96\xE3\x81\x9F"
      "\xE3\x81\x95\xE3\x81\xB2\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xB0"
      "\xE3\x81\xB0\xE3\x81\xB2\xE3\x81\xAA",
      command.output().preedit().segment(0).value());
  EXPECT_EQ(13, command.output().preedit().cursor());
}

TEST_F(SessionTest, KeitaiInput_flick) {
  config::Config config;
  config.set_session_keymap(config::Config::MSIME);
  config::ConfigHandler::SetConfig(config);
  commands::Command command;

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get(), *mobile_request_);
    // "は"
    InsertCharacterCodeAndString('6', "\xE3\x81\xAF", session.get(), &command);
    // "し"
    InsertCharacterCodeAndString('3', "\xE3\x81\x97", session.get(), &command);
    SendKey("*", session.get(), &command);
    // "ょ"
    InsertCharacterCodeAndString('3', "\xE3\x82\x87", session.get(), &command);
    // "う"
    InsertCharacterCodeAndString('1', "\xE3\x81\x86", session.get(), &command);
    // "はじょう"
    EXPECT_EQ("\xE3\x81\xAF\xE3\x81\x98\xE3\x82\x87\xE3\x81\x86",
        command.output().preedit().segment(0).value());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get(), *mobile_request_);

    SendKey("6", session.get(), &command);
    SendKey("3", session.get(), &command);
    SendKey("3", session.get(), &command);
    SendKey("*", session.get(), &command);
    // "ょ"
    InsertCharacterCodeAndString('3', "\xE3\x82\x87", session.get(), &command);
    // "う"
    InsertCharacterCodeAndString('1', "\xE3\x81\x86", session.get(), &command);
    // "はじょう"
    EXPECT_EQ("\xE3\x81\xAF\xE3\x81\x98\xE3\x82\x87\xE3\x81\x86",
        command.output().preedit().segment(0).value());
  }

  {
    scoped_ptr<Session> session(new Session(engine_.get()));
    InitSessionToPrecomposition(session.get(), *mobile_request_);

    SendKey("1", session.get(), &command);
    SendKey("2", session.get(), &command);
    SendKey("3", session.get(), &command);
    SendKey("3", session.get(), &command);
    // "あかし"
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8B\xE3\x81\x97",
        command.output().preedit().segment(0).value());
    // "の"
    InsertCharacterCodeAndString('5', "\xE3\x81\xAE", session.get(), &command);
    // "く"
    InsertCharacterCodeAndString('2', "\xE3\x81\x8F", session.get(), &command);
    // "し"
    InsertCharacterCodeAndString('3', "\xE3\x81\x97", session.get(), &command);
    // "あかしのくし"
    EXPECT_EQ("\xE3\x81\x82\xE3\x81\x8B\xE3\x81\x97\xE3\x81\xAE\xE3\x81\x8F\xE3"
        "\x81\x97",
        command.output().preedit().segment(0).value());
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendKey("9", session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    // "ん"
    InsertCharacterCodeAndString('0', "\xE3\x82\x93", session.get(), &command);
    SendSpecialKey(commands::KeyEvent::END, session.get(), &command);
    SendKey("1", session.get(), &command);
    SendKey("1", session.get(), &command);
    SendKey("1", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);
    // "ゆ"
    InsertCharacterCodeAndString('8', "\xE3\x82\x86", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendKey("*", session.get(), &command);
    SendKey("*", session.get(), &command);
    // "あるかしんのくしゅう"
    EXPECT_EQ("\xE3\x81\x82\xE3\x82\x8B\xE3\x81\x8B\xE3\x81\x97\xE3\x82\x93\xE3"
        "\x81\xAE\xE3\x81\x8F\xE3\x81\x97\xE3\x82\x85\xE3\x81\x86",
        command.output().preedit().segment(0).value());
    SendSpecialKey(commands::KeyEvent::HOME, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    // "は"
    InsertCharacterCodeAndString('6', "\xE3\x81\xAF", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendKey("*", session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendSpecialKey(commands::KeyEvent::RIGHT, session.get(), &command);
    SendKey("6", session.get(), &command);
    SendKey("6", session.get(), &command);
    SendKey("6", session.get(), &command);
    // "あるぱかしんのふくしゅう"
    EXPECT_EQ("\xE3\x81\x82\xE3\x82\x8B\xE3\x81\xB1\xE3\x81\x8B\xE3\x81\x97\xE3"
        "\x82\x93\xE3\x81\xAE\xE3\x81\xB5\xE3\x81\x8F\xE3\x81\x97\xE3\x82\x85"
        "\xE3\x81\x86",
        command.output().preedit().segment(0).value());
  }
}

TEST_F(SessionTest, CommitCandidateAt2ndOf3Segments) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  ConversionRequest request;
  SetComposer(session.get(), &request);

  commands::Command command;
  InsertCharacterChars("nekonoshippowonuita", session.get(), &command);

  {  // Segments as conversion result.
    Segments segments;
    Segment *segment;
    Segment::Candidate *candidate;

    segment = segments.add_segment();
    // "ねこの"
    segment->set_key("\xE3\x81\xAD\xE3\x81\x93\xE3\x81\xAE");
    candidate = segment->add_candidate();
    // "猫の"
    candidate->value = "\xE7\x8C\xAB\xE3\x81\xAE";

    segment = segments.add_segment();
    // "しっぽを"
    segment->set_key("\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92");
    candidate = segment->add_candidate();
    // "しっぽを"
    candidate->value = "\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92";

    segment = segments.add_segment();
    // "ぬいた"
    segment->set_key("\xE3\x81\xAC\xE3\x81\x84\xE3\x81\x9F");
    candidate = segment->add_candidate();
    // "抜いた"
    candidate->value = "\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F";

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  command.Clear();
  session->Convert(&command);
  // "[猫の]|しっぽを|抜いた"

  command.Clear();
  session->SegmentFocusRight(&command);
  // "猫の|[しっぽを]|抜いた"

  {  // Segments as result of CommitHeadToFocusedSegments
    Segments segments;
    Segment *segment;
    Segment::Candidate *candidate;

    segment = segments.add_segment();
    // "ぬいた"
    segment->set_key("\xE3\x81\xAC\xE3\x81\x84\xE3\x81\x9F");
    candidate = segment->add_candidate();
    // "抜いた"
    candidate->value = "\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F";

    GetConverterMock()->SetCommitSegments(&segments, true);
  }

  command.Clear();
  command.mutable_input()->mutable_command()->set_id(0);
  ASSERT_TRUE(session->CommitCandidate(&command));
  // "抜いた"
  EXPECT_PREEDIT("\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F", command);
  // "抜いた" "ぬいた"
  EXPECT_SINGLE_SEGMENT_AND_KEY("\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F",
                                "\xE3\x81\xAC\xE3\x81\x84\xE3\x81\x9F",
                                command);
  // "猫のしっぽを"
  EXPECT_RESULT("\xE7\x8C\xAB\xE3\x81\xAE"
                "\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92", command);
}

TEST_F(SessionTest, CommitCandidateAt3rdOf3Segments) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  ConversionRequest request;
  SetComposer(session.get(), &request);

  commands::Command command;
  InsertCharacterChars("nekonoshippowonuita", session.get(), &command);

  {  // Segments as conversion result.
    Segments segments;
    Segment *segment;
    Segment::Candidate *candidate;

    segment = segments.add_segment();
    // "ねこの"
    segment->set_key("\xE3\x81\xAD\xE3\x81\x93\xE3\x81\xAE");
    candidate = segment->add_candidate();
    // "猫の"
    candidate->value = "\xE7\x8C\xAB\xE3\x81\xAE";

    segment = segments.add_segment();
    // "しっぽを"
    segment->set_key("\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92");
    candidate = segment->add_candidate();
    // "しっぽを"
    candidate->value = "\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92";

    segment = segments.add_segment();
    // "ぬいた"
    segment->set_key("\xE3\x81\xAC\xE3\x81\x84\xE3\x81\x9F");
    candidate = segment->add_candidate();
    // "抜いた"
    candidate->value = "\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F";

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
  }

  command.Clear();
  session->Convert(&command);
  // "[猫の]|しっぽを|抜いた"

  command.Clear();
  session->SegmentFocusRight(&command);
  session->SegmentFocusRight(&command);
  // "猫の|しっぽを|[抜いた]"

  {  // Segments as result of CommitHeadToFocusedSegments
    Segments segments;
    GetConverterMock()->SetCommitSegments(&segments, true);
  }

  command.Clear();
  command.mutable_input()->mutable_command()->set_id(0);
  ASSERT_TRUE(session->CommitCandidate(&command));
  EXPECT_FALSE(command.output().has_preedit());
  // "猫のしっぽを抜いた"
  EXPECT_RESULT("\xE7\x8C\xAB\xE3\x81\xAE"
                "\xE3\x81\x97\xE3\x81\xA3\xE3\x81\xBD\xE3\x82\x92"
                "\xE6\x8A\x9C\xE3\x81\x84\xE3\x81\x9F" , command);
}

TEST_F(SessionTest, CommitCandidate_suggestion) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get(), *mobile_request_);

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    AddCandidate("MOCHA", "MOCHA", segment);
    AddCandidate("MOZUKU", "MOZUKU", segment);
  }

  commands::Command command;
  SendKey("M", session.get(), &command);
  command.Clear();
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  EXPECT_EQ(2, command.output().candidates().candidate_size());
  EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

  GetConverterMock()->SetFinishConversion(
      scoped_ptr<Segments>(new Segments).get(), true);
  SetSendCommandCommand(commands::SessionCommand::SUBMIT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(1);
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_RESULT_AND_KEY("MOZUKU", "MOZUKU", command);
  EXPECT_FALSE(command.output().has_preedit());
  // Zero query suggestion fills the candidates.
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_EQ(0, command.output().preedit().cursor());
}

bool FindCandidateID(const commands::Candidates &candidates,
                      const string &value, int *id) {
  CHECK(id);
  for (size_t i = 0; i < candidates.candidate_size(); ++i) {
    const commands::Candidates::Candidate &candidate =
        candidates.candidate(i);
    if (candidate.value() == value) {
      *id = candidate.id();
      return true;
    }
  }
  return false;
}

TEST_F(SessionTest, CommitCandidate_T13N) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get(), *mobile_request_);

  {
    Segments segments;
    segments.set_request_type(Segments::SUGGESTION);

    Segment *segment;
    segment = segments.add_segment();
    segment->set_key("tok");
    AddCandidate("tok", "tok", segment);
    AddMetaCandidate("tok", "tok", segment);
    AddMetaCandidate("tok", "TOK", segment);
    AddMetaCandidate("tok", "Tok", segment);
    EXPECT_EQ("tok", segment->candidate(-1).value);
    EXPECT_EQ("TOK", segment->candidate(-2).value);
    EXPECT_EQ("Tok", segment->candidate(-3).value);

    GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
  }

  {
    Segments segments;
    segments.set_request_type(Segments::PREDICTION);

    Segment *segment;
    segment = segments.add_segment();
    segment->set_key("tok");
    AddCandidate("tok", "tok", segment);
    AddMetaCandidate("tok", "tok", segment);
    AddMetaCandidate("tok", "TOK", segment);
    AddMetaCandidate("tok", "Tok", segment);
    EXPECT_EQ("tok", segment->candidate(-1).value);
    EXPECT_EQ("TOK", segment->candidate(-2).value);
    EXPECT_EQ("Tok", segment->candidate(-3).value);
    GetConverterMock()->SetStartPredictionForRequest(&segments, true);
  }

  commands::Command command;
  SendKey("k", session.get(), &command);
  ASSERT_TRUE(command.output().has_candidates());
  int id = 0;
#if defined(OS_WIN) || defined(OS_MACOSX)
  // meta candidates are in cascading window
  EXPECT_FALSE(FindCandidateID(command.output().candidates(), "TOK", &id));
#else
  EXPECT_TRUE(FindCandidateID(command.output().candidates(), "TOK", &id));
  GetConverterMock()->SetFinishConversion(
      scoped_ptr<Segments>(new Segments).get(), true);
  SetSendCommandCommand(commands::SessionCommand::SUBMIT_CANDIDATE, &command);
  command.mutable_input()->mutable_command()->set_id(id);
  session->SendCommand(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_RESULT("TOK", command);
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_EQ(0, command.output().preedit().cursor());
#endif
}

TEST_F(SessionTest, RequestConvertReverse) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  EXPECT_TRUE(session->RequestConvertReverse(&command));
  EXPECT_FALSE(command.output().has_result());
  EXPECT_FALSE(command.output().has_deletion_range());
  EXPECT_TRUE(command.output().has_callback());
  EXPECT_TRUE(command.output().callback().has_session_command());
  EXPECT_EQ(commands::SessionCommand::CONVERT_REVERSE,
            command.output().callback().session_command().type());
}

TEST_F(SessionTest, ConvertReverse) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "阿伊宇江於"
  const char kKanjiAiueo[] =
      "\xe9\x98\xbf\xe4\xbc\x8a\xe5\xae\x87\xe6\xb1\x9f\xe6\x96\xbc";
  commands::Command command;
  SetupCommandForReverseConversion(kKanjiAiueo, command.mutable_input());
  SetupMockForReverseConversion(kKanjiAiueo, kAiueo);

  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kKanjiAiueo,
            command.output().preedit().segment(0).value());
  EXPECT_EQ(kKanjiAiueo,
            command.output().all_candidate_words().candidates(0).value());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_GT(command.output().candidates().candidate_size(), 0);
}

TEST_F(SessionTest, EscapeFromConvertReverse) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "阿伊宇江於"
  const char kKanjiAiueo[] =
      "\xe9\x98\xbf\xe4\xbc\x8a\xe5\xae\x87\xe6\xb1\x9f\xe6\x96\xbc";

  commands::Command command;
  SetupCommandForReverseConversion(kKanjiAiueo, command.mutable_input());
  SetupMockForReverseConversion(kKanjiAiueo, kAiueo);

  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kKanjiAiueo, GetComposition(command));

  SendKey("ESC", session.get(), &command);

  // KANJI should be converted into HIRAGANA in pre-edit state.
  EXPECT_SINGLE_SEGMENT(kAiueo, command);

  SendKey("ESC", session.get(), &command);

  // Fixed KANJI should be output
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_RESULT(kKanjiAiueo, command);
}

TEST_F(SessionTest, SecondEscapeFromConvertReverse) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "阿伊宇江於"
  const char kKanjiAiueo[] =
      "\xe9\x98\xbf\xe4\xbc\x8a\xe5\xae\x87\xe6\xb1\x9f\xe6\x96\xbc";
  commands::Command command;
  SetupCommandForReverseConversion(kKanjiAiueo, command.mutable_input());
  SetupMockForReverseConversion(kKanjiAiueo, kAiueo);

  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kKanjiAiueo, GetComposition(command));

  SendKey("ESC", session.get(), &command);
  SendKey("ESC", session.get(), &command);

  EXPECT_FALSE(command.output().has_preedit());
  // When a reverse conversion is canceled, the converter sets the
  // original text into |command.output().result().key()|.
  EXPECT_RESULT_AND_KEY(kKanjiAiueo, kKanjiAiueo, command);

  SendKey("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  SendKey("ESC", session.get(), &command);
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, SecondEscapeFromConvertReverse_Issue5687022) {
  // This is a unittest against http://b/5687022
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  const char kInput[] = "abcde";
  const char kReading[] = "abcde";

  commands::Command command;
  SetupCommandForReverseConversion(kInput, command.mutable_input());
  SetupMockForReverseConversion(kInput, kReading);

  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kInput, GetComposition(command));

  SendKey("ESC", session.get(), &command);
  SendKey("ESC", session.get(), &command);

  EXPECT_FALSE(command.output().has_preedit());
  // When a reverse conversion is canceled, the converter sets the
  // original text into |result().key()|.
  EXPECT_RESULT_AND_KEY(kInput, kInput, command);
}

TEST_F(SessionTest, SecondEscapeFromConvertReverseKeepsOriginalText) {
  // Second escape from ConvertReverse should restore the original text
  // without any text normalization even if the input text contains any
  // special characters which Mozc usually do normalization.

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "ゔ"
  const char kInput[] = "\xE3\x82\x94";

  commands::Command command;
  SetupCommandForReverseConversion(kInput, command.mutable_input());
  SetupMockForReverseConversion(kInput, kInput);

  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kInput, GetComposition(command));

  SendKey("ESC", session.get(), &command);
  SendKey("ESC", session.get(), &command);

  EXPECT_FALSE(command.output().has_preedit());

  // When a reverse conversion is canceled, the converter sets the
  // original text into |result().key()|.
  EXPECT_RESULT_AND_KEY(kInput, kInput, command);
}

TEST_F(SessionTest, EscapeFromCompositionAfterConvertReverse) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "阿伊宇江於"
  const char kKanjiAiueo[] =
      "\xe9\x98\xbf\xe4\xbc\x8a\xe5\xae\x87\xe6\xb1\x9f\xe6\x96\xbc";

  commands::Command command;
  SetupCommandForReverseConversion(kKanjiAiueo, command.mutable_input());
  SetupMockForReverseConversion(kKanjiAiueo, kAiueo);

  // Conversion Reverse
  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kKanjiAiueo, GetComposition(command));

  session->Commit(&command);

  EXPECT_RESULT(kKanjiAiueo, command);

  // Escape in composition state
  SendKey("a", session.get(), &command);
  // "あ"
  EXPECT_EQ(kHiraganaA, GetComposition(command));

  SendKey("ESC", session.get(), &command);
  EXPECT_FALSE(command.output().has_preedit());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, ConvertReverseFromOffState) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  // "阿伊宇江於"
  const string kanji_aiueo =
      "\xe9\x98\xbf\xe4\xbc\x8a\xe5\xae\x87\xe6\xb1\x9f\xe6\x96\xbc";

  // IMEOff
  commands::Command command;
  SendSpecialKey(commands::KeyEvent::OFF, session.get(), &command);

  SetupCommandForReverseConversion(kanji_aiueo, command.mutable_input());
  SetupMockForReverseConversion(kanji_aiueo, kAiueo);
  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
}

TEST_F(SessionTest, DCHECKFailureAfterConvertReverse) {
  // This is a unittest against http://b/5145295.
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  SetupCommandForReverseConversion(kAiueo, command.mutable_input());
  SetupMockForReverseConversion(kAiueo, kAiueo);
  EXPECT_TRUE(session->SendCommand(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(kAiueo, command.output().preedit().segment(0).value());
  EXPECT_EQ(kAiueo,
      command.output().all_candidate_words().candidates(0).value());
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_GT(command.output().candidates().candidate_size(), 0);

  SendKey("ESC", session.get(), &command);
  SendKey("a", session.get(), &command);
  // "あいうえおあ"
  EXPECT_EQ(string(kAiueo) + kHiraganaA,
            command.output().preedit().segment(0).value());
  EXPECT_FALSE(command.output().has_result());
}

TEST_F(SessionTest, LaunchTool) {
  scoped_ptr<Session> session(new Session(engine_.get()));

  {
    commands::Command command;
    EXPECT_TRUE(session->LaunchConfigDialog(&command));
    EXPECT_EQ(commands::Output::CONFIG_DIALOG,
              command.output().launch_tool_mode());
    EXPECT_TRUE(command.output().consumed());
  }

  {
    commands::Command command;
    EXPECT_TRUE(session->LaunchDictionaryTool(&command));
    EXPECT_EQ(commands::Output::DICTIONARY_TOOL,
              command.output().launch_tool_mode());
    EXPECT_TRUE(command.output().consumed());
  }

  {
    commands::Command command;
    EXPECT_TRUE(session->LaunchWordRegisterDialog(&command));
    EXPECT_EQ(commands::Output::WORD_REGISTER_DIALOG,
              command.output().launch_tool_mode());
    EXPECT_TRUE(command.output().consumed());
  }
}

TEST_F(SessionTest, NotZeroQuerySuggest) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // Disable zero query suggest.
  commands::Request request;
  request.set_zero_query_suggestion(false);
  session->SetRequest(&request);

  // Type "google".
  commands::Command command;
  InsertCharacterChars("google", session.get(), &command);
  EXPECT_EQ("google", GetComposition(command));

  // Set up a mock suggestion result.
  Segments segments;
  segments.set_request_type(Segments::SUGGESTION);
  Segment *segment;
  segment = segments.add_segment();
  segment->set_key("");
  segment->add_candidate()->value = "search";
  segment->add_candidate()->value = "input";
  GetConverterMock()->SetStartSuggestionForRequest(&segments, true);

  // Commit composition and zero query suggest should not be invoked.
  command.Clear();
  session->Commit(&command);
  EXPECT_EQ("google", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_FALSE(command.output().has_candidates());

  const ImeContext &context = session->context();
  EXPECT_EQ(ImeContext::PRECOMPOSITION, context.state());
}

TEST_F(SessionTest, ZeroQuerySuggest) {
  {  // Commit
    Session session(engine_.get());
    commands::Request request;
    SetupZeroQuerySuggestionReady(true, &session, &request);

    commands::Command command;
    session.Commit(&command);
    EXPECT_EQ("GOOGLE", command.output().result().value());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("search", command.output().candidates().candidate(0).value());
    EXPECT_EQ("input", command.output().candidates().candidate(1).value());
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // CommitSegment
    Session session(engine_.get());
    commands::Request request;
    SetupZeroQuerySuggestionReady(true, &session, &request);

    commands::Command command;
    session.CommitSegment(&command);
    EXPECT_EQ("GOOGLE", command.output().result().value());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("search", command.output().candidates().candidate(0).value());
    EXPECT_EQ("input", command.output().candidates().candidate(1).value());
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // CommitCandidate
    Session session(engine_.get());
    commands::Request request;
    SetupZeroQuerySuggestionReady(true, &session, &request);

    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::SUBMIT_CANDIDATE, &command);
    command.mutable_input()->mutable_command()->set_id(0);
    session.SendCommand(&command);

    EXPECT_EQ("GOOGLE", command.output().result().value());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("search", command.output().candidates().candidate(0).value());
    EXPECT_EQ("input", command.output().candidates().candidate(1).value());
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // CommitFirstSuggestion
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Enable zero query suggest.
    commands::Request request;
    request.set_zero_query_suggestion(true);
    session.SetRequest(&request);

    // Type "g".
    commands::Command command;
    InsertCharacterChars("g", &session, &command);

    {
      // Set up a mock conversion result.
      Segments segments;
      segments.set_request_type(Segments::SUGGESTION);
      Segment *segment;
      segment = segments.add_segment();
      segment->set_key("");
      segment->add_candidate()->value = "google";
      GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
    }

    command.Clear();
    InsertCharacterChars("o", &session, &command);

    {
      // Set up a mock suggestion result.
      Segments segments;
      segments.set_request_type(Segments::SUGGESTION);
      Segment *segment;
      segment = segments.add_segment();
      segment->set_key("");
      segment->add_candidate()->value = "search";
      segment->add_candidate()->value = "input";
      GetConverterMock()->SetStartSuggestionForRequest(&segments, true);
    }

    command.Clear();
    Segments empty_segments;
    GetConverterMock()->SetFinishConversion(&empty_segments, true);
    session.CommitFirstSuggestion(&command);
    EXPECT_EQ("google", command.output().result().value());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("search", command.output().candidates().candidate(0).value());
    EXPECT_EQ("input", command.output().candidates().candidate(1).value());
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }
}

TEST_F(SessionTest, CommandsAfterZeroQuerySuggest) {
  {  // Cancel command should close the candidate window.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    command.Clear();
    session.EditCancel(&command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // PredictAndConvert should select the first candidate.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    command.Clear();
    session.PredictAndConvert(&command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result());
    // "search" is the first suggest candidate.
    EXPECT_PREEDIT("search", command);
    EXPECT_EQ(ImeContext::CONVERSION, session.context().state());
  }

  {  // CommitFirstSuggestion should insert the first candidate.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    command.Clear();
    // FinishConversion is expected to return empty Segments.
    GetConverterMock()->SetFinishConversion(
        scoped_ptr<Segments>(new Segments).get(), true);
    session.CommitFirstSuggestion(&command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_EQ("", GetComposition(command));
    // "search" is the first suggest candidate.
    EXPECT_RESULT("search", command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // Space should be inserted directly.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    SendKey("Space", &session, &command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_EQ("", GetComposition(command));
    // "　" (full-width space)
    EXPECT_RESULT(kFullWidthSpace, command);
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // 'a' should be inserted in the composition.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());

    SendKey("a", &session, &command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ(commands::HIRAGANA, command.output().mode());
    // "あ"
    EXPECT_PREEDIT(kHiraganaA, command);
    EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());
  }

  {  // Enter should be inserted directly.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    SendKey("Enter", &session, &command);
    EXPECT_FALSE(command.output().consumed());
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // Right should be inserted directly.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    SendKey("Right", &session, &command);
    EXPECT_FALSE(command.output().consumed());
    EXPECT_FALSE(command.output().has_preedit());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  }

  {  // SelectCnadidate command should work with zero query suggestion.
    Session session(engine_.get());
    commands::Request request;
    commands::Command command;
    SetupZeroQuerySuggestion(&session, &request, &command);

    // Send SELECT_CANDIDATE command.
    const int first_id = command.output().candidates().candidate(0).id();
    SetSendCommandCommand(commands::SessionCommand::SELECT_CANDIDATE, &command);
    command.mutable_input()->mutable_command()->set_id(first_id);
    EXPECT_TRUE(session.SendCommand(&command));

    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result());
    // "search" is the first suggest candidate.
    EXPECT_PREEDIT("search", command);
    EXPECT_EQ(ImeContext::CONVERSION, session.context().state());
  }
}

TEST_F(SessionTest, Issue4437420) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);
  commands::Command command;
  commands::Request request;
  // Creates overriding config.
  config::Config overriding_config;
  overriding_config.set_session_keymap(config::Config::MOBILE);
  // Change to 12keys-halfascii mode.
  SwitchInputMode(commands::HALF_ASCII, &session);

  command.Clear();
  request.set_special_romanji_table(
      commands::Request::TWELVE_KEYS_TO_HALFWIDTHASCII);
  session.SetRequest(&request);
  scoped_ptr<composer::Table> table(new composer::Table());
  table.get()->InitializeWithRequestAndConfig(
      request, config::ConfigHandler::GetConfig());
  session.SetTable(table.get());
  // Type "2*" to produce "A".
  SetSendKeyCommand("2", &command);
  command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
  session.SendKey(&command);
  SetSendKeyCommand("*", &command);
  command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
  session.SendKey(&command);
  EXPECT_EQ("A", GetComposition(command));

  // Change to 12keys-number mode.
  SwitchInputMode(commands::HALF_ASCII, &session);

  command.Clear();
  request.set_special_romanji_table(commands::Request::TWELVE_KEYS_TO_NUMBER);
  session.SetRequest(&request);
  table.reset(new composer::Table());
  table.get()->InitializeWithRequestAndConfig(
      request, config::ConfigHandler::GetConfig());
  session.SetTable(table.get());
  // Type "2" to produce "A2".
  SetSendKeyCommand("2", &command);
  command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
  session.SendKey(&command);
  EXPECT_EQ("A2", GetComposition(command));

  // Change to 12keys-halfascii mode.
  SwitchInputMode(commands::HALF_ASCII, &session);

  command.Clear();
  request.set_special_romanji_table(
      commands::Request::TWELVE_KEYS_TO_HALFWIDTHASCII);
  session.SetRequest(&request);
  table.reset(new composer::Table());
  table.get()->InitializeWithRequestAndConfig(
      request, config::ConfigHandler::GetConfig());
  session.SetTable(table.get());
  // Type "2" to produce "A2a".
  SetSendKeyCommand("2", &command);
  command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
  session.SendKey(&command);
  EXPECT_EQ("A2a", GetComposition(command));
  command.Clear();
}

// If undo context is empty, key event for UNDO should be echoed back. b/5553298
TEST_F(SessionTest, Issue5553298) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  // Undo requires capability DELETE_PRECEDING_TEXT.
  commands::Capability capability;
  capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
  session.set_client_capability(capability);

  commands::Command command;
  session.ResetContext(&command);

  SetSendKeyCommand("Ctrl Backspace", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MSIME);
  session.TestSendKey(&command);
  EXPECT_FALSE(command.output().consumed());

  SetSendKeyCommand("Ctrl Backspace", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MSIME);
  session.SendKey(&command);
  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, UndoKeyAction) {
  commands::Command command;
  commands::Request request;
  // Creates overriding config.
  config::Config overriding_config;
  overriding_config.set_session_keymap(config::Config::MOBILE);
  // Test in half width ascii mode.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Change to 12keys-halfascii mode.
    SwitchInputMode(commands::HALF_ASCII, &session);

    command.Clear();
    request.set_special_romanji_table(
        commands::Request::TWELVE_KEYS_TO_HALFWIDTHASCII);
    session.SetRequest(&request);
    composer::Table table;
    table.InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session.SetTable(&table);

    // Type "2" to produce "a".
    SetSendKeyCommand("2", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    EXPECT_EQ("a", GetComposition(command));

    // Type "2" again to produce "b".
    SetSendKeyCommand("2", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    EXPECT_EQ("b", GetComposition(command));

    // Push UNDO key to reproduce "a".
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    EXPECT_EQ("a", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());

    // Push UNDO key again to produce "2".
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    EXPECT_EQ("2", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
  }

  // Test in Hiaragana-mode.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Change to 12keys-Hiragana mode.
    SwitchInputMode(commands::HIRAGANA, &session);

    command.Clear();
    request.set_special_romanji_table(
        commands::Request::TWELVE_KEYS_TO_HIRAGANA);
    session.SetRequest(&request);
    composer::Table table;
    table.InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session.SetTable(&table);
    // Type "33{<}{<}" to produce "さ"->"し"->"さ"->"そ".
    SetSendKeyCommand("3", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "さ"
    EXPECT_EQ("\xE3\x81\x95", GetComposition(command));

    SetSendKeyCommand("3", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "し"
    EXPECT_EQ("\xE3\x81\x97", GetComposition(command));

    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    // "さ"
    EXPECT_EQ("\xE3\x81\x95", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();

    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    // "そ"
    EXPECT_EQ("\xE3\x81\x9D", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
  }

  // Test to do nothing for voiced sounds.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Change to 12keys-Hiragana mode.
    SwitchInputMode(commands::HIRAGANA, &session);

    command.Clear();
    request.set_special_romanji_table(
        commands::Request::TWELVE_KEYS_TO_HIRAGANA);
    session.SetRequest(&request);
    composer::Table table;
    table.InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session.SetTable(&table);
    // Type "3*{<}*{<}", and composition should change
    // "さ"->"ざ"->(No change)->"さ"->(No change).
    SetSendKeyCommand("3", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "さ"
    EXPECT_EQ("\xE3\x81\x95", GetComposition(command));

    SetSendKeyCommand("*", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "ざ"
    EXPECT_EQ("\xE3\x81\x96", GetComposition(command));

    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    // "ざ"
    EXPECT_EQ("\xE3\x81\x96", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());


    SetSendKeyCommand("*", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "さ"
    EXPECT_EQ("\xE3\x81\x95", GetComposition(command));
    command.Clear();

    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    // "さ"
    EXPECT_EQ("\xE3\x81\x95", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
  }

  // Test to make nothing newly in preedit for empty composition.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Change to 12keys-Hiragana mode.
    SwitchInputMode(commands::HIRAGANA, &session);

    command.Clear();
    request.set_special_romanji_table(
        commands::Request::TWELVE_KEYS_TO_HIRAGANA);
    session.SetRequest(&request);
    composer::Table table;
    table.InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session.SetTable(&table);
    // Type "{<}" and do nothing
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);

    EXPECT_FALSE(command.output().has_preedit());

    command.Clear();
  }

  // Test of acting as UNDO key. Almost same as the first section in Undo test.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    commands::Capability capability;
    capability.set_text_deletion(commands::Capability::DELETE_PRECEDING_TEXT);
    session.set_client_capability(capability);

    Segments segments;
    InsertCharacterChars("aiueo", &session, &command);
    ConversionRequest request;
    SetComposer(&session, &request);
    SetAiueo(&segments);
    Segment::Candidate *candidate;
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "aiueo";
    candidate = segments.mutable_segment(0)->add_candidate();
    candidate->value = "AIUEO";

    GetConverterMock()->SetStartConversionForRequest(&segments, true);
    command.Clear();
    session.Convert(&command);
    EXPECT_FALSE(command.output().has_result());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);

    GetConverterMock()->SetCommitSegmentValue(&segments, true);
    command.Clear();
    session.Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あいうえお"
    EXPECT_RESULT(kAiueo, command);

    command.Clear();
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_TRUE(command.output().has_deletion_range());
    EXPECT_EQ(-5, command.output().deletion_range().offset());
    EXPECT_EQ(5, command.output().deletion_range().length());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);
    EXPECT_TRUE(command.output().consumed());

    // Undo twice - do nothing and keep the previous status.
    command.Clear();
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    session.SendCommand(&command);
    EXPECT_FALSE(command.output().has_result());
    EXPECT_FALSE(command.output().has_deletion_range());
    // "あいうえお"
    EXPECT_PREEDIT(kAiueo, command);
    EXPECT_TRUE(command.output().consumed());
  }

  // Do not UNDO even if UNDO stack is not empty if it is in COMPOSITE state.
  {
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    // Change to 12keys-Hiragana mode.
    SwitchInputMode(commands::HIRAGANA, &session);

    command.Clear();
    request.set_special_romanji_table(
        commands::Request::TWELVE_KEYS_TO_HIRAGANA);
    session.SetRequest(&request);
    composer::Table table;
    table.InitializeWithRequestAndConfig(
        request, config::ConfigHandler::GetConfig());
    session.SetTable(&table);

    // commit "あ" to push UNDO stack
    SetSendKeyCommand("1", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "あ"
    EXPECT_EQ(kHiraganaA, GetComposition(command));
    command.Clear();

    session.Commit(&command);
    EXPECT_FALSE(command.output().has_preedit());
    // "あ"
    EXPECT_RESULT(kHiraganaA, command);

    // Produce "か" in composition.
    SetSendKeyCommand("2", &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendKey(&command);
    // "か"
    EXPECT_EQ("\xE3\x81\x8B", GetComposition(command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();

    // Send UNDO_OR_REWIND key, then get "こ" in composition
    SetSendCommandCommand(commands::SessionCommand::UNDO_OR_REWIND, &command);
    command.mutable_input()->mutable_config()->CopyFrom(overriding_config);
    session.SendCommand(&command);
    // "こ"
    EXPECT_PREEDIT("\xE3\x81\x93", command);
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
  }
}

TEST_F(SessionTest, TemporaryKeyMapChange) {
  config::Config config;
  config::ConfigHandler::GetDefaultConfig(&config);
  config.set_session_keymap(config::Config::ATOK);
  config::ConfigHandler::SetConfig(config);

  // Session created with keymap ATOK
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);
  EXPECT_EQ(config::Config::ATOK, session.context().keymap());

  // TestSendKey with keymap MOBLE
  commands::Command command;
  SetSendKeyCommand("G", &command);
  command.mutable_input()->mutable_config()->set_session_keymap(
      config::Config::MOBILE);
  session.TestSendKey(&command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(config::Config::MOBILE, session.context().keymap());

  // TestSendKey without keymap
  TestSendKey("G", &session, &command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(config::Config::ATOK, session.context().keymap());
}

TEST_F(SessionTest, MoveCursor) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;

  InsertCharacterChars("MOZUKU", session.get(), &command);
  EXPECT_EQ(6, command.output().preedit().cursor());
  session->MoveCursorLeft(&command);
  EXPECT_EQ(5, command.output().preedit().cursor());
  command.mutable_input()->mutable_command()->set_cursor_position(3);
  session->MoveCursorTo(&command);
  EXPECT_EQ(3, command.output().preedit().cursor());
  session->MoveCursorRight(&command);
  EXPECT_EQ(4, command.output().preedit().cursor());
}

TEST_F(SessionTest, MoveCursorRightWithCommit) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(
      commands::Request::QWERTY_MOBILE_TO_HALFWIDTHASCII);
  request.set_crossing_edge_behavior(
      commands::Request::COMMIT_WITHOUT_CONSUMING);
  InitSessionToPrecomposition(session.get(), request);
  commands::Command command;

  InsertCharacterChars("MOZC", session.get(), &command);
  EXPECT_EQ(4, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(3, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorRight(&command);
  EXPECT_EQ(4, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorRight(&command);
  EXPECT_FALSE(command.output().consumed());
  ASSERT_TRUE(command.output().has_result());
  EXPECT_EQ(commands::Result_ResultType_STRING,
            command.output().result().type());
  EXPECT_EQ("MOZC", command.output().result().value());
  EXPECT_EQ(0, command.output().result().cursor_offset());
}

TEST_F(SessionTest, MoveCursorLeftWithCommit) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(
      commands::Request::QWERTY_MOBILE_TO_HALFWIDTHASCII);
  request.set_crossing_edge_behavior(
      commands::Request::COMMIT_WITHOUT_CONSUMING);
  InitSessionToPrecomposition(session.get(), request);
  commands::Command command;

  InsertCharacterChars("MOZC", session.get(), &command);
  EXPECT_EQ(4, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(3, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(2, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(1, command.output().preedit().cursor());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(0, command.output().preedit().cursor());
  command.Clear();

  session->MoveCursorLeft(&command);
  EXPECT_FALSE(command.output().consumed());
  ASSERT_TRUE(command.output().has_result());
  EXPECT_EQ(commands::Result_ResultType_STRING,
            command.output().result().type());
  EXPECT_EQ("MOZC", command.output().result().value());
  EXPECT_EQ(-4, command.output().result().cursor_offset());
}

TEST_F(SessionTest, CommitHead) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  composer::Table table;
  // "も"
  table.AddRule("mo", "\xe3\x82\x82", "");
  // "ず"
  table.AddRule("zu", "\xe3\x81\x9a", "");

  session->get_internal_composer_only_for_unittest()->SetTable(&table);

  InitSessionToPrecomposition(session.get());
  commands::Command command;

  InsertCharacterChars("moz", session.get(), &command);
  // 'もｚ'
  EXPECT_EQ("\xe3\x82\x82\xef\xbd\x9a", GetComposition(command));
  command.Clear();
  session->CommitHead(1, &command);
  EXPECT_EQ(commands::Result_ResultType_STRING,
            command.output().result().type());
  EXPECT_EQ("\xe3\x82\x82", command.output().result().value());  // 'も'
  EXPECT_EQ("\xef\xbd\x9a", GetComposition(command));            // 'ｚ'
  InsertCharacterChars("u", session.get(), &command);
  // 'ず'
  EXPECT_EQ("\xe3\x81\x9a", GetComposition(command));
}

TEST_F(SessionTest, PasswordWithToggleAlpabetInput) {
  scoped_ptr<Session> session(new Session(engine_.get()));

  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(
      commands::Request::TWELVE_KEYS_TO_HALFWIDTHASCII);

  InitSessionToPrecomposition(session.get(), request);

  // Change to 12keys-halfascii mode.
  SwitchInputFieldType(commands::Context::PASSWORD, session.get());
  SwitchInputMode(commands::HALF_ASCII, session.get());

  commands::Command command;
  SendKey("2", session.get(), &command);
  EXPECT_EQ("a", GetComposition(command));
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendKey("2", session.get(), &command);
  EXPECT_EQ("b", GetComposition(command));
  EXPECT_EQ(1, command.output().preedit().cursor());

  // cursor key commits the preedit.
  SendKey("right", session.get(), &command);
  // "b"
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("b", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_EQ(0, command.output().preedit().cursor());

  SendKey("2", session.get(), &command);
  // "b[a]"
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  EXPECT_EQ("a", GetComposition(command));
  EXPECT_EQ(1, command.output().preedit().cursor());

  SendKey("4", session.get(), &command);
  // ba[g]
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("a", command.output().result().value());
  EXPECT_EQ("g", GetComposition(command));
  EXPECT_EQ(1, command.output().preedit().cursor());

  // cursor key commits the preedit.
  SendKey("left", session.get(), &command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("g", command.output().result().value());
  EXPECT_EQ(0, command.output().preedit().segment_size());
  EXPECT_EQ(0, command.output().preedit().cursor());
}

TEST_F(SessionTest, SwitchInputFieldType) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  // initial state is NORMAL
  EXPECT_EQ(commands::Context::NORMAL,
            session->context().composer().GetInputFieldType());

  {
    SCOPED_TRACE("Switch input field type to PASSWORD");
    SwitchInputFieldType(commands::Context::PASSWORD, session.get());
  }
  {
    SCOPED_TRACE("Switch input field type to NORMAL");
    SwitchInputFieldType(commands::Context::NORMAL, session.get());
  }
}

TEST_F(SessionTest, CursorKeysInPasswordMode) {
  scoped_ptr<Session> session(new Session(engine_.get()));

  commands::Request request;
  request.CopyFrom(*mobile_request_);
  request.set_special_romanji_table(commands::Request::DEFAULT_TABLE);
  session->SetRequest(&request);

  InitSessionToPrecomposition(session.get(), request);

  SwitchInputFieldType(commands::Context::PASSWORD, session.get());
  SwitchInputMode(commands::HALF_ASCII, session.get());

  commands::Command command;
  // cursor key commits the preedit without moving system cursor.
  SendKey("m", session.get(), &command);
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  command.Clear();
  session->MoveCursorLeft(&command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("m", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  VLOG(0) << command.DebugString();
  EXPECT_EQ(0, command.output().preedit().cursor());
  EXPECT_TRUE(command.output().consumed());

  SendKey("o", session.get(), &command);
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  command.Clear();
  session->MoveCursorRight(&command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("o", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_EQ(0, command.output().preedit().cursor());
  EXPECT_TRUE(command.output().consumed());

  SendKey("z", session.get(), &command);
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  SetSendCommandCommand(commands::SessionCommand::MOVE_CURSOR, &command);
  command.mutable_input()->mutable_command()->set_cursor_position(3);
  session->MoveCursorTo(&command);
  EXPECT_EQ("z", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_EQ(0, command.output().preedit().cursor());
  EXPECT_TRUE(command.output().consumed());
}

TEST_F(SessionTest, BackKeyCommitsPreeditInPasswordMode) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  commands::Request request;

  request.set_zero_query_suggestion(false);
  request.set_combine_all_segments(true);
  request.set_special_romanji_table(commands::Request::DEFAULT_TABLE);
  session->SetRequest(&request);

  composer::Table table;
  table.InitializeWithRequestAndConfig(
      request, config::ConfigHandler::GetConfig());
  session->SetTable(&table);

  SwitchInputFieldType(commands::Context::PASSWORD, session.get());
  SwitchInputMode(commands::HALF_ASCII, session.get());

  SendKey("m", session.get(), &command);
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  EXPECT_EQ("m", GetComposition(command));
  SendKey("esc", session.get(), &command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("m", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_FALSE(command.output().consumed());

  SendKey("o", session.get(), &command);
  SendKey("z", session.get(), &command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("o", command.output().result().value());
  EXPECT_EQ("z", GetComposition(command));
  SendKey("esc", session.get(), &command);
  EXPECT_EQ(commands::Result::STRING, command.output().result().type());
  EXPECT_EQ("z", command.output().result().value());
  EXPECT_EQ("", GetComposition(command));
  EXPECT_FALSE(command.output().consumed());

  // in normal mode, preedit is cleared without commit.
  SwitchInputFieldType(commands::Context::NORMAL, session.get());

  SendKey("m", session.get(), &command);
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  EXPECT_EQ("m", GetComposition(command));
  SendKey("esc", session.get(), &command);
  EXPECT_TRUE(command.output().consumed());
  EXPECT_EQ(commands::Result::NONE, command.output().result().type());
  EXPECT_FALSE(command.output().has_preedit());
}

TEST_F(SessionTest, EditCancel) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  {  // Cancel of Suggestion
    commands::Command command;
    SendKey("M", &session, &command);

    GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
    SendKey("O", &session, &command);
    ASSERT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

    command.Clear();
    session.EditCancel(&command);
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
  }

  {  // Cancel of Reverse conversion
    commands::Command command;

    // "[MO]" is a converted string like Kanji.
    // "MO" is an input string like Hiragana.
    SetupCommandForReverseConversion("[MO]", command.mutable_input());
    SetupMockForReverseConversion("[MO]", "MO");
    EXPECT_TRUE(session.SendCommand(&command));

    command.Clear();
    GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
    session.ConvertCancel(&command);
    ASSERT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

    command.Clear();
    session.EditCancel(&command);
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    // test case against b/5566728
    EXPECT_RESULT("[MO]", command);
  }
}

TEST_F(SessionTest, ImeOff) {
  scoped_ptr<MockConverterEngineForReset> engine(
      new MockConverterEngineForReset);
  ConverterMockForReset *convertermock = engine->mutable_converter_mock();

  convertermock->Reset();
  scoped_ptr<Session> session(new Session(engine.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  session->IMEOff(&command);

  EXPECT_TRUE(convertermock->reset_conversion_called());
}

TEST_F(SessionTest, EditCancelAndIMEOff) {
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\thankaku/zenkaku\tCancelAndIMEOff\n"
        "Composition\thankaku/zenkaku\tCancelAndIMEOff\n"
        "Conversion\thankaku/zenkaku\tCancelAndIMEOff\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);
    config::ConfigHandler::SetConfig(config);
  }

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  {  // Cancel of Precomposition and deactivate IME
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Composition and deactivate IME
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    commands::Command command;
    SendKey("M", &session, &command);

    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Suggestion and deactivate IME
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    commands::Command command;
    SendKey("M", &session, &command);

    GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
    SendKey("O", &session, &command);
    ASSERT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Conversion and deactivate IME
    Session session(engine_.get());
    InitSessionToConversionWithAiueo(&session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Reverse conversion and deactivate IME
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);

    commands::Command command;

    // "[MO]" is a converted string like Kanji.
    // "MO" is an input string like Hiragana.
    SetupCommandForReverseConversion("[MO]", command.mutable_input());
    SetupMockForReverseConversion("[MO]", "MO");
    EXPECT_TRUE(session.SendCommand(&command));

    command.Clear();
    GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
    session.ConvertCancel(&command);
    ASSERT_TRUE(command.output().has_candidates());
    EXPECT_EQ(2, command.output().candidates().candidate_size());
    EXPECT_EQ("MOCHA", command.output().candidates().candidate(0).value());

    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_RESULT("[MO]", command);
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }
}

// TODO(matsuzakit): Update the expected result when b/5955618 is fixed.
TEST_F(SessionTest, CancelInPasswordMode_Issue5955618) {
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\tESC\tCancel\n"
        "Composition\tESC\tCancel\n"
        "Conversion\tESC\tCancel\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);
    config::ConfigHandler::SetConfig(config);
  }
  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  {  // Cancel of Precomposition in password field
     // Basically this is unusual because there is no character to be canceled
     // when Precomposition state.
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());  // should be consumed, anyway.

    EXPECT_TRUE(SendKey("ESC", &session, &command));
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
  }

  {  // Cancel of Composition in password field
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("ESC", &session, &command));
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
  }

  {  // Cancel of Conversion in password field
    Session session(engine_.get());
    InitSessionToConversionWithAiueo(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    // Actualy this works well because Cancel command in conversion mode
    // is mapped into ConvertCancel not EditCancel.
    commands::Command command;
    EXPECT_TRUE(TestSendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result());

    EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());
  }

  {  // Cancel of Reverse conversion in password field
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;

    // "[MO]" is a converted string like Kanji.
    // "MO" is an input string like Hiragana.
    SetupCommandForReverseConversion("[MO]", command.mutable_input());
    SetupMockForReverseConversion("[MO]", "MO");
    EXPECT_TRUE(session.SendCommand(&command));

    // Actualy this works well because Cancel command in conversion mode
    // is mapped into ConvertCancel not EditCancel.
    EXPECT_TRUE(TestSendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result());
    EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());

    // The second escape key will be mapped into EditCancel.
    EXPECT_TRUE(TestSendKey("ESC", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("ESC", &session, &command));
    // This behavior is the bug of b/5955618.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
    EXPECT_RESULT("[MO]", command);
  }
}

// TODO(matsuzakit): Update the expected result when b/5955618 is fixed.
TEST_F(SessionTest, CancelAndIMEOffInPasswordMode_Issue5955618) {
  {
    config::Config config;
    const string custom_keymap_table =
        "status\tkey\tcommand\n"
        "Precomposition\thankaku/zenkaku\tCancelAndIMEOff\n"
        "Composition\thankaku/zenkaku\tCancelAndIMEOff\n"
        "Conversion\thankaku/zenkaku\tCancelAndIMEOff\n";
    config.set_session_keymap(config::Config::CUSTOM);
    config.set_custom_keymap_table(custom_keymap_table);
    config::ConfigHandler::SetConfig(config);
  }
  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  {  // Cancel of Precomposition and deactivate IME in password field.
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    // Current behavior seems to be a bug.
    // This command should deactivate the IME.
    ASSERT_FALSE(command.output().has_status())
        << "Congrats! b/5955618 seems to be fixed.";
    // Ideally the following condition should be satisfied.
    // EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Composition and deactivate IME in password field
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());

    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    // Following behavior seems to be a bug.
    // This command should deactivate the IME.
    ASSERT_FALSE(command.output().has_status())
        << "Congrats! b/5955618 seems to be fixed.";
    // Ideally the following condition should be satisfied.
    // EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Conversion and deactivate IME in password field
    Session session(engine_.get());
    InitSessionToConversionWithAiueo(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;
    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    command.Clear();
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
    EXPECT_EQ("", GetComposition(command));
    EXPECT_EQ(0, command.output().candidates().candidate_size());
    EXPECT_FALSE(command.output().has_result());
    // Following behavior seems to be a bug.
    // This command should deactivate the IME.
    ASSERT_FALSE(command.output().has_status())
        << "Congrats! b/5955618 seems to be fixed.";
    // Ideally the following condition should be satisfied.
    // EXPECT_FALSE(command.output().status().activated());
  }

  {  // Cancel of Reverse conversion and deactivate IME in password field
    Session session(engine_.get());
    InitSessionToPrecomposition(&session);
    SwitchInputFieldType(commands::Context::PASSWORD, &session);

    commands::Command command;

    // "[MO]" is a converted string like Kanji.
    // "MO" is an input string like Hiragana.
    SetupCommandForReverseConversion("[MO]", command.mutable_input());
    SetupMockForReverseConversion("[MO]", "MO");
    EXPECT_TRUE(session.SendCommand(&command));

    EXPECT_TRUE(TestSendKey("hankaku/zenkaku", &session, &command));
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(SendKey("hankaku/zenkaku", &session, &command));
    // This behavior is the bug of b/5955618.
    // The result of TestSendKey and SendKey should be the same in terms of
    // |consumed()|.
    EXPECT_FALSE(command.output().consumed())
        << "Congrats! b/5955618 seems to be fixed";
    EXPECT_RESULT("[MO]", command);
    ASSERT_TRUE(command.output().has_status());
    // This behavior is the bug of b/5955618. IME should be deactivated.
    EXPECT_TRUE(command.output().status().activated())
        << "Congrats! b/5955618 seems to be fixed";
  }
}

// We use following represenetaion for indicating all state-change pass.
// State:
//   [PRECOMP] : Precomposition state
//   [COMP-L]  : Composition state with cursor at left most.
//   [COMP-M]  : Composition state with cursor at middle of composition.
//   [COMP-R]  : Composition state with cursor at right most.
//   [CONV-L]  : Conversion state with cursor at left most.
//   [CONV-M]  : Conversion state with cursor at middle of composition.
// State Change:
//  "abcdef" means composition characters.
//  "^" means suggestion/conversion window left-top position
//  "|" means caret position.
// NOTE:
//  It is not necessary to test in case as follows because they never occur.
//   - [PRECOMP] -> [PRECOMP]
//   - [PRECOMP] -> [COMP-M] or [COMP-L]
//   - [PRECOMP] -> [CONV-L] or [CONV-R]
//  Also it is not necessary to test in case of changing to CONVERSION state,
//  because conversion window is always shown under current cursor.
TEST_F(SessionTest, CaretManagePrecompositionToCompositionTest) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  Segments segments;
  const int kCaretInitialXpos = 10;
  commands::Rectangle rectangle;
  rectangle.set_x(kCaretInitialXpos);
  rectangle.set_y(0);
  rectangle.set_width(0);
  rectangle.set_height(0);

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  // [PRECOMP] -> [COMP-R]:
  //  Expectation: -> ^a|
  SetCaretLocation(rectangle, session.get());

  SendKey("M", session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());
}

TEST_F(SessionTest, CaretManageCompositionToCompositionTest) {
  Segments segments_m;
  {
    segments_m.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_m.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_moz;
  {
    segments_moz.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_moz.add_segment();
    segment->set_key("MOZ");
    segment->add_candidate()->value = "MOZUKU";
  }

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  commands::Command command;
  const int kCaretInitialXpos = 10;
  commands::Rectangle rectangle;
  rectangle.set_x(kCaretInitialXpos);
  rectangle.set_y(0);
  rectangle.set_width(0);
  rectangle.set_height(0);

  SetCaretLocation(rectangle, session.get());

  SendKey("M", session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-R] -> [COMP-R]:
  //  Expectation: ^mo| -> ^moz|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_moz, true);
  SendKey("Z", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-R] -> [COMP-R]:
  //  Expectation: ^moz| -> ^mo|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("Backspace", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-R] -> [COMP-M]:
  //  Expectation: ^mo| -> ^m|o
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorLeft(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-M] -> [COMP-R]:
  //  Expectation: ^m|o -> ^mo|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorToEnd(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-R] -> [COMP-L]:
  //  Expectation: ^mo| -> ^|mo
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorToBeginning(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-L] -> [COMP-M]:
  //  Expectation: ^|mo -> ^m|o
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  command.Clear();
  EXPECT_TRUE(session->MoveCursorRight(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  // [COMP-M] -> [COMP-L]:
  //  Expectation: ^m|o -> ^m|
  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);
  command.Clear();
  EXPECT_TRUE(session->Delete(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());
}

TEST_F(SessionTest, CaretManageConversionToCompositionTest) {
  // There are two ways to change state from CONVERSION to COMPOSITION,
  // One is canceling conversion with BS key. In this case cursor location
  // becomes right most and suggest position is left most.
  // The second is continuing typing under conversion. If user types key under
  // conversion, the IME commits selected candidate and creates new composition
  // at once.
  // For example:
  //    KeySequence: 'a' -> SP -> SP -> 'i'
  //    Expectation: a^|i (a and i are corresponding japanese characters)
  //    Actual: ^a|i
  // In the session side, we can only support the former case.

  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  Segments segments;
  const int kCaretInitialXpos = 10;
  commands::Rectangle rectangle;
  rectangle.set_x(kCaretInitialXpos);
  rectangle.set_y(0);
  rectangle.set_width(0);
  rectangle.set_height(0);

  Segments segments_m;
  {
    segments_m.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_m.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_moz;
  {
    segments_moz.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_moz.add_segment();
    segment->set_key("MOZ");
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_m_conv;
  {
    segments_m_conv.set_request_type(Segments::CONVERSION);
    Segment *segment;
    segment = segments_m_conv.add_segment();
    segment->set_key("M");
    segment->add_candidate()->value = "M";
    segment->add_candidate()->value = "m";
  }

  scoped_ptr<ConversionRequest> request_m_conv;

  // [CONV-L] -> [COMP-R]
  //  Expectation: ^|a -> ^a|
  SetCaretLocation(rectangle, session.get());

  SendKey("M", session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  command.Clear();
  request_m_conv.reset(new ConversionRequest);
  SetComposer(session.get(), request_m_conv.get());
  FillT13Ns(*request_m_conv, &segments_m_conv);
  GetConverterMock()->SetStartConversionForRequest(&segments_m_conv, true);
  EXPECT_TRUE(session->Convert(&command));

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  command.Clear();
  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);
  EXPECT_TRUE(session->ConvertCancel(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());

  // [CONV-M] -> [COMP-R]
  //  Expectation: ^a|b -> ^ab|
  session.reset(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());
  rectangle.set_x(kCaretInitialXpos);

  SetCaretLocation(rectangle, session.get());

  SendKey("M", session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  command.Clear();
  request_m_conv.reset(new ConversionRequest);
  SetComposer(session.get(), request_m_conv.get());
  FillT13Ns(*request_m_conv, &segments_m_conv);
  GetConverterMock()->SetStartConversionForRequest(&segments_m_conv, true);
  EXPECT_TRUE(session->Convert(&command));

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  SendSpecialKey(commands::KeyEvent::LEFT, session.get(), &command);

  rectangle.set_x(rectangle.x() + 5);
  SetCaretLocation(rectangle, session.get());

  command.Clear();
  GetConverterMock()->SetStartSuggestionForRequest(&segments_m, true);
  EXPECT_TRUE(session->ConvertCancel(&command));
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());
}

TEST_F(SessionTest, CaretJumpCaseTest) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  commands::Command command;
  Segments segments;
  const int kCaretInitialXpos = 10;
  const int kCaretInitialYpos = 12;
  commands::Rectangle rectangle;
  rectangle.set_x(kCaretInitialXpos);
  rectangle.set_y(kCaretInitialYpos);
  rectangle.set_width(0);
  rectangle.set_height(0);

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }

  Segments segments_moz;
  {
    segments_moz.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_moz.add_segment();
    segment->set_key("MOZ");
    segment->add_candidate()->value = "MOZUKU";
  }

  SetCaretLocation(rectangle, session.get());
  SendKey("M", session.get(), &command);

  // If Y-position of caret is jumped, composition text area is reset.
  rectangle.set_y(rectangle.y() + 200);
  SetCaretLocation(rectangle, session.get());
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);
  SendKey("O", session.get(), &command);
  EXPECT_EQ(rectangle.y(),
            command.output().candidates().composition_rectangle().y());

  // Even if X-position of caret is jumped, composition text area is not reset.
  rectangle.set_x(rectangle.x() + 200);
  SetCaretLocation(rectangle, session.get());
  GetConverterMock()->SetStartSuggestionForRequest(&segments_moz, true);
  SendKey("Z", session.get(), &command);
  EXPECT_EQ(kCaretInitialXpos,
            command.output().candidates().composition_rectangle().x());
}

TEST_F(SessionTest, DoNothingOnCompositionKeepingSuggestWindow) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  Segments segments_mo;
  {
    segments_mo.set_request_type(Segments::SUGGESTION);
    Segment *segment;
    segment = segments_mo.add_segment();
    segment->set_key("MO");
    segment->add_candidate()->value = "MOCHA";
    segment->add_candidate()->value = "MOZUKU";
  }
  GetConverterMock()->SetStartSuggestionForRequest(&segments_mo, true);

  commands::Command command;
  SendKey("M", &session, &command);
  EXPECT_TRUE(command.output().has_candidates());

  SendKey("Ctrl", &session, &command);
  EXPECT_TRUE(command.output().has_candidates());
}

TEST_F(SessionTest, ModeChangeOfConvertAtPunctuations) {
  config::Config config;
  config.set_use_auto_conversion(true);
  config::ConfigHandler::SetConfig(config);

  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  Segments segments_a_conv;
  {
    segments_a_conv.set_request_type(Segments::CONVERSION);
    Segment *segment;
    segment = segments_a_conv.add_segment();
    segment->set_key(kHiraganaA);
    segment->add_candidate()->value = kHiraganaA;
  }
  GetConverterMock()->SetStartConversionForRequest(&segments_a_conv, true);

  commands::Command command;
  SendKey("a", &session, &command);     // "あ|" (composition)
  EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());

  SendKey(".", &session, &command);     // "あ。|" (conversion)
  EXPECT_EQ(ImeContext::CONVERSION, session.context().state());

  SendKey("ESC", &session, &command);   // "あ。|" (composition)
  EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());

  SendKey("Left", &session, &command);  // "あ|。" (composition)
  EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());

  SendKey("i", &session, &command);     // "あい|。" (should be composition)
  EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());
}

TEST_F(SessionTest, SuppressSuggestion) {
  Session session(mock_data_engine_.get());
  InitSessionToPrecomposition(&session);

  commands::Command command;
  SendKey("a", &session, &command);
  EXPECT_TRUE(command.output().has_candidates());

  command.Clear();
  session.EditCancel(&command);
  EXPECT_FALSE(command.output().has_candidates());

  // Default behavior.
  SendKey("d", &session, &command);
  EXPECT_TRUE(command.output().has_candidates());

  // With an invalid identifer.  It should be the same with the
  // default behavior.
  SetSendKeyCommand("i", &command);
  command.mutable_input()->mutable_context()->add_experimental_features(
      "invalid_identifier");
  session.SendKey(&command);
  EXPECT_TRUE(command.output().has_candidates());

}

TEST_F(SessionTest, DeleteHistory) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  Segments segments;
  Segment *segment = segments.add_segment();
  segment->set_key("delete");
  segment->add_candidate()->value = "DeleteHistory";
  ConversionRequest request;
  SetComposer(session.get(), &request);
  GetConverterMock()->SetStartPredictionForRequest(&segments, true);

  // Type "del". Preedit = "でｌ".
  commands::Command command;
  EXPECT_TRUE(SendKey("d", session.get(), &command));
  EXPECT_TRUE(SendKey("e", session.get(), &command));
  EXPECT_TRUE(SendKey("l", session.get(), &command));
  EXPECT_PREEDIT("\xE3\x81\xA7\xEF\xBD\x8C", command);  //  "でｌ"

  // Start prediction. Preedit = "DeleteHistory".
  command.Clear();
  EXPECT_TRUE(session->PredictAndConvert(&command));
  EXPECT_TRUE(command.output().has_candidates());
  EXPECT_EQ(ImeContext::CONVERSION, session->context().state());
  EXPECT_PREEDIT("DeleteHistory", command);

  // Do DeleteHistory command. After that, the session should be back in
  // composition state and preedit gets back to "でｌ" again.
  EXPECT_TRUE(SendKey("Ctrl Delete", session.get(), &command));
  EXPECT_EQ(ImeContext::COMPOSITION, session->context().state());
  EXPECT_PREEDIT("\xE3\x81\xA7\xEF\xBD\x8C", command);  //  "でｌ"
}

TEST_F(SessionTest, SendKeyWithKeyString_Direct) {
  Session session(engine_.get());
  InitSessionToDirect(&session);

  commands::Command command;
  const char kZa[] = "\xE3\x81\x96";  // "ざ"
  SetSendKeyCommandWithKeyString(kZa, &command);
  EXPECT_TRUE(session.TestSendKey(&command));
  EXPECT_FALSE(command.output().consumed());
  command.mutable_output()->Clear();
  EXPECT_TRUE(session.SendKey(&command));
  EXPECT_FALSE(command.output().consumed());
}

TEST_F(SessionTest, SendKeyWithKeyString) {
  Session session(engine_.get());
  InitSessionToPrecomposition(&session);

  commands::Command command;

  // Test for precomposition state.
  EXPECT_EQ(ImeContext::PRECOMPOSITION, session.context().state());
  const char kZa[] = "\xE3\x81\x96";  // "ざ"
  SetSendKeyCommandWithKeyString(kZa, &command);
  EXPECT_TRUE(session.TestSendKey(&command));
  EXPECT_TRUE(command.output().consumed());
  command.mutable_output()->Clear();
  EXPECT_TRUE(session.SendKey(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_PREEDIT(kZa, command);

  command.Clear();

  // Test for composition state.
  EXPECT_EQ(ImeContext::COMPOSITION, session.context().state());
  const char kOnsenManju[] = "\xE2\x99\xA8\xE9\xA5\x85\xE9\xA0\xAD";  // "♨饅頭"
  SetSendKeyCommandWithKeyString(kOnsenManju, &command);
  EXPECT_TRUE(session.TestSendKey(&command));
  EXPECT_TRUE(command.output().consumed());
  command.mutable_output()->Clear();
  EXPECT_TRUE(session.SendKey(&command));
  EXPECT_TRUE(command.output().consumed());
  EXPECT_PREEDIT(string(kZa) + kOnsenManju, command);
}

TEST_F(SessionTest, IndirectImeOnOff) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  {
    commands::Command command;
    // IMEOff
    SendSpecialKey(commands::KeyEvent::OFF, session.get(), &command);
  }
  {
    commands::Command command;
    // 'a'
    TestSendKeyWithModeAndActivated(
        "a", true, commands::HIRAGANA, session.get(), &command);
    EXPECT_TRUE(command.output().consumed());
  }
  {
    commands::Command command;
    // 'a'
    SendKeyWithModeAndActivated(
        "a", true, commands::HIRAGANA, session.get(), &command);
    EXPECT_TRUE(command.output().consumed());
    EXPECT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated())
        << "Should be activated.";
  }
  {
    commands::Command command;
    // 'a'
    TestSendKeyWithModeAndActivated(
        "a", false, commands::HIRAGANA, session.get(), &command);
    EXPECT_FALSE(command.output().consumed());
  }
  {
    commands::Command command;
    // 'a'
    SendKeyWithModeAndActivated(
        "a", false, commands::HIRAGANA, session.get(), &command);
    EXPECT_FALSE(command.output().consumed());
    EXPECT_FALSE(command.output().has_result())
        << "Indirect IME off flushes ongoing composition";
    EXPECT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated())
        << "Should be inactivated.";
  }
}

TEST_F(SessionTest, MakeSureIMEOn) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToDirect(session.get());

  {
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_ON_IME, &command);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
  }

  {
    // Make sure we can change the input mode.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_ON_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::FULL_KATAKANA);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
  }

  {
    // Make sure we can change the input mode again.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_ON_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::HIRAGANA);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_TRUE(command.output().status().activated());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
  }

  {
    // commands::DIRECT is not supported for the composition_mode.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_ON_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::DIRECT);
    EXPECT_FALSE(session->SendCommand(&command));
  }
}

TEST_F(SessionTest, MakeSureIMEOff) {
  scoped_ptr<Session> session(new Session(engine_.get()));
  InitSessionToPrecomposition(session.get());

  {
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_OFF_IME, &command);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
  }

  {
    // Make sure we can change the input mode.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_OFF_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::FULL_KATAKANA);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
    EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
  }

  {
    // Make sure we can change the input mode again.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_OFF_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::HIRAGANA);

    ASSERT_TRUE(session->SendCommand(&command));
    EXPECT_TRUE(command.output().consumed());
    ASSERT_TRUE(command.output().has_status());
    EXPECT_FALSE(command.output().status().activated());
    EXPECT_EQ(commands::HIRAGANA, command.output().status().mode());
  }

  {
    // commands::DIRECT is not supported for the composition_mode.
    commands::Command command;
    SetSendCommandCommand(commands::SessionCommand::TURN_OFF_IME, &command);
    command.mutable_input()->mutable_command()->set_composition_mode(
        commands::DIRECT);
    EXPECT_FALSE(session->SendCommand(&command));
  }

  {
    // Make sure SessionCommand::TURN_OFF_IME terminates the existing
    // composition.

    InitSessionToPrecomposition(session.get());

    // Set up converter.
    {
      commands::Command command;

      Segments segments;
      InsertCharacterChars("aiueo", session.get(), &command);
      ConversionRequest request;
      SetComposer(session.get(), &request);
      SetAiueo(&segments);
      FillT13Ns(request, &segments);
      GetConverterMock()->SetCommitSegmentValue(&segments, true);
    }

    // Send SessionCommand::TURN_OFF_IME to commit composition.
    {
      commands::Command command;
      SetSendCommandCommand(commands::SessionCommand::TURN_OFF_IME, &command);
      command.mutable_input()->mutable_command()->set_composition_mode(
          commands::FULL_KATAKANA);
      ASSERT_TRUE(session->SendCommand(&command));
      EXPECT_RESULT(kAiueo, command);
      EXPECT_TRUE(command.output().consumed());
      ASSERT_TRUE(command.output().has_status());
      EXPECT_FALSE(command.output().status().activated());
      EXPECT_EQ(commands::FULL_KATAKANA, command.output().status().mode());
    }
  }
}

}  // namespace session
}  // namespace mozc
