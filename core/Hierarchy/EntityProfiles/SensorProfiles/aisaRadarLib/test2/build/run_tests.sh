#!/bin/bash

TESTS=(
    test_radarantenna
    test_radarsignalprocessor
    test_radarscheduler
    test_radartracker
    test_radarsignallibrary
    test_radarmodel
)

TOTAL=0
TOTAL_PASSED=0
TOTAL_FAILED=0
ANY_FAILED=0

echo ""
echo "============================================================"
printf "%-35s %8s %8s %8s\n" "TEST SUITE" "PASSED" "FAILED" "TOTAL"
echo "============================================================"

for test in "${TESTS[@]}"; do
    if [ ! -f "./$test" ]; then
        printf "%-35s %8s %8s %8s\n" "$test" "-" "-" "NOT FOUND"
        continue
    fi

    # Run and capture full output
    OUTPUT=$(./$test 2>&1)

    # Parse from the summary line: "[  PASSED  ] 43 tests."
    PASSED=$(echo "$OUTPUT" | grep -oP '(?<=\[  PASSED  \] )\d+')
    FAILED=$(echo "$OUTPUT" | grep -oP '(?<=\[  FAILED  \] )\d+(?= test)')

    PASSED=${PASSED:-0}
    FAILED=${FAILED:-0}
    SUITE_TOTAL=$((PASSED + FAILED))

    TOTAL=$((TOTAL + SUITE_TOTAL))
    TOTAL_PASSED=$((TOTAL_PASSED + PASSED))
    TOTAL_FAILED=$((TOTAL_FAILED + FAILED))

    if [ "$FAILED" -gt 0 ]; then
        ANY_FAILED=1
    fi

    printf "%-35s %8s %8s %8s\n" "$test" "$PASSED" "$FAILED" "$SUITE_TOTAL"
done

echo "============================================================"

if [ $TOTAL -gt 0 ]; then
    PCT=$(awk "BEGIN {printf \"%.1f\", ($TOTAL_PASSED/$TOTAL)*100}")
else
    PCT="0.0"
fi

printf "%-35s %8s %8s %8s\n" "TOTAL" "$TOTAL_PASSED" "$TOTAL_FAILED" "$TOTAL"
printf "%-35s %7s%%\n" "PASS RATE" "$PCT"
echo "============================================================"
echo ""

# Print failures if any
if [ $ANY_FAILED -gt 0 ]; then
    echo "FAILED TESTS:"
    for test in "${TESTS[@]}"; do
        OUTPUT=$(./$test 2>&1)
        echo "$OUTPUT" | grep "\[  FAILED  \]" | grep -v "listed below" | \
            sed "s/\[  FAILED  \]/  FAIL [$test]/"
    done
    echo ""
    exit 1
fi

exit 0
