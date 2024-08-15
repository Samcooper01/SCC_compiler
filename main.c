/*
 * Program Name: SCC Compiler
 * Description: Compiles SCC Programming Language Syntax to SCC ASM
 * Author: Samuel Cooper
 *
 * Compilation: run 'make'
 *
 * Notes:
 *      None.
 *
 * Style:
 *  https://github.com/Samcooper01/StyleGuide/tree/main
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "./include/errors.h"
#include "./include/scc.h"

enum COMPILER_STATES current_state;

FILE *scc_fd;
FILE *samco_fd;

int line_index = 1;
int asm_instruction_addr;

int loop_amount = 0;
int loop_start_index = 0;
int inside_an_infinite_loop = 0;

int start_of_if_statement = 0;

static void
open_scc_input_file()
{
    scc_fd = fopen(scc16_filename, "r");
    if(scc_fd == NULL)
    {
        fatal_error("Failed to open input file: %s\n", scc16_filename);
    }
}

static void
close_scc_input_file()
{
    if(scc_fd != NULL) fclose(scc_fd);
}

static void
remove_newline(char *str)
{
    char *newline = strchr(str, '\n');

    if(newline)
    {
        *newline = '\0';
    }
}

static void
parse_line_precode(char * line)
{
    if(strcmp(line, "\n") == 0) return;

    remove_newline(line);
    char *column1 = strtok(line, " ");

    if(strcmp(line, "PROG_MEMORY_START") == 0)
    {
        char *column2 = strtok(NULL, " ");
        PROGRAM_MEMORY_START = atoi(column2);
    }
    else if(strcmp(line, "PROG_MEMORY_END") == 0)
    {
        char *column2 = strtok(NULL, " ");
        PROGRAM_MEMORY_END = atoi(column2);
    }
        if(strcmp(line, "DATA_MEMORY_START") == 0)
    {
        char *column2 = strtok(NULL, " ");
        DATA_MEMORY_START = atoi(column2);
    }
    else if(strcmp(line, "DATA_MEMORY_END") == 0)
    {
        char *column2 = strtok(NULL, " ");
        DATA_MEMORY_END = atoi(column2);
    }
    else if(strcmp(line, "CODE_BEGIN") == 0)
    {
        //By default variables start at the middle of data memory
        VAR_MEMORY_START = (DATA_MEMORY_END - DATA_MEMORY_START) / 2
                            + DATA_MEMORY_START;
        VAR_MEMORY_INDEX = VAR_MEMORY_START;
        current_state = CODE;
    }

}

/**
 * @brief Saves a variable to the .temp file as well as writes asm to PUT
 *        the variable into memory at the addr stored in .temp file.
 *
 * @param line var keyword - strtok is set to this so next strtok will
 *                           return next string
 *
 */
static void
save_variable(char * line)
{
    FILE *var_list_fd = fopen(".temp", "a");
    char *var_name = strtok(NULL, " ");
    strtok(NULL, " ");
    char *var_value = strtok(NULL, " ");

    fprintf(var_list_fd, "%d %s %s\n", VAR_MEMORY_INDEX, var_name, var_value);
    fclose(var_list_fd);

    VAR_MEMORY_INDEX++;

    int var_value_int = atoi(var_value);

    int upper_digits_value = (var_value_int >> 8) & 0xFF;
    int lower_digits_value = (var_value_int & 0xFF);
    fprintf(samco_fd, "//var %s = %s\n", line, var_value);
    fprintf(samco_fd, "lshf DR 0x%02x\n", upper_digits_value);
    fprintf(samco_fd, "lshf DR 0x%02x\n", lower_digits_value);

    int upper_digits_addr = (VAR_MEMORY_INDEX >> 8) & 0xFF;
    int lower_digits_addr = (VAR_MEMORY_INDEX & 0xFF);
    fprintf(samco_fd, "lshf r7 0x%02x\n", upper_digits_addr);
    fprintf(samco_fd, "lshf r7 0x%02x\n", lower_digits_addr);
    fprintf(samco_fd, "PUT DR r7\n\n");

    asm_instruction_addr = asm_instruction_addr + 5;
}

