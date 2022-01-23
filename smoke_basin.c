/** @file smoke_basin.c
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
 //#define PRINT_DEBUG // Turn this on for debug output

/**
* Give a file name, open the file of height map entries, read it into memory,
* store a \0 following the last character to signal end of file.
*
* @param    file_name   path to input file.
* @param    p_file_size_ret points to int where size of filee is returned.
* @retval   returns pointer to buffer that contains input string
*/
unsigned char* read_input(char* file_name, int *p_file_size_ret) {
    FILE* input_file_handle = fopen(file_name, "rb");
    if (input_file_handle == NULL) {
        fprintf(stderr, "Error reading input_buffer %s: %s\n", file_name, strerror(errno));
        exit(-1);
    }
    // Find out how many bytes are in the file.
    fseek(input_file_handle, 0, SEEK_END);
    int file_size = ftell(input_file_handle);
    rewind(input_file_handle);

    // Allocate a buffer to read in the file.
    // Allocate one extra byte so we can store 0 at the end.
    unsigned char* input_buffer = (char*)malloc(file_size + 1);
    if (input_buffer == NULL) {
        printf("Unable to allocate input_buffer buffer %d bytes\n", file_size);
        exit(-1);
    }

    int bytes_read = (int)fread((void*)input_buffer, sizeof(char), file_size, input_file_handle);
    // Make sure we read the whole enchilada.
    if (bytes_read != file_size) {
        printf("Error eof reading %s: %s. Expected %d bytes, read %d bytes\n",
            file_name, strerror(errno), bytes_read, file_size);
        exit(-1);
    }
    fclose(input_file_handle);
    input_buffer[file_size] = 0; // Store 0 following the last byte to assure end of string.
    *p_file_size_ret = file_size;
    return input_buffer;
}

/**
* Give a point to a buffer, determine the number of rows and columns.
*
* @param    p_input_buffer   pointer to input buffer
* @param    buffer_size size of buffer in bytes, including ending \0
* @param    p_num_rows_ret pointer to int where number of rows is returned
* @param    p_num_cols_ret pointer to int where number of cols is returned
*/
void create_height_map(char *p_input_buffer, int buffer_size, int *p_num_rows_ret, int *p_num_cols_ret) {
    *p_num_cols_ret = *p_num_rows_ret = 0;
    for (int index = 0; index < buffer_size; index++) {
        if ((p_input_buffer[index] < '0') || (p_input_buffer[index] > '9')) {
            (*p_num_rows_ret)++;
            if(*p_num_cols_ret == 0) *p_num_cols_ret = index;
            // scan to next character
            while ((p_input_buffer[index] < '0') || (p_input_buffer[index] > '9')) index++;
        }
    }
    if (*p_num_cols_ret == 0) {
        printf("No newline character found in input buffer\n");
        exit(-1);
    }
    (*p_num_rows_ret)++;
}

int find_low_points(unsigned char* height_map, int num_rows, int num_cols) {
#ifdef _WIN32 
    // On Windows, a newline is composed of two characters: 0x0D0A
    // N.B. the parentheses around row are essential. 
    // e.g., if row is 2-1, the expression will not give the expected result without ().
#define INDEX(row, col) ((row) * (num_cols + 2) + col)
#else
    // One character for newline
#define INDEX(row, col) ((row) * (num_cols + 1) + col)
#endif
#define HIGH_VALUE ('9' + 1)
    int risk_level = 0;
    unsigned char up, down, left, right;
    int last_row = num_rows - 1;
    int last_col = num_cols - 1;
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            unsigned char height = height_map[INDEX(row, col)];
            if (row == 0) up = HIGH_VALUE; else up = height_map[INDEX(row - 1, col)];
            if (col == 0) left = HIGH_VALUE; else left = height_map[INDEX(row, col - 1)];
            if (row == last_row) down = HIGH_VALUE; else down = height_map[INDEX(row + 1, col)];
            if (col == last_col) right = HIGH_VALUE; else right = height_map[INDEX(row, col + 1)];
            if ((height < up) && (height < down) && (height < right) && (height < left)) {
                // We found a low point
                // increment the risk level.
                risk_level += (height - '0' + 1);
            }
        }
    }
    return risk_level;
}

