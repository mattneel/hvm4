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

  printf("  V cloned: V0=0x%llx V1=0x%llx\n", (unsigned long long)V.k0, (unsigned long long)V.k1);
  printf("  B cloned: B0=0x%llx B1=0x%llx\n", (unsigned long long)B.k0, (unsigned long long)B.k1);

  // Create two MOV nodes
  Term r0    = term_new_mov_at(at + 2, V.k0, B.k0);
  Term r1    = term_new_mov_at(at + 4, V.k1, B.k1);

  // Try WITHOUT substitution - just duplicate the ALO terms
  // The ALO mechanism should handle binding resolution automatically
  printf("  Created r0=0x%llx at loc %u, r1=0x%llx at loc %u\n",
         (unsigned long long)r0, at + 2, (unsigned long long)r1, at + 4);

  return heap_subst_cop(side, loc, r0, r1);
}