/**
 * @brief Checks if ALL chars in param are ints
 *
 * @param str char * to check
 *
 */
int
is_integer_string(const char *str) {
    if (*str == '\0') {
        return 0;
    }
    if (*str == '-' || *str == '+') {
        str++;
    }
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return 0;
        }
        str++;
    }
    return 1;
}

/**
 * @brief Returns addr from .temp file
 *
 * @param operand name of variable to search for
 *
 */
static int
get_operand_addr(char * operand)
{
    char name_buffer[MAX_LINE_SIZE_CHAR];

    FILE *var_list_fd = fopen(".temp", "r");
    if(var_list_fd == NULL) fatal_error("Failed to open var_list_fd\n");

    while(fgets(name_buffer, sizeof(name_buffer), var_list_fd) != NULL)
    {
        char *name_addr = strtok(name_buffer, " ");
        char *name_name = strtok(NULL, " ");
        if(strcmp(name_name, operand) == 0)
        {
            if(var_list_fd != NULL) fclose(var_list_fd);
            return atoi(name_addr);
        }
    }
    if(var_list_fd != NULL) fclose(var_list_fd);
    fatal_error("Couldnt find name for operand on line: %d\n", line_index);
}

/**
 * @brief Returns value of variable
 *
 * @param operand name of variable to search for
 *
 */
static int
get_operand_value(char * operand)
{
    char name_buffer[MAX_LINE_SIZE_CHAR];

    FILE *var_list_fd = fopen(".temp", "r");
    if(var_list_fd == NULL) fatal_error("Failed to open var_list_fd\n");

    while(fgets(name_buffer, sizeof(name_buffer), var_list_fd) != NULL)
    {
        char *name_addr = strtok(name_buffer, " ");
        char *name_name = strtok(NULL, " ");
        char *name_value = strtok(NULL, " ");
        if(strcmp(name_name, operand) == 0)
        {
            if(var_list_fd != NULL) fclose(var_list_fd);
            return atoi(name_value);
        }
    }
    if(var_list_fd != NULL) fclose(var_list_fd);
    fatal_error("Couldnt find name for operand on line: %d\n", line_index);
}

/**
 * @brief Updates value of variable in .temp file
 *
 * @param name name to update
 * @param value value to replace with
 *
 */
