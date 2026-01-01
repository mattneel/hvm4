// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// When duplicating an ALO-based MOV, we need to create separate binding
// infrastructure for each branch to avoid sharing mov_term_val.
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  ITRS++;
  u32  mov_loc = term_val(mov);
  Term val     = heap_read(mov_loc + 0);
  Term bod     = heap_read(mov_loc + 1);

  // Check if this is an ALO-based MOV (from wnf_alo_mov)
  if (term_tag(val) == ALO && term_tag(bod) == ALO) {
    // Extract the ALO structure
    u32  val_len = term_ext(val);
    u32  bod_len = term_ext(bod);
    u32  val_alo_loc = term_val(val);
    u32  bod_alo_loc = term_val(bod);
    u64  val_pair = heap_read(val_alo_loc);
    u64  bod_pair = heap_read(bod_alo_loc);
    u32  val_ls = (u32)(val_pair >> 32);
    u32  val_tm = (u32)(val_pair & 0xFFFFFFFF);
    u32  bod_bind = (u32)(bod_pair >> 32);
    u32  bod_tm = (u32)(bod_pair & 0xFFFFFFFF);

    // Create independent binding infrastructure for branch 0
    u64 mov_val_0 = heap_alloc(1);
    u64 bind_0 = heap_alloc(1);
    heap_set(bind_0, ((u64)(u32)mov_val_0 << 32) | val_ls);
    u64 alo0_0 = heap_alloc(1);
    heap_set(alo0_0, ((u64)val_ls << 32) | val_tm);
    heap_set(mov_val_0, term_new(0, ALO, val_len, alo0_0));
    u64 alo1_0 = heap_alloc(1);
    heap_set(alo1_0, ((u64)val_ls << 32) | val_tm);
    u64 alo2_0 = heap_alloc(1);
    heap_set(alo2_0, ((u64)(u32)bind_0 << 32) | bod_tm);
    Term m0 = term_new_mov(term_new(0, ALO, val_len, alo1_0), term_new(0, ALO, bod_len, alo2_0));

    // Create independent binding infrastructure for branch 1
    u64 mov_val_1 = heap_alloc(1);
    u64 bind_1 = heap_alloc(1);
    heap_set(bind_1, ((u64)(u32)mov_val_1 << 32) | val_ls);
    u64 alo0_1 = heap_alloc(1);
    heap_set(alo0_1, ((u64)val_ls << 32) | val_tm);
    heap_set(mov_val_1, term_new(0, ALO, val_len, alo0_1));
    u64 alo1_1 = heap_alloc(1);
    heap_set(alo1_1, ((u64)val_ls << 32) | val_tm);
    u64 alo2_1 = heap_alloc(1);
    heap_set(alo2_1, ((u64)(u32)bind_1 << 32) | bod_tm);
    Term m1 = term_new_mov(term_new(0, ALO, val_len, alo1_1), term_new(0, ALO, bod_len, alo2_1));

    return heap_subst_cop(side, loc, m0, m1);
  }

  // For non-ALO MOV, use default duplication
  return wnf_dup_nod(lab, loc, side, mov);
}
