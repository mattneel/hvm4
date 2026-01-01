// % X = ^(f x)
// -------------- MOV-DRY
// % F = f
// % A = x
// X ← ^(F A)
fn Term wnf_mov_dry(u32 loc, Term dry) {
  ITRS++;
  u32  d_loc = term_val(dry);
  u64  base  = heap_alloc(4);
  u32  at    = (u32)base;
  heap_write(at + 0, term_sub_set(heap_read(d_loc + 0), 0));
  heap_write(at + 1, term_sub_set(heap_read(d_loc + 1), 0));
  Term f     = term_new_got(at + 0);
  Term a     = term_new_got(at + 1);
  Term res   = term_new_dry_at(at + 2, f, a);
  heap_subst_var(loc, res);
  return res;
}
