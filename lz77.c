/*
Simple implementation of LZ77 compression algorithm
Copyright (C) 2020  Andreu Carminati
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lz77.h"
#include <stdio.h>

#define WINDOW 4096 // you can reduce this value, but do not increase
#define MAX_SEQ_SIZE 15 // same as above
#define MIN_SEQ_SIZE 3
#define COMPLETE_CODE 1

static void bitmap_set(unsigned char* bitmap, int bit_number, unsigned char bit) {
    (*bitmap) |= bit << bit_number;
}

static unsigned char bitmap_get(unsigned char* bitmap, int bit_number) {
    unsigned char byte = *bitmap;
    return (byte & 1 << bit_number) >> bit_number;
}

static int find_prefix(int* size, int w_start, int w_end, int start, int end, unsigned char* in_data) {
    int max_seq_size = 0, seq_index = w_start;

    while (w_start < w_end) {
        int window_start = w_start++;
        int sequence_start = start;
        int window_start_first = window_start;
        int new_size = 0;
        while (window_start <= w_end && sequence_start <= end) {
            if (in_data[window_start++] == in_data[sequence_start++]) {
                new_size++;
                if (new_size == MAX_SEQ_SIZE) {
                    break;
                }
            } else {
                break;
            }
        }
        if (new_size >= max_seq_size) {
            max_seq_size = new_size;
            seq_index = window_start_first;
        }
    }
    if (max_seq_size < MIN_SEQ_SIZE) {
        seq_index = 0;
        *size = 0;
    } else {
        *size = max_seq_size;
    }
    return seq_index;
}

int encode(int size_in, unsigned char* in_data, int size_buffer_out, unsigned char* buffer_out) {
    int pos = 0, w_start = 0, w_end = 0, write_pos = 0, code_count = 0;
    int i = 0, l = 0;
    unsigned char c;
    unsigned char* bitmap;

    bitmap = &buffer_out[write_pos++];
    *bitmap = 0;

    while (pos < size_in) {
        int size;
        int start = find_prefix(&size, w_start, w_end, pos, size_in - 1, in_data);
        if (size > 0) {
            i = pos - start - 1;
            l = size;
            pos += size + 1;
        } else {
            pos++;
        }
        c = in_data[pos - 1];
        if ((write_pos + 3) > size_buffer_out) {
            return 0; //
        }
        if (size > 0) {
            bitmap_set(bitmap, code_count % 8, COMPLETE_CODE);
        } else {
            bitmap_set(bitmap, code_count % 8, !COMPLETE_CODE);
        }
        buffer_out[write_pos++] = c;
        if (size > 0) {
            buffer_out[write_pos++] = i >> 4;
            buffer_out[write_pos++] = ((i << 4) & 0xF0) | l;
        }
        // verify/adjust bitmap
        if (++code_count == 8) {
            code_count = 0;
            bitmap = &buffer_out[write_pos++];
            *bitmap = 0;
        }
        // adjust window
        w_end = pos - 1;
        if ((w_end - w_start + 1) > WINDOW) {
            int amt = WINDOW - (w_end - start + 1);
            w_start += amt;
        }
    }
    return write_pos;
}

int decode(int size_in, unsigned char* in_data, int size_buffer_out, unsigned char* buffer_out) {
    int read_pos = 0, write_pos = 0, code_count = 0;
    int k;
    unsigned char* bitmap;

    bitmap = &in_data[read_pos++];

    while (read_pos < size_in) {
        int old_pos = write_pos;
        int i = 0, l = 0;
        unsigned char c = in_data[read_pos++];
        if (bitmap_get(bitmap, code_count) == COMPLETE_CODE) {
            i = in_data[read_pos++];
            l = in_data[read_pos++];
            i = (i << 4) | (l >> 4);
            l &= 0x0F;
        }
        if ((write_pos + l + 1) > size_buffer_out) {
            return 0; // out of memory
        }
        for (k = old_pos - i - 1; k < old_pos - i + l - 1; k++) {
            buffer_out[write_pos++] = buffer_out[k];
        }
        buffer_out[write_pos++] = c;
        // verify/adjust bitmap
        if (++code_count == 8) {
            code_count = 0;
            bitmap = &in_data[read_pos++];
        }
    }
    return write_pos;
}