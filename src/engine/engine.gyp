# Copyright 2010-2014, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

{
  'variables': {
    'relative_dir': 'engine',
    'gen_out_dir': '<(SHARED_INTERMEDIATE_DIR)/<(relative_dir)',
  },
  'targets': [
    {
      'target_name': 'engine',
      'type': 'static_library',
      'sources': [
        '<(gen_out_dir)/../dictionary/pos_matcher.h',
        'engine.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../converter/converter.gyp:converter',
        '../converter/converter_base.gyp:connector_base',
        '../converter/converter_base.gyp:segmenter_base',
        '../dictionary/dictionary.gyp:dictionary_impl',
        '../dictionary/dictionary.gyp:suffix_dictionary',
        '../dictionary/dictionary_base.gyp:dictionary_protocol',
        '../dictionary/dictionary_base.gyp:pos_matcher',
        '../dictionary/dictionary_base.gyp:suppression_dictionary',
        '../dictionary/dictionary_base.gyp:user_dictionary',
        '../dictionary/dictionary_base.gyp:user_pos',
        '../dictionary/system/system_dictionary.gyp:system_dictionary',
        '../dictionary/system/system_dictionary.gyp:value_dictionary',
        '../prediction/prediction.gyp:prediction',
        '../prediction/prediction_base.gyp:suggestion_filter',
        '../rewriter/rewriter.gyp:rewriter',
        '../session/session_base.gyp:session_protocol',
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
    },
    {
      'target_name': 'mock_converter_engine',
      'type': 'static_library',
      'sources': [
        'mock_converter_engine.cc',
        'user_data_manager_mock.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../converter/converter_base.gyp:converter_mock'
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
    },
    {  # An engine just for converter/converter_main.cc.
      'target_name': 'chromeos_engine_factory',
      'type': 'none',
      'sources': [
        'chromeos_engine_factory.cc',
      ],
    },
    {
      'target_name': 'oss_engine_factory',
      'type': 'static_library',
      'sources': [
        'oss_engine_factory.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../data_manager/oss/oss_data_manager.gyp:oss_data_manager',
        '../prediction/prediction.gyp:prediction',
        'engine',
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
    },
    {
      'target_name': 'packed_engine_factory',
      'type': 'static_library',
      'sources': [
        'packed_engine_factory.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../data_manager/packed/packed_data_manager_base.gyp:packed_data_manager',
        '../prediction/prediction.gyp:prediction',
        'engine',
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
    },
    {
      'target_name': 'mock_data_engine_factory',
      'type': 'static_library',
      'sources': [
        'mock_data_engine_factory.cc',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../data_manager/testing/mock_data_manager.gyp:mock_data_manager',
        '../prediction/prediction.gyp:prediction',
        '../session/session_base.gyp:session_protocol',
        'engine',
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
    },
    {
      'target_name': 'engine_factory',
      'type': 'none',
      'sources': [
        'engine_factory.h',
      ],
      'dependencies': [
        'oss_engine_factory',
      ],
      'xcode_settings' : {
        'SDKROOT': 'iphoneos',
        'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        'ARCHS': '$(ARCHS_UNIVERSAL_IPHONE_OS)',
      },
      'conditions': [
        ['use_packed_dictionary==1', {
          'dependencies': [
            'packed_engine_factory'
          ],
          'dependencies!': [
            'oss_engine_factory',
          ]
        }],
      ],
    },
  ],
}
