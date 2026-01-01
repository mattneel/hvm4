// @{s} n₀
// ------- ALO-DP0
// s[n]₀ or n₀ when substitution missing (n is a de Bruijn level)
//
// @{s} n₁
// ------- ALO-DP1
// s[n]₁ or n₁ when substitution missing (n is a de Bruijn level)
fn Term wnf_alo_cop(u32 ls, u32 len, u32 lvl, u32 lab, u8 side, u8 tag) {
  if (lvl == 0 || lvl > len) {
    return term_new(0, tag, lab, lvl);
  }
  u32 idx = len - lvl;
  u32 it  = ls;
  for (u32 i = 0; i < idx && it != 0; i++) {
    it = (u32)(heap_read(it) & 0xFFFFFFFF);
  }
  u32 bind = (it != 0) ? (u32)(heap_read(it) >> 32) : 0;
  u8  rtag = side == 0 ? DP0 : DP1;
  Term result = bind ? term_new(0, rtag, lab, bind) : term_new(0, tag, lab, lvl);

  if (bind) {
    Term at_bind = heap_read(bind);
    printf("ALO-COP: lvl=%u lab=%u side=%u -> bind=%u result=%s(%u, lab=%u, val=%u)\n",
           lvl, lab, side, bind,
           term_tag(result) == 3 ? "DP0" : "DP1",
           term_tag(result), term_ext(result), term_val(result));
    printf("  Value at bind location %u: 0x%llx (tag=%u val=%u)\n",
           bind, (unsigned long long)at_bind, term_tag(at_bind), term_val(at_bind));
  } else {
    printf("ALO-COP: lvl=%u lab=%u side=%u -> NO BIND\n", lvl, lab, side);
  }
  fflush(stdout);

  return result;
}
