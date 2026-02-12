---
name: test-driven-development
description: Use when implementing any feature or bugfix in C++, before writing implementation code
---

# Test-Driven Development (TDD) for C++

## Overview
Write the test first. Watch it fail. Write minimal code to pass.

**Core principle:** If you didn't watch the test fail (or fail to compile), you don't know if it tests the right thing.

**Violating the letter of the rules is violating the spirit of the rules.**

## When to Use

**Always:**
- New features
- Bug fixes
- Refactoring
- Behavior changes

**Exceptions (ask your human partner):**
- Throwaway prototypes
- Generated code
- Configuration files

Thinking "skip TDD just this once"? Stop. That's rationalization.

## The Iron Law

NO PRODUCTION CODE WITHOUT A FAILING TEST FIRST
Write code before the test? Delete it. Start over.

**No exceptions:**
- Don't keep it as "reference"
- Don't "adapt" it while writing tests
- Don't look at it
- Delete means delete

Implement fresh from tests. Period.

## Red-Green-Refactor

### RED - Write Failing Test
Write one minimal test showing what should happen.

<Good>
```cpp
// math_test.cpp
#include <gtest/gtest.h>
#include "MathUtils.h"

TEST(MathUtilsTest, AddReturnsSum) {
    EXPECT_EQ(MathUtils::Add(2, 3), 5);
}
```
Clear name, tests real behavior, one thing. Fails to compile (Red Phase 1).
</Good>
<Bad>
```cpp
// ❌ BAD
TEST(MathTest, Works) {
    // Tests implementation details or mocks too much
    MockMath mock;
    EXPECT_CALL(mock, Add(2,3)).WillOnce(Return(5));
    // ...
}
```
Vague name, tests mock usage not logic.
</Bad>

**Requirements:**
- One behavior
- Clear name
- Real code (no mocks unless unavoidable)

### Verify RED - Watch It Fail

**MANDATORY. Never skip.**

```bash
make test
# OR
./build/tests/my_tests
```

Confirm:
- Test fails (assertion error) OR fails to compile (missing class/function).
- Failure message is expected.
- Fails because feature missing (not typos).

**Test passes (or compiles and runs)?** You're testing existing behavior. Fix test.
**Test errors (segfault/exception)?** Fix error, re-run until it fails correctly (assertion).

### GREEN - Minimal Code
Write simplest code to pass the test.

<Good>
```cpp
// MathUtils.h
class MathUtils {
public:
    static int Add(int a, int b) {
        return a + b;
    }
};
```
Just enough to pass.
</Good>
<Bad>
```cpp
// MathUtils.h
class MathUtils {
public:
    // YAGNI: Over-engineered with template metaprogramming or unnecessary flags
    template <typename T>
    static T Add(T a, T b, bool secure = false) {
        if (secure) { /* ... */ }
        return a + b;
    }
};
```
Over-engineered.
</Bad>

Don't add features, refactor other code, or "improve" beyond the test.

### Verify GREEN - Watch It Pass

**MANDATORY.**

```bash
make test
```

Confirm:
- Test passes.
- Other tests still pass.
- Output clean (no leaks/sanitizer errors).

**Test fails?** Fix code, not test.
**Other tests fail?** Fix now.

### REFACTOR - Clean Up
After green only:
- Remove duplication.
- Improve variable names.
- Extract helper functions.
- Use `const`, `auto`, standard algorithms.

Keep tests green. Don't add behavior.

### Repeat
Next failing test for next feature.

## Good Tests

| Quality | Good | Bad |
|---------|------|-----|
| **Minimal** | One assertion concept. | `TEST(User, ValidateEmailAndDomainAndWhitespace)` |
| **Clear** | Name describes behavior. | `TEST(UserTest, Test1)` |
| **Shows intent** | Demonstrates desired API usage. | Obscures what code should do. |

## Why Order Matters
**"I'll write tests after to verify it works"**
Tests written after code pass immediately. Passing immediately proves nothing:
- Might test wrong thing.
- Might test implementation logic (tautology), not requirements.
- Might miss edge cases you forgot.
- You never saw it catch the bug.

Test-first forces you to see the test fail, proving it actually tests something.

**"I already manually tested all the edge cases"**
Manual testing is ad-hoc. You think you tested everything but:
- No record of what you tested.
- Can't re-run when code changes.
- Easy to forget cases under pressure.
- "It worked when I tried it" ≠ comprehensive.

Automated tests are systematic. They run the same way every time.

**"Deleting X hours of work is wasteful"**
Sunk cost fallacy. The time is already gone. Your choice now:
- Delete and rewrite with TDD (X more hours, high confidence).
- Keep it and add tests after (30 min, low confidence, likely bugs).
The "waste" is keeping code you can't trust. Working code without real tests is technical debt.

