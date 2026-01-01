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

  // Force ALO children if needed to get concrete runtime terms
  if (term_tag(val) == ALO) {
    u64 vloc = heap_alloc(1);
    heap_write(vloc, val);
    val = wnf_at(vloc);
    heap_write(m_loc + 0, val);
    printf("  Forced val to tag=%u\n", term_tag(val));
  }
  if (term_tag(bod) == ALO) {
    u64 bloc = heap_alloc(1);
    heap_write(bloc, bod);
    bod = wnf_at(bloc);
    heap_write(m_loc + 1, bod);
    printf("  Forced bod to tag=%u\n", term_tag(bod));
  }

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

  // Create GOT references to the two new MOV value locations
  heap_write(at + 6, term_new_got(at + 2));  // GOT₀ points to r0's value location
  heap_write(at + 7, term_new_got(at + 4));  // GOT₁ points to r1's value location

  // Create SUP of the two GOT nodes
  Term su = term_new(0, SUP, lab, at + 6);

  // Substitute the original MOV's value location with the SUP
  // This redirects any GOT references in the duplicated bodies to branch correctly
  heap_subst_var(m_loc + 0, su);

  printf("  Substituted loc %u with SUP, r0=0x%llx r1=0x%llx\n",
         m_loc + 0, (unsigned long long)r0, (unsigned long long)r1);

  return heap_subst_cop(side, loc, r0, r1);
}
