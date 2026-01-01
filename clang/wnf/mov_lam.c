// % F = λx.f
// ---------- MOV-LAM
// F ← λx.G
// % G = f
fn Term wnf_mov_lam(u32 loc, Term lam) {
  ITRS++;
  u32  lam_loc = term_val(lam);
  u32  lam_ext = term_ext(lam);
  Term bod     = heap_read(lam_loc);

  printf("MOV-LAM: loc=%u lam_loc=%u bod=0x%llx\n", loc, lam_loc, (unsigned long long)bod);
  fflush(stdout);

  u64  base    = heap_alloc(2);
  u32  g_loc   = (u32)base;
  u32  x_loc   = g_loc + 1;
  heap_write(g_loc, term_sub_set(bod, 0));
  heap_write(x_loc, term_new_got(g_loc));
  heap_subst_var(lam_loc, term_new(0, VAR, 0, x_loc));
  Term res     = term_new(0, LAM, lam_ext, x_loc);

  printf("  Created LAM at %u, GOT at %u points to %u\n", x_loc, x_loc, g_loc);
  printf("  Substituting MOV loc %u with LAM\n", loc);
  fflush(stdout);

  heap_subst_var(loc, res);
  return res;
}
