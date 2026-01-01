# $10k HVM4 MOV Nodes Bounty

**Source**: https://x.com/VictorTaelin/status/2006818916211834900

## Problem Statement

HVM4 features MOV nodes designed to allow using a variable more than once in different branches without an extra lam/dup node. However, using DUP nodes and MOV nodes gives different answers in certain cases.

## Two Possibilities

1. **The implementation is wrong** (most likely)
2. **The concept is wrong** (in which case, why?)

## Context

- Implementation is AI-generated and not reviewed
- Victor spent 0 minutes debugging this
- There's a real chance this is just fixing stupid AI-generated code (easy money)
- Could also be way harder than expected

## Bounty Goal

1. Figure out where the divergence comes from
2. Explain it
3. Implement a solution that causes the test program to:
   - Output the correct result
   - Complete in **< 50k interactions** (proving the optimization works)
4. Solution must be correct: using MOV nodes or DUP nodes to pass a value to different branches produces the same output

## Submission

- Submit as PR on HVM4's repo
- Will be tested on target program and similar programs
- First correct solution wins $10k (paid in USDT)

## Target Program

See `test/mov_issue.hvm4`:

```hvm4
@tail = λ{[]:[];<>:λa.λb.b}

// List ::= Cons List | Succ List | Nil
@C = λt. λc. λs. λn. c(t)
@S = λp. λc. λs. λn. s(p)
@N =     λc. λs. λn. n

// Bits ::= O Bits | I Bits | E
@O = λp. λo. λi. λe. o(p)
@I = λp. λo. λi. λe. i(p)
@E =     λo. λi. λe. e

// ℕ → U32 → Bin -- converts a U32 to a Bin with a given size
@bin = λ{
  0n: λn. λo.λi.λe.e;
  1n+: λ&l. λ&n. (λ{
    0: λo.λi.λe.o(@bin(l, n/2))
    1: λo.λi.λe.i(@bin(l, n/2))
  })(n % 2)
}

// Bin → (P → P) → P → P -- applies a function N times to an argument
@rep = λxs.
  ! O = λp.λ&f.λx.@rep(p,λk.f(f(k)),x)
  ! I = λp.λ&f.λx.@rep(p,λk.f(f(k)),f(x))
  ! E = λf.λx.x
  xs(O, I, E)

// [1,2,3] ::= Cons (Succ (Cons (Succ (Succ (Cons (Succ (Succ (Succ Nil)))))))
@to_nats_go = λxs. λn.
  ! C = λt. λn. n <> @to_nats_go(t, 0n)
  ! S = λp. λn. @to_nats_go(p, 1n+n)
  ! N = λn. [n]
  xs(C, S, N, n)

@to_nats = λxs.
  @tail(@to_nats_go(xs,0n))

@view = λxs.
  ! O = λp. #O{@view(p)}
  ! I = λp. #I{@view(p)}
  ! E = #E
  xs(O, I, E)

// control: using DUP nodes (this works)
@insert_A = λn.
  ! O = &{}
  ! I = (λ&p. λxs. λ&o. λi. λe.
    ! O = λxs. o(@insert_A(p, xs))
    ! I = λxs. i(@insert_A(p, xs))
    ! E = o(@insert_A(p, @N))
    xs(O, I, E))
  ! E = λxs. λo. λ&i. λe.
    ! O = λxs. i(xs)
    ! I = λxs. i(xs)
    ! E = i(@N)
    xs(O, I, E)
  n(O, I, E)

// problem: using MOV nodes (this fails)
@insert_B = λn.
  ! O = &{}
  ! I = (λ&p. λxs. λ&o. λi. λe.
    % f = o
    ! O = λxs. f(@insert_B(p, xs))
    ! I = λxs. i(@insert_B(p, xs))
    ! E = f(@insert_B(p, @N))
    xs(O, I, E))
  ! E = λxs. λo. λ&i. λe.
    % k = i
    ! O = λxs. k(xs)
    ! I = λxs. k(xs)
    ! E = k(@N)
    xs(O, I, E)
  n(O, I, E)

// this should produce the same output with either @insert fns above when
// running it with `HVM4 -C1`, but, currently, the outputs mismatch...
@main =
  ! ins = @insert_B(@I(@I(@I(@I(@E)))))
  @view(@rep(@bin(16n,512), ins, @O(@O(@O(@O(@O(@O(@E))))))))
```

## Expected Behavior

Both `@insert_A` and `@insert_B` should produce the same output with `-C1` flag.

## Current Behavior

- **insert_A** (DUP version): Works correctly, produces `#O{#O{#O{#O{#I{#O{#E{}}}}}}}` in ~97,885 interactions
- **insert_B** (MOV version): Gets stuck with unresolved variable bindings at ~2,253 interactions

## Notes

- AI's suck at understanding interaction nets
- Implementation likely has bugs since it's AI-generated and unreviewed
- This could be trivial or impossible - no guarantees