**"TDD is dogmatic, being pragmatic means adapting"**
TDD IS pragmatic:
- Finds bugs before commit (faster than debugging after).
- Prevents regressions (tests catch breaks immediately).
- Documents behavior (tests show how to use code).
- Enables refactoring (change freely, tests catch breaks).

"Pragmatic" shortcuts = debugging in production = slower.

**"Tests after achieve the same goals - it's spirit not ritual"**
No. Tests-after answer "What does this do?"
Tests-first answer "What should this do?"
Tests-after are biased by your implementation. You test what you built, not what's required.
You verify remembered edge cases, not discovered ones. Tests-first force edge case discovery before implementing.
Tests-after verify you remembered everything (you didn't). 30 minutes of tests after ≠ TDD. You get coverage, lose proof tests work.

## Common Rationalizations

| Excuse | Reality |
|--------|---------|
| "Too simple to test" | Simple code breaks. Test takes 30 seconds. |
| "I'll test after" | Tests passing immediately prove nothing. |
| "Tests after achieve same goals" | Tests-after = "what does this do?" Tests-first = "what should this do?" |
| "Already manually tested" | Ad-hoc ≠ systematic. No record, can't re-run. |
| "Deleting X hours is wasteful" | Sunk cost fallacy. Keeping unverified code is technical debt. |
| "Keep as reference, write tests first" | You'll adapt it. That's testing after. Delete means delete. |
| "Need to explore first" | Fine. Throw away exploration, start with TDD. |
| "Test hard = design unclear" | Listen to test. Hard to test = hard to use. |
| "TDD will slow me down" | TDD faster than debugging. Pragmatic = test-first. |
| "Manual test faster" | Manual doesn't prove edge cases. You'll re-test every change. |
| "Existing code has no tests" | You're improving it. Add tests for existing code. |

## Red Flags - STOP and Start Over
- Code before test
- Test after implementation
- Test passes immediately
- Can't explain why test failed
- Tests added "later"
- Rationalizing "just this once"
- "I already manually tested it"
- "Tests after achieve the same purpose"
- "It's about spirit not ritual"
- "Keep as reference" or "adapt existing code"
- "Already spent X hours, deleting is wasteful"
- "TDD is dogmatic, I'm being pragmatic"
- "This is different because..."

**All of these mean: Delete code. Start over with TDD.**

## Example: Bug Fix

**Bug:** Empty email accepted

**RED**
```cpp
// UserTest.cpp
TEST(UserTest, RejectsEmptyEmail) {
    User user;
    auto result = user.setEmail("");
    EXPECT_EQ(result, User::Error::EmailRequired);
}
```

**Verify RED**
```bash
$ make test
[FAIL] Expected: EmailRequired, Actual: Success
```

**GREEN**
```cpp
// User.cpp
User::Error User::setEmail(const std::string& email) {
    if (email.empty()) {
        return Error::EmailRequired;
    }
    // ...
}
```

**Verify GREEN**
```bash
$ make test
[PASS] UserTest.RejectsEmptyEmail
```

**REFACTOR**
Extract validation for multiple fields if needed.

## Verification Checklist

Before marking work complete:
- [ ] Every new function/method has a test.
- [ ] Watched each test fail before implementing.
- [ ] Each test failed for expected reason (feature missing, not typo).
- [ ] Wrote minimal code to pass each test.
- [ ] All tests pass.
- [ ] Output pristine (no errors, warnings, sanitizer issues).
- [ ] Tests use real code (mocks/fakes only if unavoidable).
- [ ] Edge cases and errors covered.

Can't check all boxes? You skipped TDD. Start over.

## When Stuck

| Problem | Solution |
|---------|----------|
| Don't know how to test | Write wished-for API first. Write assertion first. Ask your human partner. |
| Test too complicated | Design too complicated. Simplify interface. |
| Must mock everything | Code too coupled. Use dependency injection (interfaces/templates). |
| Test setup huge | Extract helpers. Still complex? Simplify design/reduce dependencies. |

## Debugging

Integration Bug found? Write failing test reproducing it. Follow TDD cycle.
Test proves fix and prevents regression.
Never fix bugs without a test.

## Testing Anti-Patterns

When adding mocks or test utilities, read `testing_anti_patterns.md` to avoid common pitfalls:
- Testing mock behavior instead of real behavior.
- Adding test-only methods to production classes.
- Mocking without understanding dependencies.

## Final Rule
**Production code → test exists and failed first**
Otherwise → not TDD
No exceptions without your human partner's permission.
