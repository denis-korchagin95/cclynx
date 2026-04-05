# Warnings

## List

### unused-variable

**Message:** "unused variable '<name>'"

**Trigger:** A local variable is declared but never read.

**Category:** unused

### unused-parameter

**Message:** "unused parameter '<name>'"

**Trigger:** A function parameter is never read in the function body.

**Category:** unused

### empty-compound-statement

**Message:** "empty compound statement"

**Trigger:** A compound statement (block) `{ }` contains no statements.

**Category:** empty-body

### empty-if-body

**Message:** "empty if body"

**Trigger:** The if branch is an empty statement or empty compound.

**Category:** empty-body

### empty-else-body

**Message:** "empty else body"

**Trigger:** The else branch is an empty statement or empty compound.

**Category:** empty-body

### empty-while-body

**Message:** "empty while body"

**Trigger:** The while loop body is an empty statement or empty compound.

**Category:** empty-body

### empty-function-body

**Message:** "function '<name>' has empty body"

**Trigger:** A non-void function body contains no statements.

**Category:** empty-body

### missing-return

**Message:** "function '<name>' missing return statement"

**Trigger:** A non-void function does not always return a value.

**Category:** missing

### unspecified-parameters

**Message:** "function '<name>' has unspecified parameters, consider using '<name>(void)' or adding parameter types"

**Trigger:** Calling a function declared with empty parentheses `()` with arguments.

**Category:** (none)

### sign-conversion

**Message:** "implicit conversion changes signedness from '<type>' to '<type>', use an explicit cast"

**Trigger:** Implicit cast inserted between signed and unsigned in assignments, returns, or function arguments.

**Category:** signedness


## Warning Modes

### Default behavior
  Tolerant mode is active by default. All warnings are enabled except those
  in the tolerant suppression list (sign-conversion, sign-compare, etc.).
  This means correct C code compiles cleanly without noise.

### --no-warnings
  Suppress all warnings.

### -Wall
  Enable literally all warnings, overriding tolerant mode.
  This includes any other warnings that tolerant mode would normally suppress.

### -Wno-\<name\>
  Disable a specific warning by name or an entire category by category name.
  Can be combined with -Wall.
  
  Examples:
  ```
    -Wall -Wno-unused-variable      (all warnings except unused variables)
    -Wno-empty-if-body              (default + disable empty if body)
    -Wno-missing-return             (default + disable missing return)
    -Wno-empty-body                 (default + disable all empty-body warnings)
    -Wall -Wno-unused               (all warnings except unused category)
  ```

### -Wtolerant
  Explicit tolerant mode (same as default).

### Precedence (left to right)
  Flags are processed left to right. Later flags override earlier ones.
  
  Examples:
  ```
    -Wall -Wno-unused-variable      -> all except unused-variable
    --no-warnings                   -> none
  ```

## Warning Categories

  **unused**           : unused-variable, unused-parameter (enabled by default)
  
  **missing**          : missing-return (enabled by default)
  
  **empty-body**       : empty-compound-statement, empty-if-body, empty-else-body, empty-while-body, empty-function-body (enabled by default)
  
  **signedness**       : sign-conversion (suppressed by default in tolerant mode, enabled by -Wall)
