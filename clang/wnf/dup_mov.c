// ! F &L = % x = v; b
// ---------------------- DUP-MOV
// Use default node duplication which creates DUP nodes for val and bod
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  return wnf_dup_nod(lab, loc, side, mov);
}
