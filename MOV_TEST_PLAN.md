# MOV Node Test Plan - Tiger Style

## Core Principle
**Safety First**: Use deterministic invariants with fail-fast assertions to catch bugs early.

## 1. Deterministic Invariants

### INV-1: Substitution Equivalence
**Statement**: `% x = v; body` must be semantically equivalent to `[v/x]body` (substituting v for x in body)

**Test**: For any value `v` and body `body`:
```hvm4
@test_subst_equiv =
  ! v = VALUE
  ! direct = (λx. BODY)(v)
  ! with_mov = (% x = v; BODY)
  @assert_equal(direct, with_mov)
```

**Metric**: Output must match exactly, interaction count must differ by < 10%

### INV-2: DUP-MOV Equivalence
**Statement**: Duplicating a MOV node must produce the same result as duplicating the substituted form

**Test**: For any MOV node:
```hvm4
@test_dup_mov_equiv =
  ! mov_dup = &{(% x = v; body)}  // DUP the MOV node
  ! subst_dup = &{([v/x]body)}    // DUP the substituted form
  @assert_equal(mov_dup.fst, subst_dup.fst)
  @assert_equal(mov_dup.snd, subst_dup.snd)
```

**Metric**: Both branches must be identical, no unresolved variables

### INV-3: Branch Independence
**Statement**: Each copy of a MOV from DUP must independently resolve its substitution

**Test**: Each branch can be reduced independently without blocking:
```hvm4
@test_branch_independence =
  ! &{mov_node} = &{(% x = v; body)}
  ! branch_a = reduce_to_wnf(mov_node.fst)  // Must complete
  ! branch_b = reduce_to_wnf(mov_node.snd)  // Must complete independently
  @assert_both_complete(branch_a, branch_b)
```

**Metric**: No unresolved GOT references, no blocking on shared state

### INV-4: Value Preservation
**Statement**: The value in MOV must not be lost, modified, or duplicated incorrectly

**Test**: Track value through interactions:
```hvm4
@test_value_preservation =
  ! original = SOME_VALUE
  ! mov = (% x = original; x)
  ! result = reduce(mov)
  @assert_equal(original, result)
```

**Metric**: Value identity preserved, no corruption, no leaks

### INV-5: Termination Parity
**Statement**: MOV version must terminate in ≤ interactions as DUP version (optimization goal)

**Test**: Compare interaction counts:
```hvm4
@test_termination_parity =
  ! dup_result = run_with_stats(DUP_VERSION)
  ! mov_result = run_with_stats(MOV_VERSION)
  @assert_equal(dup_result.output, mov_result.output)
  @assert_less_than(mov_result.interactions, dup_result.interactions * 1.1)
```

**Metric**: MOV ≤ DUP interactions, both terminate

## 2. Test Suite Structure

### Level 1: Atomic Interaction Tests (Unit)
One test per interaction rule. Minimal, focused, deterministic.

**Files to create**:
- `test/inv1_mov_simple.hvm4` - Basic MOV substitution
- `test/inv2_dup_mov_atomic.hvm4` - Single DUP-MOV interaction
- `test/inv3_branch_independent.hvm4` - Two branches reduce separately
- `test/inv4_value_preserved.hvm4` - Value identity check
- `test/inv5_termination_count.hvm4` - Interaction count comparison

**Requirements** (Tiger Style):
- **Fixed limits**: Each test has explicit interaction limit
- **Assertions**: Check invariants at interaction boundaries
- **Fail fast**: Stop immediately on invariant violation
- **Clear naming**: Test name = invariant + scenario

### Level 2: Composite Tests (Integration)
Multiple interactions combined. Tests interaction ordering and state management.

**Files to create**:
- `test/inv2_dup_mov_nested.hvm4` - DUP of nested MOV nodes
- `test/inv2_mov_dup_dup.hvm4` - MOV followed by multiple DUPs
- `test/inv3_mov_in_branches.hvm4` - MOV used differently in each branch

### Level 3: Canonical Test (End-to-End)
The bounty test case: `test/mov_issue_canonical.hvm4`

**Success criteria**:
1. ✅ Output matches DUP version exactly
2. ✅ Completes in < 50k interactions
3. ✅ No unresolved variables
4. ✅ All invariants hold at every reduction step

### Level 4: Property Tests (Generative)
Random generation of MOV patterns to find edge cases.

**Properties to test**:
- Commutative: `% x = v; % y = w; body` = `% y = w; % x = v; body`
- Associative: Nested MOVs can be flattened
- Idempotent: `% x = y; x` = `y` when x unused in y

## 3. Test Harness Design

