/*
 * Copyright 1995, 2021, 2022, 2025 Jörg Bakker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>

#include "sis.h"


static void
print_warnings(void)
{
	if (algorithm < 4 && oversam > 1) {
		fprintf(stderr, "warning: oversampling is currently only available with algorithm 4\n");
		oversam = 1;
	}
	if (algorithm == 4 && SIStype != SIS_TEXT_MAP) {
		fprintf(stderr, "warning: random dot stereograms currently don't work properly with algorithm 4. \n");
	}
	if (algorithm == 4 && verbose == 1) {
		fprintf(stderr, "warning: verbose output is currently limited with algorithm 4\n");
	}
	// if (algorithm == 4 && SISwidth != Dwidth) {
		// fprintf(stderr, "warning: setting SIS width is currently not available with algorithm 4\n");
	// }
}


static void
print_message_header(void)
{
	printf("\n  DEPTH FILE:     %s (%ldx%ld)\n", DFileName, Dwidth, Dheight);
	printf("  SIS FILE:       %s (%ldx%ld)\n\n", SISFileName, SISwidth,
	       SISheight);
	if (SIStype == SIS_TEXT_MAP) {
		printf("  ... using texture-map: %s\n\n\n", TFileName);
	}
	printf("  ----    --- PROPAGATE ---    ---- OBSCURE ----     --- DEPTH ---\n");
	printf("  Line       inner    outer        forw    backw      min      max\n");
}


static void
print_statistics(void)
{
	printf("  %4ld    %8ld %8ld    %8ld %8ld   %6ld   %6ld\r", SISLineNumber + 1,
	       inner_propagate_c, outer_propagate_c,
	       forwards_obscure_c, backwards_obscure_c,
	       min_depth_in_row, max_depth_in_row);
	if (fflush(stdout)) {
		printf("stdout didn't flush\n");
		verbose = 0;
	}
}


static void
print_summary(void)
{
	printf("  Depth map values range: [%ld, %ld]\n", min_depth, max_depth);
	if (SIStype == SIS_TEXT_MAP) {
		printf("  Texture unique color count: %ld\n", Tcolcount);
	}
}


void
show_statistics(void)
{
	print_statistics();
}


int
main(int argc, char **argv)
{
	init_sis(argc, argv);
	print_warnings();
	if (verbose) {
		print_message_header();
	}
	render_sis();
	if (verbose) {
		puts("\n");
		print_summary();
	}
	finish_sis();
	return EXIT_SUCCESS;
}
