// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// ! V &L = v
// ! B &L = b
// F₀ ← % x = V₀; B₀
// F₁ ← % x = V₁; B₁
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  printf("!!! DUP-MOV CALLED !!!\n");
  fflush(stdout);
  ITRS++;
  u32  m_loc = term_val(mov);
  Term val   = heap_read(m_loc + 0);
  Term bod   = heap_read(m_loc + 1);

  printf("DUP-MOV: lab=%u loc=%u side=%u\n", lab, loc, side);
  printf("  MOV at %u: val=0x%llx bod=0x%llx\n", m_loc, (unsigned long long)val, (unsigned long long)bod);
  printf("  val tag=%u, bod tag=%u\n", term_tag(val), term_tag(bod));

  // DON'T force ALO children - duplicate them as-is
  // The ALO terms will be forced later when needed, and the GOT references
  // inside them will then resolve via the SUP we create

  u64  base  = heap_alloc(8);
  u32  at    = (u32)base;
  heap_write(at + 0, val);
  heap_write(at + 1, bod);
  Copy V     = term_clone_at(at + 0, lab);
  Copy B     = term_clone_at(at + 1, lab);

  printf("  V cloned: V0=0x%llx (tag=%u val=%u) V1=0x%llx (tag=%u val=%u)\n",
         (unsigned long long)V.k0, term_tag(V.k0), term_val(V.k0),
         (unsigned long long)V.k1, term_tag(V.k1), term_val(V.k1));
  printf("  B cloned: B0=0x%llx (tag=%u val=%u) B1=0x%llx (tag=%u val=%u)\n",
         (unsigned long long)B.k0, term_tag(B.k0), term_val(B.k0),
         (unsigned long long)B.k1, term_tag(B.k1), term_val(B.k1));

  // Check what's at the shared location
  if (term_tag(V.k0) == DP0 || term_tag(V.k0) == DP1) {
    u32 shared_loc = term_val(V.k0);
    printf("  V DP0/DP1 point to shared location %u: 0x%llx\n",
           shared_loc, (unsigned long long)heap_read(shared_loc));
  }

  // Create two MOV nodes
  Term r0    = term_new_mov_at(at + 2, V.k0, B.k0);
  Term r1    = term_new_mov_at(at + 4, V.k1, B.k1);

  printf("  Created MOV₀ at %u: val=0x%llx bod=0x%llx\n",
         at + 2, (unsigned long long)heap_read(at + 2), (unsigned long long)heap_read(at + 3));
  printf("  Created MOV₁ at %u: val=0x%llx bod=0x%llx\n",
         at + 4, (unsigned long long)heap_read(at + 4), (unsigned long long)heap_read(at + 5));

  return heap_subst_cop(side, loc, r0, r1);
}
