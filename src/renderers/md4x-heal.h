/*
 * MD4X: Markdown parser for C
 * (http://github.com/pi0/md4x)
 *
 * Copyright (c) 2026 Pooya Parsa <pooya@pi0.io>
 *
 * Based on logic from https://github.com/vercel/streamdown/tree/main/packages/remend
 * Written by Hayden Bleasel <https://github.com/haydenbleasel>
 * Copyright (c) 2023 Vercel Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MD4X_HEAL_H
#define MD4X_HEAL_H

#ifdef __cplusplus
    extern "C" {
#endif


/* Heal (fix/complete) incomplete streaming Markdown text.
 *
 * This is a pre-parser text transform that closes unclosed formatting markers,
 * completes incomplete links/images, closes open code blocks, and fixes other
 * partial Markdown so it renders correctly mid-stream.
 *
 * Params input and input_size specify the Markdown input.
 * Callback process_output() gets called with chunks of healed output.
 * Param userdata is just propagated back to process_output() callback.
 *
 * Returns 0 on success, -1 on error.
 */
int md_heal(const char* input, unsigned input_size,
            void (*process_output)(const char*, unsigned, void*),
            void* userdata);


#ifdef __cplusplus
    }  /* extern "C" { */
#endif

#endif  /* MD4X_HEAL_H */
