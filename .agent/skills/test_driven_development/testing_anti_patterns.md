# Testing Anti-Patterns (C++ Edition)

**Load this reference when:** writing or changing tests, adding mocks, or tempted to add test-only methods to production code.

## Overview
Tests must verify real behavior, not mock behavior. Mocks are a means to isolate, not the thing being tested.

**Core principle:** Test what the code does, not what the mocks do.

**Following strict TDD prevents these anti-patterns.**

## The Iron Laws
1. NEVER test mock behavior
2. NEVER add test-only methods to production classes
3. NEVER mock without understanding dependencies

## Anti-Pattern 1: Testing Mock Behavior

**The violation:**
```cpp
// ❌ BAD: Testing that the mock exists or was called without asserting real behavior
TEST_F(PageTest, RendersSidebar) {
  auto mockSidebar = std::make_shared<MockSidebar>();
  page->setSidebar(mockSidebar);
  
  page->render();
  
  // You're verifying the mock met expectations, but does the page actually work?
  EXPECT_CALL(*mockSidebar, render()).Times(1); 
  // Test passes even if Page renders nothing but calls the mock!
}
```

**Why this is wrong:**
- You're verifying the mock interaction, not that the component works as intended.
- Test passes when mock is called, but the app might still be broken (e.g., incorrect data passed).
- Tells you nothing about real behavior.

**your human partner's correction:** "Are we testing the behavior of a mock?"

**The fix:**
```cpp
// ✅ GOOD: Test real component behavior or state change
TEST_F(PageTest, RendersNavigationElements) {
  // Use a real sidebar if possible, or a fake that acts like one
  page->render();
  
  // Assert on the actual output or state change of the Page
  EXPECT_TRUE(page->hasElement("navigation_bar"));
}

// OR if sidebar must be mocked for isolation:
// Don't just assert on the mock - test Page's behavior with sidebar present
```

### Gate Function
BEFORE asserting on any mock expectation:
  Ask: "Am I testing real component behavior or just that a function was called?"

  IF testing function call only:
    STOP - Delete the expectation or verify the *result* of that call.

  Test real behavior (state change, return value) instead.

## Anti-Pattern 2: Test-Only Methods in Production

**The violation:**
```cpp
// ❌ BAD: public destroyForTest() only used in tests
class Session {
public:
  void destroyForTest() {  // Looks like production API!
    if (workspaceManager) {
        workspaceManager->destroyWorkspace(id);
    }
  }
};

// In tests
TearDown() {
  session->destroyForTest();
}
```

**Why this is wrong:**
- Production class polluted with test-only code.
- Dangerous if accidentally called in production.
- Violates YAGNI and separation of concerns.
- Confuses object lifecycle with entity lifecycle.

**The fix:**
```cpp
// ✅ GOOD: Test utilities handle test cleanup or use friend classes (sparingly)

// In TestUtils.h
class SessionTestHelper {
public:
    static void cleanupSession(Session& session) {
        // Access internals via friend declaration if absolutely necessary
        // or better, use public API to achieve cleanup
        auto workspace = session.getWorkspaceInfo();
        if (workspace) {
            WorkspaceManager::instance().destroyWorkspace(workspace.id);
        }
    }
};

// In tests
TearDown() {
  SessionTestHelper::cleanupSession(*session);
}
```

### Gate Function
BEFORE adding any method to production class:
  Ask: "Is this only used by tests?"

  IF yes:
    STOP - Don't add it.
    Put it in test utilities instead.

  Ask: "Does this class own this resource's lifecycle?"

  IF no:
    STOP - Wrong class for this method.

## Anti-Pattern 3: Mocking Without Understanding

**The violation:**
```cpp
// ❌ BAD: Mock breaks test logic
TEST(ServerTest, DetectsDuplicateServer) {
  // Mock prevents config write that test depends on!
  auto mockConfig = std::make_shared<MockConfig>();
  EXPECT_CALL(*mockConfig, write(_)).Times(0); // aggressive mocking

  Server server(mockConfig);
  server.add(config); 
  server.add(config); // Should throw, but might not if it depends on write!
}
```

**Why this is wrong:**
- Mocked method had side effect test depended on (writing config/state).
- Over-mocking to "be safe" breaks actual behavior.
- Test passes for wrong reason or fails mysteriously.

**The fix:**
```cpp
// ✅ GOOD: Mock at correct level or use Fake
TEST(ServerTest, DetectsDuplicateServer) {
  // Use a InMemoryConfig (Fake) instead of a Mock
  auto config = std::make_shared<InMemoryConfig>(); 

  Server server(config);
  server.add(config); 
  ASSERT_THROW(server.add(config), std::runtime_error); // Duplicate detected ✓
}
```

