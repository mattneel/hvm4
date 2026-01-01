// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// ! V &L = v
// ! B &L = b
// F₀ ← % x = V₀; B₀
// F₁ ← % x = V₁; B₁
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  ITRS++;
  u32  m_loc = term_val(mov);
  u64  base  = heap_alloc(6);
  u32  at    = (u32)base;
  heap_write(at + 0, heap_read(m_loc + 0));
  heap_write(at + 1, heap_read(m_loc + 1));
  Copy V     = term_clone_at(at + 0, lab);
  Copy B     = term_clone_at(at + 1, lab);
  Term r0    = term_new_mov_at(at + 2, V.k0, B.k0);
  Term r1    = term_new_mov_at(at + 4, V.k1, B.k1);
  return heap_subst_cop(side, loc, r0, r1);
}
