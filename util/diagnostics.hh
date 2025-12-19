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
 * 
 * This module provides functionality to display source code context
 * when reporting compilation errors, similar to how GCC displays errors.
 */

#ifndef _DIAGNOSTICS_HH
#define _DIAGNOSTICS_HH

#include <string>
#include <vector>

/*
 * Initialize the diagnostics module with the main source file.
 * This should be called once at the start of compilation with the
 * path to the main user program file.
 */
void diagnostics_init(const char *main_filename);

/*
 * Print source code context for an error location.
 * 
 * This function prints the source line and a caret underline pointing
 * to the error location, similar to GCC's error output:
 * 
 *     5 |     test_func := a + "hello";
 *       |                   ^~~~~~~~~~
 * 
 * Parameters:
 *   filename     - The source file where the error occurred
 *   first_line   - Starting line number of the error (1-based)
 *   first_column - Starting column of the error (1-based)
 *   last_line    - Ending line number of the error (1-based)
 *   last_column  - Ending column of the error (1-based)
 * 
 * If the filename doesn't match the main source file, or if the file
 * cannot be read, this function does nothing (graceful degradation).
 */
void print_source_context(const char *filename, 
                          int first_line, int first_column,
                          int last_line, int last_column);

#endif // _DIAGNOSTICS_HH
