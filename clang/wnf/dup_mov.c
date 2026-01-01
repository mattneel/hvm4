// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// For ALO-based MOV (from ALO-MOV), we need special handling to avoid
// sharing the binding location between branches. Otherwise both branches
// will share the same mov_term_val, causing incorrect behavior.
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  ITRS++;
  u32  mov_loc = term_val(mov);
  Term val     = heap_read(mov_loc + 0);
  Term bod     = heap_read(mov_loc + 1);

  // Check if this is an ALO-based MOV
  if (term_tag(val) == ALO && term_tag(bod) == ALO) {
    // For ALO-based MOV, we need to recreate the binding structure for each branch
    // to avoid sharing mov_term_val between branches.
    u32  val_alo_loc = term_val(val);
    u32  bod_alo_loc = term_val(bod);
    u32  val_len = term_ext(val);
    u32  bod_len = term_ext(bod);

    u64  val_pair = heap_read(val_alo_loc);
    u64  bod_pair = heap_read(bod_alo_loc);

    u32  val_ls = (u32)(val_pair >> 32);
    u32  val_tm = (u32)(val_pair & 0xFFFFFFFF);
    u32  bod_ls = (u32)(bod_pair >> 32);
    u32  bod_tm = (u32)(bod_pair & 0xFFFFFFFF);

    // Create new binding infrastructure for each branch
    // Branch 0
    u64  mov_val_0 = heap_alloc(1);
    u64  bind_0    = heap_alloc(1);
    u64  alo_val_0 = heap_alloc(1);
    u64  alo_bod_0 = heap_alloc(1);

    heap_set(bind_0, ((u64)(u32)mov_val_0 << 32) | val_ls);
    heap_set(alo_val_0, ((u64)val_ls << 32) | val_tm);
    heap_set(mov_val_0, term_new(0, ALO, val_len, alo_val_0));
    heap_set(alo_bod_0, ((u64)(u32)bind_0 << 32) | bod_tm);

    Term m0_val = term_new(0, ALO, val_len, alo_val_0);
    Term m0_bod = term_new(0, ALO, bod_len, alo_bod_0);
    Term m0 = term_new_mov(m0_val, m0_bod);

    // Branch 1
    u64  mov_val_1 = heap_alloc(1);
    u64  bind_1    = heap_alloc(1);
    u64  alo_val_1 = heap_alloc(1);
    u64  alo_bod_1 = heap_alloc(1);

    heap_set(bind_1, ((u64)(u32)mov_val_1 << 32) | val_ls);
    heap_set(alo_val_1, ((u64)val_ls << 32) | val_tm);
    heap_set(mov_val_1, term_new(0, ALO, val_len, alo_val_1));
    heap_set(alo_bod_1, ((u64)(u32)bind_1 << 32) | bod_tm);

    Term m1_val = term_new(0, ALO, val_len, alo_val_1);
    Term m1_bod = term_new(0, ALO, bod_len, alo_bod_1);
    Term m1 = term_new_mov(m1_val, m1_bod);

    return heap_subst_cop(side, loc, m0, m1);
  } else {
    // For non-ALO MOV (shouldn't happen from ALO-MOV, but handle it anyway)
    u64  a       = heap_alloc(2);
    heap_write(a + 0, term_sub_set(val, 0));
    heap_write(a + 1, term_sub_set(bod, 0));
    Copy V       = term_clone_at(a + 0, lab);
    Copy B       = term_clone_at(a + 1, lab);
    Term m0      = term_new_mov(V.k0, B.k0);
    Term m1      = term_new_mov(V.k1, B.k1);
    return heap_subst_cop(side, loc, m0, m1);
  }
}
