#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_SIGNALS 100
#define MAX_LINE_LENGTH 1024
#define MAX_BINARY_LENGTH 65 // 64 bits + 1 for null terminator

typedef struct {
    char identifier[10];
    char name[100];
} Signal;

// Function to convert binary string to signed int64
int64_t binary_to_int64(const char *binary) {
    int64_t value = 0;
    int length = strlen(binary);
    
    // Handle sign extension for negative numbers
    int is_negative = (binary[0] == '1' && length == 64);

    for (int i = 0; i < length; i++) {
        value = (value << 1) | (binary[i] - '0');
    }

    if (is_negative) {
        // Perform two's complement if the number is negative
        value -= (int64_t)1 << 64;
    }

    return value;
}

void process_vcd_to_csv(FILE *vcd_file, FILE *csv_file) {
    char line[MAX_LINE_LENGTH];
    Signal signals[MAX_SIGNALS];
    int signal_count = 0;
    int timestamp = 0;
    int i;

    // First pass: Parse the signal definitions
    while (fgets(line, sizeof(line), vcd_file)) {
        if (strncmp(line, "$var", 4) == 0) {
            // Extract signal information
            Signal signal;
            sscanf(line, "$var wire %*d %s %s $end", signal.identifier, signal.name);
            signals[signal_count++] = signal;
        } else if (strncmp(line, "$enddefinitions", 15) == 0) {
            break;
        }
    }

    // Write header to CSV file
    fprintf(csv_file, "Time");
    for (i = 0; i < signal_count; i++) {
        fprintf(csv_file, ",%s", signals[i].name);
    }
    fprintf(csv_file, "\n");

    // Second pass: Parse the signal values over time
    char signal_values[MAX_SIGNALS][MAX_BINARY_LENGTH];
    memset(signal_values, '0', sizeof(signal_values)); // Initialize signal values with '0'
    for (i = 0; i < signal_count; i++) {
        signal_values[i][64] = '\0'; // Null terminate the strings
    }

    while (fgets(line, sizeof(line), vcd_file)) {
        if (line[0] == '#') {
            // New timestamp
            timestamp = atoi(&line[1]);
            fprintf(csv_file, "%d", timestamp);
            for (i = 0; i < signal_count; i++) {
                int64_t int_value = binary_to_int64(signal_values[i]);
                fprintf(csv_file, ",%lld", (long long int)int_value);
            }
            fprintf(csv_file, "\n");
        } else if (line[0] == 'b') {
            // Binary signal value change
            char binary[MAX_BINARY_LENGTH], identifier[10];
            sscanf(line, "b%s %s", binary, identifier);
            for (i = 0; i < signal_count; i++) {
                if (strcmp(identifier, signals[i].identifier) == 0) {
                    strncpy(signal_values[i], binary, MAX_BINARY_LENGTH - 1);
                    break;
                }
            }
        } else if (line[0] == '0' || line[0] == '1') {
            // Single-bit signal value change
            char identifier[10];
            sscanf(line, "%s", identifier);
            for (i = 0; i < signal_count; i++) {
                if (strcmp(identifier, signals[i].identifier) == 0) {
                    signal_values[i][63] = line[0]; // Update the last bit
                    break;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_vcd_file> <output_csv_file>\n", argv[0]);
        return 1;
    }

    FILE *vcd_file = fopen(argv[1], "r");
    if (!vcd_file) {
        perror("Error opening VCD file");
        return 1;
    }

    FILE *csv_file = fopen(argv[2], "w");
    if (!csv_file) {
        perror("Error opening CSV file");
        fclose(vcd_file);
        return 1;
    }

    process_vcd_to_csv(vcd_file, csv_file);

    fclose(vcd_file);
    fclose(csv_file);

    printf("VCD file successfully converted to CSV file.\n");

    return 0;
}
