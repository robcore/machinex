/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*============================================================================

                      EDIT HISTORY FOR FILE

 This section contains comments describing changes made to this file.
 Notice that changes are listed in reverse chronological order.

 $Header: $ $DateTime: $ $Author: $

 when      who    what, where, why
 --------  -----  ----------------------------------------------------------

============================================================================*/

/* Chromatix common DMSS header file version number */
0x0204,
0, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
/* Color Correction */
{
   1.8529f,    -0.5097f,    -0.3432f,
   -0.1681f,    1.1762f,    -0.0081f,
   -0.1027f,    -0.5692f,    1.6719f,
   0,    0,    0,
   0
},
/* Low-Light Color Correction */
{
   1.5521f,    -0.2687f,    -0.2834f,
   -0.0726f,    1.0073f,    0.0653f,
   -0.0803f,    -0.3546f,    1.4349f,
   0,    0,    0,
   0
},
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
/* Color Correction */
{
   1.8529f,    -0.5097f,    -0.3432f,
   -0.1681f,    1.1762f,    -0.0081f,
   -0.1027f,    -0.5692f,    1.6719f,
   0,    0,    0,
   0
},
/* Yhi-Ylo Color Correction */
{
   1.5521f,    -0.2687f,    -0.2834f,
   -0.0726f,    1.0073f,    0.0653f,
   -0.0803f,    -0.3546f,    1.4349f,
   0,    0,    0,
   0
},
0, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
/* TL84 Color Conversion */
{
   {
      /* a_m, a_p */
      0.6943f, 0.5852f,
      /* b_m, b_p */
      -0.2208f, -0.1142f,
      /* c_m, c_p */
      0.5871f, 0.6407f,
      /* d_m, d_p */
      -0.0903f, -0.0512f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* A Color Conversion */
{
   {
      /* a_m, a_p */
      0.8510f, 0.6560f,
      /* b_m, b_p */
      -0.2111f, -0.1181f,
      /* c_m, c_p */
      0.6190f, 0.6820f,
      /* d_m, d_p */
      -0.2174f, -0.3957f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* D65 Color Conversion */
{
   {
      /* a_m, a_p */
      0.6163f, 0.5997f,
      /* b_m, b_p */
      -0.3063f, -0.2062f,
      /* c_m, c_p */
      0.7617f, 0.6459f,
      /* d_m, d_p */
      0.0025f, 0.4086f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* Yhi-Ylo Color Conversion */
{
   {
      /* a_m, a_p */
      0.6943f, 0.5852f,
      /* b_m, b_p */
      -0.2208f, -0.1142f,
      /* c_m, c_p */
      0.5871f, 0.6407f,
      /* d_m, d_p */
      -0.0903f, -0.0512f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* Monochrome Color Conversion */
{
   {
      /* a_m, a_p */
      0.0000f, 0.0000f,
      /* b_m, b_p */
      0.0000f, 0.0000f,
      /* c_m, c_p */
      0.0000f, 0.0000f,
      /* d_m, d_p */
      0.0000f, 0.0000f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* Sepia Color Conversion */
{
   {
      /* a_m, a_p */
      0.0000f, 0.0000f,
      /* b_m, b_p */
      0.0000f, 0.0000f,
      /* c_m, c_p */
      0.0000f, 0.0000f,
      /* d_m, d_p */
      0.0000f, 0.0000f,
      /* k_cb, k_cr */
      110, 140
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* Negative Color Conversion */
{
   {
      /* a_m, a_p */
      -0.5000f, -0.5000f,
      /* b_m, b_p */
      -0.3380f, -0.3380f,
      /* c_m, c_p */
      -0.5000f, -0.5000f,
      /* d_m, d_p */
      -0.1620f, -0.1620f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      -0.2990f, -0.5870f, -0.1140f,
      /* k */
      255
   }
},
/* Aqua Color Conversion */
{
   {
      /* a_m, a_p */
      0.0000f, 0.0000f,
      /* b_m, b_p */
      0.0000f, 0.0000f,
      /* c_m, c_p */
      0.0000f, 0.0000f,
      /* d_m, d_p */
      0.0000f, 0.0000f,
      /* k_cb, k_cr */
      160, 80
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
1.100000f, /* Saturated Color Conversion Factor */
/* Sunset Color Conversion */
{
   {
      /* a_m, a_p */
      0.8510f, 0.6560f,
      /* b_m, b_p */
      -0.2111f, -0.1181f,
      /* c_m, c_p */
      0.6190f, 0.6820f,
      /* d_m, d_p */
      -0.2174f, -0.3957f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* TL84 Skintone Color Conversion */
{
   {
      /* a_m, a_p */
      0.6943f, 0.5852f,
      /* b_m, b_p */
      -0.2208f, -0.1142f,
      /* c_m, c_p */
      0.5871f, 0.6407f,
      /* d_m, d_p */
      -0.0903f, -0.0512f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* D65 Skintone Color Conversion */
{
   {
      /* a_m, a_p */
      0.6163f, 0.5997f,
      /* b_m, b_p */
      -0.3063f, -0.2062f,
      /* c_m, c_p */
      0.7617f, 0.6459f,
      /* d_m, d_p */
      0.0025f, 0.4086f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* A Skintone Color Conversion */
{
   {
      /* a_m, a_p */
      0.8510f, 0.6560f,
      /* b_m, b_p */
      -0.2111f, -0.1181f,
      /* c_m, c_p */
      0.6190f, 0.6820f,
      /* d_m, d_p */
      -0.2174f, -0.3957f,
      /* k_cb, k_cr */
      128, 128
   },
   {
      /* v0, v1, v2 */
      0.2990f, 0.5870f, 0.1140f,
      /* k */
      0
   }
},
/* TL84 Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* A Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* D65 Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* Lowlight Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* Monochrome Color Conversion */
{
   0.2990f,    0.5870f,    0.1140f,
   0.0000f,    0.0000f,    0.0000f,
   0.0000f,    0.0000f,    0.0000f,
   0,    128,    128
},
/* Sepia Color Conversion */
{
   0.2990f,    0.5870f,    0.1140f,
   0.0000f,    0.0000f,    0.0000f,
   0.0000f,    0.0000f,    0.0000f,
   0,    110,    140
},
/* Negative Color Conversion */
{
   -0.2990f,    -0.5870f,    -0.1140f,
   0.1687f,    0.3313f,    -0.5000f,
   -0.5000f,    0.4187f,    0.0813f,
   255,    128,    128
},
/* Aqua Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   0.0000f,    0.0000f,    0.0000f,
   0.0000f,    0.0000f,    0.0000f,
   0,    160,    80
},
/* Sunset Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* TL84 Skintone Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* D65 Skintone Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* A Skintone Color Conversion */
/* Noise Weight: 0.000000 */
/* Saturation: Red - 1.000000, Green - 1.000000, Blue - 1.000000 */
{
   0.2990f,    0.5870f,    0.1140f,
   -0.1687f,    -0.3313f,    0.5000f,
   0.5000f,    -0.4187f,    -0.0813f,
   0,    128,    128
},
/* TL84 Whitebalance - RGB */
{1.377920f, 1.000000f, 2.247070f},
/* D50 Whitebalance - RGB */
{1.000000f, 1.000000f, 1.000000f},
/* A Whitebalance - RGB */
{1.193710f, 1.000000f, 2.743390f},
/* D65 Whitebalance - RGB */
{1.843960f, 1.000000f, 1.458900f},
/* Strobe Flash Whitebalance - RGB */
{1.000000f, 1.000000f, 1.000000f},
/* LED Flash Whitebalance - RGB */
{1.000000f, 1.000000f, 1.000000f},
/* Channel Balance Gains */
{1.000000f, 1.000000f, 1.000000f, 1.000000f},
{
   /* D65 AWB Reference Point w/HW */
   {
      0.509000f,	/* R/G ratio */
      0.692000f,	/* B/G ratio */
      0.985000f,	/* Red Gain Adjust */
      1.011000f	/* Blue Gain Adjust */
   },
   /* Shade AWB Reference Point w/HW */
   {
      0.487000f,	/* R/G ratio */
      0.741000f,	/* B/G ratio */
      0.985000f,	/* Red Gain Adjust */
      1.011000f	/* Blue Gain Adjust */
   },
   /* A AWB Reference Point w/HW */
   {
      0.781000f,	/* R/G ratio */
      0.362000f,	/* B/G ratio */
      0.975000f,	/* Red Gain Adjust */
      1.020000f	/* Blue Gain Adjust */
   },
   /* TL84 AWB Reference Point w/HW */
   {
      0.717000f,	/* R/G ratio */
      0.435000f,	/* B/G ratio */
      1.000000f,	/* Red Gain Adjust */
      1.000000f	/* Blue Gain Adjust */
   },
   /* CoolWhite AWB Reference Point w/HW */
   {
      0.629000f,	/* R/G ratio */
      0.425000f,	/* B/G ratio */
      1.000000f,	/* Red Gain Adjust */
      1.000000f	/* Blue Gain Adjust */
   },
   /* Horizon AWB Reference Point w/HW */
   {
      0.887000f,	/* R/G ratio */
      0.305000f,	/* B/G ratio */
      0.976000f,	/* Red Gain Adjust */
      1.020000f	/* Blue Gain Adjust */
   },
   /* D50 AWB Reference Point w/HW */
   {
      0.567000f,	/* R/G ratio */
      0.562000f,	/* B/G ratio */
      0.985000f,	/* Red Gain Adjust */
      1.011000f	/* Blue Gain Adjust */
   },
   /* Cust. Fluor. AWB Reference Point w/HW */
   {
      0.717000f,	/* R/G ratio */
      0.435000f,	/* B/G ratio */
      1.000000f,	/* Red Gain Adjust */
      1.000000f	/* Blue Gain Adjust */
   },
   /* Noon AWB Reference Point w/HW */
   {
      0.567000f,	/* R/G ratio */
      0.562000f,	/* B/G ratio */
      0.985000f,	/* Red Gain Adjust */
      1.011000f	/* Blue Gain Adjust */
   },
},
{
   {
      /* Y-Min */
      5,
      /* Y-Max */
      228,
      /* Slope of neutral region and line number */
      8,
      -16,
      16,
      -16,
      /* Cb intercept of neutral region and line number */
      66,
      135,
      -76,
      257
   },
   {
      /* Y-Min */
      5,
      /* Y-Max */
      228,
      /* Slope of neutral region and line number */
      8,
      -16,
      16,
      -16,
      /* Cb intercept of neutral region and line number */
      66,
      135,
      -76,
      257
   },
   {
      /* Y-Min */
      5,
      /* Y-Max */
      228,
      /* Slope of neutral region and line number */
      8,
      -16,
      16,
      -16,
      /* Cb intercept of neutral region and line number */
      66,
      135,
      -76,
      257
   },
},
240, /* Indoor Index */
140, /* Outdoor Index */
1.100000f, /* Snow Blue Gain Adj Ratio */
0.900000f, /* Beach Blue Gain Adj Ratio */
8, /* Outlier Distance */
0, /* Green Offset RG */
0, /* Green Offset BG */
3, /* Num Frames to skip after changing VFE */
/* More AWB Parameters */
60, /* Compact Cluster R2 */
101, /* Compact Cluster To Ref Point R2 */
75, /* A Cluster Threshold */
75, /* F Cluster Threshold */
60, /* Day Cluster Threshold */
12, /* Outdoor Green Threshold */
8, /* Outdoor Green Threshold Bright F */
15, /* Outdoor Green Threshold Dark F */
12, /* Day Cluster Threshold For F */
1, /* Whitebalance Allow FLine */
15, /* Outdoor Valid Sample Count Threshold */
25, /* Outdoor Green Upper Threshold */
1000, /* R2 Threshold */
8, /* Outdoor Green Threshold Bright A */
15, /* Outdoor Green Threshold Dark A */
12, /* Day Cluster Threshold For A */
1.100000f, /* CC Global Gain */
/* AWB Min Gains - RGB */
{0.250000f, 0.250000f, 0.250000f},
/* AWB Max Gains - RGB */
{3.900000f, 3.900000f, 3.900000f},
{ 1.000000f, 1.000000f }, /* AWB Sample Influence, Outdoor/Indoor */
{
   {1, 80, 20}, /* AWB Weight Vector D65 */
   {5, 40, 20}, /* AWB Weight Vector D75 */
   {5, 2, 1}, /* AWB Weight Vector A */
   {100, 1, 20}, /* AWB Weight Vector Warm F */
   {90, 1, 15}, /* AWB Weight Vector Cool F */
   {3, 1, 1}, /* AWB Weight Vector Horizon */
   {1, 100, 5}, /* AWB Weight Vector D50 */
   {5, 1, 3}, /* AWB Weight Vector Cust F */
   {1, 100, 5}, /* AWB Weight Vector Daylight Noon */
   {1, 60, 20}, /* AWB Weight Vector Daylight Hybrid */
},
70, /* AWB White World Y Min Ratio */
1, /* AWB Aggressiveness */
0, /* AWB Self-Calibrate */
1.150000f, /* AWB Self-Calibrate Adjust Ratio High */
0.900000f, /* AWB Self-Calibrate Adjust Ratio Low */
1, /* AWB Enable During Recording */
{
   1, /* Use Digital Gain */
   /* Exposure Table */
   319,
   {
      {256, 1},	/* Gain= 1.000	Exposure Index=0	*/
      {263, 1},	/* Gain= 1.027	Exposure Index=1	*/
      {270, 1},	/* Gain= 1.055	Exposure Index=2	*/
      {278, 1},	/* Gain= 1.086	Exposure Index=3	*/
      {286, 1},	/* Gain= 1.117	Exposure Index=4	*/
      {294, 1},	/* Gain= 1.148	Exposure Index=5	*/
      {302, 1},	/* Gain= 1.180	Exposure Index=6	*/
      {311, 1},	/* Gain= 1.215	Exposure Index=7	*/
      {320, 1},	/* Gain= 1.250	Exposure Index=8	*/
      {329, 1},	/* Gain= 1.285	Exposure Index=9	*/
      {338, 1},	/* Gain= 1.320	Exposure Index=10	*/
      {348, 1},	/* Gain= 1.359	Exposure Index=11	*/
      {358, 1},	/* Gain= 1.398	Exposure Index=12	*/
      {368, 1},	/* Gain= 1.438	Exposure Index=13	*/
      {379, 1},	/* Gain= 1.480	Exposure Index=14	*/
      {390, 1},	/* Gain= 1.523	Exposure Index=15	*/
      {401, 1},	/* Gain= 1.566	Exposure Index=16	*/
      {413, 1},	/* Gain= 1.613	Exposure Index=17	*/
      {425, 1},	/* Gain= 1.660	Exposure Index=18	*/
      {437, 1},	/* Gain= 1.707	Exposure Index=19	*/
      {450, 1},	/* Gain= 1.758	Exposure Index=20	*/
      {463, 1},	/* Gain= 1.809	Exposure Index=21	*/
      {476, 1},	/* Gain= 1.859	Exposure Index=22	*/
      {490, 1},	/* Gain= 1.914	Exposure Index=23	*/
      {504, 1},	/* Gain= 1.969	Exposure Index=24	*/
      {259, 2},	/* Gain= 1.012	Exposure Index=25	*/
      {266, 2},	/* Gain= 1.039	Exposure Index=26	*/
      {273, 2},	/* Gain= 1.066	Exposure Index=27	*/
      {281, 2},	/* Gain= 1.098	Exposure Index=28	*/
      {289, 2},	/* Gain= 1.129	Exposure Index=29	*/
      {297, 2},	/* Gain= 1.160	Exposure Index=30	*/
      {305, 2},	/* Gain= 1.191	Exposure Index=31	*/
      {314, 2},	/* Gain= 1.227	Exposure Index=32	*/
      {323, 2},	/* Gain= 1.262	Exposure Index=33	*/
      {332, 2},	/* Gain= 1.297	Exposure Index=34	*/
      {341, 2},	/* Gain= 1.332	Exposure Index=35	*/
      {351, 2},	/* Gain= 1.371	Exposure Index=36	*/
      {361, 2},	/* Gain= 1.410	Exposure Index=37	*/
      {371, 2},	/* Gain= 1.449	Exposure Index=38	*/
      {382, 2},	/* Gain= 1.492	Exposure Index=39	*/
      {262, 3},	/* Gain= 1.023	Exposure Index=40	*/
      {269, 3},	/* Gain= 1.051	Exposure Index=41	*/
      {277, 3},	/* Gain= 1.082	Exposure Index=42	*/
      {285, 3},	/* Gain= 1.113	Exposure Index=43	*/
      {293, 3},	/* Gain= 1.145	Exposure Index=44	*/
      {301, 3},	/* Gain= 1.176	Exposure Index=45	*/
      {310, 3},	/* Gain= 1.211	Exposure Index=46	*/
      {319, 3},	/* Gain= 1.246	Exposure Index=47	*/
      {328, 3},	/* Gain= 1.281	Exposure Index=48	*/
      {337, 3},	/* Gain= 1.316	Exposure Index=49	*/
      {260, 4},	/* Gain= 1.016	Exposure Index=50	*/
      {267, 4},	/* Gain= 1.043	Exposure Index=51	*/
      {275, 4},	/* Gain= 1.074	Exposure Index=52	*/
      {283, 4},	/* Gain= 1.105	Exposure Index=53	*/
      {291, 4},	/* Gain= 1.137	Exposure Index=54	*/
      {299, 4},	/* Gain= 1.168	Exposure Index=55	*/
      {307, 4},	/* Gain= 1.199	Exposure Index=56	*/
      {316, 4},	/* Gain= 1.234	Exposure Index=57	*/
      {260, 5},	/* Gain= 1.016	Exposure Index=58	*/
      {267, 5},	/* Gain= 1.043	Exposure Index=59	*/
      {275, 5},	/* Gain= 1.074	Exposure Index=60	*/
      {283, 5},	/* Gain= 1.105	Exposure Index=61	*/
      {291, 5},	/* Gain= 1.137	Exposure Index=62	*/
      {299, 5},	/* Gain= 1.168	Exposure Index=63	*/
      {256, 6},	/* Gain= 1.000	Exposure Index=64	*/
      {263, 6},	/* Gain= 1.027	Exposure Index=65	*/
      {270, 6},	/* Gain= 1.055	Exposure Index=66	*/
      {278, 6},	/* Gain= 1.086	Exposure Index=67	*/
      {286, 6},	/* Gain= 1.117	Exposure Index=68	*/
      {294, 6},	/* Gain= 1.148	Exposure Index=69	*/
      {259, 7},	/* Gain= 1.012	Exposure Index=70	*/
      {266, 7},	/* Gain= 1.039	Exposure Index=71	*/
      {273, 7},	/* Gain= 1.066	Exposure Index=72	*/
      {281, 7},	/* Gain= 1.098	Exposure Index=73	*/
      {289, 7},	/* Gain= 1.129	Exposure Index=74	*/
      {260, 8},	/* Gain= 1.016	Exposure Index=75	*/
      {267, 8},	/* Gain= 1.043	Exposure Index=76	*/
      {275, 8},	/* Gain= 1.074	Exposure Index=77	*/
      {283, 8},	/* Gain= 1.105	Exposure Index=78	*/
      {259, 9},	/* Gain= 1.012	Exposure Index=79	*/
      {266, 9},	/* Gain= 1.039	Exposure Index=80	*/
      {273, 9},	/* Gain= 1.066	Exposure Index=81	*/
      {281, 9},	/* Gain= 1.098	Exposure Index=82	*/
      {260, 10},	/* Gain= 1.016	Exposure Index=83	*/
      {267, 10},	/* Gain= 1.043	Exposure Index=84	*/
      {275, 10},	/* Gain= 1.074	Exposure Index=85	*/
      {257, 11},	/* Gain= 1.004	Exposure Index=86	*/
      {264, 11},	/* Gain= 1.031	Exposure Index=87	*/
      {271, 11},	/* Gain= 1.059	Exposure Index=88	*/
      {279, 11},	/* Gain= 1.090	Exposure Index=89	*/
      {263, 12},	/* Gain= 1.027	Exposure Index=90	*/
      {270, 12},	/* Gain= 1.055	Exposure Index=91	*/
      {256, 13},	/* Gain= 1.000	Exposure Index=92	*/
      {263, 13},	/* Gain= 1.027	Exposure Index=93	*/
      {270, 13},	/* Gain= 1.055	Exposure Index=94	*/
      {258, 14},	/* Gain= 1.008	Exposure Index=95	*/
      {265, 14},	/* Gain= 1.035	Exposure Index=96	*/
      {272, 14},	/* Gain= 1.063	Exposure Index=97	*/
      {261, 15},	/* Gain= 1.020	Exposure Index=98	*/
      {268, 15},	/* Gain= 1.047	Exposure Index=99	*/
      {258, 16},	/* Gain= 1.008	Exposure Index=100	*/
      {265, 16},	/* Gain= 1.035	Exposure Index=101	*/
      {256, 17},	/* Gain= 1.000	Exposure Index=102	*/
      {263, 17},	/* Gain= 1.027	Exposure Index=103	*/
      {270, 17},	/* Gain= 1.055	Exposure Index=104	*/
      {262, 18},	/* Gain= 1.023	Exposure Index=105	*/
      {269, 18},	/* Gain= 1.051	Exposure Index=106	*/
      {262, 19},	/* Gain= 1.023	Exposure Index=107	*/
      {256, 20},	/* Gain= 1.000	Exposure Index=108	*/
      {263, 20},	/* Gain= 1.027	Exposure Index=109	*/
      {257, 21},	/* Gain= 1.004	Exposure Index=110	*/
      {264, 21},	/* Gain= 1.031	Exposure Index=111	*/
      {259, 22},	/* Gain= 1.012	Exposure Index=112	*/
      {266, 22},	/* Gain= 1.039	Exposure Index=113	*/
      {262, 23},	/* Gain= 1.023	Exposure Index=114	*/
      {258, 24},	/* Gain= 1.008	Exposure Index=115	*/
      {265, 24},	/* Gain= 1.035	Exposure Index=116	*/
      {262, 25},	/* Gain= 1.023	Exposure Index=117	*/
      {259, 26},	/* Gain= 1.012	Exposure Index=118	*/
      {256, 27},	/* Gain= 1.000	Exposure Index=119	*/
      {263, 27},	/* Gain= 1.027	Exposure Index=120	*/
      {261, 28},	/* Gain= 1.020	Exposure Index=121	*/
      {259, 29},	/* Gain= 1.012	Exposure Index=122	*/
      {257, 30},	/* Gain= 1.004	Exposure Index=123	*/
      {256, 31},	/* Gain= 1.000	Exposure Index=124	*/
      {263, 31},	/* Gain= 1.027	Exposure Index=125	*/
      {262, 32},	/* Gain= 1.023	Exposure Index=126	*/
      {261, 33},	/* Gain= 1.020	Exposure Index=127	*/
      {260, 34},	/* Gain= 1.016	Exposure Index=128	*/
      {260, 35},	/* Gain= 1.016	Exposure Index=129	*/
      {260, 36},	/* Gain= 1.016	Exposure Index=130	*/
      {260, 37},	/* Gain= 1.016	Exposure Index=131	*/
      {260, 38},	/* Gain= 1.016	Exposure Index=132	*/
      {260, 39},	/* Gain= 1.016	Exposure Index=133	*/
      {261, 40},	/* Gain= 1.020	Exposure Index=134	*/
      {256, 42},	/* Gain= 1.000	Exposure Index=135	*/
      {257, 43},	/* Gain= 1.004	Exposure Index=136	*/
      {258, 44},	/* Gain= 1.008	Exposure Index=137	*/
      {259, 45},	/* Gain= 1.012	Exposure Index=138	*/
      {260, 46},	/* Gain= 1.016	Exposure Index=139	*/
      {256, 48},	/* Gain= 1.000	Exposure Index=140	*/
      {258, 49},	/* Gain= 1.008	Exposure Index=141	*/
      {260, 50},	/* Gain= 1.016	Exposure Index=142	*/
      {257, 52},	/* Gain= 1.004	Exposure Index=143	*/
      {259, 53},	/* Gain= 1.012	Exposure Index=144	*/
      {257, 55},	/* Gain= 1.004	Exposure Index=145	*/
      {259, 56},	/* Gain= 1.012	Exposure Index=146	*/
      {257, 58},	/* Gain= 1.004	Exposure Index=147	*/
      {260, 59},	/* Gain= 1.016	Exposure Index=148	*/
      {259, 61},	/* Gain= 1.012	Exposure Index=149	*/
      {258, 63},	/* Gain= 1.008	Exposure Index=150	*/
      {257, 65},	/* Gain= 1.004	Exposure Index=151	*/
      {256, 67},	/* Gain= 1.000	Exposure Index=152	*/
      {256, 69},	/* Gain= 1.000	Exposure Index=153	*/
      {256, 71},	/* Gain= 1.000	Exposure Index=154	*/
      {256, 73},	/* Gain= 1.000	Exposure Index=155	*/
      {256, 75},	/* Gain= 1.000	Exposure Index=156	*/
      {256, 77},	/* Gain= 1.000	Exposure Index=157	*/
      {257, 79},	/* Gain= 1.004	Exposure Index=158	*/
      {258, 81},	/* Gain= 1.008	Exposure Index=159	*/
      {256, 84},	/* Gain= 1.000	Exposure Index=160	*/
      {257, 86},	/* Gain= 1.004	Exposure Index=161	*/
      {258, 88},	/* Gain= 1.008	Exposure Index=162	*/
      {256, 91},	/* Gain= 1.000	Exposure Index=163	*/
      {258, 93},	/* Gain= 1.008	Exposure Index=164	*/
      {257, 96},	/* Gain= 1.004	Exposure Index=165	*/
      {256, 99},	/* Gain= 1.000	Exposure Index=166	*/
      {258, 101},	/* Gain= 1.008	Exposure Index=167	*/
      {258, 104},	/* Gain= 1.008	Exposure Index=168	*/
      {258, 107},	/* Gain= 1.008	Exposure Index=169	*/
      {256, 111},	/* Gain= 1.000	Exposure Index=170	*/
      {256, 114},	/* Gain= 1.000	Exposure Index=171	*/
      {256, 117},	/* Gain= 1.000	Exposure Index=172	*/
      {257, 120},	/* Gain= 1.004	Exposure Index=173	*/
      {256, 124},	/* Gain= 1.000	Exposure Index=174	*/
      {257, 127},	/* Gain= 1.004	Exposure Index=175	*/
      {256, 131},	/* Gain= 1.000	Exposure Index=176	*/
      {257, 134},	/* Gain= 1.004	Exposure Index=177	*/
      {257, 138},	/* Gain= 1.004	Exposure Index=178	*/
      {257, 142},	/* Gain= 1.004	Exposure Index=179	*/
      {257, 146},	/* Gain= 1.004	Exposure Index=180	*/
      {257, 150},	/* Gain= 1.004	Exposure Index=181	*/
      {256, 155},	/* Gain= 1.000	Exposure Index=182	*/
      {257, 159},	/* Gain= 1.004	Exposure Index=183	*/
      {256, 164},	/* Gain= 1.000	Exposure Index=184	*/
      {257, 168},	/* Gain= 1.004	Exposure Index=185	*/
      {257, 173},	/* Gain= 1.004	Exposure Index=186	*/
      {257, 178},	/* Gain= 1.004	Exposure Index=187	*/
      {256, 184},	/* Gain= 1.000	Exposure Index=188	*/
      {256, 189},	/* Gain= 1.000	Exposure Index=189	*/
      {256, 194},	/* Gain= 1.000	Exposure Index=190	*/
      {257, 199},	/* Gain= 1.004	Exposure Index=191	*/
      {256, 205},	/* Gain= 1.000	Exposure Index=192	*/
      {256, 211},	/* Gain= 1.000	Exposure Index=193	*/
      {256, 217},	/* Gain= 1.000	Exposure Index=194	*/
      {256, 223},	/* Gain= 1.000	Exposure Index=195	*/
      {256, 229},	/* Gain= 1.000	Exposure Index=196	*/
      {256, 235},	/* Gain= 1.000	Exposure Index=197	*/
      {256, 242},	/* Gain= 1.000	Exposure Index=198	*/
      {256, 249},	/* Gain= 1.000	Exposure Index=199	*/
      {256, 256},	/* Gain= 1.000	Exposure Index=200	*/
      {256, 263},	/* Gain= 1.000	Exposure Index=201	*/
      {256, 270},	/* Gain= 1.000	Exposure Index=202	*/
      {256, 278},	/* Gain= 1.000	Exposure Index=203	*/
      {256, 286},	/* Gain= 1.000	Exposure Index=204	*/
      {256, 294},	/* Gain= 1.000	Exposure Index=205	*/
      {256, 302},	/* Gain= 1.000	Exposure Index=206	*/
      {256, 311},	/* Gain= 1.000	Exposure Index=207	*/
      {256, 320},	/* Gain= 1.000	Exposure Index=208	*/
      {256, 329},	/* Gain= 1.000	Exposure Index=209	*/
      {256, 338},	/* Gain= 1.000	Exposure Index=210	*/
      {256, 348},	/* Gain= 1.000	Exposure Index=211	*/
      {256, 358},	/* Gain= 1.000	Exposure Index=212	*/
      {256, 368},	/* Gain= 1.000	Exposure Index=213	*/
      {256, 379},	/* Gain= 1.000	Exposure Index=214	*/
      {256, 390},	/* Gain= 1.000	Exposure Index=215	*/
      {256, 401},	/* Gain= 1.000	Exposure Index=216	*/
      {256, 413},	/* Gain= 1.000	Exposure Index=217	*/
      {256, 425},	/* Gain= 1.000	Exposure Index=218	*/
      {256, 437},	/* Gain= 1.000	Exposure Index=219	*/
      {256, 450},	/* Gain= 1.000	Exposure Index=220	*/
      {256, 463},	/* Gain= 1.000	Exposure Index=221	*/
      {256, 476},	/* Gain= 1.000	Exposure Index=222	*/
      {256, 490},	/* Gain= 1.000	Exposure Index=223	*/
      {256, 504},	/* Gain= 1.000	Exposure Index=224	*/
      {256, 519},	/* Gain= 1.000	Exposure Index=225	*/
      {256, 534},	/* Gain= 1.000	Exposure Index=226	*/
      {256, 550},	/* Gain= 1.000	Exposure Index=227	*/
      {256, 566},	/* Gain= 1.000	Exposure Index=228	*/
      {256, 582},	/* Gain= 1.000	Exposure Index=229	*/
      {256, 599},	/* Gain= 1.000	Exposure Index=230	*/
      {256, 616},	/* Gain= 1.000	Exposure Index=231	*/
      {256, 634},	/* Gain= 1.000	Exposure Index=232	*/
      {256, 653},	/* Gain= 1.000	Exposure Index=233	*/
      {256, 672},	/* Gain= 1.000	Exposure Index=234	*/
      {256, 692},	/* Gain= 1.000	Exposure Index=235	*/
      {256, 712},	/* Gain= 1.000	Exposure Index=236	*/
      {256, 733},	/* Gain= 1.000	Exposure Index=237	*/
      {256, 754},	/* Gain= 1.000	Exposure Index=238	*/
      {257, 772},	/* Gain= 1.004	Exposure Index=239	*/
      {264, 772},	/* Gain= 1.031	Exposure Index=240	*/
      {271, 772},	/* Gain= 1.059	Exposure Index=241	*/
      {279, 772},	/* Gain= 1.090	Exposure Index=242	*/
      {287, 772},	/* Gain= 1.121	Exposure Index=243	*/
      {295, 772},	/* Gain= 1.152	Exposure Index=244	*/
      {303, 772},	/* Gain= 1.184	Exposure Index=245	*/
      {312, 772},	/* Gain= 1.219	Exposure Index=246	*/
      {321, 772},	/* Gain= 1.254	Exposure Index=247	*/
      {330, 772},	/* Gain= 1.289	Exposure Index=248	*/
      {339, 772},	/* Gain= 1.324	Exposure Index=249	*/
      {349, 772},	/* Gain= 1.363	Exposure Index=250	*/
      {359, 772},	/* Gain= 1.402	Exposure Index=251	*/
      {369, 772},	/* Gain= 1.441	Exposure Index=252	*/
      {380, 772},	/* Gain= 1.484	Exposure Index=253	*/
      {391, 772},	/* Gain= 1.527	Exposure Index=254	*/
      {402, 772},	/* Gain= 1.570	Exposure Index=255	*/
      {414, 772},	/* Gain= 1.617	Exposure Index=256	*/
      {426, 772},	/* Gain= 1.664	Exposure Index=257	*/
      {438, 772},	/* Gain= 1.711	Exposure Index=258	*/
      {451, 772},	/* Gain= 1.762	Exposure Index=259	*/
      {464, 772},	/* Gain= 1.813	Exposure Index=260	*/
      {477, 772},	/* Gain= 1.863	Exposure Index=261	*/
      {491, 772},	/* Gain= 1.918	Exposure Index=262	*/
      {505, 772},	/* Gain= 1.973	Exposure Index=263	*/
      {520, 772},	/* Gain= 2.031	Exposure Index=264	*/
      {535, 772},	/* Gain= 2.090	Exposure Index=265	*/
      {551, 772},	/* Gain= 2.152	Exposure Index=266	*/
      {567, 772},	/* Gain= 2.215	Exposure Index=267	*/
      {584, 772},	/* Gain= 2.281	Exposure Index=268	*/
      {601, 772},	/* Gain= 2.348	Exposure Index=269	*/
      {619, 772},	/* Gain= 2.418	Exposure Index=270	*/
      {637, 772},	/* Gain= 2.488	Exposure Index=271	*/
      {656, 772},	/* Gain= 2.563	Exposure Index=272	*/
      {675, 772},	/* Gain= 2.637	Exposure Index=273	*/
      {695, 772},	/* Gain= 2.715	Exposure Index=274	*/
      {715, 772},	/* Gain= 2.793	Exposure Index=275	*/
      {736, 772},	/* Gain= 2.875	Exposure Index=276	*/
      {758, 772},	/* Gain= 2.961	Exposure Index=277	*/
      {780, 772},	/* Gain= 3.047	Exposure Index=278	*/
      {803, 772},	/* Gain= 3.137	Exposure Index=279	*/
      {827, 772},	/* Gain= 3.230	Exposure Index=280	*/
      {851, 772},	/* Gain= 3.324	Exposure Index=281	*/
      {876, 772},	/* Gain= 3.422	Exposure Index=282	*/
      {902, 772},	/* Gain= 3.523	Exposure Index=283	*/
      {929, 772},	/* Gain= 3.629	Exposure Index=284	*/
      {956, 772},	/* Gain= 3.734	Exposure Index=285	*/
      {984, 772},	/* Gain= 3.844	Exposure Index=286	*/
      {1013, 772},	/* Gain= 3.957	Exposure Index=287	*/
      {1043, 772},	/* Gain= 4.074	Exposure Index=288	*/
      {1074, 772},	/* Gain= 4.195	Exposure Index=289	*/
      {1106, 772},	/* Gain= 4.320	Exposure Index=290	*/
      {1139, 772},	/* Gain= 4.449	Exposure Index=291	*/
      {1173, 772},	/* Gain= 4.582	Exposure Index=292	*/
      {1208, 772},	/* Gain= 4.719	Exposure Index=293	*/
      {1244, 772},	/* Gain= 4.859	Exposure Index=294	*/
      {1281, 772},	/* Gain= 5.004	Exposure Index=295	*/
      {1319, 772},	/* Gain= 5.152	Exposure Index=296	*/
      {1358, 772},	/* Gain= 5.305	Exposure Index=297	*/
      {1398, 772},	/* Gain= 5.461	Exposure Index=298	*/
      {1439, 772},	/* Gain= 5.621	Exposure Index=299	*/
      {1482, 772},	/* Gain= 5.789	Exposure Index=300	*/
      {1526, 772},	/* Gain= 5.961	Exposure Index=301	*/
      {1571, 772},	/* Gain= 6.137	Exposure Index=302	*/
      {1618, 772},	/* Gain= 6.320	Exposure Index=303	*/
      {1666, 772},	/* Gain= 6.508	Exposure Index=304	*/
      {1715, 772},	/* Gain= 6.699	Exposure Index=305	*/
      {1766, 772},	/* Gain= 6.898	Exposure Index=306	*/
      {1818, 772},	/* Gain= 7.102	Exposure Index=307	*/
      {1872, 772},	/* Gain= 7.313	Exposure Index=308	*/
      {1928, 772},	/* Gain= 7.531	Exposure Index=309	*/
      {1985, 772},	/* Gain= 7.754	Exposure Index=310	*/
      {2044, 772},	/* Gain= 7.984	Exposure Index=311	*/
      {2105, 772},	/* Gain= 8.223	Exposure Index=312	*/
      {2168, 772},	/* Gain= 8.469	Exposure Index=313	*/
      {2233, 772},	/* Gain= 8.723	Exposure Index=314	*/
      {2299, 772},	/* Gain= 8.980	Exposure Index=315	*/
      {2367, 772},	/* Gain= 9.246	Exposure Index=316	*/
      {2438, 772},	/* Gain= 9.523	Exposure Index=317	*/
      {2511, 772}	/* Gain= 9.809	Exposure Index=318	*/
   },
},
/* Default Luma Target */
50,
/* Outdoor Luma Target */
50,
/* Low-Light Luma Target */
49,
/* Backlight Luma Target */
19,
2, /* Luma Tolerance */
77.900002f, /* Exposure Index Adj Step */
1.000000f, /* ISO 100 Gain */
240, /* AEC Indoor Index */
140, /* AEC Outdoor Index */
10.000000f, /* Max Preview Gain Allowed */
1.000000f, /* Min Preview Gain Allowed */
10.000000f, /* Max Snapshot Gain Allowed */
1.000000f, /* Min Snapshot Gain Allowed */
0.500000f, /* Max Snapshot Exposure Time Allowed */
0.500000f, /* Aggressiveness */
318, /* Fix FPS AEC Table Index */
0, /* Linear AFR Support */
130, /* High Luma Region Threshold */
1, /* Use Digital Gain */
{
   1, /* Is Supported? */
   0.250000f, /* Reduction */
   200, /* Threshold Low */
   200, /* Lux Index Low */
   125, /* Threshold High */
   200, /* Lux Index High */
   0.500000f, /* Discard Ratio */
},
{
   1, /* Is Supported? */
   0.100000f, /* Threshold Low */
   0.500000f, /* Threshold Hi */
   0.500000f, /* Discard Ratio */
},
0, /* wLED Trigger Index */
0.000000f, /* AEC LED Preview Flux */
0.000000f, /* AEC LED Snapshot Flux */
{
   1, /* Enable */
   0.500000f, /* Aggressiveness */
   40.000000f, /* Threshold */
   0.000000f, /* Max Gain */
},
{
   1, /* Enable */
   0.500000f, /* Aggressiveness */
   40.000000f, /* Threshold */
   0.000000f, /* Max Gain */
},
{
   1, /* Enable */
   0.500000f, /* Aggressiveness */
   40.000000f, /* Threshold */
   0.000000f, /* Max Gain */
},
30.000000f, /* Max Video FPS */
15.000000f, /* Video FPS */
30.000000f, /* Max Preview FPS */
30.000000f, /* Preview FPS */
15.000000f, /* Nightshot FPS */
1, /* Nightshot Table Index */
1, /* AEC Enable During Recording */
30, /* Total Steps */
3, /* Num Gross Steps Between Stat Points */
1, /* Num Fine Steps Between Stat Points */
7, /* Num Fine Search Points */
1, /* Process Type */
1, /* Near End */
10, /* Default In Macro */
20, /* Boundary */
30, /* Defult In Normal */
30, /* Far End */
4.000000f, /* Steps Gain */
0, /* Initial Current */
30, /* Num Steps Near to Far */
0, /* Undershoot Protect */
0, /* Undershoot Adjust */
1, /* Reset Lens After Snap */
0, /* Minimum Y */
255, /* Maximum Y */
0.250000f, /* Horizontal Offset Ratio */
0.250000f, /* Vertical Offset Ratio */
0.50000f, /* Horizontal Clip Ratio */
0.50000f, /* Vertical Clip Ratio */
1, /* FV Metric */
{
   -2, /* A00 */
   0, /* A02 */
   -2, /* A04 */
   -1, /* A20 */
   -1, /* A21 */
   8, /* A22 */
   -1, /* A23 */
   -1, /* A24 */
},
0, /* Window Type */
0, /* Skin Assisted */
0, /* Max Number of Faces */
0, /* Face Priority Type */
1, /* Is Partial Face Okay */
0, /* Continuous AF Enable During Recording */
0, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
{
   1.964286f, /* Gain Start */
   1.785714f, /* Gain End */
   71, /* Lux Index Start */
   65, /* Lux Index End */
},
/* Gamma Table */
{
   {
      0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 
      5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 
      14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 24, 24, 
      25, 26, 27, 28, 28, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 
      37, 38, 39, 39, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 
      48, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58, 
      58, 59, 60, 60, 61, 62, 62, 63, 64, 64, 65, 66, 66, 67, 67, 68, 
      69, 69, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75, 76, 77, 77, 78, 
      78, 79, 80, 80, 81, 81, 82, 82, 83, 84, 84, 85, 85, 86, 86, 87, 
      87, 88, 88, 89, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 
      96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 100, 101, 101, 102, 102, 103, 
      103, 104, 104, 105, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 109, 110, 
      110, 111, 111, 111, 112, 112, 113, 113, 114, 114, 114, 115, 115, 116, 116, 116, 
      117, 117, 118, 118, 118, 119, 119, 119, 120, 120, 121, 121, 121, 122, 122, 123, 
      123, 123, 124, 124, 124, 125, 125, 125, 126, 126, 127, 127, 127, 128, 128, 128, 
      129, 129, 129, 130, 130, 130, 131, 131, 131, 132, 132, 132, 133, 133, 133, 134, 
      134, 134, 135, 135, 135, 136, 136, 136, 137, 137, 137, 138, 138, 138, 138, 139, 
      139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 142, 142, 143, 143, 143, 144, 
      144, 144, 144, 145, 145, 145, 146, 146, 146, 146, 147, 147, 147, 148, 148, 148, 
      148, 149, 149, 149, 149, 150, 150, 150, 150, 151, 151, 151, 151, 152, 152, 152, 
      153, 153, 153, 153, 154, 154, 154, 154, 155, 155, 155, 155, 156, 156, 156, 156, 
      156, 157, 157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 
      160, 160, 161, 161, 161, 161, 162, 162, 162, 162, 162, 163, 163, 163, 163, 164, 
      164, 164, 164, 164, 165, 165, 165, 165, 165, 166, 166, 166, 166, 166, 167, 167, 
      167, 167, 167, 168, 168, 168, 168, 168, 169, 169, 169, 169, 169, 170, 170, 170, 
      170, 170, 171, 171, 171, 171, 171, 172, 172, 172, 172, 172, 173, 173, 173, 173, 
      173, 173, 174, 174, 174, 174, 174, 175, 175, 175, 175, 175, 175, 176, 176, 176, 
      176, 176, 177, 177, 177, 177, 177, 177, 178, 178, 178, 178, 178, 178, 179, 179, 
      179, 179, 179, 179, 180, 180, 180, 180, 180, 181, 181, 181, 181, 181, 181, 182, 
      182, 182, 182, 182, 182, 183, 183, 183, 183, 183, 183, 183, 184, 184, 184, 184, 
      184, 184, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 186, 187, 187, 
      187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 189, 189, 189, 189, 189, 189, 
      189, 190, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 192, 192, 192, 
      192, 192, 192, 193, 193, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 196, 196, 197, 197, 197, 
      197, 197, 197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 199, 199, 
      200, 200, 200, 200, 200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 
      202, 202, 202, 203, 203, 203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 204, 
      205, 205, 205, 205, 205, 205, 205, 206, 206, 206, 206, 206, 206, 207, 207, 207, 
      207, 207, 207, 207, 208, 208, 208, 208, 208, 208, 209, 209, 209, 209, 209, 209, 
      209, 210, 210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 211, 211, 212, 
      212, 212, 212, 212, 212, 212, 213, 213, 213, 213, 213, 213, 214, 214, 214, 214, 
      214, 214, 214, 215, 215, 215, 215, 215, 215, 215, 216, 216, 216, 216, 216, 216, 
      216, 216, 217, 217, 217, 217, 217, 217, 217, 218, 218, 218, 218, 218, 218, 218, 
      219, 219, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 220, 220, 221, 221, 
      221, 221, 221, 221, 221, 221, 222, 222, 222, 222, 222, 222, 222, 223, 223, 223, 
      223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 224, 224, 225, 225, 225, 
      225, 225, 225, 225, 226, 226, 226, 226, 226, 226, 226, 226, 227, 227, 227, 227, 
      227, 227, 227, 227, 228, 228, 228, 228, 228, 228, 228, 228, 229, 229, 229, 229, 
      229, 229, 229, 229, 229, 230, 230, 230, 230, 230, 230, 230, 230, 231, 231, 231, 
      231, 231, 231, 231, 231, 232, 232, 232, 232, 232, 232, 232, 232, 232, 233, 233, 
      233, 233, 233, 233, 233, 233, 233, 234, 234, 234, 234, 234, 234, 234, 234, 235, 
      235, 235, 235, 235, 235, 235, 235, 235, 236, 236, 236, 236, 236, 236, 236, 236, 
      236, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 238, 238, 238, 238, 238, 
      238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 239, 239, 239, 240, 240, 240, 
      240, 240, 240, 240, 240, 240, 241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 
      242, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 243, 
      243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, 
      245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 246, 246, 246, 246, 
      247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 248, 
      248, 248, 248, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 250, 250, 250, 
      250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 
      252, 252, 252, 252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 
      253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255
   },
   0 /* Linear */
},
/* Low-Light Gamma Table */
{
   {
      0, 1, 1, 2, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 
      11, 12, 12, 13, 14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 20, 21, 
      21, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 31, 
      31, 32, 32, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39, 39, 40, 
      41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 
      49, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 
      58, 58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 
      66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 
      74, 74, 75, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 80, 
      81, 81, 82, 82, 83, 83, 84, 84, 84, 85, 85, 86, 86, 87, 87, 87, 
      88, 88, 89, 89, 90, 90, 90, 91, 91, 92, 92, 93, 93, 93, 94, 94, 
      95, 95, 95, 96, 96, 97, 97, 97, 98, 98, 99, 99, 99, 100, 100, 101, 
      101, 101, 102, 102, 103, 103, 103, 104, 104, 104, 105, 105, 106, 106, 106, 107, 
      107, 107, 108, 108, 109, 109, 109, 110, 110, 110, 111, 111, 112, 112, 112, 113, 
      113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 117, 117, 117, 118, 118, 118, 
      119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 124, 
      124, 124, 125, 125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 
      129, 130, 130, 130, 131, 131, 131, 131, 132, 132, 132, 133, 133, 133, 134, 134, 
      134, 135, 135, 135, 135, 136, 136, 136, 137, 137, 137, 138, 138, 138, 138, 139, 
      139, 139, 140, 140, 140, 141, 141, 141, 141, 142, 142, 142, 143, 143, 143, 143, 
      144, 144, 144, 145, 145, 145, 145, 146, 146, 146, 147, 147, 147, 147, 148, 148, 
      148, 148, 149, 149, 149, 150, 150, 150, 150, 151, 151, 151, 151, 152, 152, 152, 
      153, 153, 153, 153, 154, 154, 154, 154, 155, 155, 155, 155, 156, 156, 156, 156, 
      157, 157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 160, 
      161, 161, 161, 161, 162, 162, 162, 162, 163, 163, 163, 163, 164, 164, 164, 164, 
      165, 165, 165, 165, 166, 166, 166, 166, 167, 167, 167, 167, 167, 168, 168, 168, 
      168, 169, 169, 169, 169, 170, 170, 170, 170, 170, 171, 171, 171, 171, 172, 172, 
      172, 172, 172, 173, 173, 173, 173, 174, 174, 174, 174, 174, 175, 175, 175, 175, 
      176, 176, 176, 176, 176, 177, 177, 177, 177, 178, 178, 178, 178, 178, 179, 179, 
      179, 179, 179, 180, 180, 180, 180, 180, 181, 181, 181, 181, 181, 182, 182, 182, 
      182, 183, 183, 183, 183, 183, 184, 184, 184, 184, 184, 185, 185, 185, 185, 185, 
      186, 186, 186, 186, 186, 187, 187, 187, 187, 187, 187, 188, 188, 188, 188, 188, 
      189, 189, 189, 189, 189, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 
      192, 192, 192, 192, 192, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 197, 197, 197, 197, 197, 
      197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 200, 
      200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 202, 203, 203, 
      203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 205, 205, 205, 205, 205, 205, 
      206, 206, 206, 206, 206, 206, 206, 207, 207, 207, 207, 207, 207, 208, 208, 208, 
      208, 208, 208, 209, 209, 209, 209, 209, 209, 209, 210, 210, 210, 210, 210, 210, 
      211, 211, 211, 211, 211, 211, 211, 212, 212, 212, 212, 212, 212, 213, 213, 213, 
      213, 213, 213, 213, 214, 214, 214, 214, 214, 214, 214, 215, 215, 215, 215, 215, 
      215, 215, 216, 216, 216, 216, 216, 216, 216, 217, 217, 217, 217, 217, 217, 217, 
      218, 218, 218, 218, 218, 218, 218, 219, 219, 219, 219, 219, 219, 219, 220, 220, 
      220, 220, 220, 220, 220, 221, 221, 221, 221, 221, 221, 221, 221, 222, 222, 222, 
      222, 222, 222, 222, 223, 223, 223, 223, 223, 223, 223, 223, 224, 224, 224, 224, 
      224, 224, 224, 225, 225, 225, 225, 225, 225, 225, 225, 226, 226, 226, 226, 226, 
      226, 226, 226, 227, 227, 227, 227, 227, 227, 227, 227, 228, 228, 228, 228, 228, 
      228, 228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 230, 230, 230, 
      230, 230, 230, 231, 231, 231, 231, 231, 231, 231, 231, 232, 232, 232, 232, 232, 
      232, 232, 232, 233, 233, 233, 233, 233, 233, 233, 233, 233, 234, 234, 234, 234, 
      234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 235, 236, 236, 236, 
      236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 237, 237, 237, 237, 238, 
      238, 238, 238, 238, 238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 239, 239, 
      239, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 241, 241, 241, 241, 241, 
      241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 
      243, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244, 244, 
      244, 245, 245, 245, 245, 245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 
      246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 
      248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 249, 249, 
      249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 251, 
      251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 
      252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 254, 
      254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255
   },
   0 /* Linear */
},
/* Outdoor Gamma Table */
{
   {
      2, 2, 3, 4, 4, 5, 6, 6, 8, 8, 9, 10, 10, 11, 12, 12, 
      13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 
      26, 27, 28, 29, 30, 30, 31, 32, 33, 34, 35, 35, 36, 37, 38, 39, 
      39, 40, 41, 42, 42, 43, 44, 44, 45, 46, 47, 48, 49, 49, 50, 51, 
      51, 52, 53, 53, 54, 55, 55, 56, 56, 57, 58, 59, 59, 60, 61, 61, 
      62, 63, 63, 64, 64, 65, 66, 66, 67, 67, 68, 69, 69, 70, 70, 71, 
      71, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 80, 
      80, 81, 81, 82, 82, 83, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 
      87, 88, 88, 89, 89, 90, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 
      95, 95, 96, 96, 96, 97, 97, 98, 98, 99, 99, 99, 100, 100, 100, 101, 
      101, 102, 102, 102, 103, 103, 104, 104, 105, 105, 105, 106, 106, 107, 107, 107, 
      108, 108, 109, 109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 114, 
      114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 119, 119, 
      119, 120, 120, 121, 121, 121, 122, 122, 123, 123, 123, 124, 124, 124, 124, 125, 
      125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 
      130, 131, 131, 132, 132, 132, 132, 133, 133, 133, 134, 134, 134, 134, 135, 135, 
      135, 136, 136, 136, 136, 137, 137, 137, 138, 138, 138, 139, 139, 139, 140, 140, 
      140, 141, 141, 141, 142, 142, 142, 142, 143, 143, 143, 143, 144, 144, 144, 144, 
      145, 145, 145, 145, 146, 146, 146, 146, 147, 147, 147, 148, 148, 148, 148, 149, 
      149, 149, 150, 150, 150, 150, 151, 151, 151, 152, 152, 152, 152, 153, 153, 153, 
      153, 154, 154, 154, 154, 155, 155, 155, 155, 155, 156, 156, 156, 156, 157, 157, 
      157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 160, 161, 
      161, 161, 161, 162, 162, 162, 162, 163, 163, 163, 163, 163, 164, 164, 164, 164, 
      165, 165, 165, 165, 166, 166, 166, 166, 167, 167, 167, 167, 167, 168, 168, 168, 
      168, 168, 169, 169, 169, 169, 169, 170, 170, 170, 170, 170, 171, 171, 171, 171, 
      171, 172, 172, 172, 172, 172, 173, 173, 173, 173, 174, 174, 174, 174, 174, 174, 
      175, 175, 175, 175, 175, 176, 176, 176, 176, 176, 177, 177, 177, 177, 177, 178, 
      178, 178, 178, 178, 178, 179, 179, 179, 179, 179, 180, 180, 180, 180, 180, 180, 
      181, 181, 181, 181, 181, 182, 182, 182, 182, 182, 183, 183, 183, 183, 183, 184, 
      184, 184, 184, 184, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 187, 
      187, 187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 189, 189, 189, 189, 189, 
      189, 190, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 191, 192, 192, 
      192, 192, 192, 192, 193, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 196, 197, 197, 197, 197, 
      197, 197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 
      200, 200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 202, 203, 
      203, 203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 204, 205, 205, 205, 205, 
      205, 205, 205, 205, 206, 206, 206, 206, 206, 206, 206, 206, 207, 207, 207, 207, 
      207, 207, 207, 208, 208, 208, 208, 208, 208, 208, 209, 209, 209, 209, 209, 209, 
      210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 211, 211, 212, 212, 212, 
      212, 212, 212, 213, 213, 213, 213, 213, 213, 213, 213, 214, 214, 214, 214, 214, 
      214, 214, 214, 215, 215, 215, 215, 215, 215, 215, 216, 216, 216, 216, 216, 216, 
      216, 216, 217, 217, 217, 217, 217, 217, 217, 218, 218, 218, 218, 218, 218, 218, 
      218, 219, 219, 219, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 220, 220, 
      220, 221, 221, 221, 221, 221, 221, 221, 221, 222, 222, 222, 222, 222, 222, 222, 
      222, 223, 223, 223, 223, 223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 
      224, 224, 225, 225, 225, 225, 225, 225, 225, 225, 226, 226, 226, 226, 226, 226, 
      226, 226, 227, 227, 227, 227, 227, 227, 227, 227, 228, 228, 228, 228, 228, 228, 
      228, 228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 230, 230, 
      230, 230, 230, 230, 230, 231, 231, 231, 231, 231, 231, 231, 231, 231, 232, 232, 
      232, 232, 232, 232, 232, 232, 232, 232, 233, 233, 233, 233, 233, 233, 233, 233, 
      233, 234, 234, 234, 234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 
      235, 235, 236, 236, 236, 236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 
      237, 237, 237, 237, 238, 238, 238, 238, 238, 238, 238, 238, 238, 239, 239, 239, 
      239, 239, 239, 239, 239, 239, 240, 240, 240, 240, 240, 240, 240, 240, 240, 241, 
      241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 
      242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 243, 243, 243, 243, 243, 244, 
      244, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, 245, 245, 245, 245, 245, 
      245, 245, 246, 246, 246, 246, 246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 
      247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 
      249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 
      250, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 
      252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 
      253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255
   },
   0 /* Linear */
},
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
{
   1.964286f, /* Gain Start */
   1.785714f, /* Gain End */
   71, /* Lux Index Start */
   65, /* Lux Index End */
},
/* Default Preview Gamma Table */
{
   {
      0, 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 
      5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 
      14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 21, 21, 22, 23, 24, 24, 
      25, 26, 27, 28, 28, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 
      37, 38, 39, 39, 40, 41, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 
      48, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58, 
      58, 59, 60, 60, 61, 62, 62, 63, 64, 64, 65, 66, 66, 67, 67, 68, 
      69, 69, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75, 76, 77, 77, 78, 
      78, 79, 80, 80, 81, 81, 82, 82, 83, 84, 84, 85, 85, 86, 86, 87, 
      87, 88, 88, 89, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 
      96, 96, 97, 97, 98, 98, 99, 99, 100, 100, 100, 101, 101, 102, 102, 103, 
      103, 104, 104, 105, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 109, 110, 
      110, 111, 111, 111, 112, 112, 113, 113, 114, 114, 114, 115, 115, 116, 116, 116, 
      117, 117, 118, 118, 118, 119, 119, 119, 120, 120, 121, 121, 121, 122, 122, 123, 
      123, 123, 124, 124, 124, 125, 125, 125, 126, 126, 127, 127, 127, 128, 128, 128, 
      129, 129, 129, 130, 130, 130, 131, 131, 131, 132, 132, 132, 133, 133, 133, 134, 
      134, 134, 135, 135, 135, 136, 136, 136, 137, 137, 137, 138, 138, 138, 138, 139, 
      139, 139, 140, 140, 140, 141, 141, 141, 142, 142, 142, 142, 143, 143, 143, 144, 
      144, 144, 144, 145, 145, 145, 146, 146, 146, 146, 147, 147, 147, 148, 148, 148, 
      148, 149, 149, 149, 149, 150, 150, 150, 150, 151, 151, 151, 151, 152, 152, 152, 
      153, 153, 153, 153, 154, 154, 154, 154, 155, 155, 155, 155, 156, 156, 156, 156, 
      156, 157, 157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 
      160, 160, 161, 161, 161, 161, 162, 162, 162, 162, 162, 163, 163, 163, 163, 164, 
      164, 164, 164, 164, 165, 165, 165, 165, 165, 166, 166, 166, 166, 166, 167, 167, 
      167, 167, 167, 168, 168, 168, 168, 168, 169, 169, 169, 169, 169, 170, 170, 170, 
      170, 170, 171, 171, 171, 171, 171, 172, 172, 172, 172, 172, 173, 173, 173, 173, 
      173, 173, 174, 174, 174, 174, 174, 175, 175, 175, 175, 175, 175, 176, 176, 176, 
      176, 176, 177, 177, 177, 177, 177, 177, 178, 178, 178, 178, 178, 178, 179, 179, 
      179, 179, 179, 179, 180, 180, 180, 180, 180, 181, 181, 181, 181, 181, 181, 182, 
      182, 182, 182, 182, 182, 183, 183, 183, 183, 183, 183, 183, 184, 184, 184, 184, 
      184, 184, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 186, 187, 187, 
      187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 189, 189, 189, 189, 189, 189, 
      189, 190, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 192, 192, 192, 
      192, 192, 192, 193, 193, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 196, 196, 197, 197, 197, 
      197, 197, 197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 199, 199, 
      200, 200, 200, 200, 200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 
      202, 202, 202, 203, 203, 203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 204, 
      205, 205, 205, 205, 205, 205, 205, 206, 206, 206, 206, 206, 206, 207, 207, 207, 
      207, 207, 207, 207, 208, 208, 208, 208, 208, 208, 209, 209, 209, 209, 209, 209, 
      209, 210, 210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 211, 211, 212, 
      212, 212, 212, 212, 212, 212, 213, 213, 213, 213, 213, 213, 214, 214, 214, 214, 
      214, 214, 214, 215, 215, 215, 215, 215, 215, 215, 216, 216, 216, 216, 216, 216, 
      216, 216, 217, 217, 217, 217, 217, 217, 217, 218, 218, 218, 218, 218, 218, 218, 
      219, 219, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 220, 220, 221, 221, 
      221, 221, 221, 221, 221, 221, 222, 222, 222, 222, 222, 222, 222, 223, 223, 223, 
      223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 224, 224, 225, 225, 225, 
      225, 225, 225, 225, 226, 226, 226, 226, 226, 226, 226, 226, 227, 227, 227, 227, 
      227, 227, 227, 227, 228, 228, 228, 228, 228, 228, 228, 228, 229, 229, 229, 229, 
      229, 229, 229, 229, 229, 230, 230, 230, 230, 230, 230, 230, 230, 231, 231, 231, 
      231, 231, 231, 231, 231, 232, 232, 232, 232, 232, 232, 232, 232, 232, 233, 233, 
      233, 233, 233, 233, 233, 233, 233, 234, 234, 234, 234, 234, 234, 234, 234, 235, 
      235, 235, 235, 235, 235, 235, 235, 235, 236, 236, 236, 236, 236, 236, 236, 236, 
      236, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 238, 238, 238, 238, 238, 
      238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 239, 239, 239, 240, 240, 240, 
      240, 240, 240, 240, 240, 240, 241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 
      242, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 243, 
      243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, 
      245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 246, 246, 246, 246, 
      247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 248, 
      248, 248, 248, 249, 249, 249, 249, 249, 249, 249, 249, 249, 249, 250, 250, 250, 
      250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 
      252, 252, 252, 252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 
      253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255
   },
   0 /* Linear */
},
/* Low-Light Preview Gamma Table */
{
   {
      0, 1, 1, 2, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 
      11, 12, 12, 13, 14, 14, 15, 16, 16, 17, 18, 18, 19, 20, 20, 21, 
      21, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 31, 
      31, 32, 32, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39, 39, 40, 
      41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 
      49, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 
      58, 58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 
      66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 
      74, 74, 75, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 80, 
      81, 81, 82, 82, 83, 83, 84, 84, 84, 85, 85, 86, 86, 87, 87, 87, 
      88, 88, 89, 89, 90, 90, 90, 91, 91, 92, 92, 93, 93, 93, 94, 94, 
      95, 95, 95, 96, 96, 97, 97, 97, 98, 98, 99, 99, 99, 100, 100, 101, 
      101, 101, 102, 102, 103, 103, 103, 104, 104, 104, 105, 105, 106, 106, 106, 107, 
      107, 107, 108, 108, 109, 109, 109, 110, 110, 110, 111, 111, 112, 112, 112, 113, 
      113, 113, 114, 114, 114, 115, 115, 115, 116, 116, 117, 117, 117, 118, 118, 118, 
      119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 124, 
      124, 124, 125, 125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 
      129, 130, 130, 130, 131, 131, 131, 131, 132, 132, 132, 133, 133, 133, 134, 134, 
      134, 135, 135, 135, 135, 136, 136, 136, 137, 137, 137, 138, 138, 138, 138, 139, 
      139, 139, 140, 140, 140, 141, 141, 141, 141, 142, 142, 142, 143, 143, 143, 143, 
      144, 144, 144, 145, 145, 145, 145, 146, 146, 146, 147, 147, 147, 147, 148, 148, 
      148, 148, 149, 149, 149, 150, 150, 150, 150, 151, 151, 151, 151, 152, 152, 152, 
      153, 153, 153, 153, 154, 154, 154, 154, 155, 155, 155, 155, 156, 156, 156, 156, 
      157, 157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 160, 
      161, 161, 161, 161, 162, 162, 162, 162, 163, 163, 163, 163, 164, 164, 164, 164, 
      165, 165, 165, 165, 166, 166, 166, 166, 167, 167, 167, 167, 167, 168, 168, 168, 
      168, 169, 169, 169, 169, 170, 170, 170, 170, 170, 171, 171, 171, 171, 172, 172, 
      172, 172, 172, 173, 173, 173, 173, 174, 174, 174, 174, 174, 175, 175, 175, 175, 
      176, 176, 176, 176, 176, 177, 177, 177, 177, 178, 178, 178, 178, 178, 179, 179, 
      179, 179, 179, 180, 180, 180, 180, 180, 181, 181, 181, 181, 181, 182, 182, 182, 
      182, 183, 183, 183, 183, 183, 184, 184, 184, 184, 184, 185, 185, 185, 185, 185, 
      186, 186, 186, 186, 186, 187, 187, 187, 187, 187, 187, 188, 188, 188, 188, 188, 
      189, 189, 189, 189, 189, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 
      192, 192, 192, 192, 192, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 197, 197, 197, 197, 197, 
      197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 200, 
      200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 202, 203, 203, 
      203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 205, 205, 205, 205, 205, 205, 
      206, 206, 206, 206, 206, 206, 206, 207, 207, 207, 207, 207, 207, 208, 208, 208, 
      208, 208, 208, 209, 209, 209, 209, 209, 209, 209, 210, 210, 210, 210, 210, 210, 
      211, 211, 211, 211, 211, 211, 211, 212, 212, 212, 212, 212, 212, 213, 213, 213, 
      213, 213, 213, 213, 214, 214, 214, 214, 214, 214, 214, 215, 215, 215, 215, 215, 
      215, 215, 216, 216, 216, 216, 216, 216, 216, 217, 217, 217, 217, 217, 217, 217, 
      218, 218, 218, 218, 218, 218, 218, 219, 219, 219, 219, 219, 219, 219, 220, 220, 
      220, 220, 220, 220, 220, 221, 221, 221, 221, 221, 221, 221, 221, 222, 222, 222, 
      222, 222, 222, 222, 223, 223, 223, 223, 223, 223, 223, 223, 224, 224, 224, 224, 
      224, 224, 224, 225, 225, 225, 225, 225, 225, 225, 225, 226, 226, 226, 226, 226, 
      226, 226, 226, 227, 227, 227, 227, 227, 227, 227, 227, 228, 228, 228, 228, 228, 
      228, 228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 230, 230, 230, 
      230, 230, 230, 231, 231, 231, 231, 231, 231, 231, 231, 232, 232, 232, 232, 232, 
      232, 232, 232, 233, 233, 233, 233, 233, 233, 233, 233, 233, 234, 234, 234, 234, 
      234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 235, 236, 236, 236, 
      236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 237, 237, 237, 237, 238, 
      238, 238, 238, 238, 238, 238, 238, 238, 239, 239, 239, 239, 239, 239, 239, 239, 
      239, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 241, 241, 241, 241, 241, 
      241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 
      243, 243, 243, 243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244, 244, 
      244, 245, 245, 245, 245, 245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 
      246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 248, 
      248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 249, 249, 
      249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 251, 
      251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 
      252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 254, 
      254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255
   },
   0 /* Linear */
},
/* Outdoor Preview Gamma Table */
{
   {
      2, 2, 3, 4, 4, 5, 6, 6, 8, 8, 9, 10, 10, 11, 12, 12, 
      13, 14, 15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 
      26, 27, 28, 29, 30, 30, 31, 32, 33, 34, 35, 35, 36, 37, 38, 39, 
      39, 40, 41, 42, 42, 43, 44, 44, 45, 46, 47, 48, 49, 49, 50, 51, 
      51, 52, 53, 53, 54, 55, 55, 56, 56, 57, 58, 59, 59, 60, 61, 61, 
      62, 63, 63, 64, 64, 65, 66, 66, 67, 67, 68, 69, 69, 70, 70, 71, 
      71, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 80, 
      80, 81, 81, 82, 82, 83, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 
      87, 88, 88, 89, 89, 90, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 
      95, 95, 96, 96, 96, 97, 97, 98, 98, 99, 99, 99, 100, 100, 100, 101, 
      101, 102, 102, 102, 103, 103, 104, 104, 105, 105, 105, 106, 106, 107, 107, 107, 
      108, 108, 109, 109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 114, 
      114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 119, 119, 
      119, 120, 120, 121, 121, 121, 122, 122, 123, 123, 123, 124, 124, 124, 124, 125, 
      125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 
      130, 131, 131, 132, 132, 132, 132, 133, 133, 133, 134, 134, 134, 134, 135, 135, 
      135, 136, 136, 136, 136, 137, 137, 137, 138, 138, 138, 139, 139, 139, 140, 140, 
      140, 141, 141, 141, 142, 142, 142, 142, 143, 143, 143, 143, 144, 144, 144, 144, 
      145, 145, 145, 145, 146, 146, 146, 146, 147, 147, 147, 148, 148, 148, 148, 149, 
      149, 149, 150, 150, 150, 150, 151, 151, 151, 152, 152, 152, 152, 153, 153, 153, 
      153, 154, 154, 154, 154, 155, 155, 155, 155, 155, 156, 156, 156, 156, 157, 157, 
      157, 157, 157, 158, 158, 158, 158, 159, 159, 159, 159, 160, 160, 160, 160, 161, 
      161, 161, 161, 162, 162, 162, 162, 163, 163, 163, 163, 163, 164, 164, 164, 164, 
      165, 165, 165, 165, 166, 166, 166, 166, 167, 167, 167, 167, 167, 168, 168, 168, 
      168, 168, 169, 169, 169, 169, 169, 170, 170, 170, 170, 170, 171, 171, 171, 171, 
      171, 172, 172, 172, 172, 172, 173, 173, 173, 173, 174, 174, 174, 174, 174, 174, 
      175, 175, 175, 175, 175, 176, 176, 176, 176, 176, 177, 177, 177, 177, 177, 178, 
      178, 178, 178, 178, 178, 179, 179, 179, 179, 179, 180, 180, 180, 180, 180, 180, 
      181, 181, 181, 181, 181, 182, 182, 182, 182, 182, 183, 183, 183, 183, 183, 184, 
      184, 184, 184, 184, 185, 185, 185, 185, 185, 185, 186, 186, 186, 186, 186, 187, 
      187, 187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 189, 189, 189, 189, 189, 
      189, 190, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 191, 191, 192, 192, 
      192, 192, 192, 192, 193, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 194, 
      195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 196, 197, 197, 197, 197, 
      197, 197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 
      200, 200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 202, 203, 
      203, 203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 204, 205, 205, 205, 205, 
      205, 205, 205, 205, 206, 206, 206, 206, 206, 206, 206, 206, 207, 207, 207, 207, 
      207, 207, 207, 208, 208, 208, 208, 208, 208, 208, 209, 209, 209, 209, 209, 209, 
      210, 210, 210, 210, 210, 210, 211, 211, 211, 211, 211, 211, 211, 212, 212, 212, 
      212, 212, 212, 213, 213, 213, 213, 213, 213, 213, 213, 214, 214, 214, 214, 214, 
      214, 214, 214, 215, 215, 215, 215, 215, 215, 215, 216, 216, 216, 216, 216, 216, 
      216, 216, 217, 217, 217, 217, 217, 217, 217, 218, 218, 218, 218, 218, 218, 218, 
      218, 219, 219, 219, 219, 219, 219, 219, 219, 220, 220, 220, 220, 220, 220, 220, 
      220, 221, 221, 221, 221, 221, 221, 221, 221, 222, 222, 222, 222, 222, 222, 222, 
      222, 223, 223, 223, 223, 223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 
      224, 224, 225, 225, 225, 225, 225, 225, 225, 225, 226, 226, 226, 226, 226, 226, 
      226, 226, 227, 227, 227, 227, 227, 227, 227, 227, 228, 228, 228, 228, 228, 228, 
      228, 228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 230, 230, 
      230, 230, 230, 230, 230, 231, 231, 231, 231, 231, 231, 231, 231, 231, 232, 232, 
      232, 232, 232, 232, 232, 232, 232, 232, 233, 233, 233, 233, 233, 233, 233, 233, 
      233, 234, 234, 234, 234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 
      235, 235, 236, 236, 236, 236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 
      237, 237, 237, 237, 238, 238, 238, 238, 238, 238, 238, 238, 238, 239, 239, 239, 
      239, 239, 239, 239, 239, 239, 240, 240, 240, 240, 240, 240, 240, 240, 240, 241, 
      241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 242, 242, 242, 242, 242, 242, 
      242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 243, 243, 243, 243, 243, 244, 
      244, 244, 244, 244, 244, 244, 244, 244, 245, 245, 245, 245, 245, 245, 245, 245, 
      245, 245, 246, 246, 246, 246, 246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 
      247, 247, 247, 247, 248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 
      249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 
      250, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 
      252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 
      253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255
   },
   0 /* Linear */
},
/* Backlight Preview Gamma Table */
{
   {
      0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5, 
      6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 
      15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 24, 25, 26, 27, 
      28, 29, 29, 30, 31, 32, 33, 33, 34, 35, 36, 37, 38, 38, 39, 40, 
      41, 41, 42, 43, 44, 44, 45, 46, 46, 47, 48, 49, 49, 50, 51, 51, 
      52, 53, 53, 54, 54, 55, 56, 56, 57, 57, 58, 59, 59, 60, 60, 61, 
      61, 62, 63, 63, 64, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 70, 
      70, 71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 75, 76, 76, 77, 77, 
      78, 78, 79, 79, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 
      86, 86, 87, 87, 87, 88, 88, 89, 89, 90, 90, 91, 91, 92, 92, 93, 
      93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 97, 98, 98, 99, 99, 100, 
      100, 101, 101, 101, 102, 102, 103, 103, 104, 104, 104, 105, 105, 106, 106, 107, 
      107, 107, 108, 108, 109, 109, 109, 110, 110, 111, 111, 111, 112, 112, 113, 113, 
      113, 114, 114, 115, 115, 115, 116, 116, 117, 117, 117, 118, 118, 118, 119, 119, 
      120, 120, 120, 121, 121, 121, 122, 122, 123, 123, 123, 124, 124, 124, 125, 125, 
      125, 126, 126, 127, 127, 127, 128, 128, 128, 129, 129, 129, 130, 130, 130, 131, 
      131, 131, 132, 132, 132, 133, 133, 133, 134, 134, 134, 135, 135, 135, 136, 136, 
      136, 137, 137, 137, 138, 138, 138, 138, 139, 139, 139, 140, 140, 140, 141, 141, 
      141, 142, 142, 142, 142, 143, 143, 143, 144, 144, 144, 145, 145, 145, 145, 146, 
      146, 146, 147, 147, 147, 147, 148, 148, 148, 149, 149, 149, 149, 150, 150, 150, 
      150, 151, 151, 151, 152, 152, 152, 152, 153, 153, 153, 153, 154, 154, 154, 154, 
      155, 155, 155, 156, 156, 156, 156, 157, 157, 157, 157, 158, 158, 158, 158, 159, 
      159, 159, 159, 160, 160, 160, 160, 161, 161, 161, 161, 161, 162, 162, 162, 162, 
      163, 163, 163, 163, 164, 164, 164, 164, 165, 165, 165, 165, 165, 166, 166, 166, 
      166, 167, 167, 167, 167, 167, 168, 168, 168, 168, 169, 169, 169, 169, 169, 170, 
      170, 170, 170, 171, 171, 171, 171, 171, 172, 172, 172, 172, 172, 173, 173, 173, 
      173, 173, 174, 174, 174, 174, 175, 175, 175, 175, 175, 176, 176, 176, 176, 176, 
      177, 177, 177, 177, 177, 178, 178, 178, 178, 178, 179, 179, 179, 179, 179, 179, 
      180, 180, 180, 180, 180, 181, 181, 181, 181, 181, 182, 182, 182, 182, 182, 183, 
      183, 183, 183, 183, 184, 184, 184, 184, 184, 184, 185, 185, 185, 185, 185, 186, 
      186, 186, 186, 186, 186, 187, 187, 187, 187, 187, 188, 188, 188, 188, 188, 188, 
      189, 189, 189, 189, 189, 190, 190, 190, 190, 190, 190, 191, 191, 191, 191, 191, 
      192, 192, 192, 192, 192, 192, 193, 193, 193, 193, 193, 194, 194, 194, 194, 194, 
      194, 195, 195, 195, 195, 195, 195, 196, 196, 196, 196, 196, 197, 197, 197, 197, 
      197, 197, 198, 198, 198, 198, 198, 198, 199, 199, 199, 199, 199, 200, 200, 200, 
      200, 200, 200, 201, 201, 201, 201, 201, 201, 202, 202, 202, 202, 202, 202, 203, 
      203, 203, 203, 203, 203, 204, 204, 204, 204, 204, 204, 205, 205, 205, 205, 205, 
      205, 206, 206, 206, 206, 206, 206, 207, 207, 207, 207, 207, 207, 208, 208, 208, 
      208, 208, 208, 209, 209, 209, 209, 209, 209, 209, 210, 210, 210, 210, 210, 210, 
      211, 211, 211, 211, 211, 211, 212, 212, 212, 212, 212, 212, 212, 213, 213, 213, 
      213, 213, 213, 214, 214, 214, 214, 214, 214, 214, 215, 215, 215, 215, 215, 215, 
      215, 216, 216, 216, 216, 216, 216, 217, 217, 217, 217, 217, 217, 217, 218, 218, 
      218, 218, 218, 218, 218, 219, 219, 219, 219, 219, 219, 219, 220, 220, 220, 220, 
      220, 220, 220, 220, 221, 221, 221, 221, 221, 221, 221, 222, 222, 222, 222, 222, 
      222, 222, 223, 223, 223, 223, 223, 223, 223, 223, 224, 224, 224, 224, 224, 224, 
      224, 224, 225, 225, 225, 225, 225, 225, 225, 225, 226, 226, 226, 226, 226, 226, 
      226, 226, 227, 227, 227, 227, 227, 227, 227, 227, 228, 228, 228, 228, 228, 228, 
      228, 228, 229, 229, 229, 229, 229, 229, 229, 229, 230, 230, 230, 230, 230, 230, 
      230, 230, 230, 231, 231, 231, 231, 231, 231, 231, 231, 231, 232, 232, 232, 232, 
      232, 232, 232, 232, 232, 233, 233, 233, 233, 233, 233, 233, 233, 233, 234, 234, 
      234, 234, 234, 234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 235, 
      235, 236, 236, 236, 236, 236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237, 
      237, 237, 237, 237, 237, 238, 238, 238, 238, 238, 238, 238, 238, 238, 238, 239, 
      239, 239, 239, 239, 239, 239, 239, 239, 239, 239, 240, 240, 240, 240, 240, 240, 
      240, 240, 240, 240, 241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 241, 242, 
      242, 242, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243, 
      243, 243, 243, 243, 244, 244, 244, 244, 244, 244, 244, 244, 244, 244, 244, 245, 
      245, 245, 245, 245, 245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 
      246, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 
      248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 248, 249, 249, 249, 249, 249, 
      249, 249, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 
      250, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 251, 252, 252, 252, 252, 
      252, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 
      253, 254, 254, 254, 254, 254, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255
   },
   0 /* Linear */
},
128, /* Solarize Gamma Reflection Point */
0, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
0, /* Max Black Increase Snapshot */
/* Black level offset */
{253, 253},
/* Four Channel Normal Light Black-Level */
{252, 253, 254, 254},
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
0, /* Max Black Increase Preview */
/* Black level offset */
{253, 253},
/* Four Channel Preview Normal Light Black-Level */
{252, 253, 254, 254},
/* Snapshot Black-Level Value */
253,
/* Preview Black-Level Value */
253,
0, /* Use Gain for control */
9.000000f, /* Gain */
348, /* Exposure Index */
{
   /* TL84 VFE Lens Rolloff */
   /* Red Correction Percent: 100 */
   /* Green Correction Percent: 100 */
   /* Blue Correction Percent: 100 */
   {
      /* Image Center x */
      972,
      /* Image Center y */
      793,
      /* Image Width */
      2064,
      /* Image Height */
      1542,
      /* Number of Intervals */
      32,
      /* Radius square table */
      {
         0, 1936, 12100, 17424, 23716, 39204, 48400, 58564,
         81796, 94864, 123904, 139876, 174724, 256036, 302500, 407044,
         435600, 559504, 627264, 698896, 853776, 937024, 980100, 1162084,
         1258884, 1359556, 1411344, 1517824, 1572516, 1628176, 1684804, 1742400
      },
      /* Red correction table */
      {
         1.000000, 1.001514, 1.005968, 1.009038, 1.012805, 1.020388, 1.025333, 1.031104,
         1.044443, 1.051878, 1.067828, 1.076331, 1.094932, 1.140134, 1.165602, 1.221822,
         1.237306, 1.303093, 1.336780, 1.372768, 1.447349, 1.485604, 1.504237, 1.580366,
         1.623589, 1.671634, 1.696897, 1.747170, 1.778590, 1.820719, 1.869613, 1.924034
      },
      /* Green correction table */
      {
         1.000000, 1.006303, 1.011452, 1.014257, 1.017802, 1.025462, 1.029952, 1.035415,
         1.047649, 1.054196, 1.068401, 1.076146, 1.093099, 1.133074, 1.155768, 1.204415,
         1.217660, 1.273347, 1.302116, 1.331168, 1.390038, 1.419800, 1.434363, 1.492090,
         1.526772, 1.567755, 1.588623, 1.627582, 1.655888, 1.697378, 1.754157, 1.820331
      },
      /* Blue correction table */
      {
         1.000000, 1.008054, 1.015202, 1.017512, 1.021057, 1.028648, 1.033172, 1.038430,
         1.050633, 1.056785, 1.068751, 1.075626, 1.090934, 1.126184, 1.145337, 1.189216,
         1.200427, 1.249311, 1.274592, 1.302961, 1.359305, 1.388510, 1.402456, 1.458375,
         1.488588, 1.526371, 1.546457, 1.581396, 1.606549, 1.641974, 1.681284, 1.716827
      }
      },
   /* A VFE Lens Rolloff */
   /* Red Correction Percent: 100 */
   /* Green Correction Percent: 100 */
   /* Blue Correction Percent: 100 */
   {
      /* Image Center x */
      1026,
      /* Image Center y */
      795,
      /* Image Width */
      2064,
      /* Image Height */
      1542,
      /* Number of Intervals */
      32,
      /* Radius square table */
      {
         0, 1764, 11025, 15876, 21609, 35721, 44100, 63504,
         86436, 112896, 142884, 176400, 213444, 254016, 345744, 370881,
         480249, 540225, 603729, 741321, 815409, 893025, 1058841, 1102500,
         1192464, 1238769, 1334025, 1432809, 1483524, 1535121, 1587600, 1640961
      },
      /* Red correction table */
      {
         1.000000, 1.003633, 1.011324, 1.014859, 1.019112, 1.030160, 1.036650, 1.051165,
         1.068195, 1.088286, 1.110555, 1.135336, 1.162521, 1.193367, 1.264420, 1.283395,
         1.366503, 1.411255, 1.455218, 1.549788, 1.599369, 1.647621, 1.739193, 1.762012,
         1.805872, 1.827700, 1.872036, 1.928013, 1.963636, 2.005422, 2.050820, 2.109509
      },
      /* Green correction table */
      {
         1.000000, 1.003617, 1.009144, 1.011836, 1.015417, 1.024394, 1.029518, 1.041061,
         1.054518, 1.069286, 1.085296, 1.103321, 1.123497, 1.145295, 1.194709, 1.207954,
         1.264365, 1.293653, 1.323401, 1.383588, 1.414657, 1.444957, 1.502725, 1.517337,
         1.546634, 1.562195, 1.598259, 1.642938, 1.672931, 1.709419, 1.753621, 1.811372
      },
      /* Blue correction table */
      {
         1.000000, 1.000072, 1.000450, 1.003342, 1.007545, 1.016248, 1.021392, 1.032093,
         1.043809, 1.055440, 1.070645, 1.085971, 1.103846, 1.121168, 1.162650, 1.173751,
         1.221169, 1.246379, 1.270863, 1.325242, 1.354031, 1.380433, 1.432949, 1.444822,
         1.469458, 1.482975, 1.515746, 1.556099, 1.581449, 1.611463, 1.646665, 1.677372
      }
      },
   /* D65 VFE Lens Rolloff */
   /* Red Correction Percent: 100 */
   /* Green Correction Percent: 100 */
   /* Blue Correction Percent: 100 */
   {
      /* Image Center x */
      1036,
      /* Image Center y */
      765,
      /* Image Width */
      2064,
      /* Image Height */
      1542,
      /* Number of Intervals */
      32,
      /* Radius square table */
      {
         0, 1764, 7056, 15876, 28224, 44100, 63504, 74529,
         99225, 112896, 142884, 176400, 213444, 254016, 345744, 370881,
         480249, 540225, 670761, 705600, 777924, 815409, 893025, 1058841,
         1102500, 1192464, 1238769, 1285956, 1334025, 1432809, 1535121, 1587600
      },
      /* Red correction table */
      {
         1.000000, 1.003542, 1.006556, 1.011625, 1.019859, 1.030427, 1.043359, 1.050609,
         1.066603, 1.075567, 1.094925, 1.115833, 1.139923, 1.167377, 1.229253, 1.246296,
         1.322407, 1.363817, 1.451784, 1.475065, 1.523364, 1.548145, 1.596784, 1.698620,
         1.728589, 1.792263, 1.827290, 1.866533, 1.910820, 2.016112, 2.147240, 2.218427
      },
      /* Green correction table */
      {
         1.000000, 1.003778, 1.006193, 1.011152, 1.017597, 1.026561, 1.037228, 1.042971,
         1.055923, 1.063054, 1.078102, 1.095126, 1.114057, 1.135605, 1.183271, 1.196296,
         1.253166, 1.284768, 1.349919, 1.367331, 1.403259, 1.421587, 1.458268, 1.534550,
         1.557998, 1.609414, 1.638533, 1.671230, 1.708377, 1.799242, 1.914372, 1.972280
      },
      /* Blue correction table */
      {
         1.000000, 1.003051, 1.005806, 1.011329, 1.018100, 1.026529, 1.036126, 1.040947,
         1.051927, 1.058048, 1.071240, 1.085855, 1.101884, 1.120414, 1.160529, 1.171549,
         1.219675, 1.245596, 1.304286, 1.320596, 1.354249, 1.371552, 1.405880, 1.479132,
         1.501064, 1.552129, 1.580386, 1.612537, 1.649281, 1.739908, 1.842355, 1.902735
      }
      },
   /* Low-Light VFE Lens Rolloff */
   /* Red Correction Percent: 100 */
   /* Green Correction Percent: 100 */
   /* Blue Correction Percent: 100 */
   {
      /* Image Center x */
      972,
      /* Image Center y */
      793,
      /* Image Width */
      2064,
      /* Image Height */
      1542,
      /* Number of Intervals */
      32,
      /* Radius square table */
      {
         0, 1936, 12100, 17424, 23716, 39204, 48400, 58564,
         81796, 94864, 123904, 139876, 174724, 256036, 302500, 407044,
         435600, 559504, 627264, 698896, 853776, 937024, 980100, 1162084,
         1258884, 1359556, 1411344, 1517824, 1572516, 1628176, 1684804, 1742400
      },
      /* Red correction table */
      {
         1.000000, 1.001230, 1.004849, 1.007343, 1.010404, 1.016565, 1.020583, 1.025272,
         1.036110, 1.042151, 1.055110, 1.062019, 1.077132, 1.113859, 1.134552, 1.180230,
         1.192811, 1.246263, 1.273634, 1.302874, 1.363471, 1.394553, 1.409693, 1.471547,
         1.506666, 1.545703, 1.566229, 1.607075, 1.632605, 1.666834, 1.706561, 1.750778
      },
      /* Green correction table */
      {
         1.000000, 1.005121, 1.009305, 1.011584, 1.014465, 1.020688, 1.024336, 1.028774,
         1.038715, 1.044034, 1.055576, 1.061868, 1.075643, 1.108123, 1.126561, 1.166087,
         1.176849, 1.222094, 1.245470, 1.269074, 1.316906, 1.341088, 1.352920, 1.399823,
         1.428002, 1.461301, 1.478256, 1.509910, 1.532909, 1.566619, 1.612752, 1.666519
      },
      /* Blue correction table */
      {
         1.000000, 1.006544, 1.012351, 1.014228, 1.017109, 1.023276, 1.026953, 1.031224,
         1.041139, 1.046138, 1.055860, 1.061446, 1.073884, 1.102524, 1.118086, 1.153738,
         1.162847, 1.202565, 1.223106, 1.246156, 1.291935, 1.315664, 1.326996, 1.372430,
         1.396978, 1.427677, 1.443997, 1.472384, 1.492821, 1.521604, 1.553543, 1.582422
      }
      },
   /* Preview VFE Lens Rolloff */
   /* Red Correction Percent: 100 */
   /* Green Correction Percent: 100 */
   /* Blue Correction Percent: 100 */
   {
      /* Image Center x */
      972,
      /* Image Center y */
      793,
      /* Image Width */
      2064,
      /* Image Height */
      1542,
      /* Number of Intervals */
      32,
      /* Radius square table */
      {
         0, 1936, 12100, 17424, 23716, 39204, 48400, 58564,
         81796, 94864, 123904, 139876, 174724, 256036, 302500, 407044,
         435600, 559504, 627264, 698896, 853776, 937024, 980100, 1162084,
         1258884, 1359556, 1411344, 1517824, 1572516, 1628176, 1684804, 1742400
      },
      /* Red correction table */
      {
         1.000000, 1.001514, 1.005968, 1.009038, 1.012805, 1.020388, 1.025333, 1.031104,
         1.044443, 1.051878, 1.067828, 1.076331, 1.094932, 1.140134, 1.165602, 1.221822,
         1.237306, 1.303093, 1.336780, 1.372768, 1.447349, 1.485604, 1.504237, 1.580366,
         1.623589, 1.671634, 1.696897, 1.747170, 1.778590, 1.820719, 1.869613, 1.924034
      },
      /* Green correction table */
      {
         1.000000, 1.006303, 1.011452, 1.014257, 1.017802, 1.025462, 1.029952, 1.035415,
         1.047649, 1.054196, 1.068401, 1.076146, 1.093099, 1.133074, 1.155768, 1.204415,
         1.217660, 1.273347, 1.302116, 1.331168, 1.390038, 1.419800, 1.434363, 1.492090,
         1.526772, 1.567755, 1.588623, 1.627582, 1.655888, 1.697378, 1.754157, 1.820331
      },
      /* Blue correction table */
      {
         1.000000, 1.008054, 1.015202, 1.017512, 1.021057, 1.028648, 1.033172, 1.038430,
         1.050633, 1.056785, 1.068751, 1.075626, 1.090934, 1.126184, 1.145337, 1.189216,
         1.200427, 1.249311, 1.274592, 1.302961, 1.359305, 1.388510, 1.402456, 1.458375,
         1.488588, 1.526371, 1.546457, 1.581396, 1.606549, 1.641974, 1.681284, 1.716827
      }
      },
},
{
   /* TL84 Mesh Lens Rolloff */
   {
      221,
      /* R Gain */
      {
         1.332603f, 1.332500f, 1.307317f, 1.283333f, 1.238636f, 1.217778f, 1.197826f, 1.197826f, 1.197826f, 1.197826f, 1.217778f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.386842f, 1.386951f,
         1.358974f, 1.307317f, 1.283333f, 1.238636f, 1.197826f, 1.178723f, 1.160417f, 1.142857f, 1.142857f, 1.160417f, 1.178723f, 1.197826f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.416216f,
         1.332500f, 1.283333f, 1.238636f, 1.197826f, 1.160417f, 1.126000f, 1.109804f, 1.094231f, 1.094231f, 1.109804f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.283333f, 1.332500f, 1.358974f,
         1.307317f, 1.260465f, 1.197826f, 1.160417f, 1.126000f, 1.094231f, 1.064815f, 1.050909f, 1.050909f, 1.064815f, 1.094231f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.307317f, 1.332500f,
         1.283333f, 1.238636f, 1.178723f, 1.126000f, 1.094231f, 1.050909f, 1.037500f, 1.024561f, 1.024561f, 1.037500f, 1.050909f, 1.094231f, 1.126000f, 1.178723f, 1.217778f, 1.283333f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.037500f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.283333f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.024561f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.260465f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.037500f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.260465f, 1.307317f,
         1.283333f, 1.238636f, 1.178723f, 1.126000f, 1.094231f, 1.050909f, 1.037500f, 1.024561f, 1.024561f, 1.037500f, 1.050909f, 1.094231f, 1.126000f, 1.178723f, 1.238636f, 1.283333f, 1.332500f,
         1.307317f, 1.238636f, 1.197826f, 1.142857f, 1.109804f, 1.079245f, 1.064815f, 1.050909f, 1.050909f, 1.064815f, 1.079245f, 1.109804f, 1.160417f, 1.197826f, 1.260465f, 1.307317f, 1.332500f,
         1.332500f, 1.283333f, 1.238636f, 1.197826f, 1.160417f, 1.126000f, 1.109804f, 1.094231f, 1.094231f, 1.109804f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.283333f, 1.332500f, 1.358974f,
         1.358974f, 1.307317f, 1.260465f, 1.238636f, 1.197826f, 1.160417f, 1.142857f, 1.142857f, 1.142857f, 1.160417f, 1.178723f, 1.197826f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.416216f,
         1.332603f, 1.332500f, 1.307317f, 1.260465f, 1.238636f, 1.197826f, 1.178723f, 1.178723f, 1.178723f, 1.197826f, 1.217778f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.386842f, 1.386951f
      },
      /* GR Gain */
      {
         1.350105f, 1.357143f, 1.315686f, 1.277359f, 1.259259f, 1.241818f, 1.208772f, 1.208772f, 1.208772f, 1.225000f, 1.225000f, 1.259259f, 1.296154f, 1.315686f, 1.357143f, 1.402128f, 1.386822f,
         1.379167f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.163333f, 1.149180f, 1.163333f, 1.177966f, 1.208772f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 1.402128f,
         1.315686f, 1.277359f, 1.241818f, 1.193103f, 1.163333f, 1.135484f, 1.122222f, 1.109375f, 1.109375f, 1.122222f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.296154f, 1.315686f, 1.357143f,
         1.296154f, 1.259259f, 1.208772f, 1.163333f, 1.135484f, 1.096923f, 1.073134f, 1.061765f, 1.061765f, 1.073134f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.259259f, 1.296154f, 1.336000f,
         1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.096923f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.040000f, 1.061765f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.296154f,
         1.277359f, 1.225000f, 1.163333f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.009589f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.208772f, 1.163333f, 1.109375f, 1.073134f, 1.040000f, 1.019444f, 1.000000f, 1.000000f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.208772f, 1.163333f, 1.109375f, 1.073134f, 1.040000f, 1.019444f, 1.009589f, 1.009589f, 1.019444f, 1.050725f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.225000f, 1.163333f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.029577f, 1.029577f, 1.040000f, 1.061765f, 1.084849f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.296154f,
         1.277359f, 1.241818f, 1.193103f, 1.149180f, 1.109375f, 1.084849f, 1.061765f, 1.050725f, 1.061765f, 1.061765f, 1.084849f, 1.109375f, 1.149180f, 1.193103f, 1.241818f, 1.296154f, 1.315686f,
         1.296154f, 1.259259f, 1.225000f, 1.177966f, 1.149180f, 1.122222f, 1.096923f, 1.084849f, 1.084849f, 1.096923f, 1.122222f, 1.149180f, 1.193103f, 1.225000f, 1.259259f, 1.296154f, 1.336000f,
         1.357143f, 1.296154f, 1.259259f, 1.225000f, 1.193103f, 1.163333f, 1.135484f, 1.135484f, 1.135484f, 1.149180f, 1.163333f, 1.193103f, 1.225000f, 1.259259f, 1.296154f, 1.336000f, 1.379167f,
         1.329242f, 1.336000f, 1.277359f, 1.259259f, 1.225000f, 1.208772f, 1.177966f, 1.163333f, 1.177966f, 1.177966f, 1.193103f, 1.225000f, 1.259259f, 1.277359f, 1.315686f, 1.357143f, 1.357249f
      },
      /* GB Gain */
      {
         1.350105f, 1.357143f, 1.315686f, 1.296154f, 1.259259f, 1.241818f, 1.208772f, 1.208772f, 1.208772f, 1.225000f, 1.241818f, 1.259259f, 1.277359f, 1.315686f, 1.357143f, 1.402128f, 1.386822f, 
         1.379167f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.163333f, 1.163333f, 1.163333f, 1.177966f, 1.208772f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 1.402128f, 
         1.336000f, 1.296154f, 1.259259f, 1.208772f, 1.177966f, 1.149180f, 1.122222f, 1.109375f, 1.109375f, 1.122222f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 
         1.315686f, 1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.109375f, 1.084849f, 1.073134f, 1.073134f, 1.084849f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.259259f, 1.296154f, 1.336000f, 
         1.296154f, 1.259259f, 1.193103f, 1.149180f, 1.109375f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.050725f, 1.073134f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.315686f, 
         1.277359f, 1.241818f, 1.177966f, 1.135484f, 1.096923f, 1.061765f, 1.040000f, 1.019444f, 1.019444f, 1.029577f, 1.050725f, 1.084849f, 1.122222f, 1.163333f, 1.225000f, 1.277359f, 1.315686f, 
         1.277359f, 1.225000f, 1.177966f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.009589f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.225000f, 1.259259f, 1.315686f, 
         1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.019444f, 1.029577f, 1.050725f, 1.084849f, 1.122222f, 1.163333f, 1.225000f, 1.277359f, 1.315686f, 
         1.296154f, 1.241818f, 1.193103f, 1.149180f, 1.109375f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.050725f, 1.073134f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.315686f, 
         1.296154f, 1.259259f, 1.225000f, 1.177966f, 1.135484f, 1.096923f, 1.073134f, 1.073134f, 1.073134f, 1.073134f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.241818f, 1.296154f, 1.336000f, 
         1.336000f, 1.296154f, 1.259259f, 1.208772f, 1.177966f, 1.135484f, 1.122222f, 1.109375f, 1.109375f, 1.109375f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 
         1.379167f, 1.336000f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.149180f, 1.149180f, 1.163333f, 1.177966f, 1.208772f, 1.225000f, 1.259259f, 1.296154f, 1.336000f, 1.402128f, 
         1.357249f, 1.357143f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.193103f, 1.193103f, 1.193103f, 1.193103f, 1.208772f, 1.225000f, 1.259259f, 1.277359f, 1.315686f, 1.379167f, 1.371831f
      },
      /* B Gain */
      {
         1.200090f, 1.210000f, 1.180645f, 1.153125f, 1.153125f, 1.127273f, 1.102941f, 1.102941f, 1.102941f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.210000f, 1.220317f, 
         1.210000f, 1.180645f, 1.153125f, 1.153125f, 1.127273f, 1.102941f, 1.102941f, 1.080000f, 1.080000f, 1.080000f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f, 
         1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.058333f, 1.058333f, 1.058333f, 1.080000f, 1.080000f, 1.127273f, 1.153125f, 1.180645f, 1.180645f, 1.210000f, 
         1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.037838f, 1.037838f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.180645f, 
         1.153125f, 1.153125f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.018421f, 1.018421f, 1.018421f, 1.018421f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.180645f, 1.180645f, 
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.018421f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.037838f, 1.058333f, 1.102941f, 1.127273f, 1.180645f, 1.180645f, 
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.037838f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.037838f, 1.058333f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.037838f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.018421f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.180645f, 1.180645f, 
         1.180645f, 1.153125f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.018421f, 1.018421f, 1.018421f, 1.018421f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.153125f, 1.180645f, 1.180645f, 
         1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.037838f, 1.037838f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.180645f, 
         1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.058333f, 1.058333f, 1.058333f, 1.080000f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 
         1.241379f, 1.180645f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.080000f, 1.080000f, 1.080000f, 1.102941f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f, 
         1.210091f, 1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.127273f, 1.102941f, 1.102941f, 1.102941f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f, 1.230775f
      }
      },
   /* A Mesh Lens Rolloff */
   {
      221,
      /* R Gain */
      {
         1.456895f, 1.466667f, 1.409756f, 1.383333f, 1.358140f, 1.311111f, 1.289130f, 1.268085f, 1.289130f, 1.289130f, 1.311111f, 1.358140f, 1.383333f, 1.437500f, 1.466667f, 1.497368f, 1.497488f, 
         1.497368f, 1.409756f, 1.383333f, 1.334091f, 1.289130f, 1.247917f, 1.228571f, 1.210000f, 1.210000f, 1.228571f, 1.247917f, 1.289130f, 1.334091f, 1.383333f, 1.437500f, 1.466667f, 1.529730f, 
         1.437500f, 1.383333f, 1.334091f, 1.268085f, 1.228571f, 1.192157f, 1.158491f, 1.142593f, 1.142593f, 1.158491f, 1.175000f, 1.228571f, 1.268085f, 1.334091f, 1.383333f, 1.437500f, 1.466667f, 
         1.409756f, 1.358140f, 1.289130f, 1.228571f, 1.175000f, 1.127273f, 1.098246f, 1.084483f, 1.084483f, 1.098246f, 1.127273f, 1.158491f, 1.228571f, 1.289130f, 1.358140f, 1.409756f, 1.437500f, 
         1.383333f, 1.334091f, 1.247917f, 1.192157f, 1.142593f, 1.084483f, 1.058333f, 1.033871f, 1.033871f, 1.058333f, 1.084483f, 1.127273f, 1.175000f, 1.247917f, 1.311111f, 1.383333f, 1.437500f, 
         1.383333f, 1.311111f, 1.228571f, 1.175000f, 1.112500f, 1.058333f, 1.033871f, 1.010937f, 1.010937f, 1.022222f, 1.058333f, 1.098246f, 1.158491f, 1.228571f, 1.311111f, 1.383333f, 1.437500f, 
         1.383333f, 1.311111f, 1.228571f, 1.158491f, 1.098246f, 1.045902f, 1.022222f, 1.000000f, 1.000000f, 1.022222f, 1.045902f, 1.098246f, 1.158491f, 1.210000f, 1.289130f, 1.358140f, 1.409756f, 
         1.383333f, 1.311111f, 1.228571f, 1.158491f, 1.112500f, 1.058333f, 1.022222f, 1.010937f, 1.010937f, 1.033871f, 1.058333f, 1.098246f, 1.158491f, 1.228571f, 1.311111f, 1.383333f, 1.409756f, 
         1.383333f, 1.334091f, 1.247917f, 1.192157f, 1.127273f, 1.084483f, 1.045902f, 1.033871f, 1.033871f, 1.045902f, 1.084483f, 1.127273f, 1.192157f, 1.247917f, 1.311111f, 1.383333f, 1.437500f, 
         1.409756f, 1.358140f, 1.289130f, 1.228571f, 1.175000f, 1.127273f, 1.098246f, 1.071186f, 1.084483f, 1.098246f, 1.127273f, 1.158491f, 1.228571f, 1.289130f, 1.358140f, 1.409756f, 1.466667f, 
         1.437500f, 1.383333f, 1.334091f, 1.268085f, 1.228571f, 1.175000f, 1.142593f, 1.127273f, 1.127273f, 1.142593f, 1.175000f, 1.210000f, 1.268085f, 1.334091f, 1.409756f, 1.437500f, 1.497368f, 
         1.466667f, 1.409756f, 1.383333f, 1.334091f, 1.289130f, 1.247917f, 1.210000f, 1.210000f, 1.210000f, 1.228571f, 1.247917f, 1.289130f, 1.334091f, 1.383333f, 1.437500f, 1.466667f, 1.529730f, 
         1.447173f, 1.466667f, 1.409756f, 1.383333f, 1.334091f, 1.289130f, 1.268085f, 1.268085f, 1.268085f, 1.289130f, 1.311111f, 1.334091f, 1.383333f, 1.437500f, 1.466667f, 1.529730f, 1.508085f
      },
      /* GR Gain */
      {
         1.350105f, 1.350000f, 1.328571f, 1.308000f, 1.269231f, 1.250943f, 1.233333f, 1.216364f, 1.216364f, 1.233333f, 1.250943f, 1.288235f, 1.308000f, 1.328571f, 1.350000f, 1.395652f, 1.387879f, 
         1.372340f, 1.328571f, 1.288235f, 1.250943f, 1.216364f, 1.200000f, 1.168965f, 1.168965f, 1.168965f, 1.168965f, 1.200000f, 1.216364f, 1.250943f, 1.288235f, 1.328571f, 1.372340f, 1.395652f, 
         1.328571f, 1.288235f, 1.250943f, 1.216364f, 1.168965f, 1.140000f, 1.126230f, 1.112903f, 1.112903f, 1.126230f, 1.140000f, 1.168965f, 1.216364f, 1.250943f, 1.308000f, 1.328571f, 1.372340f, 
         1.308000f, 1.269231f, 1.216364f, 1.168965f, 1.140000f, 1.100000f, 1.075385f, 1.063636f, 1.063636f, 1.075385f, 1.100000f, 1.126230f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 1.350000f, 
         1.288235f, 1.250943f, 1.200000f, 1.140000f, 1.100000f, 1.075385f, 1.041176f, 1.030435f, 1.030435f, 1.041176f, 1.063636f, 1.100000f, 1.140000f, 1.184211f, 1.233333f, 1.288235f, 1.308000f, 
         1.269231f, 1.233333f, 1.184211f, 1.126230f, 1.087500f, 1.052239f, 1.020000f, 1.009859f, 1.009859f, 1.020000f, 1.041176f, 1.075385f, 1.126230f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 
         1.269231f, 1.233333f, 1.168965f, 1.112903f, 1.075385f, 1.041176f, 1.009859f, 1.000000f, 1.000000f, 1.009859f, 1.030435f, 1.063636f, 1.112903f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 
         1.269231f, 1.216364f, 1.168965f, 1.126230f, 1.075385f, 1.041176f, 1.020000f, 1.000000f, 1.000000f, 1.020000f, 1.041176f, 1.075385f, 1.112903f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 
         1.269231f, 1.233333f, 1.184211f, 1.126230f, 1.087500f, 1.063636f, 1.041176f, 1.020000f, 1.020000f, 1.041176f, 1.063636f, 1.087500f, 1.140000f, 1.184211f, 1.233333f, 1.288235f, 1.308000f, 
         1.288235f, 1.250943f, 1.200000f, 1.154237f, 1.126230f, 1.087500f, 1.063636f, 1.052239f, 1.052239f, 1.063636f, 1.087500f, 1.112903f, 1.168965f, 1.200000f, 1.250943f, 1.308000f, 1.328571f, 
         1.308000f, 1.269231f, 1.233333f, 1.200000f, 1.168965f, 1.126230f, 1.112903f, 1.087500f, 1.100000f, 1.100000f, 1.126230f, 1.154237f, 1.200000f, 1.233333f, 1.288235f, 1.328571f, 1.350000f, 
         1.372340f, 1.308000f, 1.269231f, 1.233333f, 1.200000f, 1.168965f, 1.154237f, 1.140000f, 1.140000f, 1.154237f, 1.184211f, 1.200000f, 1.233333f, 1.269231f, 1.308000f, 1.350000f, 1.395652f, 
         1.335720f, 1.328571f, 1.288235f, 1.269231f, 1.233333f, 1.216364f, 1.200000f, 1.184211f, 1.184211f, 1.200000f, 1.216364f, 1.233333f, 1.269231f, 1.288235f, 1.328571f, 1.372340f, 1.372448f
      },
      /* GB Gain */
      {
         1.357448f, 1.372340f, 1.328571f, 1.308000f, 1.269231f, 1.250943f, 1.233333f, 1.233333f, 1.216364f, 1.233333f, 1.250943f, 1.288235f, 1.308000f, 1.328571f, 1.372340f, 1.420000f, 1.403760f, 
         1.372340f, 1.328571f, 1.308000f, 1.250943f, 1.216364f, 1.200000f, 1.168965f, 1.168965f, 1.168965f, 1.168965f, 1.200000f, 1.216364f, 1.250943f, 1.288235f, 1.328571f, 1.372340f, 1.420000f, 
         1.350000f, 1.308000f, 1.269231f, 1.216364f, 1.184211f, 1.154237f, 1.126230f, 1.112903f, 1.112903f, 1.126230f, 1.140000f, 1.168965f, 1.216364f, 1.250943f, 1.308000f, 1.350000f, 1.372340f, 
         1.308000f, 1.288235f, 1.233333f, 1.184211f, 1.140000f, 1.112903f, 1.087500f, 1.075385f, 1.075385f, 1.075385f, 1.100000f, 1.126230f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 1.350000f, 
         1.308000f, 1.269231f, 1.216364f, 1.154237f, 1.112903f, 1.075385f, 1.052239f, 1.041176f, 1.041176f, 1.052239f, 1.075385f, 1.100000f, 1.140000f, 1.200000f, 1.250943f, 1.288235f, 1.328571f, 
         1.308000f, 1.250943f, 1.200000f, 1.140000f, 1.100000f, 1.063636f, 1.030435f, 1.020000f, 1.020000f, 1.030435f, 1.052239f, 1.087500f, 1.126230f, 1.184211f, 1.233333f, 1.288235f, 1.328571f, 
         1.288235f, 1.250943f, 1.184211f, 1.140000f, 1.087500f, 1.052239f, 1.020000f, 1.009859f, 1.009859f, 1.020000f, 1.041176f, 1.075385f, 1.126230f, 1.168965f, 1.233333f, 1.288235f, 1.328571f, 
         1.308000f, 1.250943f, 1.184211f, 1.140000f, 1.087500f, 1.052239f, 1.030435f, 1.020000f, 1.020000f, 1.030435f, 1.052239f, 1.087500f, 1.126230f, 1.184211f, 1.233333f, 1.288235f, 1.328571f, 
         1.288235f, 1.250943f, 1.200000f, 1.154237f, 1.112903f, 1.075385f, 1.052239f, 1.041176f, 1.041176f, 1.052239f, 1.075385f, 1.100000f, 1.140000f, 1.200000f, 1.250943f, 1.288235f, 1.328571f, 
         1.328571f, 1.288235f, 1.233333f, 1.184211f, 1.140000f, 1.100000f, 1.075385f, 1.063636f, 1.063636f, 1.075385f, 1.100000f, 1.126230f, 1.168965f, 1.216364f, 1.269231f, 1.308000f, 1.350000f, 
         1.350000f, 1.308000f, 1.269231f, 1.216364f, 1.184211f, 1.140000f, 1.126230f, 1.112903f, 1.112903f, 1.112903f, 1.140000f, 1.168965f, 1.216364f, 1.250943f, 1.288235f, 1.328571f, 1.372340f, 
         1.395652f, 1.350000f, 1.308000f, 1.250943f, 1.216364f, 1.184211f, 1.168965f, 1.154237f, 1.154237f, 1.168965f, 1.184211f, 1.216364f, 1.250943f, 1.288235f, 1.328571f, 1.350000f, 1.395652f, 
         1.364895f, 1.350000f, 1.328571f, 1.288235f, 1.250943f, 1.216364f, 1.200000f, 1.200000f, 1.200000f, 1.200000f, 1.216364f, 1.250943f, 1.269231f, 1.308000f, 1.350000f, 1.372340f, 1.372448f
      },
      /* B Gain */
      {
         1.196431f, 1.207407f, 1.175000f, 1.144828f, 1.144828f, 1.116667f, 1.116667f, 1.116667f, 1.116667f, 1.116667f, 1.116667f, 1.144828f, 1.144828f, 1.175000f, 1.175000f, 1.207407f, 1.207498f, 
         1.207407f, 1.175000f, 1.144828f, 1.144828f, 1.116667f, 1.116667f, 1.090323f, 1.090323f, 1.090323f, 1.090323f, 1.116667f, 1.116667f, 1.144828f, 1.144828f, 1.175000f, 1.207407f, 1.207407f, 
         1.175000f, 1.175000f, 1.144828f, 1.116667f, 1.090323f, 1.090323f, 1.065625f, 1.065625f, 1.065625f, 1.065625f, 1.065625f, 1.090323f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 1.207407f, 
         1.175000f, 1.144828f, 1.116667f, 1.090323f, 1.065625f, 1.065625f, 1.042424f, 1.042424f, 1.042424f, 1.042424f, 1.042424f, 1.065625f, 1.090323f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 
         1.144828f, 1.144828f, 1.116667f, 1.090323f, 1.065625f, 1.042424f, 1.020588f, 1.020588f, 1.020588f, 1.020588f, 1.042424f, 1.042424f, 1.065625f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 
         1.144828f, 1.144828f, 1.116667f, 1.065625f, 1.042424f, 1.020588f, 1.020588f, 1.000000f, 1.000000f, 1.000000f, 1.020588f, 1.042424f, 1.065625f, 1.090323f, 1.116667f, 1.175000f, 1.175000f, 
         1.175000f, 1.144828f, 1.116667f, 1.065625f, 1.042424f, 1.020588f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.020588f, 1.042424f, 1.065625f, 1.090323f, 1.116667f, 1.175000f, 1.175000f, 
         1.175000f, 1.144828f, 1.090323f, 1.065625f, 1.042424f, 1.020588f, 1.020588f, 1.000000f, 1.000000f, 1.020588f, 1.020588f, 1.042424f, 1.065625f, 1.090323f, 1.144828f, 1.175000f, 1.175000f, 
         1.175000f, 1.144828f, 1.116667f, 1.090323f, 1.065625f, 1.042424f, 1.020588f, 1.020588f, 1.020588f, 1.020588f, 1.042424f, 1.065625f, 1.090323f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 
         1.175000f, 1.144828f, 1.116667f, 1.090323f, 1.090323f, 1.065625f, 1.042424f, 1.042424f, 1.042424f, 1.042424f, 1.042424f, 1.065625f, 1.090323f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 
         1.175000f, 1.175000f, 1.144828f, 1.116667f, 1.090323f, 1.090323f, 1.065625f, 1.065625f, 1.065625f, 1.065625f, 1.065625f, 1.090323f, 1.116667f, 1.144828f, 1.175000f, 1.175000f, 1.207407f, 
         1.207407f, 1.175000f, 1.175000f, 1.144828f, 1.116667f, 1.116667f, 1.090323f, 1.090323f, 1.090323f, 1.090323f, 1.090323f, 1.116667f, 1.144828f, 1.144828f, 1.175000f, 1.207407f, 1.207407f, 
         1.196431f, 1.207407f, 1.175000f, 1.144828f, 1.144828f, 1.116667f, 1.116667f, 1.090323f, 1.116667f, 1.116667f, 1.116667f, 1.144828f, 1.144828f, 1.175000f, 1.175000f, 1.207407f, 1.207498f
      }
      },
   /* D65 Mesh Lens Rolloff */
   {
      221,
      /* R Gain */
      {
         1.435828f, 1.459375f, 1.360000f, 1.330556f, 1.302703f, 1.251282f, 1.251282f, 1.227500f, 1.227500f, 1.227500f, 1.276316f, 1.276316f, 1.330556f, 1.360000f, 1.424242f, 1.496774f, 1.471696f,
         1.459375f, 1.391176f, 1.330556f, 1.276316f, 1.251282f, 1.204878f, 1.183333f, 1.183333f, 1.183333f, 1.183333f, 1.204878f, 1.227500f, 1.276316f, 1.330556f, 1.391176f, 1.424242f, 1.496774f,
         1.391176f, 1.330556f, 1.276316f, 1.227500f, 1.183333f, 1.162791f, 1.124444f, 1.106522f, 1.106522f, 1.124444f, 1.143182f, 1.183333f, 1.227500f, 1.276316f, 1.330556f, 1.391176f, 1.459375f,
         1.360000f, 1.302703f, 1.251282f, 1.204878f, 1.143182f, 1.106522f, 1.089362f, 1.057143f, 1.072917f, 1.072917f, 1.106522f, 1.143182f, 1.183333f, 1.227500f, 1.302703f, 1.360000f, 1.424242f,
         1.360000f, 1.276316f, 1.227500f, 1.162791f, 1.106522f, 1.072917f, 1.042000f, 1.027451f, 1.027451f, 1.042000f, 1.072917f, 1.106522f, 1.143182f, 1.204878f, 1.276316f, 1.330556f, 1.391176f,
         1.330556f, 1.276316f, 1.204878f, 1.143182f, 1.089362f, 1.057143f, 1.027451f, 1.000000f, 1.000000f, 1.013462f, 1.042000f, 1.072917f, 1.124444f, 1.183333f, 1.251282f, 1.330556f, 1.360000f,
         1.330556f, 1.251282f, 1.204878f, 1.143182f, 1.089362f, 1.042000f, 1.013462f, 1.000000f, 1.000000f, 1.013462f, 1.027451f, 1.072917f, 1.124444f, 1.183333f, 1.251282f, 1.330556f, 1.360000f,
         1.330556f, 1.276316f, 1.204878f, 1.143182f, 1.089362f, 1.042000f, 1.013462f, 1.000000f, 1.000000f, 1.013462f, 1.042000f, 1.072917f, 1.124444f, 1.183333f, 1.251282f, 1.330556f, 1.360000f,
         1.360000f, 1.276316f, 1.227500f, 1.162791f, 1.106522f, 1.072917f, 1.042000f, 1.027451f, 1.027451f, 1.042000f, 1.057143f, 1.106522f, 1.143182f, 1.204878f, 1.276316f, 1.330556f, 1.391176f,
         1.391176f, 1.302703f, 1.251282f, 1.183333f, 1.143182f, 1.106522f, 1.072917f, 1.057143f, 1.057143f, 1.072917f, 1.089362f, 1.124444f, 1.183333f, 1.227500f, 1.302703f, 1.360000f, 1.424242f,
         1.424242f, 1.360000f, 1.276316f, 1.227500f, 1.183333f, 1.143182f, 1.124444f, 1.106522f, 1.106522f, 1.124444f, 1.143182f, 1.183333f, 1.227500f, 1.276316f, 1.330556f, 1.391176f, 1.459375f,
         1.459375f, 1.391176f, 1.330556f, 1.276316f, 1.251282f, 1.204878f, 1.183333f, 1.162791f, 1.162791f, 1.183333f, 1.204878f, 1.227500f, 1.276316f, 1.330556f, 1.391176f, 1.459375f, 1.536667f,
         1.435828f, 1.459375f, 1.391176f, 1.330556f, 1.302703f, 1.251282f, 1.227500f, 1.227500f, 1.227500f, 1.227500f, 1.251282f, 1.302703f, 1.330556f, 1.391176f, 1.424242f, 1.496774f, 1.496894f
      },
      /* GR Gain */
      {
         1.463630f, 1.495833f, 1.383019f, 1.343636f, 1.289655f, 1.272881f, 1.240984f, 1.240984f, 1.240984f, 1.256667f, 1.272881f, 1.307018f, 1.343636f, 1.383019f, 1.448000f, 1.547826f, 1.512797f,
         1.521277f, 1.383019f, 1.325000f, 1.272881f, 1.240984f, 1.211111f, 1.183077f, 1.169697f, 1.169697f, 1.183077f, 1.211111f, 1.240984f, 1.289655f, 1.343636f, 1.383019f, 1.448000f, 1.547826f,
         1.425490f, 1.343636f, 1.272881f, 1.225806f, 1.183077f, 1.156716f, 1.131884f, 1.120000f, 1.120000f, 1.131884f, 1.156716f, 1.183077f, 1.240984f, 1.289655f, 1.343636f, 1.403846f, 1.471429f,
         1.383019f, 1.307018f, 1.240984f, 1.196875f, 1.144118f, 1.108451f, 1.086301f, 1.065333f, 1.065333f, 1.086301f, 1.108451f, 1.144118f, 1.183077f, 1.240984f, 1.307018f, 1.362963f, 1.425490f,
         1.343636f, 1.272881f, 1.211111f, 1.156716f, 1.108451f, 1.075676f, 1.045455f, 1.035897f, 1.035897f, 1.045455f, 1.075676f, 1.108451f, 1.156716f, 1.211111f, 1.272881f, 1.343636f, 1.403846f,
         1.325000f, 1.256667f, 1.196875f, 1.131884f, 1.086301f, 1.055263f, 1.026582f, 1.008642f, 1.008642f, 1.017500f, 1.045455f, 1.086301f, 1.131884f, 1.196875f, 1.256667f, 1.325000f, 1.383019f,
         1.325000f, 1.256667f, 1.183077f, 1.131884f, 1.075676f, 1.035897f, 1.017500f, 1.000000f, 1.000000f, 1.008642f, 1.035897f, 1.075676f, 1.131884f, 1.183077f, 1.256667f, 1.325000f, 1.383019f,
         1.325000f, 1.256667f, 1.183077f, 1.131884f, 1.075676f, 1.045455f, 1.017500f, 1.008642f, 1.008642f, 1.026582f, 1.045455f, 1.086301f, 1.131884f, 1.196875f, 1.256667f, 1.325000f, 1.383019f,
         1.343636f, 1.272881f, 1.196875f, 1.144118f, 1.097222f, 1.065333f, 1.035897f, 1.026582f, 1.026582f, 1.045455f, 1.065333f, 1.097222f, 1.144118f, 1.211111f, 1.272881f, 1.343636f, 1.403846f,
         1.362963f, 1.289655f, 1.225806f, 1.169697f, 1.131884f, 1.097222f, 1.065333f, 1.055263f, 1.055263f, 1.065333f, 1.086301f, 1.131884f, 1.183077f, 1.225806f, 1.307018f, 1.362963f, 1.425490f,
         1.425490f, 1.343636f, 1.272881f, 1.211111f, 1.169697f, 1.131884f, 1.108451f, 1.097222f, 1.097222f, 1.108451f, 1.131884f, 1.169697f, 1.211111f, 1.272881f, 1.343636f, 1.403846f, 1.471429f,
         1.495833f, 1.403846f, 1.325000f, 1.272881f, 1.225806f, 1.183077f, 1.156716f, 1.144118f, 1.144118f, 1.156716f, 1.183077f, 1.225806f, 1.272881f, 1.325000f, 1.383019f, 1.448000f, 1.521277f,
         1.455820f, 1.471429f, 1.383019f, 1.307018f, 1.272881f, 1.240984f, 1.211111f, 1.196875f, 1.196875f, 1.211111f, 1.225806f, 1.272881f, 1.307018f, 1.362963f, 1.425490f, 1.495833f, 1.487705f
      },
      /* GB Gain */
      {
         1.463630f, 1.471429f, 1.403846f, 1.343636f, 1.307018f, 1.272881f, 1.256667f, 1.240984f, 1.225806f, 1.240984f, 1.272881f, 1.289655f, 1.343636f, 1.383019f, 1.448000f, 1.547826f, 1.512797f, 
         1.521277f, 1.403846f, 1.343636f, 1.289655f, 1.256667f, 1.211111f, 1.183077f, 1.169697f, 1.169697f, 1.183077f, 1.211111f, 1.240984f, 1.272881f, 1.325000f, 1.383019f, 1.448000f, 1.547826f, 
         1.425490f, 1.362963f, 1.289655f, 1.240984f, 1.196875f, 1.169697f, 1.131884f, 1.120000f, 1.120000f, 1.131884f, 1.156716f, 1.183077f, 1.225806f, 1.272881f, 1.325000f, 1.403846f, 1.471429f, 
         1.383019f, 1.325000f, 1.256667f, 1.211111f, 1.156716f, 1.120000f, 1.086301f, 1.075676f, 1.075676f, 1.086301f, 1.108451f, 1.131884f, 1.183077f, 1.225806f, 1.289655f, 1.362963f, 1.425490f, 
         1.362963f, 1.307018f, 1.240984f, 1.183077f, 1.131884f, 1.086301f, 1.055263f, 1.045455f, 1.035897f, 1.045455f, 1.065333f, 1.097222f, 1.144118f, 1.196875f, 1.256667f, 1.325000f, 1.403846f, 
         1.343636f, 1.289655f, 1.225806f, 1.156716f, 1.108451f, 1.065333f, 1.035897f, 1.017500f, 1.017500f, 1.026582f, 1.045455f, 1.075676f, 1.120000f, 1.183077f, 1.240984f, 1.325000f, 1.383019f, 
         1.343636f, 1.272881f, 1.211111f, 1.144118f, 1.097222f, 1.055263f, 1.026582f, 1.008642f, 1.008642f, 1.017500f, 1.035897f, 1.075676f, 1.120000f, 1.183077f, 1.240984f, 1.325000f, 1.383019f, 
         1.343636f, 1.289655f, 1.211111f, 1.156716f, 1.097222f, 1.065333f, 1.035897f, 1.017500f, 1.017500f, 1.026582f, 1.055263f, 1.086301f, 1.131884f, 1.183077f, 1.256667f, 1.325000f, 1.383019f, 
         1.362963f, 1.289655f, 1.225806f, 1.169697f, 1.120000f, 1.086301f, 1.055263f, 1.045455f, 1.035897f, 1.045455f, 1.065333f, 1.097222f, 1.144118f, 1.196875f, 1.272881f, 1.343636f, 1.403846f, 
         1.383019f, 1.325000f, 1.256667f, 1.211111f, 1.156716f, 1.120000f, 1.086301f, 1.075676f, 1.065333f, 1.075676f, 1.097222f, 1.131884f, 1.169697f, 1.225806f, 1.289655f, 1.362963f, 1.425490f, 
         1.448000f, 1.362963f, 1.307018f, 1.256667f, 1.196875f, 1.156716f, 1.131884f, 1.108451f, 1.108451f, 1.120000f, 1.144118f, 1.169697f, 1.225806f, 1.272881f, 1.343636f, 1.403846f, 1.471429f, 
         1.521277f, 1.425490f, 1.343636f, 1.307018f, 1.240984f, 1.211111f, 1.183077f, 1.169697f, 1.169697f, 1.183077f, 1.196875f, 1.225806f, 1.272881f, 1.325000f, 1.383019f, 1.448000f, 1.547826f, 
         1.471546f, 1.471429f, 1.383019f, 1.343636f, 1.289655f, 1.256667f, 1.225806f, 1.211111f, 1.211111f, 1.225806f, 1.240984f, 1.272881f, 1.307018f, 1.362963f, 1.425490f, 1.495833f, 1.495953f
      },
      /* B Gain */
      {
         1.359236f, 1.368421f, 1.266667f, 1.244186f, 1.202222f, 1.182609f, 1.163830f, 1.163830f, 1.163830f, 1.163830f, 1.182609f, 1.202222f, 1.244186f, 1.266667f, 1.315000f, 1.368421f, 1.368528f, 
         1.397297f, 1.315000f, 1.244186f, 1.202222f, 1.182609f, 1.145833f, 1.128571f, 1.128571f, 1.128571f, 1.128571f, 1.145833f, 1.163830f, 1.202222f, 1.244186f, 1.290244f, 1.341026f, 1.397297f, 
         1.341026f, 1.266667f, 1.202222f, 1.163830f, 1.145833f, 1.112000f, 1.096078f, 1.080769f, 1.080769f, 1.096078f, 1.112000f, 1.128571f, 1.163830f, 1.202222f, 1.244186f, 1.315000f, 1.341026f, 
         1.290244f, 1.222727f, 1.182609f, 1.145833f, 1.096078f, 1.080769f, 1.051852f, 1.051852f, 1.051852f, 1.051852f, 1.080769f, 1.096078f, 1.145833f, 1.182609f, 1.222727f, 1.266667f, 1.315000f, 
         1.266667f, 1.202222f, 1.163830f, 1.112000f, 1.080769f, 1.051852f, 1.025000f, 1.025000f, 1.025000f, 1.025000f, 1.051852f, 1.080769f, 1.112000f, 1.145833f, 1.202222f, 1.266667f, 1.290244f, 
         1.244186f, 1.202222f, 1.145833f, 1.096078f, 1.066038f, 1.038182f, 1.012281f, 1.000000f, 1.000000f, 1.012281f, 1.038182f, 1.066038f, 1.096078f, 1.145833f, 1.202222f, 1.266667f, 1.290244f, 
         1.244186f, 1.202222f, 1.145833f, 1.096078f, 1.066038f, 1.025000f, 1.012281f, 1.000000f, 1.000000f, 1.012281f, 1.038182f, 1.066038f, 1.096078f, 1.145833f, 1.202222f, 1.266667f, 1.290244f, 
         1.244186f, 1.202222f, 1.145833f, 1.096078f, 1.066038f, 1.038182f, 1.012281f, 1.012281f, 1.012281f, 1.025000f, 1.038182f, 1.066038f, 1.112000f, 1.145833f, 1.202222f, 1.266667f, 1.290244f, 
         1.266667f, 1.222727f, 1.163830f, 1.112000f, 1.080769f, 1.051852f, 1.038182f, 1.025000f, 1.025000f, 1.038182f, 1.051852f, 1.080769f, 1.112000f, 1.163830f, 1.222727f, 1.266667f, 1.315000f, 
         1.315000f, 1.244186f, 1.182609f, 1.145833f, 1.112000f, 1.080769f, 1.066038f, 1.051852f, 1.051852f, 1.066038f, 1.080769f, 1.112000f, 1.145833f, 1.182609f, 1.244186f, 1.290244f, 1.315000f, 
         1.341026f, 1.266667f, 1.222727f, 1.182609f, 1.145833f, 1.112000f, 1.096078f, 1.080769f, 1.080769f, 1.080769f, 1.112000f, 1.128571f, 1.163830f, 1.202222f, 1.266667f, 1.315000f, 1.368421f, 
         1.427778f, 1.315000f, 1.266667f, 1.222727f, 1.182609f, 1.145833f, 1.128571f, 1.112000f, 1.112000f, 1.128571f, 1.145833f, 1.163830f, 1.202222f, 1.244186f, 1.315000f, 1.368421f, 1.427778f, 
         1.368528f, 1.368421f, 1.290244f, 1.244186f, 1.202222f, 1.182609f, 1.163830f, 1.145833f, 1.145833f, 1.163830f, 1.182609f, 1.222727f, 1.244186f, 1.290244f, 1.341026f, 1.397297f, 1.397407f
      }
      },
   /* Low-Light Mesh Lens Rolloff */
   {
      221,
      /* R Gain */
      {
         1.261331f, 1.261250f, 1.241463f, 1.222619f, 1.187500f, 1.171111f, 1.155435f, 1.155435f, 1.155435f, 1.155435f, 1.171111f, 1.187500f, 1.222619f, 1.241463f, 1.282051f, 1.303947f, 1.304033f,
         1.282051f, 1.241463f, 1.222619f, 1.187500f, 1.155435f, 1.140426f, 1.126042f, 1.112245f, 1.112245f, 1.126042f, 1.140426f, 1.155435f, 1.187500f, 1.222619f, 1.241463f, 1.282051f, 1.327027f,
         1.261250f, 1.222619f, 1.187500f, 1.155435f, 1.126042f, 1.099000f, 1.086275f, 1.074039f, 1.074039f, 1.086275f, 1.099000f, 1.126042f, 1.155435f, 1.187500f, 1.222619f, 1.261250f, 1.282051f,
         1.241463f, 1.204651f, 1.155435f, 1.126042f, 1.099000f, 1.074039f, 1.050926f, 1.040000f, 1.040000f, 1.050926f, 1.074039f, 1.099000f, 1.126042f, 1.155435f, 1.187500f, 1.241463f, 1.261250f,
         1.222619f, 1.187500f, 1.140426f, 1.099000f, 1.074039f, 1.040000f, 1.029464f, 1.019298f, 1.019298f, 1.029464f, 1.040000f, 1.074039f, 1.099000f, 1.140426f, 1.171111f, 1.222619f, 1.241463f,
         1.204651f, 1.171111f, 1.126042f, 1.086275f, 1.050926f, 1.029464f, 1.009483f, 1.000000f, 1.000000f, 1.009483f, 1.029464f, 1.050926f, 1.086275f, 1.126042f, 1.171111f, 1.222619f, 1.241463f,
         1.204651f, 1.171111f, 1.126042f, 1.086275f, 1.050926f, 1.019298f, 1.009483f, 1.000000f, 1.000000f, 1.009483f, 1.029464f, 1.050926f, 1.086275f, 1.126042f, 1.171111f, 1.204651f, 1.241463f,
         1.204651f, 1.171111f, 1.126042f, 1.086275f, 1.050926f, 1.029464f, 1.009483f, 1.000000f, 1.000000f, 1.009483f, 1.029464f, 1.050926f, 1.086275f, 1.126042f, 1.171111f, 1.204651f, 1.241463f,
         1.222619f, 1.187500f, 1.140426f, 1.099000f, 1.074039f, 1.040000f, 1.029464f, 1.019298f, 1.019298f, 1.029464f, 1.040000f, 1.074039f, 1.099000f, 1.140426f, 1.187500f, 1.222619f, 1.261250f,
         1.241463f, 1.187500f, 1.155435f, 1.112245f, 1.086275f, 1.062264f, 1.050926f, 1.040000f, 1.040000f, 1.050926f, 1.062264f, 1.086275f, 1.126042f, 1.155435f, 1.204651f, 1.241463f, 1.261250f,
         1.261250f, 1.222619f, 1.187500f, 1.155435f, 1.126042f, 1.099000f, 1.086275f, 1.074039f, 1.074039f, 1.086275f, 1.099000f, 1.126042f, 1.155435f, 1.187500f, 1.222619f, 1.261250f, 1.282051f,
         1.282051f, 1.241463f, 1.204651f, 1.187500f, 1.155435f, 1.126042f, 1.112245f, 1.112245f, 1.112245f, 1.126042f, 1.140426f, 1.155435f, 1.187500f, 1.222619f, 1.241463f, 1.282051f, 1.327027f,
         1.261331f, 1.261250f, 1.241463f, 1.204651f, 1.187500f, 1.155435f, 1.140426f, 1.140426f, 1.140426f, 1.155435f, 1.171111f, 1.187500f, 1.222619f, 1.241463f, 1.282051f, 1.303947f, 1.304033f
      },
      /* GR Gain */
      {
         1.275082f, 1.280612f, 1.248039f, 1.217924f, 1.203704f, 1.190000f, 1.164035f, 1.164035f, 1.164035f, 1.176786f, 1.176786f, 1.203704f, 1.232692f, 1.248039f, 1.280612f, 1.315957f, 1.303932f,
         1.297917f, 1.248039f, 1.217924f, 1.190000f, 1.164035f, 1.139830f, 1.128333f, 1.128333f, 1.117213f, 1.128333f, 1.139830f, 1.164035f, 1.190000f, 1.217924f, 1.248039f, 1.280612f, 1.315957f,
         1.248039f, 1.217924f, 1.190000f, 1.151724f, 1.128333f, 1.106452f, 1.096032f, 1.085938f, 1.085938f, 1.096032f, 1.106452f, 1.128333f, 1.151724f, 1.190000f, 1.232692f, 1.248039f, 1.280612f,
         1.232692f, 1.203704f, 1.164035f, 1.128333f, 1.106452f, 1.076154f, 1.057463f, 1.048529f, 1.048529f, 1.057463f, 1.076154f, 1.096032f, 1.128333f, 1.164035f, 1.203704f, 1.232692f, 1.264000f,
         1.217924f, 1.176786f, 1.139830f, 1.106452f, 1.076154f, 1.057463f, 1.039855f, 1.031429f, 1.031429f, 1.031429f, 1.048529f, 1.076154f, 1.106452f, 1.139830f, 1.176786f, 1.217924f, 1.232692f,
         1.217924f, 1.176786f, 1.128333f, 1.096032f, 1.066667f, 1.039855f, 1.023239f, 1.015278f, 1.007534f, 1.015278f, 1.031429f, 1.057463f, 1.085938f, 1.128333f, 1.164035f, 1.203704f, 1.232692f,
         1.203704f, 1.164035f, 1.128333f, 1.085938f, 1.057463f, 1.031429f, 1.015278f, 1.000000f, 1.000000f, 1.015278f, 1.031429f, 1.057463f, 1.085938f, 1.128333f, 1.164035f, 1.203704f, 1.232692f,
         1.203704f, 1.164035f, 1.128333f, 1.085938f, 1.057463f, 1.031429f, 1.015278f, 1.007534f, 1.007534f, 1.015278f, 1.039855f, 1.057463f, 1.085938f, 1.128333f, 1.164035f, 1.203704f, 1.232692f,
         1.203704f, 1.176786f, 1.128333f, 1.096032f, 1.066667f, 1.039855f, 1.023239f, 1.023239f, 1.023239f, 1.031429f, 1.048529f, 1.066667f, 1.106452f, 1.139830f, 1.176786f, 1.217924f, 1.232692f,
         1.217924f, 1.190000f, 1.151724f, 1.117213f, 1.085938f, 1.066667f, 1.048529f, 1.039855f, 1.048529f, 1.048529f, 1.066667f, 1.085938f, 1.117213f, 1.151724f, 1.190000f, 1.232692f, 1.248039f,
         1.232692f, 1.203704f, 1.176786f, 1.139830f, 1.117213f, 1.096032f, 1.076154f, 1.066667f, 1.066667f, 1.076154f, 1.096032f, 1.117213f, 1.151724f, 1.176786f, 1.203704f, 1.232692f, 1.264000f,
         1.280612f, 1.232692f, 1.203704f, 1.176786f, 1.151724f, 1.128333f, 1.106452f, 1.106452f, 1.106452f, 1.117213f, 1.128333f, 1.151724f, 1.176786f, 1.203704f, 1.232692f, 1.264000f, 1.297917f,
         1.258690f, 1.264000f, 1.217924f, 1.203704f, 1.176786f, 1.164035f, 1.139830f, 1.128333f, 1.139830f, 1.139830f, 1.151724f, 1.176786f, 1.203704f, 1.217924f, 1.248039f, 1.280612f, 1.280695f
      },
      /* GB Gain */
      {
         1.275082f, 1.280612f, 1.248039f, 1.232692f, 1.203704f, 1.190000f, 1.164035f, 1.164035f, 1.164035f, 1.176786f, 1.190000f, 1.203704f, 1.217924f, 1.248039f, 1.280612f, 1.315957f, 1.303932f, 
         1.297917f, 1.248039f, 1.217924f, 1.190000f, 1.164035f, 1.139830f, 1.128333f, 1.128333f, 1.128333f, 1.128333f, 1.139830f, 1.164035f, 1.190000f, 1.217924f, 1.248039f, 1.280612f, 1.315957f, 
         1.264000f, 1.232692f, 1.203704f, 1.164035f, 1.139830f, 1.117213f, 1.096032f, 1.085938f, 1.085938f, 1.096032f, 1.106452f, 1.128333f, 1.151724f, 1.190000f, 1.217924f, 1.248039f, 1.280612f, 
         1.248039f, 1.217924f, 1.176786f, 1.139830f, 1.106452f, 1.085938f, 1.066667f, 1.057463f, 1.057463f, 1.066667f, 1.076154f, 1.096032f, 1.128333f, 1.164035f, 1.203704f, 1.232692f, 1.264000f, 
         1.232692f, 1.203704f, 1.151724f, 1.117213f, 1.085938f, 1.057463f, 1.039855f, 1.031429f, 1.031429f, 1.039855f, 1.057463f, 1.076154f, 1.106452f, 1.139830f, 1.176786f, 1.217924f, 1.248039f, 
         1.217924f, 1.190000f, 1.139830f, 1.106452f, 1.076154f, 1.048529f, 1.031429f, 1.015278f, 1.015278f, 1.023239f, 1.039855f, 1.066667f, 1.096032f, 1.128333f, 1.176786f, 1.217924f, 1.248039f, 
         1.217924f, 1.176786f, 1.139830f, 1.096032f, 1.066667f, 1.039855f, 1.023239f, 1.015278f, 1.007534f, 1.015278f, 1.031429f, 1.057463f, 1.085938f, 1.128333f, 1.176786f, 1.203704f, 1.248039f, 
         1.217924f, 1.176786f, 1.139830f, 1.106452f, 1.066667f, 1.039855f, 1.023239f, 1.015278f, 1.015278f, 1.023239f, 1.039855f, 1.066667f, 1.096032f, 1.128333f, 1.176786f, 1.217924f, 1.248039f, 
         1.232692f, 1.190000f, 1.151724f, 1.117213f, 1.085938f, 1.057463f, 1.039855f, 1.031429f, 1.031429f, 1.039855f, 1.057463f, 1.076154f, 1.106452f, 1.139830f, 1.176786f, 1.217924f, 1.248039f, 
         1.232692f, 1.203704f, 1.176786f, 1.139830f, 1.106452f, 1.076154f, 1.057463f, 1.057463f, 1.057463f, 1.057463f, 1.076154f, 1.096032f, 1.128333f, 1.164035f, 1.190000f, 1.232692f, 1.264000f, 
         1.264000f, 1.232692f, 1.203704f, 1.164035f, 1.139830f, 1.106452f, 1.096032f, 1.085938f, 1.085938f, 1.085938f, 1.106452f, 1.128333f, 1.151724f, 1.190000f, 1.217924f, 1.248039f, 1.280612f, 
         1.297917f, 1.264000f, 1.217924f, 1.190000f, 1.164035f, 1.139830f, 1.128333f, 1.117213f, 1.117213f, 1.128333f, 1.139830f, 1.164035f, 1.176786f, 1.203704f, 1.232692f, 1.264000f, 1.315957f, 
         1.280695f, 1.280612f, 1.248039f, 1.217924f, 1.190000f, 1.164035f, 1.151724f, 1.151724f, 1.151724f, 1.151724f, 1.164035f, 1.176786f, 1.203704f, 1.217924f, 1.248039f, 1.297917f, 1.292153f
      },
      /* B Gain */
      {
         1.157214f, 1.165000f, 1.141935f, 1.120312f, 1.120312f, 1.100000f, 1.080882f, 1.080882f, 1.080882f, 1.080882f, 1.100000f, 1.100000f, 1.120312f, 1.141935f, 1.165000f, 1.165000f, 1.173106f, 
         1.165000f, 1.141935f, 1.120312f, 1.120312f, 1.100000f, 1.080882f, 1.080882f, 1.062857f, 1.062857f, 1.062857f, 1.080882f, 1.100000f, 1.100000f, 1.120312f, 1.141935f, 1.165000f, 1.189655f, 
         1.165000f, 1.141935f, 1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.045833f, 1.045833f, 1.045833f, 1.045833f, 1.062857f, 1.062857f, 1.100000f, 1.120312f, 1.141935f, 1.141935f, 1.165000f, 
         1.141935f, 1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.045833f, 1.029730f, 1.029730f, 1.029730f, 1.029730f, 1.045833f, 1.062857f, 1.080882f, 1.100000f, 1.120312f, 1.141935f, 1.141935f, 
         1.120312f, 1.120312f, 1.080882f, 1.062857f, 1.045833f, 1.029730f, 1.014474f, 1.014474f, 1.014474f, 1.014474f, 1.029730f, 1.045833f, 1.062857f, 1.080882f, 1.100000f, 1.141935f, 1.141935f, 
         1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.045833f, 1.014474f, 1.014474f, 1.000000f, 1.000000f, 1.000000f, 1.014474f, 1.029730f, 1.045833f, 1.080882f, 1.100000f, 1.141935f, 1.141935f, 
         1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.029730f, 1.014474f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.014474f, 1.029730f, 1.045833f, 1.080882f, 1.100000f, 1.120312f, 1.141935f, 
         1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.029730f, 1.014474f, 1.000000f, 1.000000f, 1.000000f, 1.014474f, 1.014474f, 1.045833f, 1.062857f, 1.080882f, 1.100000f, 1.141935f, 1.141935f, 
         1.141935f, 1.120312f, 1.080882f, 1.062857f, 1.045833f, 1.029730f, 1.014474f, 1.014474f, 1.014474f, 1.014474f, 1.029730f, 1.045833f, 1.062857f, 1.080882f, 1.120312f, 1.141935f, 1.141935f, 
         1.141935f, 1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.045833f, 1.029730f, 1.029730f, 1.029730f, 1.029730f, 1.045833f, 1.062857f, 1.080882f, 1.100000f, 1.120312f, 1.141935f, 1.141935f, 
         1.165000f, 1.141935f, 1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.045833f, 1.045833f, 1.045833f, 1.045833f, 1.062857f, 1.062857f, 1.080882f, 1.100000f, 1.120312f, 1.141935f, 1.165000f, 
         1.189655f, 1.141935f, 1.141935f, 1.120312f, 1.100000f, 1.080882f, 1.062857f, 1.062857f, 1.062857f, 1.062857f, 1.080882f, 1.080882f, 1.100000f, 1.120312f, 1.141935f, 1.165000f, 1.189655f, 
         1.165071f, 1.165000f, 1.141935f, 1.120312f, 1.100000f, 1.100000f, 1.080882f, 1.080882f, 1.080882f, 1.080882f, 1.100000f, 1.100000f, 1.120312f, 1.141935f, 1.165000f, 1.189655f, 1.181323f
      }
      },
   /* Preview Mesh Lens Rolloff */
   {
      221,
      /* R Gain */
      {
         1.332603f, 1.332500f, 1.307317f, 1.283333f, 1.238636f, 1.217778f, 1.197826f, 1.197826f, 1.197826f, 1.197826f, 1.217778f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.386842f, 1.386951f,
         1.358974f, 1.307317f, 1.283333f, 1.238636f, 1.197826f, 1.178723f, 1.160417f, 1.142857f, 1.142857f, 1.160417f, 1.178723f, 1.197826f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.416216f,
         1.332500f, 1.283333f, 1.238636f, 1.197826f, 1.160417f, 1.126000f, 1.109804f, 1.094231f, 1.094231f, 1.109804f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.283333f, 1.332500f, 1.358974f,
         1.307317f, 1.260465f, 1.197826f, 1.160417f, 1.126000f, 1.094231f, 1.064815f, 1.050909f, 1.050909f, 1.064815f, 1.094231f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.307317f, 1.332500f,
         1.283333f, 1.238636f, 1.178723f, 1.126000f, 1.094231f, 1.050909f, 1.037500f, 1.024561f, 1.024561f, 1.037500f, 1.050909f, 1.094231f, 1.126000f, 1.178723f, 1.217778f, 1.283333f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.037500f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.283333f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.024561f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.260465f, 1.307317f,
         1.260465f, 1.217778f, 1.160417f, 1.109804f, 1.064815f, 1.037500f, 1.012069f, 1.000000f, 1.000000f, 1.012069f, 1.037500f, 1.064815f, 1.109804f, 1.160417f, 1.217778f, 1.260465f, 1.307317f,
         1.283333f, 1.238636f, 1.178723f, 1.126000f, 1.094231f, 1.050909f, 1.037500f, 1.024561f, 1.024561f, 1.037500f, 1.050909f, 1.094231f, 1.126000f, 1.178723f, 1.238636f, 1.283333f, 1.332500f,
         1.307317f, 1.238636f, 1.197826f, 1.142857f, 1.109804f, 1.079245f, 1.064815f, 1.050909f, 1.050909f, 1.064815f, 1.079245f, 1.109804f, 1.160417f, 1.197826f, 1.260465f, 1.307317f, 1.332500f,
         1.332500f, 1.283333f, 1.238636f, 1.197826f, 1.160417f, 1.126000f, 1.109804f, 1.094231f, 1.094231f, 1.109804f, 1.126000f, 1.160417f, 1.197826f, 1.238636f, 1.283333f, 1.332500f, 1.358974f,
         1.358974f, 1.307317f, 1.260465f, 1.238636f, 1.197826f, 1.160417f, 1.142857f, 1.142857f, 1.142857f, 1.160417f, 1.178723f, 1.197826f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.416216f,
         1.332603f, 1.332500f, 1.307317f, 1.260465f, 1.238636f, 1.197826f, 1.178723f, 1.178723f, 1.178723f, 1.197826f, 1.217778f, 1.238636f, 1.283333f, 1.307317f, 1.358974f, 1.386842f, 1.386951f
      },
      /* GR Gain */
      {
         1.350105f, 1.357143f, 1.315686f, 1.277359f, 1.259259f, 1.241818f, 1.208772f, 1.208772f, 1.208772f, 1.225000f, 1.225000f, 1.259259f, 1.296154f, 1.315686f, 1.357143f, 1.402128f, 1.386822f,
         1.379167f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.163333f, 1.149180f, 1.163333f, 1.177966f, 1.208772f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 1.402128f,
         1.315686f, 1.277359f, 1.241818f, 1.193103f, 1.163333f, 1.135484f, 1.122222f, 1.109375f, 1.109375f, 1.122222f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.296154f, 1.315686f, 1.357143f,
         1.296154f, 1.259259f, 1.208772f, 1.163333f, 1.135484f, 1.096923f, 1.073134f, 1.061765f, 1.061765f, 1.073134f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.259259f, 1.296154f, 1.336000f,
         1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.096923f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.040000f, 1.061765f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.296154f,
         1.277359f, 1.225000f, 1.163333f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.009589f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.208772f, 1.163333f, 1.109375f, 1.073134f, 1.040000f, 1.019444f, 1.000000f, 1.000000f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.208772f, 1.163333f, 1.109375f, 1.073134f, 1.040000f, 1.019444f, 1.009589f, 1.009589f, 1.019444f, 1.050725f, 1.073134f, 1.109375f, 1.163333f, 1.208772f, 1.259259f, 1.296154f,
         1.259259f, 1.225000f, 1.163333f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.029577f, 1.029577f, 1.040000f, 1.061765f, 1.084849f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.296154f,
         1.277359f, 1.241818f, 1.193103f, 1.149180f, 1.109375f, 1.084849f, 1.061765f, 1.050725f, 1.061765f, 1.061765f, 1.084849f, 1.109375f, 1.149180f, 1.193103f, 1.241818f, 1.296154f, 1.315686f,
         1.296154f, 1.259259f, 1.225000f, 1.177966f, 1.149180f, 1.122222f, 1.096923f, 1.084849f, 1.084849f, 1.096923f, 1.122222f, 1.149180f, 1.193103f, 1.225000f, 1.259259f, 1.296154f, 1.336000f,
         1.357143f, 1.296154f, 1.259259f, 1.225000f, 1.193103f, 1.163333f, 1.135484f, 1.135484f, 1.135484f, 1.149180f, 1.163333f, 1.193103f, 1.225000f, 1.259259f, 1.296154f, 1.336000f, 1.379167f,
         1.329242f, 1.336000f, 1.277359f, 1.259259f, 1.225000f, 1.208772f, 1.177966f, 1.163333f, 1.177966f, 1.177966f, 1.193103f, 1.225000f, 1.259259f, 1.277359f, 1.315686f, 1.357143f, 1.357249f
      },
      /* GB Gain */
      {
         1.350105f, 1.357143f, 1.315686f, 1.296154f, 1.259259f, 1.241818f, 1.208772f, 1.208772f, 1.208772f, 1.225000f, 1.241818f, 1.259259f, 1.277359f, 1.315686f, 1.357143f, 1.402128f, 1.386822f,
         1.379167f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.163333f, 1.163333f, 1.163333f, 1.177966f, 1.208772f, 1.241818f, 1.277359f, 1.315686f, 1.357143f, 1.402128f,
         1.336000f, 1.296154f, 1.259259f, 1.208772f, 1.177966f, 1.149180f, 1.122222f, 1.109375f, 1.109375f, 1.122222f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.277359f, 1.315686f, 1.357143f,
         1.315686f, 1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.109375f, 1.084849f, 1.073134f, 1.073134f, 1.084849f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.259259f, 1.296154f, 1.336000f,
         1.296154f, 1.259259f, 1.193103f, 1.149180f, 1.109375f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.050725f, 1.073134f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.315686f,
         1.277359f, 1.241818f, 1.177966f, 1.135484f, 1.096923f, 1.061765f, 1.040000f, 1.019444f, 1.019444f, 1.029577f, 1.050725f, 1.084849f, 1.122222f, 1.163333f, 1.225000f, 1.277359f, 1.315686f,
         1.277359f, 1.225000f, 1.177966f, 1.122222f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.009589f, 1.019444f, 1.040000f, 1.073134f, 1.109375f, 1.163333f, 1.225000f, 1.259259f, 1.315686f,
         1.277359f, 1.225000f, 1.177966f, 1.135484f, 1.084849f, 1.050725f, 1.029577f, 1.019444f, 1.019444f, 1.029577f, 1.050725f, 1.084849f, 1.122222f, 1.163333f, 1.225000f, 1.277359f, 1.315686f,
         1.296154f, 1.241818f, 1.193103f, 1.149180f, 1.109375f, 1.073134f, 1.050725f, 1.040000f, 1.040000f, 1.050725f, 1.073134f, 1.096923f, 1.135484f, 1.177966f, 1.225000f, 1.277359f, 1.315686f,
         1.296154f, 1.259259f, 1.225000f, 1.177966f, 1.135484f, 1.096923f, 1.073134f, 1.073134f, 1.073134f, 1.073134f, 1.096923f, 1.122222f, 1.163333f, 1.208772f, 1.241818f, 1.296154f, 1.336000f,
         1.336000f, 1.296154f, 1.259259f, 1.208772f, 1.177966f, 1.135484f, 1.122222f, 1.109375f, 1.109375f, 1.109375f, 1.135484f, 1.163333f, 1.193103f, 1.241818f, 1.277359f, 1.315686f, 1.357143f,
         1.379167f, 1.336000f, 1.277359f, 1.241818f, 1.208772f, 1.177966f, 1.163333f, 1.149180f, 1.149180f, 1.163333f, 1.177966f, 1.208772f, 1.225000f, 1.259259f, 1.296154f, 1.336000f, 1.402128f,
         1.357249f, 1.357143f, 1.315686f, 1.277359f, 1.241818f, 1.208772f, 1.193103f, 1.193103f, 1.193103f, 1.193103f, 1.208772f, 1.225000f, 1.259259f, 1.277359f, 1.315686f, 1.379167f, 1.371831f
      },
      /* B Gain */
      {
         1.200090f, 1.210000f, 1.180645f, 1.153125f, 1.153125f, 1.127273f, 1.102941f, 1.102941f, 1.102941f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.210000f, 1.220317f,
         1.210000f, 1.180645f, 1.153125f, 1.153125f, 1.127273f, 1.102941f, 1.102941f, 1.080000f, 1.080000f, 1.080000f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f,
         1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.058333f, 1.058333f, 1.058333f, 1.080000f, 1.080000f, 1.127273f, 1.153125f, 1.180645f, 1.180645f, 1.210000f,
         1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.037838f, 1.037838f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.180645f,
         1.153125f, 1.153125f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.018421f, 1.018421f, 1.018421f, 1.018421f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.180645f, 1.180645f,
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.018421f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.037838f, 1.058333f, 1.102941f, 1.127273f, 1.180645f, 1.180645f,
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.037838f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.037838f, 1.058333f, 1.102941f, 1.127273f, 1.153125f, 1.180645f,
         1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.037838f, 1.018421f, 1.000000f, 1.000000f, 1.000000f, 1.018421f, 1.018421f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.180645f, 1.180645f,
         1.180645f, 1.153125f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.018421f, 1.018421f, 1.018421f, 1.018421f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.153125f, 1.180645f, 1.180645f,
         1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.037838f, 1.037838f, 1.037838f, 1.037838f, 1.058333f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.180645f,
         1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.058333f, 1.058333f, 1.058333f, 1.058333f, 1.080000f, 1.080000f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.210000f,
         1.241379f, 1.180645f, 1.180645f, 1.153125f, 1.127273f, 1.102941f, 1.080000f, 1.080000f, 1.080000f, 1.080000f, 1.102941f, 1.102941f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f,
         1.210091f, 1.210000f, 1.180645f, 1.153125f, 1.127273f, 1.127273f, 1.102941f, 1.102941f, 1.102941f, 1.102941f, 1.127273f, 1.127273f, 1.153125f, 1.180645f, 1.210000f, 1.241379f, 1.230775f
      }
      },
},
1, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
{
   5.000000f, /* Gain Start */
   4.500000f, /* Gain End */
   193, /* Lux Index Start */
   174, /* Lux Index End */
},
0.500000f, /* Sharp Min DS Factor */
4.000000f, /* Sharp Max DS Factor */
2.00000f, /* Sharp Max Factor */
/* 3x3 ASF */
{
   /* Edge Filter */
   {
      {-0.125000f, -0.125000f, -0.125000f},
      {-0.125000f, 2.000000f, -0.125000f},
      {-0.125000f, -0.125000f, -0.125000f}
   },
   /* Noise Filter */
   {
      {0.109375f, 0.109375f, 0.109375f},
      {0.109375f, 0.125000f, 0.109375f},
      {0.109375f, 0.109375f, 0.109375f}
   },
   /* Noise Threshold (10-bit) */
   160,
   /* Edge Threshold (10-bit) */
   240,
   /* Edge filter q factor */
   5,
   /* Noise filter q factor */
   6,
   /* Edge detection flag */
   0
},
/* 3x3 Low-Light ASF */
{
   /* Edge Filter */
   {
      {-0.125000f, -0.125000f, -0.125000f},
      {-0.125000f, 2.000000f, -0.125000f},
      {-0.125000f, -0.125000f, -0.125000f}
   },
   /* Noise Filter */
   {
      {0.109375f, 0.109375f, 0.109375f},
      {0.109375f, 0.125000f, 0.109375f},
      {0.109375f, 0.109375f, 0.109375f}
   },
   /* Noise Threshold (10-bit) */
   160,
   /* Edge Threshold (10-bit) */
   240,
   /* Edge filter q factor */
   5,
   /* Noise filter q factor */
   6,
   /* Edge detection flag */
   0
},
1, /* Use Gain for control */
{
   9.000000f, /* Gain Start */
   9.800000f, /* Gain End */
   348, /* Lux Index Start */
   379, /* Lux Index End */
},
{
   1.100000f, /* Gain Start */
   1.000000f, /* Gain End */
   193, /* Lux Index Start */
   174, /* Lux Index End */
},
/* 5x5 ASF */
{
   2, /* Filter Mode */
   /* Smoothing Filter */
   {
      1, 1, 1, 
      1, 1, 1, 
      1, 1, 1
   },
   /* Laplacian Filter */
   {
      -1, -1, -1, 
      -1, 8, -1, 
      -1, -1, -1
   },
   0.250000f, /* Normalize Factor 1 */
   0.250000f, /* Normalize Factor 2 */
   /* 5x5 Filter 1 */
   {
      0, -1, -2, -1, 0, 
      0, 0, 0, 0, 0, 
      0, 2, 4, 2, 0, 
      0, 0, 0, 0, 0, 
      0, -1, -2, -1, 0
   },
   /* 5x5 Filter 2 */
   {
      0, 0, 0, 0, 0, 
      -1, 0, 2, 0, -1, 
      -2, 0, 4, 0, -2, 
      -1, 0, 2, 0, -1, 
      0, 0, 0, 0, 0
   },
   80, /* Extraction Factor */
   /* Settings */
   {
      /* Low Light */
      {
         30, /* e1 */
         60, /* e2 */
         -60, /* e3 */
         60, /* e4 */
         -60, /* e5 */
         0.000000f, /* k1 */
         0.000000f, /* k2 */
         100, /* sp */
      },
      /* Normal Light */
      {
         13, /* e1 */
         40, /* e2 */
         -40, /* e3 */
         40, /* e4 */
         -40, /* e5 */
         0.750000f, /* k1 */
         0.750000f, /* k2 */
         40, /* sp */
      },
      /* Bright Light */
      {
         10, /* e1 */
         40, /* e2 */
         -40, /* e3 */
         40, /* e4 */
         -40, /* e5 */
         0.950000f, /* k1 */
         0.950000f, /* k2 */
         30, /* sp */
      }
   }
},
0.900000f, /* Soft Focus Degree */
/* 5x5 ASF */
{
   2, /* Filter Mode */
   /* Smoothing Filter */
   {
      1, 1, 1, 
      1, 1, 1, 
      1, 1, 1
   },
   /* Laplacian Filter */
   {
      -1, -1, -1, 
      -1, 8, -1, 
      -1, -1, -1
   },
   0.250000f, /* Normalize Factor 1 */
   0.250000f, /* Normalize Factor 2 */
   /* 5x5 Filter 1 */
   {
      0, 0, 0, 0, 0, 
      0, -1, -2, -1, 0, 
      0, 2, 4, 2, 0, 
      0, -1, -2, -1, 0, 
      0, 0, 0, 0, 0
   },
   /* 5x5 Filter 2 */
   {
      0, 0, 0, 0, 0, 
      0, -1, 2, -1, 0, 
      0, -2, 4, -2, 0, 
      0, -1, 2, -1, 0, 
      0, 0, 0, 0, 0
   },
   80, /* Extraction Factor */
   /* Settings */
   {
      /* Low Light */
      {
         30, /* e1 */
         60, /* e2 */
         -60, /* e3 */
         60, /* e4 */
         -60, /* e5 */
         0.000000f, /* k1 */
         0.000000f, /* k2 */
         100, /* sp */
      },
      /* Normal Light */
      {
         13, /* e1 */
         40, /* e2 */
         -40, /* e3 */
         40, /* e4 */
         -40, /* e5 */
         0.750000f, /* k1 */
         0.750000f, /* k2 */
         40, /* sp */
      },
      /* Bright Light */
      {
         10, /* e1 */
         40, /* e2 */
         -40, /* e3 */
         40, /* e4 */
         -40, /* e5 */
         0.950000f, /* k1 */
         0.950000f, /* k2 */
         30, /* sp */
      }
   }
},
0.500000f, /* 5x5 Sharp Min DS Factor */
4.000000f, /* 5x5 Sharp Max DS Factor */
1.000000f, /* 5x5 Sharp Max Factor */
50, /* Number of Iterations */
10, /* Low Percentage */
30, /* Low Range */
15, /* High Percentage */
229, /* High Range */
3, /* Luma Adaptation Threshold */
0, /* Use Gain for control */
{
   9.500000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
/* Low-Light Snapshot Chroma Suppression */
{
   5, /* Luma Threshold 1 */
   10, /* Luma Threshold 2 */
   180, /* Luma Threshold 3 */
   240, /* Luma Threshold 4 */
   15, /* Chroma Threshold 1 */
   30, /* Chroma Threshold 2 */
},
/* Chroma Suppression */
{
   5, /* Luma Threshold 1 */
   10, /* Luma Threshold 2 */
   180, /* Luma Threshold 3 */
   240, /* Luma Threshold 4 */
   15, /* Chroma Threshold 1 */
   30, /* Chroma Threshold 2 */
},
{
   9.500000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
/* Low-Light Preview Chroma Suppression */
{
   5, /* Luma Threshold 1 */
   10, /* Luma Threshold 2 */
   180, /* Luma Threshold 3 */
   240, /* Luma Threshold 4 */
   15, /* Chroma Threshold 1 */
   30, /* Chroma Threshold 2 */
},
/* Preview Chroma Suppression */
{
   5, /* Luma Threshold 1 */
   10, /* Luma Threshold 2 */
   180, /* Luma Threshold 3 */
   240, /* Luma Threshold 4 */
   15, /* Chroma Threshold 1 */
   30, /* Chroma Threshold 2 */
},
3, /* Max Number of Frames */
0, /* One to Two Frame HJR Offset */
4, /* Flat Area Noise Reduction Level */
12, /* Texture Noise Reduction Level */
3, /* Texture Threshold */
/* HJR K Table */
{2, 2, 2, 2, 2,
 2, 2, 3, 3, 3,
 3, 4, 4, 5, 5,
 6, 7, 9, 11, 16
},
58, /* Bayer Filter Enable Index */
1, /* Control */
{
   /* LP Threshold Table */
   {48, 48, 48, 48, 48,
 48, 48, 48, 48, 64,
 64, 64, 64, 64, 80,
 80, 96, 112, 114, 240
},
   4, /* Shift */
   /* Ratio Table */
   {0.015625, 0.015625, 0.015625, 0.015625, 0.031250,
 0.031250, 0.031250, 0.031250, 0.031250, 0.031250,
 0.046875, 0.046875, 0.046875, 0.062500, 0.062500,
 0.078125, 0.093750, 0.109375, 0.125000, 0.171875
},
   /* Min Table */
   {16, 16, 16, 16, 16,
 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16,
 16, 16, 32, 32, 48
},
   /* Max Table */
   {64, 64, 64, 64, 64,
 80, 80, 80, 80, 80,
 80, 80, 96, 96, 96,
 112, 128, 128, 128, 160
},
},
{
   /* LP Threshold Table */
   {48, 48, 48, 48, 48,
 48, 48, 48, 48, 64,
 64, 64, 64, 64, 80,
 80, 96, 112, 114, 240
},
   4, /* Shift */
   /* Ratio Table */
   {0.015625, 0.015625, 0.015625, 0.015625, 0.031250,
 0.031250, 0.031250, 0.031250, 0.031250, 0.031250,
 0.046875, 0.046875, 0.046875, 0.062500, 0.062500,
 0.078125, 0.093750, 0.109375, 0.125000, 0.171875
},
   /* Min Table */
   {16, 16, 16, 16, 16,
 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16,
 16, 16, 32, 32, 48
},
   /* Max Table */
   {64, 64, 64, 64, 64,
 80, 80, 80, 80, 80,
 80, 80, 96, 96, 96,
 112, 128, 128, 128, 160
},
},
58, /* Enable Lux Index Preview */
58, /* Enable Lux Index Snapshot */
{
   1.500000f, /* Index Gain Start */
   10.000000f, /* Index Gain End */
   58, /* Index Lux Index Start */
   387, /* Index Lux Index End */
},
{
   1.500000f, /* Index Gain Start */
   10.000000f, /* Index Gain End */
   58, /* Index Lux Index Start */
   387, /* Index Lux Index End */
},
{
   0.150000f, /* Standard Threshold */
   50, /* Percent Threshold */
   90, /* Difference Threshold */
   60, /* Frame CT Threshold */
   6, /* Number of Frames */
   1, /* Frame Skip */
   480, /* Number of Rows */
},
64, /* Preview Fmin */
64, /* Preview Fmax */
64, /* Preview Lowlight Fmin */
64, /* Preview Lowlight Fmax */
64, /* Snapshot Fmin */
64, /* Snapshot Fmax */
64, /* Snapshot Lowlight Fmin */
64, /* Snapshot Lowlight Fmax */
0, /* Use Gain for control */
{
   9.500000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
{
   {
      20, /* Red Difference Threshold */
      20, /* Green Difference Threshold */
      20, /* Blue Difference Threshold */
   },
   {
      30, /* Low-Light Red Difference Threshold */
      30, /* Low-Light Green Difference Threshold */
      30, /* Low-Light Blue Difference Threshold */
   },
},
{
   9.500000f, /* Gain Start */
   10.000000f, /* Gain End */
   348, /* Lux Index Start */
   387, /* Lux Index End */
},
{
   {
      25, /* Red Difference Threshold */
      25, /* Green Difference Threshold */
      25, /* Blue Difference Threshold */
   },
   {
      35, /* Low-Light Red Difference Threshold */
      35, /* Low-Light Green Difference Threshold */
      35, /* Low-Light Blue Difference Threshold */
   },
},
1.500000f, /* H Min */
0.500000f, /* H Max */
0.100000f, /* Y Min */
0.900000f, /* Y Max */
0.050000f, /* S HY Min */
0.250000f, /* S HY Max */
0.250000f, /* S LY Min */
0.600000f, /* S LY Max */
50, /* Percentage */
/* AFR Table */
{
   {
      7680.000000f, /* FPS */
      1, /* Use In Auto Frame Rate */
      0.000000f, /* Faster FPS Gain Trigger */
      5.200000f, /* Slower FPS Gain Trigger */
      0, /* Faster FPS Exp Table Index Mod */
      -23, /* Slower FPS Exp Table Index Mod */
   },
   {
      3840.000000f, /* FPS */
      1, /* Use In Auto Frame Rate */
      2.000000f, /* Faster FPS Gain Trigger */
      9.000000f, /* Slower FPS Gain Trigger */
      23, /* Faster FPS Exp Table Index Mod */
      -13, /* Slower FPS Exp Table Index Mod */
   },
   {
      2560.000000f, /* FPS */
      1, /* Use In Auto Frame Rate */
      4.000000f, /* Faster FPS Gain Trigger */
      10.000000f, /* Slower FPS Gain Trigger */
      13, /* Faster FPS Exp Table Index Mod */
      -9, /* Slower FPS Exp Table Index Mod */
   },
   {
      1920.000000f, /* FPS */
      1, /* Use In Auto Frame Rate */
      5.000000f, /* Faster FPS Gain Trigger */
      100.000000f, /* Slower FPS Gain Trigger */
      9, /* Faster FPS Exp Table Index Mod */
      0, /* Slower FPS Exp Table Index Mod */
   },
   {
      0.000000f, /* FPS */
      0, /* Use In Auto Frame Rate */
      0.000000f, /* Faster FPS Gain Trigger */
      0.000000f, /* Slower FPS Gain Trigger */
      0, /* Faster FPS Exp Table Index Mod */
      0, /* Slower FPS Exp Table Index Mod */
   },
   {
      0.000000f, /* FPS */
      0, /* Use In Auto Frame Rate */
      0.000000f, /* Faster FPS Gain Trigger */
      0.000000f, /* Slower FPS Gain Trigger */
      0, /* Faster FPS Exp Table Index Mod */
      0, /* Slower FPS Exp Table Index Mod */
   },
   {
      0.000000f, /* FPS */
      0, /* Use In Auto Frame Rate */
      0.000000f, /* Faster FPS Gain Trigger */
      0.000000f, /* Slower FPS Gain Trigger */
      0, /* Faster FPS Exp Table Index Mod */
      0, /* Slower FPS Exp Table Index Mod */
   },
   {
      0.000000f, /* FPS */
      0, /* Use In Auto Frame Rate */
      0.000000f, /* Faster FPS Gain Trigger */
      0.000000f, /* Slower FPS Gain Trigger */
      0, /* Faster FPS Exp Table Index Mod */
      0, /* Slower FPS Exp Table Index Mod */
   },
},
0, /* Demosaic Slope Shift */
