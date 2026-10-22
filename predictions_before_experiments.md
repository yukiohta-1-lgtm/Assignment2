# Section 2 — Predictions BEFORE experiments (DRAFT)

**Important:** Review/edit this document BEFORE running `benchmark` and preserve
that version unchanged after collecting results. This document proposes
hypotheses; it is NOT a record of actual measurements or a guarantee of outcomes.

## Conditions
- Two C++17 implementations: full Boyer–Moore (bad character + good suffix)
  versus Karp–Rabin (rolling hash, seeded random base, default mod 1,000,000,007).
- Same text/pattern inputs, search ALL matches, include pattern preprocessing.

## Hypotheses to approve or edit
1. **Pattern length:** For random text, I expect Boyer–Moore to gain an
   advantage as the pattern gets longer because mismatches permit larger shifts.
   I tentatively predict a noticeable change in relative performance around
   8–16 characters, but this is an *unverified guess*, not a known threshold.
2. **Text length:** As text grows with a fixed pattern, I expect any Boyer–Moore
   skipping advantage to become more visible, since preprocessing becomes less
   significant. The *speed ratio may plateau* after setup cost is negligible.
3. **Repetition:** For the chosen repeating input (`A...A` with pattern
   `A...AB`), Boyer–Moore may have only small shifts; I expect its performance
   advantage versus Karp–Rabin to shrink or disappear. Other repetitive
   patterns can behave differently.
4. **Hash collisions:** On an intentionally weak modulus of 5 and adversarial
   input (`A...A` versus pattern `A...AF`), I predict Karp–Rabin will perform
   many false-positive checks and slow sharply compared with its default
   modulus. Boyer–Moore is unaffected by the hash modulus.

## Freeze log (fill in before running the timed benchmark)
- Date/time and timezone:
- Name or Git commit of frozen prediction file:
- Is this the FINAL pre-experiment prediction? (yes/no):

Keep this frozen document even if the experiment contradicts it.
