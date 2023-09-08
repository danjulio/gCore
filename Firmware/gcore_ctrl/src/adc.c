/*
 * adc.c
 *
 * ADC Measurement module
 *  Implement an interrupt-driven sampling system for USB input voltage and current, battery
 *  voltage, load current and internal temp.  TIMER0 IRQ triggers ADC readings.  ADC IRQ used
 *  to update values.
 *
 * Functions:
 *  1. Initialization
 *    - Initialize filter used for noisy voltage and current measurements
 *    - Read temp - initialize averaging array
 *    - Enable timer + adc interrupts
 *  2. Digitally filtered measurements for voltage and current to handle varying values that
 *     make it through the external low-pass hardware filters.  Measurements made for each
 *     voltage and current channel at approximately 1 kHz.
 *  3. Skew TIMER0 reload value back and forth around the center sample rate to jitter ADC sampling.
 *  4. Temperature sensor measurements every second and averaging over a several second period.
 *  5. Value access routines for other modules.
 *
 * Copyright (c) 2021-2022 danjuliodesigns, LLC.  All rights reserved.
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "intrins.h"
#include "adc.h"
#include "config.h"
#include "smbus.h"


//-----------------------------------------------------------------------------
// Module constants
//-----------------------------------------------------------------------------

// ADC Inputs
#define _ADC_VU_CH 1
#define _ADC_IU_CH 2
#define _ADC_VB_CH 3
#define _ADC_IL_CH 4
#define _ADC_TI_CH 27

// Timer0 reload min/max counts
//  Chosen to allow the sample point to skew around the entire PWM period to
//  more accurately measure a varying signal during that time.
//    - Skew around timer 0 reload base value of 0x80 (~250 uSec/sample)
//    - Timer 0 count = 1.96 uSec
//    - PWM Period = 41.8 uSec
//    => Minimum of 41.8/1.96 = ~21.3 counts
//  However the following values were empirically found to work well with
//  the selected filter constant K.
#define _ADC_TH0_MIN 0x77
#define _ADC_TH0_MAX 0x88



//-----------------------------------------------------------------------------
// Voltage and Current ADC Count to output lookup tables
//-----------------------------------------------------------------------------

// mV = (ADC_COUNT * VREF * V_SF) / 1023 = (ADC_COUNT * 1250 * 5.02) / 1023
code const uint16_t adc_2_mv[] = {
	0, 6, 12, 18, 25, 31, 37, 43, 49, 55, 61, 67, 74, 80, 86, 92,
	98, 104, 110, 117, 123, 129, 135, 141, 147, 153, 159, 166, 172, 178, 184, 190,
	196, 202, 209, 215, 221, 227, 233, 239, 245, 251, 258, 264, 270, 276, 282, 288,
	294, 301, 307, 313, 319, 325, 331, 337, 343, 350, 356, 362, 368, 374, 380, 386,
	393, 399, 405, 411, 417, 423, 429, 436, 442, 448, 454, 460, 466, 472, 478, 485,
	491, 497, 503, 509, 515, 521, 528, 534, 540, 546, 552, 558, 564, 570, 577, 583,
	589, 595, 601, 607, 613, 620, 626, 632, 638, 644, 650, 656, 662, 669, 675, 681,
	687, 693, 699, 705, 712, 718, 724, 730, 736, 742, 748, 754, 761, 767, 773, 779,
	785, 791, 797, 804, 810, 816, 822, 828, 834, 840, 846, 853, 859, 865, 871, 877,
	883, 889, 896, 902, 908, 914, 920, 926, 932, 938, 945, 951, 957, 963, 969, 975,
	981, 988, 994, 1000, 1006, 1012, 1018, 1024, 1030, 1037, 1043, 1049, 1055, 1061, 1067, 1073,
	1080, 1086, 1092, 1098, 1104, 1110, 1116, 1123, 1129, 1135, 1141, 1147, 1153, 1159, 1165, 1172,
	1178, 1184, 1190, 1196, 1202, 1208, 1215, 1221, 1227, 1233, 1239, 1245, 1251, 1257, 1264, 1270,
	1276, 1282, 1288, 1294, 1300, 1307, 1313, 1319, 1325, 1331, 1337, 1343, 1349, 1356, 1362, 1368,
	1374, 1380, 1386, 1392, 1399, 1405, 1411, 1417, 1423, 1429, 1435, 1441, 1448, 1454, 1460, 1466,
	1472, 1478, 1484, 1491, 1497, 1503, 1509, 1515, 1521, 1527, 1533, 1540, 1546, 1552, 1558, 1564,
	1570, 1576, 1583, 1589, 1595, 1601, 1607, 1613, 1619, 1625, 1632, 1638, 1644, 1650, 1656, 1662,
	1668, 1675, 1681, 1687, 1693, 1699, 1705, 1711, 1717, 1724, 1730, 1736, 1742, 1748, 1754, 1760,
	1767, 1773, 1779, 1785, 1791, 1797, 1803, 1810, 1816, 1822, 1828, 1834, 1840, 1846, 1852, 1859,
	1865, 1871, 1877, 1883, 1889, 1895, 1902, 1908, 1914, 1920, 1926, 1932, 1938, 1944, 1951, 1957,
	1963, 1969, 1975, 1981, 1987, 1994, 2000, 2006, 2012, 2018, 2024, 2030, 2036, 2043, 2049, 2055,
	2061, 2067, 2073, 2079, 2086, 2092, 2098, 2104, 2110, 2116, 2122, 2128, 2135, 2141, 2147, 2153,
	2159, 2165, 2171, 2178, 2184, 2190, 2196, 2202, 2208, 2214, 2220, 2227, 2233, 2239, 2245, 2251,
	2257, 2263, 2270, 2276, 2282, 2288, 2294, 2300, 2306, 2312, 2319, 2325, 2331, 2337, 2343, 2349,
	2355, 2362, 2368, 2374, 2380, 2386, 2392, 2398, 2404, 2411, 2417, 2423, 2429, 2435, 2441, 2447,
	2454, 2460, 2466, 2472, 2478, 2484, 2490, 2497, 2503, 2509, 2515, 2521, 2527, 2533, 2539, 2546,
	2552, 2558, 2564, 2570, 2576, 2582, 2589, 2595, 2601, 2607, 2613, 2619, 2625, 2631, 2638, 2644,
	2650, 2656, 2662, 2668, 2674, 2681, 2687, 2693, 2699, 2705, 2711, 2717, 2723, 2730, 2736, 2742,
	2748, 2754, 2760, 2766, 2773, 2779, 2785, 2791, 2797, 2803, 2809, 2815, 2822, 2828, 2834, 2840,
	2846, 2852, 2858, 2865, 2871, 2877, 2883, 2889, 2895, 2901, 2907, 2914, 2920, 2926, 2932, 2938,
	2944, 2950, 2957, 2963, 2969, 2975, 2981, 2987, 2993, 2999, 3006, 3012, 3018, 3024, 3030, 3036,
	3042, 3049, 3055, 3061, 3067, 3073, 3079, 3085, 3091, 3098, 3104, 3110, 3116, 3122, 3128, 3134,
	3141, 3147, 3153, 3159, 3165, 3171, 3177, 3184, 3190, 3196, 3202, 3208, 3214, 3220, 3226, 3233,
	3239, 3245, 3251, 3257, 3263, 3269, 3276, 3282, 3288, 3294, 3300, 3306, 3312, 3318, 3325, 3331,
	3337, 3343, 3349, 3355, 3361, 3368, 3374, 3380, 3386, 3392, 3398, 3404, 3410, 3417, 3423, 3429,
	3435, 3441, 3447, 3453, 3460, 3466, 3472, 3478, 3484, 3490, 3496, 3502, 3509, 3515, 3521, 3527,
	3533, 3539, 3545, 3552, 3558, 3564, 3570, 3576, 3582, 3588, 3594, 3601, 3607, 3613, 3619, 3625,
	3631, 3637, 3644, 3650, 3656, 3662, 3668, 3674, 3680, 3686, 3693, 3699, 3705, 3711, 3717, 3723,
	3729, 3736, 3742, 3748, 3754, 3760, 3766, 3772, 3778, 3785, 3791, 3797, 3803, 3809, 3815, 3821,
	3828, 3834, 3840, 3846, 3852, 3858, 3864, 3871, 3877, 3883, 3889, 3895, 3901, 3907, 3913, 3920,
	3926, 3932, 3938, 3944, 3950, 3956, 3963, 3969, 3975, 3981, 3987, 3993, 3999, 4005, 4012, 4018,
	4024, 4030, 4036, 4042, 4048, 4055, 4061, 4067, 4073, 4079, 4085, 4091, 4097, 4104, 4110, 4116,
	4122, 4128, 4134, 4140, 4147, 4153, 4159, 4165, 4171, 4177, 4183, 4189, 4196, 4202, 4208, 4214,
	4220, 4226, 4232, 4239, 4245, 4251, 4257, 4263, 4269, 4275, 4281, 4288, 4294, 4300, 4306, 4312,
	4318, 4324, 4331, 4337, 4343, 4349, 4355, 4361, 4367, 4373, 4380, 4386, 4392, 4398, 4404, 4410,
	4416, 4423, 4429, 4435, 4441, 4447, 4453, 4459, 4465, 4472, 4478, 4484, 4490, 4496, 4502, 4508,
	4515, 4521, 4527, 4533, 4539, 4545, 4551, 4558, 4564, 4570, 4576, 4582, 4588, 4594, 4600, 4607,
	4613, 4619, 4625, 4631, 4637, 4643, 4650, 4656, 4662, 4668, 4674, 4680, 4686, 4692, 4699, 4705,
	4711, 4717, 4723, 4729, 4735, 4742, 4748, 4754, 4760, 4766, 4772, 4778, 4784, 4791, 4797, 4803,
	4809, 4815, 4821, 4827, 4834, 4840, 4846, 4852, 4858, 4864, 4870, 4876, 4883, 4889, 4895, 4901,
	4907, 4913, 4919, 4926, 4932, 4938, 4944, 4950, 4956, 4962, 4968, 4975, 4981, 4987, 4993, 4999,
	5005, 5011, 5018, 5024, 5030, 5036, 5042, 5048, 5054, 5060, 5067, 5073, 5079, 5085, 5091, 5097,
	5103, 5110, 5116, 5122, 5128, 5134, 5140, 5146, 5152, 5159, 5165, 5171, 5177, 5183, 5189, 5195,
	5202, 5208, 5214, 5220, 5226, 5232, 5238, 5245, 5251, 5257, 5263, 5269, 5275, 5281, 5287, 5294,
	5300, 5306, 5312, 5318, 5324, 5330, 5337, 5343, 5349, 5355, 5361, 5367, 5373, 5379, 5386, 5392,
	5398, 5404, 5410, 5416, 5422, 5429, 5435, 5441, 5447, 5453, 5459, 5465, 5471, 5478, 5484, 5490,
	5496, 5502, 5508, 5514, 5521, 5527, 5533, 5539, 5545, 5551, 5557, 5563, 5570, 5576, 5582, 5588,
	5594, 5600, 5606, 5613, 5619, 5625, 5631, 5637, 5643, 5649, 5655, 5662, 5668, 5674, 5680, 5686,
	5692, 5698, 5705, 5711, 5717, 5723, 5729, 5735, 5741, 5747, 5754, 5760, 5766, 5772, 5778, 5784,
	5790, 5797, 5803, 5809, 5815, 5821, 5827, 5833, 5839, 5846, 5852, 5858, 5864, 5870, 5876, 5882,
	5889, 5895, 5901, 5907, 5913, 5919, 5925, 5932, 5938, 5944, 5950, 5956, 5962, 5968, 5974, 5981,
	5987, 5993, 5999, 6005, 6011, 6017, 6024, 6030, 6036, 6042, 6048, 6054, 6060, 6066, 6073, 6079,
	6085, 6091, 6097, 6103, 6109, 6116, 6122, 6128, 6134, 6140, 6146, 6152, 6158, 6165, 6171, 6177,
	6183, 6189, 6195, 6201, 6208, 6214, 6220, 6226, 6232, 6238, 6244, 6250, 6257, 6263, 6269, 6275,
};

//  mA = (ADC_COUNT * VREF) / (1023 * I_GAIN * I_RESISTOR) = (ADC_COUNT * 1250) / (1023 * 50 * 0.02)
code const uint16_t adc_2_ma[] = {
	0, 1, 2, 4, 5, 6, 7, 9, 10, 11, 12, 13, 15, 16, 17, 18,
	20, 21, 22, 23, 24, 26, 27, 28, 29, 31, 32, 33, 34, 35, 37, 38,
	39, 40, 42, 43, 44, 45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 57,
	59, 60, 61, 62, 64, 65, 66, 67, 68, 70, 71, 72, 73, 75, 76, 77,
	78, 79, 81, 82, 83, 84, 86, 87, 88, 89, 90, 92, 93, 94, 95, 97,
	98, 99, 100, 101, 103, 104, 105, 106, 108, 109, 110, 111, 112, 114, 115, 116,
	117, 119, 120, 121, 122, 123, 125, 126, 127, 128, 130, 131, 132, 133, 134, 136,
	137, 138, 139, 141, 142, 143, 144, 145, 147, 148, 149, 150, 152, 153, 154, 155,
	156, 158, 159, 160, 161, 163, 164, 165, 166, 167, 169, 170, 171, 172, 174, 175,
	176, 177, 178, 180, 181, 182, 183, 185, 186, 187, 188, 189, 191, 192, 193, 194,
	196, 197, 198, 199, 200, 202, 203, 204, 205, 207, 208, 209, 210, 211, 213, 214,
	215, 216, 217, 219, 220, 221, 222, 224, 225, 226, 227, 228, 230, 231, 232, 233,
	235, 236, 237, 238, 239, 241, 242, 243, 244, 246, 247, 248, 249, 250, 252, 253,
	254, 255, 257, 258, 259, 260, 261, 263, 264, 265, 266, 268, 269, 270, 271, 272,
	274, 275, 276, 277, 279, 280, 281, 282, 283, 285, 286, 287, 288, 290, 291, 292,
	293, 294, 296, 297, 298, 299, 301, 302, 303, 304, 305, 307, 308, 309, 310, 312,
	313, 314, 315, 316, 318, 319, 320, 321, 323, 324, 325, 326, 327, 329, 330, 331,
	332, 334, 335, 336, 337, 338, 340, 341, 342, 343, 345, 346, 347, 348, 349, 351,
	352, 353, 354, 356, 357, 358, 359, 360, 362, 363, 364, 365, 367, 368, 369, 370,
	371, 373, 374, 375, 376, 378, 379, 380, 381, 382, 384, 385, 386, 387, 389, 390,
	391, 392, 393, 395, 396, 397, 398, 400, 401, 402, 403, 404, 406, 407, 408, 409,
	411, 412, 413, 414, 415, 417, 418, 419, 420, 422, 423, 424, 425, 426, 428, 429,
	430, 431, 433, 434, 435, 436, 437, 439, 440, 441, 442, 444, 445, 446, 447, 448,
	450, 451, 452, 453, 455, 456, 457, 458, 459, 461, 462, 463, 464, 466, 467, 468,
	469, 470, 472, 473, 474, 475, 477, 478, 479, 480, 481, 483, 484, 485, 486, 488,
	489, 490, 491, 492, 494, 495, 496, 497, 499, 500, 501, 502, 503, 505, 506, 507,
	508, 510, 511, 512, 513, 514, 516, 517, 518, 519, 521, 522, 523, 524, 525, 527,
	528, 529, 530, 532, 533, 534, 535, 536, 538, 539, 540, 541, 543, 544, 545, 546,
	547, 549, 550, 551, 552, 554, 555, 556, 557, 558, 560, 561, 562, 563, 565, 566,
	567, 568, 569, 571, 572, 573, 574, 576, 577, 578, 579, 580, 582, 583, 584, 585,
	587, 588, 589, 590, 591, 593, 594, 595, 596, 598, 599, 600, 601, 602, 604, 605,
	606, 607, 609, 610, 611, 612, 613, 615, 616, 617, 618, 620, 621, 622, 623, 624,
	626, 627, 628, 629, 630, 632, 633, 634, 635, 637, 638, 639, 640, 641, 643, 644,
	645, 646, 648, 649, 650, 651, 652, 654, 655, 656, 657, 659, 660, 661, 662, 663,
	665, 666, 667, 668, 670, 671, 672, 673, 674, 676, 677, 678, 679, 681, 682, 683,
	684, 685, 687, 688, 689, 690, 692, 693, 694, 695, 696, 698, 699, 700, 701, 703,
	704, 705, 706, 707, 709, 710, 711, 712, 714, 715, 716, 717, 718, 720, 721, 722,
	723, 725, 726, 727, 728, 729, 731, 732, 733, 734, 736, 737, 738, 739, 740, 742,
	743, 744, 745, 747, 748, 749, 750, 751, 753, 754, 755, 756, 758, 759, 760, 761,
	762, 764, 765, 766, 767, 769, 770, 771, 772, 773, 775, 776, 777, 778, 780, 781,
	782, 783, 784, 786, 787, 788, 789, 791, 792, 793, 794, 795, 797, 798, 799, 800,
	802, 803, 804, 805, 806, 808, 809, 810, 811, 813, 814, 815, 816, 817, 819, 820,
	821, 822, 824, 825, 826, 827, 828, 830, 831, 832, 833, 835, 836, 837, 838, 839,
	841, 842, 843, 844, 846, 847, 848, 849, 850, 852, 853, 854, 855, 857, 858, 859,
	860, 861, 863, 864, 865, 866, 868, 869, 870, 871, 872, 874, 875, 876, 877, 879,
	880, 881, 882, 883, 885, 886, 887, 888, 890, 891, 892, 893, 894, 896, 897, 898,
	899, 901, 902, 903, 904, 905, 907, 908, 909, 910, 912, 913, 914, 915, 916, 918,
	919, 920, 921, 923, 924, 925, 926, 927, 929, 930, 931, 932, 934, 935, 936, 937,
	938, 940, 941, 942, 943, 945, 946, 947, 948, 949, 951, 952, 953, 954, 956, 957,
	958, 959, 960, 962, 963, 964, 965, 967, 968, 969, 970, 971, 973, 974, 975, 976,
	978, 979, 980, 981, 982, 984, 985, 986, 987, 989, 990, 991, 992, 993, 995, 996,
	997, 998, 1000, 1001, 1002, 1003, 1004, 1006, 1007, 1008, 1009, 1011, 1012, 1013, 1014, 1015,
	1017, 1018, 1019, 1020, 1022, 1023, 1024, 1025, 1026, 1028, 1029, 1030, 1031, 1033, 1034, 1035,
	1036, 1037, 1039, 1040, 1041, 1042, 1043, 1045, 1046, 1047, 1048, 1050, 1051, 1052, 1053, 1054,
	1056, 1057, 1058, 1059, 1061, 1062, 1063, 1064, 1065, 1067, 1068, 1069, 1070, 1072, 1073, 1074,
	1075, 1076, 1078, 1079, 1080, 1081, 1083, 1084, 1085, 1086, 1087, 1089, 1090, 1091, 1092, 1094,
	1095, 1096, 1097, 1098, 1100, 1101, 1102, 1103, 1105, 1106, 1107, 1108, 1109, 1111, 1112, 1113,
	1114, 1116, 1117, 1118, 1119, 1120, 1122, 1123, 1124, 1125, 1127, 1128, 1129, 1130, 1131, 1133,
	1134, 1135, 1136, 1138, 1139, 1140, 1141, 1142, 1144, 1145, 1146, 1147, 1149, 1150, 1151, 1152,
	1153, 1155, 1156, 1157, 1158, 1160, 1161, 1162, 1163, 1164, 1166, 1167, 1168, 1169, 1171, 1172,
	1173, 1174, 1175, 1177, 1178, 1179, 1180, 1182, 1183, 1184, 1185, 1186, 1188, 1189, 1190, 1191,
	1193, 1194, 1195, 1196, 1197, 1199, 1200, 1201, 1202, 1204, 1205, 1206, 1207, 1208, 1210, 1211,
	1212, 1213, 1215, 1216, 1217, 1218, 1219, 1221, 1222, 1223, 1224, 1226, 1227, 1228, 1229, 1230,
	1232, 1233, 1234, 1235, 1237, 1238, 1239, 1240, 1241, 1243, 1244, 1245, 1246, 1248, 1249, 1250,
};




//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------

// Filtered measurement variables
static volatile uint32_t SI_SEG_IDATA adcFilterSum[4];

// Temperature measurement variables
static volatile uint16_t SI_SEG_IDATA adcTempAvgArray[ADC_NUM_TEMP_SMPLS];
static volatile uint8_t SI_SEG_IDATA adcTempAvgIndex;

// State management variables
static volatile uint8_t adcMeasIndex;	     // Current index being measured
static volatile uint8_t adcStoredMeasIndex;  // Index to restore after temperature measurement "interruption"
static volatile uint16_t adcTempMeasCount;	 // Counts down series between temperature measurements (slow)

// Timer 0 sample control
static volatile uint8_t adcTimer0Reload;
static volatile bit adcTimer0ReloadInc;



//-----------------------------------------------------------------------------
// Forward declarations for internal functions
//-----------------------------------------------------------------------------
static void _ADC_DelayMsec(uint8_t mSec);
static uint16_t _ADC_GetSingleReading(uint8_t adcChannel);
static void _ADC_PushFilteredVal(uint16_t val, uint8_t index);
static void _ADC_PushTemp(uint16_t val);
static int16_t _adc2IntT10(uint16_t adcVal);



//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void ADC_Init()
{
	uint8_t i;
	uint16_t v;

	// Make sure ADC Interrupts are disabled so ISRs don't fire while we're initializing
	ADC_DIS_INT();

	// Delay to allow voltages to stabilize
	_ADC_DelayMsec(50);

	// Initialize variables
	adcMeasIndex = 0;
	adcTempMeasCount = ADC_TEMP_EVAL_COUNT;
	adcTimer0Reload = 0x80;
	adcTimer0ReloadInc = 1;

	// Setup the filtered ADC value filters
	v = _ADC_GetSingleReading(_ADC_VU_CH);
	adcFilterSum[ADC_MEAS_VU_INDEX] = (v << ADC_V_FILTER_SHIFT);
	SMB_SetVU(adc_2_mv[v]);
	v = _ADC_GetSingleReading(_ADC_IU_CH);
	adcFilterSum[ADC_MEAS_IU_INDEX] = (v << ADC_I_FILTER_SHIFT);
	SMB_SetIU(adc_2_ma[v]);
	v = _ADC_GetSingleReading(_ADC_VB_CH);
	adcFilterSum[ADC_MEAS_VB_INDEX] = (v << ADC_V_FILTER_SHIFT);
	SMB_SetVB(adc_2_mv[v]);
	v = _ADC_GetSingleReading(_ADC_IL_CH);
	adcFilterSum[ADC_MEAS_IL_INDEX] = (v << ADC_I_FILTER_SHIFT);
	SMB_SetIL(adc_2_ma[v]);

	// Load the temperature averaging arrays
	v = _ADC_GetSingleReading(_ADC_TI_CH);
	for (i=0; i<ADC_NUM_TEMP_SMPLS-1; i++) {
		_ADC_PushTemp(v);
	}

	// Configure the ADC input for the initial reading
	ADC0MX = _ADC_VU_CH;

	// Enable Timer0
	TCON_TR0 = 1;

	// Enable ADC Interrupts
	ADC_EN_INT();
}



//-----------------------------------------------------------------------------
// Interrupt Handlers
//-----------------------------------------------------------------------------
// TIMER0_IRQ requires ~1.3 - 1.5 uSec typically
SI_INTERRUPT (TIMER0_ISR, TIMER0_IRQn)
{
	// Trigger an ADC reading
	ADC0CN0_ADBUSY = 1;

	// Update the reload value to skew the period between samples
	if (adcTimer0ReloadInc == 1) {
		if (++adcTimer0Reload == _ADC_TH0_MAX) {
			adcTimer0ReloadInc = 0;
		}
	} else {
		if (--adcTimer0Reload == _ADC_TH0_MIN) {
			adcTimer0ReloadInc = 1;
		}
	}
	TH0 = adcTimer0Reload;

	// Clear TCON::TF0 (Timer 0 Overflow Flag) - done by HW on entry to ISR
}


// ADC0E0C_IRQ requires ~22-27 uSec
// (greater if blocked while writing to SMBus, typ up to 400 uSec if reading time)
SI_INTERRUPT (ADC0EOC_ISR, ADC0EOC_IRQn)
{
	// Store the ADC result
	if (adcMeasIndex <= ADC_MEAS_IL_INDEX) {
		_ADC_PushFilteredVal(ADC0, adcMeasIndex);
	} else {
		// Temperature
		_ADC_PushTemp(ADC0);
	}

	// Setup the next ADC reading
	if (adcMeasIndex == ADC_MEAS_T_INDEX) {
		// Restore normal operation
		adcMeasIndex = adcStoredMeasIndex;
	} else {
		// Setup for next channel
		if (++adcMeasIndex > ADC_MEAS_IL_INDEX) {
			adcMeasIndex = 0;
		}
		if (--adcTempMeasCount == 0) {
			// Setup special case of occasional temperature measurement
			adcTempMeasCount = ADC_TEMP_EVAL_COUNT; // Reset temperature timer
			adcStoredMeasIndex = adcMeasIndex;      // Save current normal channel to restore
			adcMeasIndex = ADC_MEAS_T_INDEX;        // Internal temperature sensor
		}
	}

	// Configure the ADC input for the next reading
	switch (adcMeasIndex) {
	case ADC_MEAS_VU_INDEX:
		ADC0MX = _ADC_VU_CH;
		break;
	case ADC_MEAS_IU_INDEX:
		ADC0MX = _ADC_IU_CH;
		break;
	case ADC_MEAS_VB_INDEX:
		ADC0MX = _ADC_VB_CH;
		break;
	case ADC_MEAS_IL_INDEX:
		ADC0MX = _ADC_IL_CH;
		break;
	default:
		ADC0MX = _ADC_TI_CH;
		break;
	}

	// Clear ADC0CN0::ADINT (Conversion Complete Interrupt Flag)
	ADC0CN0_ADINT = 0;
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------

// Delay function designed for use during initialization - ADC/Timer0 interrupts must be disabled
void _ADC_DelayMsec(uint8_t mSec)
{
	uint8_t i;

	// Uses Timer0 overflow at ~250 uSec
	TCON_TF0 = 0;  // Clear overflow flag
	TL0 = TH0;     // Manually set timer for first time
	TCON_TR0 = 1;  // Enable Timer 0
	while (mSec--) {
		for (i=0; i<4; i++) {
			// Spin until timer overflows
			while (TCON_TF0 != 1) {};

			// Reset for next period
			TCON_TF0 = 0;
		}
	}

	// Disable Timer 0
	TCON_TR0 = 0;
	TCON_TF0 = 0;
}


// Single ADC measurement - ADC Interrupts must be disabled
uint16_t _ADC_GetSingleReading(uint8_t adcChannel)
{
	// Set the ADC Channel
	ADC0MX = adcChannel;

	// Wait >5 uSec for input to settle
	_ADC_DelayMsec(10);

	// Trigger ADC
	ADC0CN0_ADINT = 0;
	ADC0CN0_ADBUSY = 1;

	// Wait for ADC to finish
	while (ADC0CN0_ADINT != 1) {};
	ADC0CN0_ADINT = 0;

	return(ADC0);
}


void _ADC_PushFilteredVal(uint16_t val, uint8_t index)
{
	uint16_t v;

	// Even indices are voltage, odd are current
	if (index & 0x01) {
		// Update current filter with current sample
		adcFilterSum[index] = adcFilterSum[index] - (adcFilterSum[index] >> ADC_I_FILTER_SHIFT) + val;

		// Scale for unity gain
		v = adcFilterSum[index] >> ADC_I_FILTER_SHIFT;

		// Push value into SMBUS register
		if (index & 0x02) {
			SMB_SetIL(adc_2_ma[v]);
		} else {
			SMB_SetIU(adc_2_ma[v]);
		}
	} else {
		// Update current filter with current sample
		adcFilterSum[index] = adcFilterSum[index] - (adcFilterSum[index] >> ADC_V_FILTER_SHIFT) + val;

		// Scale for unity gain
		v = adcFilterSum[index] >> ADC_V_FILTER_SHIFT;

		// Push value into SMBUS register
		if (index & 0x02) {
			SMB_SetVB(adc_2_mv[v]);
		} else {
			SMB_SetVU(adc_2_mv[v]);
		}
	}
}


void _ADC_PushTemp(uint16_t val)
{
	uint8_t i;
	uint16_t tempAvgAdcVal = 0;

	// Push current value
	adcTempAvgArray[adcTempAvgIndex] = val;
	if (++adcTempAvgIndex == ADC_NUM_TEMP_SMPLS) adcTempAvgIndex = 0;

	// Compute current averaged ADC value
	tempAvgAdcVal = 0;
	for (i=0; i<ADC_NUM_TEMP_SMPLS; i++) {
		tempAvgAdcVal += adcTempAvgArray[i];
	}

	// Round up the pre-divided value if necessary and then scale to divide
	if (tempAvgAdcVal & ADC_TEMP_RND_MASK) {
		// Round up unshifted sum
		tempAvgAdcVal += ADC_TEMP_RND_MASK;
	}
	tempAvgAdcVal = tempAvgAdcVal >> ADC_TEMP_SHIFT;

	// Push value into SMBUS
	SMB_SetTemp(_adc2IntT10(tempAvgAdcVal));
}


// Return temp in units of C * 10
//
// Internal Temperature Sensor spec says slope is 3.4 mV/C and a 10-bit
// measurement for each device is held in the TOFF register (measured
// at 25C).  An offset for 0C can be calculated by subtracting the
// computed ADC count for 25 degrees C from the TOFF register and
// scaling to 10-bits.
int16_t _adc2IntT10(uint16_t adcVal)
{
	int32_t t;

	// Offset for 0-degree with internal calibration offset
	//   - Offset @ 0C is 940 mV nominal (10-bit ADC count = (940/ADC_VREF_MV)*1023 = 769 ideal
	//   - {TOFFH[7:0], TOFFL[7:6]} contain 10-bit calibrated offset voltage
	//   - Adjust ADC count to 0-degrees with calibration: adcVal - 769 - TOFF
	//
	//t = 961620 / ADC_VREF_MV;
	//t = (int32_t) adcVal - t;
	t = (int32_t) adcVal - 769;
	t = t - (((int32_t) TOFFH << 2) | ((int32_t) TOFFL >> 6));

	// Compute temperature
	//   T_C_10 = (100*ADC_CAL_MV*ADC_VREF_MV)/(1023 * 34)
	t = t * ADC_VREF_MV * 100;
	t = t / 34782;

	return (t);
}
