/*
 *  matiec - a compiler for the programming languages defined in IEC 61131-3
 *
 *  Copyright (C) 2024  Autonomy Logic
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * This code is made available on the understanding that it will not be
 * used in safety-critical situations without a full and competent review.
 */

/*
 * Diagnostics module for GCC-style error reporting.
 */

#include "diagnostics.hh"
#include <stdio.h>
#include <string.h>
#include <climits>
#include <fstream>
#include <sstream>

/* The main source file path */
static std::string main_source_file;

/* Cached lines from the main source file */
static std::vector<std::string> source_lines;

/* Flag indicating if the source file has been loaded */
static bool source_loaded = false;

/*
 * Load the source file into memory (lazy loading).
 * Returns true if successful, false otherwise.
 */
static bool load_source_file(void) {
    if (source_loaded) {
        return !source_lines.empty();
    }
    
    source_loaded = true;
    
    if (main_source_file.empty()) {
        return false;
    }
    
    std::ifstream file(main_source_file);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        source_lines.push_back(line);
    }
    
    file.close();
    return !source_lines.empty();
}

/*
 * Check if the given filename matches the main source file.
 * Handles both exact matches and basename matches.
 */
static bool is_main_source_file(const char *filename) {
    if (filename == NULL || main_source_file.empty()) {
        return false;
    }
    
    /* Exact match */
    if (main_source_file == filename) {
        return true;
    }
    
    /* Check if filename is a suffix of main_source_file (handles relative vs absolute paths) */
    size_t filename_len = strlen(filename);
    if (filename_len > 0 && main_source_file.length() >= filename_len) {
        size_t pos = main_source_file.length() - filename_len;
        if (main_source_file.substr(pos) == filename) {
            /* Make sure we're matching at a path separator boundary */
            if (pos == 0 || main_source_file[pos - 1] == '/' || main_source_file[pos - 1] == '\\') {
                return true;
            }
        }
    }
    
    /* Check if main_source_file is a suffix of filename */
    if (main_source_file.length() > 0 && filename_len >= main_source_file.length()) {
        size_t pos = filename_len - main_source_file.length();
        if (std::string(filename).substr(pos) == main_source_file) {
            if (pos == 0 || filename[pos - 1] == '/' || filename[pos - 1] == '\\') {
                return true;
            }
        }
    }
    
    return false;
}

/*
 * Get a specific line from the source file (1-based line number).
 * Returns empty string if line doesn't exist.
 */
static std::string get_source_line(int line_number) {
    if (!load_source_file()) {
        return "";
    }
    
    /* Use size_t for safe comparison without narrowing cast */
    if (line_number <= 0) {
        return "";
    }
    size_t index = static_cast<size_t>(line_number - 1);
    if (index >= source_lines.size()) {
        return "";
    }
    
    return source_lines[index];
}

/*
 * Calculate the display width of a string, expanding tabs to 8 spaces.
 */
static int display_width(const std::string &str, int up_to_pos) {
    int width = 0;
    int pos = 0;
    for (size_t i = 0; i < str.length() && pos < up_to_pos; i++, pos++) {
        if (str[i] == '\t') {
            width = ((width / 8) + 1) * 8;
        } else {
            width++;
        }
    }
    return width;
}

/*
 * Print a string with tabs expanded to spaces.
 */
static void print_with_tabs_expanded(FILE *out, const std::string &str) {
    int col = 0;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '\t') {
            int next_tab = ((col / 8) + 1) * 8;
            while (col < next_tab) {
                fputc(' ', out);
                col++;
            }
        } else {
            fputc(str[i], out);
            col++;
        }
    }
}

void diagnostics_init(const char *main_filename) {
    if (main_filename != NULL) {
        main_source_file = main_filename;
    } else {
        main_source_file.clear();
    }
    source_lines.clear();
    source_loaded = false;
}

void print_source_context(const char *filename, 
                          int first_line, int first_column,
                          int last_line, int last_column) {
    /* Only print context for the main source file */
    if (!is_main_source_file(filename)) {
        return;
    }
    
    /* Get the source line */
    std::string line = get_source_line(first_line);
    if (line.empty()) {
        return;
    }
    
    /* Calculate line number width for alignment */
    char line_num_str[32];
    snprintf(line_num_str, sizeof(line_num_str), "%d", first_line);
    int line_num_width = strlen(line_num_str);
    
    /* Print the source line with line number */
    fprintf(stderr, " %*d | ", line_num_width, first_line);
    print_with_tabs_expanded(stderr, line);
    fprintf(stderr, "\n");
    
    /* Print the caret line */
    fprintf(stderr, " %*s | ", line_num_width, "");
    
    /* Calculate display positions considering tabs */
    int display_first_col = display_width(line, first_column - 1);
    
    /* Print spaces up to the caret position */
    for (int i = 0; i < display_first_col; i++) {
        fputc(' ', stderr);
    }
    
    /* Print the caret */
    fputc('^', stderr);
    
    /* Print tildes for the rest of the error span */
    int end_col;
    if (first_line == last_line) {
        /* Error is on a single line */
        end_col = last_column;
    } else {
        /* Error spans multiple lines - underline to end of first line */
        /* Use safe conversion to avoid potential overflow on extremely long lines */
        size_t line_len = line.length();
        end_col = (line_len > INT_MAX) ? INT_MAX : static_cast<int>(line_len);
    }
    
    /* Calculate how many tildes to print */
    int display_last_col = display_width(line, end_col);
    int tilde_count = display_last_col - display_first_col - 1;
    
    for (int i = 0; i < tilde_count; i++) {
        fputc('~', stderr);
    }
    
    fprintf(stderr, "\n");
}