int search(unsigned char* height_map, int num_rows, int num_cols, int row, int col) {
#ifdef _WIN32 
    // On Windows, a newline is composed of two characters: 0x0D0A
    // N.B. the parentheses around row are essential. 
    // e.g., if row is 2-1, the expression will not give the expected result without ().
#define INDEX(row, col) ((row) * (num_cols + 2) + col)
#else
    // One character for newline
#define INDEX(row, col) ((row) * (num_cols + 1) + col)
#endif
    int last_row = num_rows - 1;
    int last_col = num_cols - 1;
    int basin_value = 1;

    // Replace the contents of this cell with a '9' so we won't search it again.
    height_map[INDEX(row, col)] = '9';

    // Search right if basin to the right
    if ((col != last_col) && (height_map[INDEX(row, col + 1)] != '9'))
        basin_value += search(height_map, num_rows, num_cols, row, col + 1);
    // Search down if basin down one row
    if ((row != last_row) && (height_map[INDEX(row + 1, col)] != '9'))
        basin_value += search(height_map, num_rows, num_cols, row + 1, col);
    // Search left if basin to the left
    if ((col != 0) && (height_map[INDEX(row, col - 1)] != '9'))
        basin_value += search(height_map, num_rows, num_cols, row, col - 1);
    // Search up if basin up one row
    if ((row != 0) && (height_map[INDEX(row - 1, col)] != '9'))
        basin_value += search(height_map, num_rows, num_cols, row - 1, col);
    return basin_value;
}

/**
* The idea is to find all adjacent cells in the height map that are not '9'.
* Start in the upper right corner. 
* For each cell in the row, if the cell is a basin (not a '9'), call search(row, col).
*/
int find_basins(unsigned char* height_map, int num_rows, int num_cols) {
#ifdef _WIN32 
    // On Windows, a newline is composed of two characters: 0x0D0A
    // N.B. the parentheses around row are essential. 
    // e.g., if row is 2-1, the expression will not give the expected result without ().
#define INDEX(row, col) ((row) * (num_cols + 2) + col)
#else
    // One character for newline
#define INDEX(row, col) ((row) * (num_cols + 1) + col)
#endif
    int basin_value;
    int best_basin_values[4] = { 0, 0, 0, 0 };
    int num_best_basin_values = 0;
    for (int row = 0; row < num_rows; row++) {
        for (int col = 0; col < num_cols; col++) {
            unsigned char height = height_map[INDEX(row, col)];
            if (height != '9') {
                basin_value = search(height_map, num_rows, num_cols, row, col);
                printf("Basin value for [%d, %d]=%d\n", row, col, basin_value);
                // Save the new basin value in the sorted array
                if (num_best_basin_values == 4)
                    best_basin_values[3] = basin_value;
                else
                    best_basin_values[num_best_basin_values++] = basin_value;
                // Sort the values to keep the best 3.
                for (int i = 0; i < num_best_basin_values - 1; i++) {
                    for (int j = 0; j < num_best_basin_values - i - 1; j++) {
                        if (best_basin_values[j]
                            < best_basin_values[j + 1]) {

                            // Switch order
                            int best_value_temp = best_basin_values[j];
                            best_basin_values[j] = best_basin_values[j + 1];
                            best_basin_values[j + 1] = best_value_temp;
                        }
                    }
                }
            }
        }
    }
    printf("Three top basin values: %d, %d, %d\n", best_basin_values[0], best_basin_values[1], best_basin_values[2]);
    int return_value = best_basin_values[0] * best_basin_values[1] * best_basin_values[2];
    printf("returning %d\n", return_value);
    return return_value;
}

int main(int argc, char* argv[]) {
    int buffer_size;
    int num_rows, num_cols;
    
    // Read the example 
    unsigned char* height_map = read_input("example.txt", &buffer_size);
    create_height_map(height_map, buffer_size, &num_rows, &num_cols);
    int risk_level = find_low_points(height_map, num_rows, num_cols);
    printf("risk level of example.txt: %d\n", risk_level);
    find_basins(height_map, num_rows, num_cols);
    free(height_map);

    // Read the puzzle 
    height_map = read_input("puzzle_input.txt", &buffer_size);
    create_height_map(height_map, buffer_size, &num_rows, &num_cols);
    risk_level = find_low_points(height_map, num_rows, num_cols);
    printf("risk level of puzzle_input.txt: %d\n", risk_level);
    find_basins(height_map, num_rows, num_cols);
    free(height_map);
    return 0;
}