### Instrumentation Points
Following Tiger Style "pair assertions", check invariants at:
1. **Entry**: Before interaction starts
2. **Exit**: After interaction completes
3. **Boundaries**: When crossing term types (MOV→LAM, MOV→DUP, etc.)

### Assertion Strategy
```c
// In clang/wnf/dup_mov.c
fn Term wnf_dup_mov(u32 lab, u32 loc, u8 side, Term mov) {
  // PRE-CONDITIONS (entry assertions)
  assert(term_tag(mov) == TAG_MOV);
  assert(lab < MAX_LABELS);

  Term mov_val = heap[term_val(mov) + 0];  // Get MOV value
  Term mov_bod = heap[term_val(mov) + 1];  // Get MOV body

  // INVARIANT: Value must be valid term
  assert(term_tag(mov_val) != TAG_ERR);

  // Perform duplication
  Term result = wnf_dup_nod(lab, loc, side, mov);

  // POST-CONDITIONS (exit assertions)
  // INV-2: Both branches must exist and be valid
  Term dup_fst = heap[term_val(result) + 0];
  Term dup_snd = heap[term_val(result) + 1];
  assert(term_tag(dup_fst) != TAG_ERR);
  assert(term_tag(dup_snd) != TAG_ERR);

  // INV-4: Value preservation - both copies should reference the value
  // (This is where the bug likely is!)

  return result;
}
```

### Error Reporting
**Fail fast with context**:
```c
#define ASSERT_INV(cond, inv_num, msg) \
  if (!(cond)) { \
    fprintf(stderr, "INVARIANT VIOLATION: INV-%d\n", inv_num); \
    fprintf(stderr, "Location: %s:%d\n", __FILE__, __LINE__); \
    fprintf(stderr, "Details: %s\n", msg); \
    fprintf(stderr, "Interaction count: %u\n", interaction_count); \
    print_term_debug(current_term); \
    abort(); \
  }
```

## 4. Debugging Strategy

### Hypothesis Testing
Using Tiger Style principles, test one hypothesis at a time:

**H1**: DUP-MOV doesn't copy the value correctly
- **Test**: Check if both DUP branches have access to mov_val
- **Fix**: Ensure value is duplicated, not just referenced

**H2**: DUP-MOV doesn't create independent GOT nodes
- **Test**: Check if each branch has its own GOT reference
- **Fix**: Create separate GOT nodes for each branch

**H3**: MOV substitution happens too early/late in reduction
- **Test**: Check interaction ordering with debug prints
- **Fix**: Adjust when MOV value is forced/resolved

**H4**: wnf_dup_nod doesn't handle MOV's internal structure
- **Test**: Compare heap layout before/after DUP-MOV
- **Fix**: Write specialized DUP-MOV that understands MOV structure

### Execution Plan
1. Run atomic tests with assertions enabled
2. Identify first failing invariant
3. Add instrumentation at failure point
4. Form hypothesis about root cause
5. Test hypothesis with minimal reproduction
6. Implement fix
7. Verify all invariants pass
8. Run canonical test to confirm

## 5. Performance Metrics (Napkin Math)

### Current State
- DUP version: ~97,885 interactions ✅
- MOV version: ~2,253 interactions (fails) ❌

### Target State
- MOV version: < 50,000 interactions ✅
- MOV version: same output as DUP ✅
- Speedup: ~2x fewer interactions (optimization benefit)

### Interaction Budget
For canonical test with `-C1`:
- **Strict limit**: 50,000 interactions
- **Warning threshold**: 40,000 interactions (80% of budget)
- **Optimal**: 30,000-40,000 interactions (showing real benefit)

## 6. Success Criteria

### Must Have
1. ✅ All 5 invariants pass on all tests
2. ✅ Canonical test produces correct output
3. ✅ Canonical test completes in < 50k interactions
4. ✅ No compiler warnings (`-Wall -Werror`)
5. ✅ All existing tests still pass

### Nice to Have
- 📊 Detailed interaction count comparison (DUP vs MOV)
- 📊 Memory usage comparison
- 📝 Documentation explaining the fix
- 🧪 Additional test cases for edge cases

## 7. Implementation Order

Following Tiger Style "do it right the first time":

1. **Understand** - Read all MOV interaction code, understand current behavior
2. **Instrument** - Add assertions for all 5 invariants
3. **Reproduce** - Run canonical test, capture exact failure mode
4. **Minimize** - Create smallest test case that triggers bug
5. **Diagnose** - Use assertions to pinpoint exact violation
6. **Fix** - Implement minimal correct solution
7. **Verify** - All invariants pass on all tests
8. **Document** - Explain what was wrong and why fix is correct

---

**Next Action**: Start with Level 1 atomic tests to identify which invariant fails first.
