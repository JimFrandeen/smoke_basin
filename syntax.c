/** @file smoke_basin.c
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

 //#define PRINT_DEBUG // Turn this on for debug output

/**
* Give a file name, open the file of entries, read it into memory,
* store a \0 following the last character to signal end of file.
*
* @param    file_name   path to input file.
* @param    p_file_size_ret points to int where size of filee is returned.
* @retval   returns pointer to buffer that contains input string
*           The caller must free this buffer.
*/
unsigned char* read_input(const char* file_name, int* p_file_size_ret) {
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
* Scan the next line in the input buffer. 
* Stop when an unexpected character is found.
* Find the completion string for each incomplete line.
* Print the completion value 
*
* @param    p_input_buffer   pointer to the first character in the input buffer.
* @param    buffer_size Number of characters in the input buffer.
* @param    p_index_ret pointer to an int that contains the index 
*           of the next character in the input buffer.
*           This value is updated to point to the first character in the next line.
* @param    p_completion_score_ret pointer to uint64_t where completion score is returned.
* @retval   score for illegal character found:
*           ): 3 points.
*           ]: 57 points.
*           }: 1197 points.
*           >: 25137 points.
*/
uint64_t scan(const char* p_input_buffer, int buffer_size,
    int* p_index_ret, uint64_t* p_completion_score_ret) {
    #define STACK_SIZE 512
    char stack[STACK_SIZE];
    int stack_index = 0;
    uint64_t score, completion_score = 0;
    char pop_char;
    while(*p_index_ret < buffer_size) {
        if (stack_index >= STACK_SIZE) {
            printf("Stack overflow\n");
            exit(-1);
        }
        score = 0; // Assume syntax is correct
        // Get the next character from the input buffer
        char next_char = p_input_buffer[(*p_index_ret)++];
        switch (next_char) {
            case '(':
                // Push what we expect to pop
                stack[stack_index++] = ')'; break;
            case '[':
                stack[stack_index++] = ']'; break;
            case '{':
                stack[stack_index++] = '}'; break;
            case '<':
                stack[stack_index++] = '>'; break;
            case ')':
                score = 3;
                goto pop_stack;
            case ']':
                score = 57;
                goto pop_stack;
            case '}':
                score = 1197;
                goto pop_stack;
            case '>':
                score = 25137;
            pop_stack:
                if (stack_index == 0) {
                    printf("Unexpected end of line\n");
                    goto syntax_error;
                }
                // Pop the stack and see if this is what we were expecting.
                pop_char = stack[--stack_index];
                if (pop_char == next_char) continue;
#ifdef PRINT_DEBUG
                printf("Expected %c, but found %c instead, score %d\n", 
                    pop_char, next_char, score);
#endif
            syntax_error:
                while (*p_index_ret < buffer_size) {
                    if (p_input_buffer[(*p_index_ret)++] == '\n') break;
                }
                goto return_score;
            default: goto end_of_line;
        }
    }
end_of_line:
    // Fall through if we find end of buffer and no error.
    if (stack_index) {
        printf("Incomplete line -- adding");
        while (stack_index) {
            pop_char = stack[--stack_index];
            switch (pop_char) {
            case ')':
                completion_score = completion_score * 5 + 1;
                break;
            case ']':
                completion_score = completion_score * 5 + 2;
                break;
            case '}':
                completion_score = completion_score * 5 + 3;
                break;
            case '>':
                completion_score = completion_score * 5 + 4;
            }
            printf("%c", pop_char);
        }
        printf("completion_score=%lld\n", completion_score);
    }
return_score:
    *p_completion_score_ret = completion_score;
    return score;
}

/**
* Give a file name, open the file of entries, read it into memory,
* Sort the completion codes.
* Print the middle completion code.
*
* @param    file_name   path to input file.
*/
void score_input_file(const char* file_name) {
    printf("scoring input file %s\n", file_name);
    int buffer_size, buffer_index, completion_score_index;
    uint64_t score, completion_score;
#define COMPLETION_SCORE_ARRAY_SIZE 512
    uint64_t ar_completion_score[COMPLETION_SCORE_ARRAY_SIZE];

    // Read the input file into a guffer. 
    unsigned char* input_buffer = read_input(file_name, &buffer_size);
    buffer_index = completion_score_index = 0;
    score = completion_score = 0;
    while (buffer_index < buffer_size) {
        if (completion_score_index >= COMPLETION_SCORE_ARRAY_SIZE) {
            printf("Unexpected completion score array overflow\n");
            exit(-1);
        }
        score += scan(input_buffer, buffer_size, &buffer_index, &completion_score);
        if (completion_score) {
            ar_completion_score[completion_score_index++] = completion_score;
        }
    }
    
    // Sort the completion scores
    if ((completion_score_index & 1) == 0) {
        printf("Expecting an odd number of completion scores. We have %d\n", completion_score_index);
        exit(-1);
    }
    for (int i = 0; i < completion_score_index - 1; i++) {
        for (int j = 0; j < completion_score_index - i - 1; j++) {
            if (ar_completion_score[j]
                < ar_completion_score[j + 1]) {

                // Switch order
                uint64_t temp_score = ar_completion_score[j];
                ar_completion_score[j] = ar_completion_score[j + 1];
                ar_completion_score[j + 1] = temp_score;
            }
        }
    }

    printf("Middle completion score for %s=%lld\n", file_name, ar_completion_score[completion_score_index/2]);
    free(input_buffer);
}

int main(int argc, char* argv[]) {
    score_input_file("sample_syntax.txt");
    score_input_file("puzzle_syntax.txt");
    return 0;
}