### Gate Function
BEFORE mocking any method:
  STOP - Don't mock yet.

  1. Ask: "What side effects does the real method have?"
  2. Ask: "Does this test depend on any of those side effects?"
  3. Ask: "Do I fully understand what this test needs?"

  IF depends on side effects:
    Use a FAKE (in-memory implementation) instead of a MOCK.
    OR invoke the real method if it's fast enough.

  IF unsure what test depends on:
    Run test with real implementation FIRST.
    Observe what actually needs to happen.
    THEN add minimal mocking/stubbing at the right level.

  Red flags:
    - "I'll mock this to be safe"
    - "This might be slow, better mock it" (measure it first!)
    - Mocking `std::string`, `std::vector` or simple value types.

## Anti-Pattern 4: Incomplete Mocks

**The violation:**
```cpp
// ❌ BAD: Partial mock - returns uninitialized/garbage data for fields not "needed"
struct UserData {
    int id;
    std::string name;
    std::string role;
};

// In test
auto mockRepo = std::make_shared<MockUserRepository>();
UserData partialUser; 
partialUser.id = 123; // Didn't set name or role!
EXPECT_CALL(*mockRepo, getUser(123)).WillOnce(Return(partialUser));

// Later: system crashes when accessing user.role
```

**Why this is wrong:**
- **Partial mocks hide structural assumptions**.
- **Downstream code may depend on fields you didn't include**.
- **Undefined Behavior (UB)** in C++ if accessing uninitialized memory.
- **False confidence**: Test proves nothing about real behavior.

**The Iron Rule:** Mock the COMPLETE data structure as it exists in reality, not just fields your immediate test uses.

**The fix:**
```cpp
// ✅ GOOD: Mirror real API completeness
UserData completeUser;
completeUser.id = 123;
completeUser.name = "Alice";
completeUser.role = "Admin";
// All fields initialized

EXPECT_CALL(*mockRepo, getUser(123)).WillOnce(Return(completeUser));
```

### Gate Function
BEFORE creating mock responses:
  Check: "Are all fields valid and initialized?"

  Actions:
    1. Check struct/class definition.
    2. Ensure valid state (no null pointers, no uninitialized bools/ints).
    3. Verify mock matches real response schema completely.

  Critical:
    If you're returning a struct/class, you must understand its INVARIANTS.
    Partial mocks = Undefined Behavior waiting to happen.

## Anti-Pattern 5: Integration Tests as Afterthought

**The violation:**
✅ Implementation complete
❌ No tests written
"Ready for testing"

**Why this is wrong:**
- Testing is part of implementation, not optional follow-up.
- TDD would have caught design flaws (hard-to-test code).
- Can't claim complete without tests.

**The fix:**
TDD cycle:
1. Write failing test (compilation error or assertion failure).
2. Implement to pass.
3. Refactor.
4. THEN claim complete.

## When Mocks Become Too Complex

**Warning signs:**
- `EXPECT_CALL` setup longer than test logic.
- Mocking `std::filesystem`, `std::chrono` triggers headaches (use Fakes!).
- Mocks missing overloads real components have.
- Test breaks when mock internal logic changes.

**your human partner's question:** "Do we need to be using a mock here?"

**Consider:** Integration tests with real components or **Fakes** (simple in-memory implementations) are often simpler than complex mocks.

## TDD Prevents These Anti-Patterns

**Why TDD helps:**
1. **Write test first** → Forces you to think about the public API.
2. **Watch it fail** → Confirms test tests real behavior, not mocks.
3. **Minimal implementation** → No test-only methods creep in.
4. **Real dependencies** → You see what the test actually needs before mocking.

**If you're testing mock behavior, you violated TDD** - you added mocks without watching test fail against real code first.

## Quick Reference

| Anti-Pattern | Fix |
|--------------|-----|
| Assert on mock calls only | Test state change or return value |
| Test-only methods in production | Move to test utilities / Friend classes |
| Mock without understanding | Understand dependencies first, use Fakes |
| Incomplete mocks (uninit vars) | Initialize ALL fields, valid state |
| Tests as afterthought | TDD - tests first |
| Over-complex mocks | Use Fakes or Integration tests |

## Red Flags
- Assertion checks for verification counts only.
- Methods `public` but only used in `test/` folder.
- Mock setup is >50% of test body.
- Test fails when you remove mock.
- You are mocking value types or data structures (DTOs).
- Mocking "just to be safe".

## The Bottom Line
**Mocks are tools to isolate, not things to test.**
If TDD reveals you're testing mock behavior, you've gone wrong.
Fix: Test real behavior or question why you're mocking at all. Use **Fakes** over Mocks whenever possible in C++.
