/*
 * Copyright 1995 Joerg Bakker
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The author makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "defines.h"
#include "sis.h"

static void print_usage (void)
{
    fprintf(stderr,
            "\nUsage: sis [DEPTH FILE] [SIS FILE] [OPTIONS]\n\n"
            "(...;...) = (range; default value)\n"
            "#  = integer value.\n"
            "OPTIONS:\n"
            "   -a #     : algorithm number (1-3; 2)\n"
            "   -c #     : output is random color with # colors\n"
            "   -d #     : density of black dots in per-cent, only with -g2 (1-99; 50)\n"
            "   -e #     : eye distance in dots (>0; 160)\n"
            "   -e #m|i# : eye distance in tenths of (cm | inch) with resolution in dpi\n"
            "              example: -e20i300 means 2inch at 300dpi\n"
            "   -f #     : distance of far plane in per-cent of distance to the monitor (>0; 100)\n"
            "   -g #     : output is random grey with # scales\n"
            "   -h       : print this help message\n"
            "   -i       : invert depth map\n"
            "   -m       : add triangles at top of SIS (makes SIS watching easier)\n"
            "   -n #     : distance of near plane in per-cent of distance of the far plane (>0; 33)\n"
            "   -o #     : position where algorithm starts (0-SISwidth; SISwidth/2)\n"
            "   -q       : don\'t print any message\n"
            "   -s #     : seed value for random dots (>0; 1)\n"
            "   -t file  : texture filename (texture.tif)\n"
            "   -v       : print some information\n"
            "   -x #     : width of SIS in dots (>0; width of depth-map)\n"
            "   -x #m|i# : width of SIS in tenths of (cm | inch) with resolution in dpi\n"
            "              example: -e45i300 means 4.5inch at 300dpi\n"
            "   -y #     : height of SIS in dots (>0; height of depth-map)\n"
            "   -y #m|i# : height of SIS in tenths of (cm | inch) with resolution in dpi\n"
            "              example: -e32i300 means 3.2inch at 300dpi\n"
            "   -z       : output is compressed if possible\n"
            "\n");
    exit(1);
}

ind_t metric2pixel (int metric_val, int resolution)
{
    switch (metric) {
        case 'm':
        case 'c':
            return metric_val/25.4 * resolution;
        case 'i':
            return metric_val/10.0 * resolution;
        default:
            return 0;
            print_usage ();
    }
}

void get_options (int argc, char **argv)
{
    int opt_ind = 1;
    int str_ind, i;

    if (opt_ind == argc)
        print_usage ();
    if ((opt_ind < argc) && (argv[opt_ind][0] != '-')) {
        DFileName = argv[opt_ind];
        opt_ind++;
    }

    if ((opt_ind < argc) && (argv[opt_ind][0] != '-')) {
        SISFileName = argv[opt_ind];
        opt_ind++;
    }

    while (opt_ind < argc) {
        if (argv[opt_ind][0] != '-') print_usage();
        switch (argv[opt_ind][1]) {
            case 'a':
                if (argv[opt_ind][2] != 0)
                    algorithm = atoi (argv[opt_ind]+2);
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        algorithm = atoi (argv[opt_ind]);
                    else print_usage();
                }
                if ((algorithm < 1) || (algorithm > 3)) print_usage ();
                break;
            case 'c':
                SIStype = SIS_RANDOM_COLOR;
                if (argv[opt_ind][2] != 0)
                    rand_col_num = atoi (argv[opt_ind]+2);
                else {
                    if ((opt_ind+1 < argc) && (argv[opt_ind+1][0] != '-')) {
                        opt_ind++;
                        rand_col_num = atoi (argv[opt_ind]);
                    }
                }
                if ((rand_col_num < 1) || (rand_col_num > SIS_MAX_COLORS+1))
                    print_usage ();
                break;
            case 'd':
                SIStype = SIS_RANDOM_GREY;
                rand_grey_num = 2;
                if (argv[opt_ind][2] != 0)
                    density = (double) atoi(argv[opt_ind]+2)/100.0;
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        density = (double) atoi(argv[opt_ind])/100.0;
                    else print_usage ();
                }
                if ((density <= 0.0) || (density >= 100.0)) print_usage ();
                break;
            case 'e':
                if (argv[opt_ind][2] != 0)
                    str_ind = 2;
                else {
                    opt_ind++; str_ind = 0;
                }
                if ((opt_ind >= argc) || !isdigit(argv[opt_ind][str_ind]))
                    print_usage ();
                for (i=str_ind; isdigit (argv[opt_ind][i]); i++);
                if (!argv[opt_ind][i])
                    E = atoi (argv[opt_ind]+str_ind);
                else {
                    if (!isdigit(argv[opt_ind][i+1])) print_usage ();
                    metric = argv[opt_ind][i];
                    argv[opt_ind][i] = 0;
                    resolution = atoi (argv[opt_ind]+i+1);
                    E = metric2pixel (atoi (argv[opt_ind]+str_ind),
                            resolution);
                }
                if (E <= 0) print_usage ();
                break;
            case 'f':
                if (argv[opt_ind][2] != 0)
                    t = (double) atoi (argv[opt_ind]+2)/100.0;
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        t = (double) atoi (argv[opt_ind])/100.0;
                    else print_usage ();
                }
                break;
            case 'g':
                SIStype = SIS_RANDOM_GREY;
                if (argv[opt_ind][2] != 0)
                    rand_grey_num = atoi (argv[opt_ind]+2);
                else {
                    if ((opt_ind+1 < argc) && (argv[opt_ind+1][0] != '-')) {
                        opt_ind++;
                        rand_grey_num = atoi (argv[opt_ind]);
                    }
                }
                if ((rand_grey_num < 1) || (rand_grey_num > SIS_MAX_COLORS+1))
                    print_usage ();
                break;
            case 'h':
            case 'i':
            case 'm':
            case 'q':
            case 'v':
            case 'z':
                for (str_ind = 1; argv[opt_ind][str_ind]; str_ind++)
                    switch (argv[opt_ind][str_ind]) {
                        case 'h':
                            print_usage ();
                            break;
                        case 'i':
                            invert = !invert;
                            break;
                        case 'm':
                            mark = 1;
                            break;
                        case 'q':
                            verbose = 0;
                            break;
                        case 'v':
                            verbose = 1;
                            break;
                        case 'z':
                            SIScompress = 1;
                            break;
                        default:
                            print_usage ();
                    }
                break;
            case 'n':
                if (argv[opt_ind][2] != 0)
                    u = 1.0 - (double) atoi(argv[opt_ind]+2)/100.0;
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        u = 1.0 - (double) atoi(argv[opt_ind])/100.0;
                    else print_usage();
                }
                if ((u <= 0.0) || (u >= 1.0)) print_usage ();
                break;
            case 'o':
                if (argv[opt_ind][2] != 0)
                    origin = atoi(argv[opt_ind]+2);
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        origin = atoi(argv[opt_ind]);
                    else print_usage();
                }
                /*
TODO: cannot check error-condition right here !
*/
                break;
            case 's':
                if (argv[opt_ind][2] != 0)
                    srand (atoi (argv[opt_ind]+2));
                else {
                    opt_ind++;
                    if ((opt_ind < argc) && (argv[opt_ind][0] != '-'))
                        srand (atoi (argv[opt_ind]));
                    else print_usage();
                }
                /*
                   let srand () do error checking.
                   */
                break;
            case 't':
                SIStype = SIS_TEXT_MAP;
                if (argv[opt_ind][2] != 0)
                    TFileName = argv[opt_ind]+2;
                else {
                    if ((opt_ind+1 < argc) && (argv[opt_ind+1][0] != '-')) {
                        opt_ind++;
                        TFileName = argv[opt_ind];
                    }
                }
                break;
            case 'x':
                if (argv[opt_ind][2] != 0)
                    str_ind = 2;
                else {
                    opt_ind++; str_ind = 0;
                }
                if ((opt_ind >= argc) || !isdigit(argv[opt_ind][str_ind]))
                    print_usage ();
                for (i=str_ind; isdigit (argv[opt_ind][i]); i++);
                if (!argv[opt_ind][i])
                    SISwidth = atoi (argv[opt_ind]+str_ind);
                else {
                    if (!isdigit(argv[opt_ind][i+1])) print_usage ();
                    metric = argv[opt_ind][i];
                    argv[opt_ind][i] = 0;
                    resolution = atoi (argv[opt_ind]+i+1);
                    SISwidth = metric2pixel (atoi (argv[opt_ind]+str_ind),
                            resolution);
                }
                if (SISwidth < 1) print_usage ();
                break;
            case 'y':
                if (argv[opt_ind][2] != 0)
                    str_ind = 2;
                else {
                    opt_ind++; str_ind = 0;
                }
                if ((opt_ind >= argc) || !isdigit(argv[opt_ind][str_ind]))
                    print_usage ();
                for (i=str_ind; isdigit (argv[opt_ind][i]); i++);
                if (!argv[opt_ind][i])
                    SISheight = atoi (argv[opt_ind]+str_ind);
                else {
                    if (!isdigit(argv[opt_ind][i+1])) print_usage ();
                    metric = argv[opt_ind][i];
                    argv[opt_ind][i] = 0;
                    resolution = atoi (argv[opt_ind]+i+1);
                    SISheight = metric2pixel (atoi (argv[opt_ind]+str_ind),
                            resolution);
                }
                if (SISheight < 1) print_usage ();
                break;
            default:
                print_usage();
        }
        opt_ind++;
    }
}

