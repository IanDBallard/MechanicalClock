# Code Analysis Verification

## Rule Overview
Ensures that any claims about code size, deletion impact, or refactoring scope are based on measured evidence rather than assumptions. Prevents analytical errors that could lead to incorrect architectural decisions or accidental deletion of functional code.

## Key Principles

### When Analysis REQUIRES Verification
- **Code deletion recommendations**: Any claim about lines of code to be removed
- **Refactoring scope estimates**: Estimates of files/components affected 
- **Legacy code identification**: Claims about what code is "obsolete" or "unused"
- **Interdependency analysis**: Statements about what code depends on what
- **Performance impact claims**: Assertions about code that affects system performance

### When Simple Estimation Is Acceptable
- **Feature addition estimates**: New code that doesn't affect existing systems
- **Documentation tasks**: Writing/updating docs with known scope
- **Configuration changes**: Simple parameter adjustments
- **Single-file modifications**: Changes contained within one known file

## Decision Tree

**Ask these questions in order:**

1. **Am I making a claim about existing code size or deletion impact?**
   - YES → **REQUIRED: Measure first with verification commands**
   - NO → Continue to Q2

2. **Am I recommending deletion or major refactoring?**
   - YES → **REQUIRED: Verify zero dependencies and create checkpoint**
   - NO → Continue to Q3

3. **Am I estimating impact on multiple files/components?**
   - YES → **REQUIRED: List and measure each component individually**
   - NO → Continue to Q4

4. **Are my estimates based on search results or assumptions?**
   - YES → **REQUIRED: Convert to measured facts**
   - NO → Estimation acceptable with uncertainty acknowledgment

## Required Verification Commands

### **File Size Verification**
```bash
# Get actual file sizes (always run before deletion claims)
find . -name "*.dart" -exec wc -l {} \; | sort -nr | head -20

# Count lines in specific file
wc -l path/to/specific_file.dart
```

### **Dependency Verification**
```bash
# Find actual usage of class/function
grep -r "TargetClass" --include="*.dart" | wc -l

# Find import dependencies
grep -r "import.*target_file" --include="*.dart"

# Check for zero dependencies before deletion
grep -r "TrackerStateManager" --include="*.dart"
```

### **Interdependency Verification**
```bash
# Find what files reference the target
find . -name "*.dart" -exec grep -l "TargetClass" {} \;

# Verify provider usage
grep -r "targetProvider" --include="*.dart"
```

## Project-Specific Applications

### Cat Tracker Examples

#### ✅ GOOD: Measured Analysis
```bash
# Before claiming "thousands of lines to delete"
$ wc -l tracker_state_manager.dart
478 tracker_state_manager.dart

# Before claiming "no dependencies"
$ grep -r "TrackerStateManager" --include="*.dart"
(no results = truly no dependencies)
```

#### ✅ GOOD: Evidence-Based Claims
```
"The tracker_state_manager.dart file contains 478 lines and has zero dependencies based on grep search results."
```

#### ❌ BAD: Assumption-Based Claims
```
"You have 17,000+ lines of legacy code that can be safely deleted."
(Made without measuring actual file sizes)
```

#### ✅ GOOD: Incremental Verification
```bash
# Verify each deletion step
git add -A && git commit -m "CHECKPOINT: Before deleting tracker_state_manager.dart"
rm tracker_state_manager.dart
flutter analyze
# If passes: commit, continue. If fails: restore, investigate
```

## Error Prevention Patterns

### **Red Flags Requiring Verification**
- Claims of "thousands of lines" without measurements
- Estimates based on search result volume
- Deletion recommendations without dependency checks
- Assumptions about code organization
- Extrapolations from limited evidence

### **Safe Estimation Patterns**
```bash
# GOOD: Verify before claiming
if [ -f "target_file.dart" ]; then
    lines=$(wc -l < target_file.dart)
    echo "File contains $lines lines"
else
    echo "File does not exist"
fi

# GOOD: Measure interdependencies
deps=$(grep -r "TargetClass" --include="*.dart" | wc -l)
if [ $deps -eq 0 ]; then
    echo "Zero dependencies found - safe to delete"
else
    echo "$deps dependencies found - investigate before deletion"
fi
```

## Recovery Protocol

### When Analysis Proves Wrong
1. **Acknowledge Error**: "My analysis was incorrect - let me re-measure"
2. **Use Backup**: Restore from checkpoint commit
3. **Re-analyze**: Apply verification-first approach
4. **Document Lesson**: Update memory/rules with corrected process

### Estimation Calibration
- Track estimation accuracy over time
- Goal: Keep errors under 2x actual values
- Red flag: Errors over 5x indicate systematic analysis problems

## Guidelines
- **Measurement Over Assumption**: Always measure before making claims about code size
- **Incremental Verification**: Delete one file at a time with verification between steps
- **Checkpoint Protocol**: Create git commits before any major deletions
- **Dependency Validation**: Verify zero dependencies before deletion recommendations
- **Evidence-Based Claims**: Replace assumptions with measured facts
- **Cat Tracker Specific**: Flutter projects require `flutter analyze` verification after changes
- **Backup Strategy**: Always create recovery points before implementing recommendations

## Integration with Other Rules

- **File Existence Verification**: Verify files exist before measuring their size
- **Simplicity First**: Simple measurements before complex analysis
- **Documentation First**: Document the verification process in commit messages
- **Context Window Management**: Include verification results in session summaries

This rule prevents analytical errors that could lead to accidental deletion of functional code or incorrect architectural decisions by requiring evidence-based analysis. 