static void
update_saved_var(char * name, int value)
{
    FILE *file;
    FILE *tempFile;
    char buffer[MAX_LINE_SIZE_CHAR];
    // Open the original file for reading
    file = fopen(".temp", "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // Create a temporary file for writing
    tempFile = fopen("tempfile.tmp", "w");
    if (tempFile == NULL) {
        perror("Error creating temporary file");
        fclose(file);
        return;
    }

    // Read the original file line by line
    while (fgets(buffer, sizeof(buffer), file)) {
       // printf("%s\n", buffer);
        char *buffer_copy = (char *)malloc(sizeof(buffer));
        strcpy(buffer_copy, buffer);
        char* line_addr = strtok(buffer_copy, " ");
        char* line_name = strtok(NULL, " ");

        if (strcmp(line_name, name) == 0)
        {
            char str[20];
            sprintf(str, "%d", value);

            int new_length = strlen(line_addr) + strlen(line_name) + strlen(str);
            char *new_line = (char *)malloc(new_length + 3);
            new_line[0] = '\0';
            strcat(new_line, line_addr);
            strcat(new_line, " ");
            strcat(new_line, line_name);
            strcat(new_line, " ");
            strcat(new_line, str);

            fprintf(tempFile, "%s\n", new_line);
        } else {
            // Write the existing line to the temporary file
            fprintf(tempFile, "%s", buffer);
        }
        free(buffer_copy);
    }

    // Close both files
    fclose(file);
    fclose(tempFile);
    // Replace the original file with the temporary file
    if (remove(".temp") != 0) fatal_error("Couldn't remove .temp file\n");
    if (rename("tempfile.tmp", ".temp") != 0)
    {
        fatal_error("Couldn't remove tempfile.tmp\n");
    }

}

/**
 * @brief Write ASM assignment operation with variable addr and value
 *
 */
static void
write_assignment_operation(char * operations_args[], char * source)
{
    int operand_values[2];
    int int_check = is_integer_string(operations_args[2]);
    if(int_check)
    {
        operand_values[0] = atoi(operations_args[2]);
        int source_addr = get_operand_addr(source);

        int upper_digits_value_o1 = (operand_values[0] >> 8) & 0xFF;
        int lower_digits_value_01 = (operand_values[0] & 0xFF);

        fprintf(samco_fd, "lshf DR 0x%02x\n", upper_digits_value_o1);
        fprintf(samco_fd, "lshf DR 0x%02x\n", lower_digits_value_01);

        int upper_digits_source = source_addr / 100;
        int lower_digits_source = source_addr % 100;

        fprintf(samco_fd, "lshf r6 0x%02d\n", upper_digits_source);
        fprintf(samco_fd, "lshf r6 0x%02d\n", lower_digits_source);

        fprintf(samco_fd, "put DR r6\n");
        asm_instruction_addr = asm_instruction_addr + 5;
        update_saved_var(operations_args[0], operand_values[0]);
    }
    else
    {
        operand_values[0] = get_operand_addr(operations_args[2]);
        int source_addr = get_operand_addr(source);

        int upper_digits_dest = operand_values[0] / 100;
        int lower_digits_dest = operand_values[0] % 100;

        fprintf(samco_fd, "lshf DR 0x%02d\n", upper_digits_dest);
        fprintf(samco_fd, "lshf DR 0x%02d\n", lower_digits_dest);
        fprintf(samco_fd, "get r6 DR\n");

        int upper_digits_source = source_addr / 100;
        int lower_digits_source = source_addr % 100;

        fprintf(samco_fd, "lshf r5 0x%02d\n", upper_digits_source);
        fprintf(samco_fd, "lshf r5 0x%02d\n", lower_digits_source);

        fprintf(samco_fd, "put r6 r5");
        asm_instruction_addr = asm_instruction_addr + 6;
    }

}

/**
 * @brief writes asm equivalent of operation which is one of + - / *
 *
 */
static void
perform_operation(char * line)
{
    char *operations_args[MAX_OPERATION_ARGS];
    operations_args[0] = line;

    char *arg1 = strtok(NULL, " ");
    operations_args[1] = arg1;
    char *arg2 = strtok(NULL, " ");
    operations_args[2] = arg2;
    char *arg3 = strtok(NULL, " ");
    operations_args[3] = arg3;
    char *arg4 = strtok(NULL, " ");
    operations_args[4] = arg4;

    if(strcmp(operations_args[1], "=") != 0)
    {
        fatal_error("Instruction on line: %d is not valid\n", line_index);
    }

    if(operations_args[3] != NULL || operations_args[4] != NULL)
    {
        fprintf(samco_fd, "\n//%s %s %s %s %s\n", operations_args[0],
                                                operations_args[1],
                                                operations_args[2],
                                                operations_args[3],
                                                operations_args[4]);
    }
    else
    {
        fprintf(samco_fd, "\n//%s %s %s\n", operations_args[0],
                                            operations_args[1],
                                            operations_args[2]);
    }
    //Get operands
    int operand_values[2];

    if(NULL == operations_args[2])
    {
        fatal_error("Instruction on line: %d is not valid\n", line_index);
    }
    else if (NULL == operations_args[3])
    {
        write_assignment_operation(operations_args, line);
        return;
    }

    if(NULL == operations_args[2] || NULL == operations_args[4]
        || NULL == operations_args[3])
    {
        fatal_error("Instruction on line: %d is not valid\n", line_index);
    }
    else
    {
        int int_check0 = is_integer_string(operations_args[2]);
        if(int_check0)
        {
            operand_values[0] = atoi(operations_args[2]);

            int upper_digits_operand0 = (operand_values[0] >> 8) & 0xFF;
            int lower_digits_operand0 = operand_values[0] & 0xFF;

            fprintf(samco_fd, "lshf r6 0x%02x\n", upper_digits_operand0);
            fprintf(samco_fd, "lshf r6 0x%02x\n", lower_digits_operand0);
            asm_instruction_addr = asm_instruction_addr + 2;
        }
        else
        {
            int operand0_addr = get_operand_addr(operations_args[2]);
            operand_values[0] = get_operand_value(operations_args[2]);

            int upper_digits_operand0_addr = operand0_addr / 100;
            int lower_digits_operand0_addr = operand0_addr % 100;

            fprintf(samco_fd, "lshf DR 0x%02d\n", upper_digits_operand0_addr);
            fprintf(samco_fd, "lshf DR 0x%02d\n", lower_digits_operand0_addr);
            fprintf(samco_fd, "GET r6 DR\n");
            asm_instruction_addr = asm_instruction_addr + 3;
        }

        int int_check1 = is_integer_string(operations_args[4]);
        if(int_check1)
        {
            operand_values[1] = atoi(operations_args[4]);

            int upper_digits_operand1 = (operand_values[1] >> 8) & 0xFF;
            int lower_digits_operand1 = operand_values[1] & 0xFF;

            fprintf(samco_fd, "lshf r5 0x%02x\n", upper_digits_operand1);
            fprintf(samco_fd, "lshf r5 0x%02x\n", lower_digits_operand1);
            asm_instruction_addr = asm_instruction_addr + 2;
        }
        else
        {
            int operand1_addr = get_operand_addr(operations_args[4]);
            operand_values[1] = get_operand_value(operations_args[4]);

            int upper_digits_operand1_addr = operand1_addr / 100;
            int lower_digits_operand1_addr = operand1_addr % 100;

            fprintf(samco_fd, "lshf DR 0x%02d\n", upper_digits_operand1_addr);
            fprintf(samco_fd, "lshf DR 0x%02d\n", lower_digits_operand1_addr);
            fprintf(samco_fd, "GET r5 DR\n");
            asm_instruction_addr = asm_instruction_addr + 3;
        }

        int result_addr = get_operand_addr(line);

        int upper_digits_result_addr = result_addr / 100;
        int lower_digits_result_addr = result_addr % 100;

        fprintf(samco_fd, "lshf r3 0x%02d\n", upper_digits_result_addr);
        fprintf(samco_fd, "lshf r3 0x%02d\n", lower_digits_result_addr);
        asm_instruction_addr = asm_instruction_addr + 2;


        // r4 = r5 <operation> r6
        if(strcmp("+", operations_args[3]) == 0)
        {
            fprintf(samco_fd, "add r6 r5\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "add r4, r6\n");
            fprintf(samco_fd, "PUT r4, r3\n");
            asm_instruction_addr = asm_instruction_addr + 5;
            update_saved_var(line, (operand_values[0] + operand_values[1]));
        }
        else if(strcmp("-", operations_args[3]) == 0)
        {
            fprintf(samco_fd, "sub r6 r5\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "add r4, r5\n");
            fprintf(samco_fd, "PUT r4, r3\n");
            asm_instruction_addr = asm_instruction_addr + 5;
            if(operand_values[1] > operand_values[0]) update_saved_var(line, 0);
            else update_saved_var(line, (operand_values[0] - operand_values[1]));
        }
        else if(strcmp("*", operations_args[3]) == 0)
        {
            fprintf(samco_fd, "mul r6 r5\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "add r4, r5\n");
            fprintf(samco_fd, "PUT r4, r3\n");
            asm_instruction_addr = asm_instruction_addr + 5;
            update_saved_var(line, (operand_values[0] * operand_values[1]));
        }
        else if(strcmp("/", operations_args[3]) == 0)
        {
            fprintf(samco_fd, "div r6 r5\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "lshf r4 0x00\n");
            fprintf(samco_fd, "add r4, r5\n");
            fprintf(samco_fd, "PUT r4, r3\n");
            asm_instruction_addr = asm_instruction_addr + 5;
            if(operand_values[1] == 0) update_saved_var(line, 0);
            else update_saved_var(line, (operand_values[0] / operand_values[1]));
        }
        else
        {
            fatal_error("Operation not recognized on line: %d\n", line);
        }
    }
}

/**
 * @brief Setup loop count and index
 *
 */
static void
entering_loop(char * line)
{
    loop_amount = atoi(strtok(NULL, " "));
    if(loop_amount == -1) inside_an_infinite_loop = 1;
    fprintf(samco_fd, "\n//Loop begins\n");
    fprintf(samco_fd, "lshf r1 0x%02x\n", (loop_amount >> 8) & 0xFF);
    fprintf(samco_fd, "lshf r1 0x%02x\n", loop_amount & 0xFF);
    fprintf(samco_fd, "lshf r2 0x00\n");
    fprintf(samco_fd, "lshf r2 0x00\n");
    asm_instruction_addr = asm_instruction_addr + 4;

    loop_start_index = asm_instruction_addr + 2;
    fprintf(samco_fd, "lshf r3 0x%02x\n", (loop_start_index >> 8) & 0xFF);
    fprintf(samco_fd, "lshf r3 0x%02x\n", loop_start_index & 0xFF);
    asm_instruction_addr = asm_instruction_addr + 2;
}

/**
 * @brief checks if loop should continue
 *
 */
static void
end_loop(char * line)
{
    fprintf(samco_fd, "\n//Loop end\n");
    fprintf(samco_fd, "lshf r4 0x00\n");
    fprintf(samco_fd, "lshf r4 0x01\n");
    fprintf(samco_fd, "add r2 r4\n");
    fprintf(samco_fd, "lshf r4 0x00\n");
    fprintf(samco_fd, "lshf r4 0x00\n");
    fprintf(samco_fd, "add r4 r2\n");
    fprintf(samco_fd, "sub r1 r4\n");
    if(inside_an_infinite_loop == 1) fprintf(samco_fd, "lshf DR 0x00");
    fprintf(samco_fd, "jz r3\n");
    asm_instruction_addr = asm_instruction_addr + 8;
}

/**
 * @brief Setup if statement with values to compare
 *        Also writes FAKE jz instruction to be replaced once > bracket is found
 *
 */
static void
entering_if_statement(char * line)
{

    char * var_name = strtok(NULL, " ");
    int var_addr = get_operand_addr(var_name);

    int upper_digits_var_addr = var_addr / 100;
    int lower_digits_var_addr = var_addr % 100;
    fprintf(samco_fd, "\n//If statement begins\n");
    fprintf(samco_fd, "lshf r1 0x%02d\n", upper_digits_var_addr);
    fprintf(samco_fd, "lshf r1 0x%02d\n", lower_digits_var_addr);
    fprintf(samco_fd, "get r2 r1\n");

    int compare_value = atoi(strtok(NULL, " "));

    fprintf(samco_fd, "lshf r1 0x%02x\n", (compare_value >> 8) & 0xFF);
    fprintf(samco_fd, "lshf r1 0x%02x\n", compare_value & 0xFF);

    fprintf(samco_fd, "sub r2 r1\n");

    fprintf(samco_fd, "IF JZ\n"); //this line gets edited to correct value for r3
    asm_instruction_addr = asm_instruction_addr + 7;
    start_of_if_statement = asm_instruction_addr - 1;
}

/**
 * @brief Replaces fake JZ "IZ JZ" with correct JZ statement and loads r3 addr
 *
 */
static void
replace_if_jz_with_correct_value()
{
    char buffer[MAX_LINE_SIZE_CHAR];
    fclose(samco_fd);
    FILE *file = fopen(samco_filename, "r");
    if(file == NULL) fatal_error("Failed to open samco_fd\n");
    FILE *tempFile = fopen("if_tempfile.tmp", "w");
    if (tempFile == NULL) fatal_error("Failed to make if_tempfile.tmp\n");

    while (fgets(buffer, sizeof(buffer), file))
    {
        if (strstr(buffer, "sub r2 r1") != NULL)
        {
            int upper_digits_asm_instruct = (asm_instruction_addr+3) / 100;
            int lower_digits_asm_instruct = (asm_instruction_addr+3) % 100;

            fprintf(tempFile, "lshf r3 0x%02x\n", upper_digits_asm_instruct);
            fprintf(tempFile, "lshf r3 0x%02x\n", lower_digits_asm_instruct);
            fprintf(tempFile, "sub r2 r1\n");
            fprintf(tempFile, "JZ r3\n");

            asm_instruction_addr = asm_instruction_addr + 2;
        }
        else if (strstr(buffer, "IF JZ") != NULL)
        {
            //Nothing we are removing this line
        }
        else
        {
            fputs(buffer, tempFile);
        }
    }

    // Close both files
    fclose(tempFile);
    fclose(file);
    if (remove(samco_filename) != 0) fatal_error("Failed to remove file\n");
    if (rename("if_tempfile.tmp", samco_filename) != 0)
    {
        fatal_error("Failed to remove file\n");
    }

    samco_fd = fopen(samco_filename, "a");
    if(samco_fd == NULL) fatal_error("Failed to open file %s\n", samco_filename);
}

static void
end_of_if_statement(char *line)
{
    replace_if_jz_with_correct_value();
}

/**
 * @brief Main parser while in code state
 *
 */
static void
parse_line_code(char * line)
{
    if(strcmp(line, "\n") == 0) return;
    remove_newline(line);
    if(strcmp(line, "CODE_END") == 0)
    {
        current_state = CLEANUP;
        return;
    }

    char *column_0 = strtok(line, " ");

    if(strcmp(column_0, "var") == 0)
    {
        save_variable(line);
    }
    else if(strcmp(column_0, "//") == 0)
    {
        //This is a comment do nothing
    }
    else if(strcmp(column_0, "loop") == 0)
    {
        entering_loop(line);
    }
    else if(strcmp(column_0, "{") == 0)
    {
        //nothing
    }
    else if(strcmp(column_0, "}") == 0)
    {
        end_loop(line);
    }
    else if(strcmp(column_0, "if") == 0)
    {
        entering_if_statement(line);
    }
    else if(strcmp(column_0, "<") == 0)
    {
        //nothing
    }
    else if(strcmp(column_0, ">") == 0)
    {
        end_of_if_statement(line);
    }
    else
    {
        //If none of the keywords then a variable name
        perform_operation(line);
    }

}

static void
open_samco_output_file()
{
    samco_fd = fopen(samco_filename, "w");
    if(samco_fd == NULL) fatal_error("SamCO output file failed to open\n");
}

static void
close_samco_output_file()
{
    if(samco_fd != NULL) fclose(samco_fd);
}

static void
clear_saved_vars()
{
    FILE *saved_vars_fd = fopen(".temp", "w");
    if(saved_vars_fd == NULL) fatal_error("TEMP file failed to open.");
    fclose(saved_vars_fd);
}

/**
 * @brief Main state machine
 *
 */
static void
compile()
{

    if (current_state == INIT)
    {
        open_samco_output_file();
        clear_saved_vars();
        open_scc_input_file();
        current_state = PRECODE;
    }
    else return;

    char line_buffer[MAX_LINE_SIZE_CHAR];
    asm_instruction_addr = PROGRAM_MEMORY_START;

    while(fgets(line_buffer, sizeof(line_buffer), scc_fd) != NULL)
    {
        if(current_state == PRECODE) parse_line_precode(line_buffer);
        else if(current_state == CODE) parse_line_code(line_buffer);
        line_index++;
    }

    if(current_state = CLEANUP)
    {
        close_scc_input_file();
        close_samco_output_file();
        return;
    }
    fatal_error("CODE_END keyword not found\n");
}

static void
usage()
{
    printf("./SCC <Optional_input_name> <Optional_output_name>\n");
    printf("\n");
    printf("<Optional_input_name>: Specifies input filepath\n");
    printf("<Optional_output_name>: Specifies output filepath\n");
}

int
main(int argc, char **argv)
{
    if(argc == 2)
    {
        if(strcmp(argv[1], "usage") == 0)
        {
            usage();
            exit(0);
        }
        else fatal_error("Arg1 not understood. './SCC usage' for usage\n");
    }
    else if(argc == 3)
    {
        scc16_filename = argv[1];
        samco_filename =  argv[2];
    }
    else if(argc == 1)
    {
        //defaults
        scc16_filename = "main.scc";
        samco_filename = "main.samco";
    }
    else fatal_error("./SCC usage\n");
    current_state = INIT;

    compile();

    exit(0);
}

/* End of file: main.c */
