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
unsigned char* read_input(char* file_name, int* p_file_size_ret) {
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

int scan(const char* p_input_buffer, int buffer_size, int* p_index_ret) {
    char stack[512]; 
    int stack_index = 0;
    int score = 0;
    char pop_char;
    while(*p_index_ret < buffer_size) {
        char next_char = p_input_buffer[*p_index_ret++];
        switch (next_char) {
            case '(':
            case '[':
            case '{':
            case '<':
                stack[stack_index++] = next_char; break;
            case ')':
                pop_char = stack[stack_index--];
                if (pop_char == '(' ) continue;
                score = 3;
                goto scan_to_end_line;
            case ']':
                pop_char = stack[stack_index--];
                if (pop_char == '[' ) continue;
                score = 57;
                goto scan_to_end_line;
            case '}':
                pop_char = stack[stack_index--];
                if (pop_char == '{' ) continue;
                score = 1197;
                goto scan_to_end_line;
            case '>'
                pop_char = stack[stack_index--];
                if (pop_char == '<') continue;
                score = 25137;
                goto scan_to_end_line;
            default: return 0;
        }
    }
    return 0;
scan_to_end_line:
    while (*p_index_ret < buffer_size) {
        if (p_input_buffer[*p_index_ret++] == '\n') break;
    }
    return score;
}

int main(int argc, char* argv[]) {
    int buffer_size;
    int num_rows, num_cols;

    // Read the example 
    int buffer_size;        
    unsigned char* input_buffer = read_input("example.txt", &buffer_size);
    int buffer_index = 0;
    while (buffer_index < buffer_size) {
        int score = scan(input_buffer, buffer_size , &buffer_index);

    }

   
    return 0;
}