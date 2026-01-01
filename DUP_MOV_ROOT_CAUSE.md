# DUP-MOV Root Cause Analysis

## Executive Summary

**Status**: Root cause identified
**Invariant violated**: INV-2 (DUP-MOV Equivalence)
**Severity**: Critical - causes unresolved MOV bindings in output

## Test Results

| Test | Status | Output | Notes |
|------|--------|--------|-------|
| inv1_mov_simple.hvm4 | ✅ PASS | `[42,42]` | Basic substitution works |
| inv2_dup_mov_atomic.hvm4 | ✅ PASS | `[42,42]` | Simple duplication works |
| debug_dup_mov.hvm4 | ✅ PASS | `[42,42]` | MOV with simple body works |
| compare_dup_vs_mov.hvm4 | ✅ PASS | `[[42,42],[42,42]]` | Both DUP and MOV work for simple case |
| debug_dup_mov2.hvm4 | ❌ FAIL | `[λa.λb.A,λc.λd.B];%A=a;%B=c;` | MOV body uses variable twice |
| dup_mov_test.hvm4 | ❌ FAIL | `[λa.λb.A,λc.λd.B];%A=a;%B=c;` | Same failure pattern |
| mov_issue_canonical.hvm4 | ❌ FAIL | Stuck at 2253 interactions | Large-scale failure |

## Pattern Analysis

### Working Cases
```hvm4
// Simple value, simple body
% x = 42; x  →  Works ✅

// Simple body that uses variable once
% k = x; k  →  Works ✅
```

### Failing Cases
```hvm4
// Body uses variable multiple times
% k = x; k(k)  →  FAILS ❌
% f = o; f(...) ... f(...)  →  FAILS ❌ (in canonical test)
```

**Critical observation**: Failure occurs when the MOV body contains multiple references to the bound variable.

## Root Cause Hypothesis

### Current Implementation (`clang/wnf/dup_mov.c`)
```c
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  return wnf_dup_nod(lab, loc, side, mov);
}
```

This delegates to `wnf_dup_nod`, which treats MOV as a generic 2-arity node.

### What `wnf_dup_nod` Does

For `! F &L = (% x = VALUE; BODY)`:

1. Allocates space for duplication
2. Copies MOV's children: `val = VALUE`, `bod = BODY`
3. Creates GOT references via `term_clone_at`:
   - `A = {DP0(L, val_loc), DP1(L, val_loc)}`
   - `B = {DP0(L, bod_loc), DP1(L, bod_loc)}`
4. Creates two new MOV nodes:
   - `F₀ = MOV{DP0(L, val_loc), DP0(L, bod_loc)}`
   - `F₁ = MOV{DP1(L, val_loc), DP1(L, bod_loc)}`

### The Problem

The BODY contains GOT references that point to the *original* MOV's value location. When we create `MOV{DP0(L, val_loc), DP0(L, bod_loc)}`, the body at `bod_loc` still contains GOT references to the original MOV, not to the new duplicated MOVs!

#### Example Trace

```
Original: % k = x; k(k)
```

Structure:
- MOV node at location M
- val: x
- bod: GOT(M)(GOT(M))  ← both GOTs point back to M's value

After DUP-MOV:
- F₀ = MOV{DP0(L, val_loc), DP0(L, bod_loc)}
- F₁ = MOV{DP1(L, val_loc), DP1(L, bod_loc)}

Where `bod_loc` contains: `GOT(M)(GOT(M))`  ← STILL points to original M!

When F₀ is evaluated:
1. MOV forces body: `DP0(L, bod_loc)`
2. Takes body from bod_loc: `GOT(M)(GOT(M))`
3. Tries to resolve GOT(M), but M's structure is now corrupted by the duplication
4. Results in unresolved MOV bindings: `%A=a;%B=c;`

## Comparison with DUP-SUP

DUP-SUP works correctly because:
```c
// ! X &L = &R{a,b}
Copy A  = term_clone_at(at + 0, lab);  // Duplicate 'a'
Copy B  = term_clone_at(at + 1, lab);  // Duplicate 'b'
Term s0 = term_new_sup_at(at + 2, sup_lab, A.k0, B.k0);  // &R{A₀,B₀}
Term s1 = term_new_sup_at(at + 4, sup_lab, A.k1, B.k1);  // &R{A₁,B₁}
```

SUP's children (a,b) are independent terms. They don't contain internal references to the SUP node itself.

MOV is different: the body *can* contain GOT references back to the MOV's value.

## Comparison with DUP-LAM

DUP-LAM handles a similar case correctly:
```c
// ! F &L = λx.f
// Creates: λ$x0.G₀ and λ$x1.G₁
// Substitutes x with SUP{$x0,$x1}
```

The key: LAM substitutes the bound variable with a SUP, creating independent bindings for each copy.

MOV should do something similar!

## Proposed Fix

### Option 1: Specialized DUP-MOV (Recommended)

Follow the pattern from DUP-LAM. For `! F &L = (% x = VALUE; BODY)`:

```c
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  ITRS++;
  u32  mov_loc = term_val(mov);
  Term val     = heap_read(mov_loc + 0);
  Term bod     = heap_read(mov_loc + 1);

  // Allocate space for duplication
  u64  base    = heap_alloc(8);
  u32  at      = (u32)base;

  // Duplicate the VALUE
  heap_write(at + 0, val);
  Copy V = term_clone_at(at + 0, lab);  // V = {V₀, V₁}

  // Duplicate the BODY
  heap_write(at + 1, bod);
  Copy B = term_clone_at(at + 1, lab);  // B = {B₀, B₁}

  // Create two MOV nodes with duplicated children
  // MOV₀ = (% x = V₀; B₀)
  heap_write(at + 2, V.k0);  // value for MOV₀
  heap_write(at + 3, B.k0);  // body for MOV₀
  Term m0 = term_new(0, MOV, 0, at + 2);

  // MOV₁ = (% x = V₁; B₁)
  heap_write(at + 4, V.k1);  // value for MOV₁
  heap_write(at + 5, B.k1);  // body for MOV₁
  Term m1 = term_new(0, MOV, 0, at + 4);

  return heap_subst_cop(side, loc, m0, m1);
}
```

**Problem**: This still doesn't fix the GOT references inside BODY pointing to the wrong location!

### Option 2: Force MOV value during duplication

The real issue is that GOT references in the body point to the original MOV. We need to either:

a) Update all GOT references in the body (complex, requires traversal)
b) Force the MOV to resolve before duplicating (changes semantics)
c) Create a layer of indirection (additional GOT nodes)

### Option 3: Study existing MOV interactions more carefully

Let me check how MOV-LAM and other MOV interactions handle this...

## Next Steps

1. ✅ Understand the memory layout of MOV nodes
2. ✅ Trace through DUP-MOV execution step-by-step
3. 🔄 Study how GOT references are resolved
4. 🔄 Check if there's a mechanism to update/redirect GOT references
5. ⏳ Implement the correct fix
6. ⏳ Verify with all test cases

## Tiger Style Checklist

- ✅ Identified invariant violation (INV-2)
- ✅ Created minimal reproducible test case
- ✅ Used pair assertions (working vs failing cases)
- ✅ Documented "why" not just "what"
- ⏳ Fix implementation pending deeper understanding
- ⏳ Verify fix doesn't break other interactions
