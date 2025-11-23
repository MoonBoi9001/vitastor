// Test for checksum padding calculation (Issue #114)
// Verifies that padding values are correctly calculated for various write scenarios

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// Test helper to calculate padding values
struct padding_result {
    int64_t left_pad;
    int64_t right_pad;
};

// Current (buggy) formula from line 183
padding_result calculate_padding_buggy(uint64_t offset, uint64_t len, uint64_t csum_block_size)
{
    uint32_t start = offset / csum_block_size;
    uint32_t end = (offset + len - 1) / csum_block_size;

    return {
        .left_pad = (int64_t)(offset - start * csum_block_size),
        .right_pad = (int64_t)(end * csum_block_size - (offset + len))  // BUGGY
    };
}

// Fixed formula using (end+1) like line 195
padding_result calculate_padding_fixed(uint64_t offset, uint64_t len, uint64_t csum_block_size)
{
    uint32_t start = offset / csum_block_size;
    uint32_t end = (offset + len - 1) / csum_block_size;

    return {
        .left_pad = (int64_t)(offset - start * csum_block_size),
        .right_pad = (int64_t)((end + 1) * csum_block_size - (offset + len))  // FIXED
    };
}

void test_padding_calculation()
{
    printf("Testing checksum padding calculations...\n\n");

    // Arrange: Test cases for various write scenarios
    struct test_case {
        const char* description;
        uint64_t offset;
        uint64_t len;
        uint64_t csum_block_size;
        int64_t expected_left_pad;
        int64_t expected_right_pad;
    };

    test_case cases[] = {
        // Test 1: 4KB write at offset 0 with 8KB csum blocks (Issue #114 scenario)
        {"4KB write at offset 0 (8KB csum)", 0, 4096, 8192, 0, 4096},

        // Test 2: 4KB write at offset 4096 (second half of csum block)
        {"4KB write at offset 4096 (8KB csum)", 4096, 4096, 8192, 4096, 0},

        // Test 3: 8KB write at offset 0 (full csum block)
        {"8KB write at offset 0 (8KB csum)", 0, 8192, 8192, 0, 0},

        // Test 4: 2KB write at offset 1024 (unaligned)
        {"2KB write at offset 1024 (8KB csum)", 1024, 2048, 8192, 1024, 5120},

        // Test 5: 16KB write at offset 0 (16KB csum) - single block
        {"16KB write at offset 0 (16KB csum)", 0, 16384, 16384, 0, 0},

        // Test 6: 8KB write at offset 0 (16KB csum) - partial block
        {"8KB write at offset 0 (16KB csum)", 0, 8192, 16384, 0, 8192},
    };

    int buggy_failures = 0;
    int fixed_failures = 0;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        test_case& tc = cases[i];

        // Act: Calculate padding with both formulas
        padding_result buggy = calculate_padding_buggy(tc.offset, tc.len, tc.csum_block_size);
        padding_result fixed = calculate_padding_fixed(tc.offset, tc.len, tc.csum_block_size);

        printf("Test %zu: %s\n", i + 1, tc.description);
        printf("  Input: offset=%lu, len=%lu, csum_block_size=%lu\n",
               tc.offset, tc.len, tc.csum_block_size);
        printf("  Expected: left_pad=%ld, right_pad=%ld\n",
               tc.expected_left_pad, tc.expected_right_pad);
        printf("  Buggy:    left_pad=%ld, right_pad=%ld",
               buggy.left_pad, buggy.right_pad);

        // Assert: Verify buggy formula fails
        if (buggy.left_pad != tc.expected_left_pad || buggy.right_pad != tc.expected_right_pad)
        {
            printf(" [FAIL - demonstrates bug]\n");
            buggy_failures++;
        }
        else
        {
            printf(" [PASS]\n");
        }

        printf("  Fixed:    left_pad=%ld, right_pad=%ld",
               fixed.left_pad, fixed.right_pad);

        // Assert: Verify fixed formula passes
        if (fixed.left_pad != tc.expected_left_pad || fixed.right_pad != tc.expected_right_pad)
        {
            printf(" [FAIL]\n");
            fixed_failures++;
        }
        else
        {
            printf(" [PASS]\n");
        }

        // Assert: Padding must be non-negative
        assert(fixed.left_pad >= 0 && "Left padding must be non-negative");
        assert(fixed.right_pad >= 0 && "Right padding must be non-negative");

        printf("\n");
    }

    printf("Summary:\n");
    printf("  Buggy formula: %d/%zu tests failed (expected failures)\n",
           buggy_failures, sizeof(cases) / sizeof(cases[0]));
    printf("  Fixed formula: %d/%zu tests failed\n",
           fixed_failures, sizeof(cases) / sizeof(cases[0]));

    // Final assertion: Fixed formula should pass all tests
    assert(fixed_failures == 0 && "All tests should pass with fixed formula");

    printf("\n✓ All tests passed with fixed formula!\n");
}

int main()
{
    test_padding_calculation();
    return 0;
}
