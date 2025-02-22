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

package org.mozc.android.inputmethod.japanese.keyboard;

import org.mozc.android.inputmethod.japanese.keyboard.Key.Stick;
import org.mozc.android.inputmethod.japanese.keyboard.KeyState.MetaState;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.Collections;
import java.util.EnumSet;

/**
 * Test for Key class.
 */
public class KeyTest extends TestCase {

  public void testKey_keyStateListOrder() {
    KeyState state1 = new KeyState("", EnumSet.of(MetaState.SHIFT),
        Collections.<MetaState>emptySet(), Collections.<MetaState>emptySet(),
        Collections.<Flick>emptySet());
    KeyState state2 = new KeyState("", EnumSet.of(MetaState.CAPS_LOCK),
        Collections.<MetaState>emptySet(), Collections.<MetaState>emptySet(),
        Collections.<Flick>emptySet());
    KeyState defaultState = new KeyState("", Collections.<MetaState>emptySet(),
        Collections.<MetaState>emptySet(), Collections.<MetaState>emptySet(),
        Collections.<Flick>emptySet());

    EnumSet<MetaState> givenMetaStates =
        EnumSet.of(MetaState.SHIFT, MetaState.CAPS_LOCK);

    Key key12 = new Key(0, 0, 0, 0, 0, 0, false, false, false, Stick.EVEN,
                        Arrays.asList(defaultState, state1, state2));
    // Both state1 and state2 are eligible to givenMetaStates but state1 is expected as
    // it is registered earlier.
    assertSame(state1, key12.getKeyState(givenMetaStates).get());

    Key key21 = new Key(0, 0, 0, 0, 0, 0, false, false, false, Stick.EVEN,
                        Arrays.asList(defaultState, state2, state1));
    assertSame(state2, key21.getKeyState(givenMetaStates).get());
  }
}
