// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// F₀ ← % x = V₀; B₀
// F₁ ← % x = V₁; B₁
// ! V &L = v
// ! B &L = b
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  ITRS++;
  u32  mov_loc = term_val(mov);
  Term val     = heap_read(mov_loc + 0);
  Term bod     = heap_read(mov_loc + 1);

  u64  a       = heap_alloc(2);
  heap_write(a + 0, term_sub_set(val, 0));
  heap_write(a + 1, term_sub_set(bod, 0));
  Copy V       = term_clone_at(a + 0, lab);
  Copy B       = term_clone_at(a + 1, lab);
  Term m0      = term_new_mov(V.k0, B.k0);
  Term m1      = term_new_mov(V.k1, B.k1);
  return heap_subst_cop(side, loc, m0, m1);
}
