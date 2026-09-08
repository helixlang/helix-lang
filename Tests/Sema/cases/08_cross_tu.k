// EXPECT: 0 errors.
// EXPECT: `Cfg` (default arg) resolves Hidden in _lib_a's scope, NOT here (this file never imports _lib_b).
// EXPECT: `_lib_a::Alias` expands to Hidden through _lib_a's overlay (not ours).
// EXPECT: `Hidden` selective import via _lib_a's re-export resolves.
import _lib_a
import _lib_a::{Hidden}

fn use(c: _lib_a::Cfg, a: _lib_a::Alias, h: Hidden, o: _lib_a::Own) { }