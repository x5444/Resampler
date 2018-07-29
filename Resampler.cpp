#include "Resampler.h"


Resampler::Resampler(){
	
	// Clear member variables.
	//
	m_nextSamplePos = 0;
	for(int k=0; k<5; k++){
		m_sampleBuffer[k] = 0;
	}
}


void Resampler::process(
				const ap_uint<c_nrOfBitsResampleFactor> sampleRateFactor,
				const ap_int<14> DataIn,
				ap_int<14> &DataOut,
				bool &enableOut
		){
	#pragma HLS pipeline
	
	ap_uint<c_nrOfBitsResampleFactorFractional> curSamplePos;
	
	// Each cycle is c_stepWidth steps.
	// In case we need to wait for more than a step width, we don't output
	// anything.
	//
	if (m_nextSamplePos >= c_stepWidth){
		curSamplePos     = 0;
		m_nextSamplePos -= c_stepWidth;
		enableOut        = 0;
	
	// If it's less than a step width, we need to output something right away.
	//
	} else {
		curSamplePos    = m_nextSamplePos;
		m_nextSamplePos = curSamplePos + sampleRateFactor - c_stepWidth;
		enableOut       = 1;
	}
	
	// Load/interpolate coefficients.
	//
	ap_uint<c_nrOfBitsResampleFactorFractional> mirrorSamplePos = ap_uint<c_nrOfBitsResampleFactorFractional>(c_stepWidth - curSamplePos);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientP2 = (curSamplePos == 0) ? ap_int<c_nrOfBitsCoefficientResolution>(0) : getCoefficient(mirrorSamplePos, sincRange2);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientP1 = (curSamplePos == 0) ? ap_int<c_nrOfBitsCoefficientResolution>(0) : getCoefficient(mirrorSamplePos, sincRange1);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientP0 = (curSamplePos == 0) ? ap_int<c_nrOfBitsCoefficientResolution>(0) : getCoefficient(mirrorSamplePos, sincRange0);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientN0 =                                                                    getCoefficient(   curSamplePos, sincRange0);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientN1 =                                                                    getCoefficient(   curSamplePos, sincRange1);
	ap_int<c_nrOfBitsCoefficientResolution> coefficientN2 =                                                                    getCoefficient(   curSamplePos, sincRange2);
	
	// Combine coefficients and samples.
	//
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend1 = coefficientP2 * m_sampleBuffer[0];
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend2 = coefficientP1 * m_sampleBuffer[1];
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend3 = coefficientP0 * m_sampleBuffer[2];
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend4 = coefficientN0 * m_sampleBuffer[3];
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend5 = coefficientN1 * m_sampleBuffer[4];
	ap_int<c_nrOfBitsCoefficientResolution + 14> addend6 = coefficientN2 * m_sampleBuffer[5];
	std::cout << "sampleBuffer[0]: " << m_sampleBuffer[0] <<  ", sampleBuffer[1]: " << m_sampleBuffer[1] <<  ", sampleBuffer[2]: " << m_sampleBuffer[2] <<  ", sampleBuffer[3]: " << m_sampleBuffer[3] << ", sampleBuffer[4]: " << m_sampleBuffer[4] <<  ", sampleBuffer[5]: " << m_sampleBuffer[5] << std::endl;
	
	// Generate output.
	//
	if (enableOut) {
		DataOut = (addend1 + addend2 + addend3 + addend4 + addend5 + addend6) >> (c_nrOfBitsCoefficientResolution - 1);
	}
	
	// Put samples into the buffer.
	//
	for (int k = 6-1; k > 0; k--){
		m_sampleBuffer[k] = m_sampleBuffer[k-1];
	}
	m_sampleBuffer[0] = DataIn;
}


ap_int<c_nrOfBitsCoefficientResolution> Resampler::getCoefficient(ap_int<c_nrOfBitsResampleFactorFractional> pos, const ap_int<c_nrOfBitsTableResolution> coeffTable[]){
	
	// Some magic happening here: First, generate the indices of relevant
	// samples. These are just two adjacent samples. Note that the right one may
	// be out of range if the left one is the last sample, so we need one bit
	// more and we'll take care of it later. The sample index itself is just the
	// upper bits of the position.
	//
	ap_uint<c_nrOfBitsTableTimeResolution  > idxLeft  = pos >> c_nrOfBitsTimeInterpolationFactor;
	ap_uint<c_nrOfBitsTableTimeResolution+1> idxRight = idxLeft + 1;
	
	// The fractional part is a bit more tricky. We're looking only at the lower
	// bits of the position. The higher these bits are, the further right we need
	// to go. Thus the lower bits correspond to the fractional "right" part. The
	// left part is then simply whatever is left from the total number of steps.
	// Note that the right part goes from 0 to almost the maximum and the left
	// part from 1 to the maximum. This is b/c if the right part were at the
	// maximum, we would just be one full sample further right.
	// Another thing to take care of here is that the number of bits for
	// amplitude interpolation is less than the number of bits for time
	// interpolation, so that's why the value is divided some more.
	//
	ap_uint<c_nrOfBitsResolutionInterpolationFactor  > fracRight = (pos % (1 << c_nrOfBitsTimeInterpolationFactor)) / (c_nrOfBitsTimeInterpolationFactor - c_nrOfBitsResolutionInterpolationFactor);
	ap_uint<c_nrOfBitsResolutionInterpolationFactor+1> fracLeft  = (1 << c_nrOfBitsResolutionInterpolationFactor) - fracRight;
	
	// The samples can be read from the table. The right sample may be out of
	// range, we return 0 in that case.
	//
	ap_int<c_nrOfBitsTableResolution> sampleLeft  =  coeffTable[idxLeft];
	ap_int<c_nrOfBitsTableResolution> sampleRight = (idxRight < (1 << c_nrOfBitsTableTimeResolution)) ? coeffTable[idxRight] : ap_int<12>(0);
	
	std::cout << "idxLeft = " << idxLeft << ", sampleLeft = " << sampleLeft << ", fracLeft = " << fracLeft << ", result = " << (fracLeft  * sampleLeft + fracRight * sampleRight) << std::endl;
	// Finally, do the actual interpolation.
	//
	return (fracLeft  * sampleLeft +
	        fracRight * sampleRight);
}


const ap_int<c_nrOfBitsTableResolution> Resampler::sincRange0[1 << c_nrOfBitsTableTimeResolution] = {
	2047,   2047,   2047,   2047,   2047,   2047,   2047,   2047,   2047,   2047,   2047,   2047,   2046,   2046,   2046,   2046,   2046,   2046,   2046,   2046,   2046,   2045,   2045,   2045,   2045,   2045,   2045,   2044,   2044,   2044,   2044,   2044,\
	2043,   2043,   2043,   2043,   2042,   2042,   2042,   2042,   2041,   2041,   2041,   2040,   2040,   2040,   2039,   2039,   2039,   2038,   2038,   2038,   2037,   2037,   2037,   2036,   2036,   2035,   2035,   2035,   2034,   2034,   2033,   2033,\
	2032,   2032,   2031,   2031,   2031,   2030,   2030,   2029,   2029,   2028,   2028,   2027,   2026,   2026,   2025,   2025,   2024,   2024,   2023,   2023,   2022,   2021,   2021,   2020,   2019,   2019,   2018,   2018,   2017,   2016,   2016,   2015,\
	2014,   2014,   2013,   2012,   2012,   2011,   2010,   2009,   2009,   2008,   2007,   2006,   2006,   2005,   2004,   2003,   2003,   2002,   2001,   2000,   1999,   1999,   1998,   1997,   1996,   1995,   1994,   1993,   1993,   1992,   1991,   1990,\
	1989,   1988,   1987,   1986,   1985,   1985,   1984,   1983,   1982,   1981,   1980,   1979,   1978,   1977,   1976,   1975,   1974,   1973,   1972,   1971,   1970,   1969,   1968,   1967,   1966,   1965,   1964,   1962,   1961,   1960,   1959,   1958,\
	1957,   1956,   1955,   1954,   1953,   1951,   1950,   1949,   1948,   1947,   1946,   1944,   1943,   1942,   1941,   1940,   1938,   1937,   1936,   1935,   1934,   1932,   1931,   1930,   1929,   1927,   1926,   1925,   1923,   1922,   1921,   1920,\
	1918,   1917,   1916,   1914,   1913,   1912,   1910,   1909,   1908,   1906,   1905,   1903,   1902,   1901,   1899,   1898,   1896,   1895,   1894,   1892,   1891,   1889,   1888,   1886,   1885,   1884,   1882,   1881,   1879,   1878,   1876,   1875,\
	1873,   1872,   1870,   1869,   1867,   1866,   1864,   1862,   1861,   1859,   1858,   1856,   1855,   1853,   1851,   1850,   1848,   1847,   1845,   1843,   1842,   1840,   1839,   1837,   1835,   1834,   1832,   1830,   1829,   1827,   1825,   1824,\
	1822,   1820,   1819,   1817,   1815,   1813,   1812,   1810,   1808,   1807,   1805,   1803,   1801,   1800,   1798,   1796,   1794,   1792,   1791,   1789,   1787,   1785,   1783,   1782,   1780,   1778,   1776,   1774,   1772,   1771,   1769,   1767,\
	1765,   1763,   1761,   1759,   1758,   1756,   1754,   1752,   1750,   1748,   1746,   1744,   1742,   1740,   1738,   1737,   1735,   1733,   1731,   1729,   1727,   1725,   1723,   1721,   1719,   1717,   1715,   1713,   1711,   1709,   1707,   1705,\
	1703,   1701,   1699,   1697,   1695,   1693,   1691,   1689,   1687,   1684,   1682,   1680,   1678,   1676,   1674,   1672,   1670,   1668,   1666,   1664,   1662,   1659,   1657,   1655,   1653,   1651,   1649,   1647,   1644,   1642,   1640,   1638,\
	1636,   1634,   1631,   1629,   1627,   1625,   1623,   1621,   1618,   1616,   1614,   1612,   1610,   1607,   1605,   1603,   1601,   1598,   1596,   1594,   1592,   1589,   1587,   1585,   1583,   1580,   1578,   1576,   1574,   1571,   1569,   1567,\
	1564,   1562,   1560,   1557,   1555,   1553,   1550,   1548,   1546,   1544,   1541,   1539,   1537,   1534,   1532,   1529,   1527,   1525,   1522,   1520,   1518,   1515,   1513,   1511,   1508,   1506,   1503,   1501,   1499,   1496,   1494,   1491,\
	1489,   1487,   1484,   1482,   1479,   1477,   1474,   1472,   1470,   1467,   1465,   1462,   1460,   1457,   1455,   1452,   1450,   1447,   1445,   1443,   1440,   1438,   1435,   1433,   1430,   1428,   1425,   1423,   1420,   1418,   1415,   1413,\
	1410,   1408,   1405,   1403,   1400,   1398,   1395,   1393,   1390,   1387,   1385,   1382,   1380,   1377,   1375,   1372,   1370,   1367,   1365,   1362,   1359,   1357,   1354,   1352,   1349,   1347,   1344,   1341,   1339,   1336,   1334,   1331,\
	1328,   1326,   1323,   1321,   1318,   1315,   1313,   1310,   1308,   1305,   1302,   1300,   1297,   1295,   1292,   1289,   1287,   1284,   1281,   1279,   1276,   1274,   1271,   1268,   1266,   1263,   1260,   1258,   1255,   1252,   1250,   1247,\
	1244,   1242,   1239,   1236,   1234,   1231,   1228,   1226,   1223,   1220,   1218,   1215,   1212,   1210,   1207,   1204,   1202,   1199,   1196,   1194,   1191,   1188,   1186,   1183,   1180,   1178,   1175,   1172,   1169,   1167,   1164,   1161,\
	1159,   1156,   1153,   1150,   1148,   1145,   1142,   1140,   1137,   1134,   1132,   1129,   1126,   1123,   1121,   1118,   1115,   1112,   1110,   1107,   1104,   1102,   1099,   1096,   1093,   1091,   1088,   1085,   1082,   1080,   1077,   1074,\
	1072,   1069,   1066,   1063,   1061,   1058,   1055,   1052,   1050,   1047,   1044,   1041,   1039,   1036,   1033,   1030,   1028,   1025,   1022,   1019,   1017,   1014,   1011,   1008,   1006,   1003,   1000,    998,    995,    992,    989,    987,\
	 984,    981,    978,    976,    973,    970,    967,    965,    962,    959,    956,    954,    951,    948,    945,    943,    940,    937,    934,    932,    929,    926,    923,    921,    918,    915,    912,    910,    907,    904,    901,    899,\
	 896,    893,    890,    888,    885,    882,    879,    877,    874,    871,    868,    866,    863,    860,    857,    855,    852,    849,    847,    844,    841,    838,    836,    833,    830,    827,    825,    822,    819,    817,    814,    811,\
	 808,    806,    803,    800,    797,    795,    792,    789,    787,    784,    781,    778,    776,    773,    770,    768,    765,    762,    759,    757,    754,    751,    749,    746,    743,    741,    738,    735,    732,    730,    727,    724,\
	 722,    719,    716,    714,    711,    708,    706,    703,    700,    698,    695,    692,    690,    687,    684,    682,    679,    676,    674,    671,    668,    666,    663,    660,    658,    655,    652,    650,    647,    644,    642,    639,\
	 636,    634,    631,    629,    626,    623,    621,    618,    615,    613,    610,    608,    605,    602,    600,    597,    595,    592,    589,    587,    584,    581,    579,    576,    574,    571,    569,    566,    563,    561,    558,    556,\
	 553,    551,    548,    545,    543,    540,    538,    535,    533,    530,    527,    525,    522,    520,    517,    515,    512,    510,    507,    505,    502,    500,    497,    495,    492,    490,    487,    485,    482,    480,    477,    475,\
	 472,    470,    467,    465,    462,    460,    457,    455,    452,    450,    447,    445,    442,    440,    437,    435,    432,    430,    428,    425,    423,    420,    418,    415,    413,    411,    408,    406,    403,    401,    399,    396,\
	 394,    391,    389,    387,    384,    382,    379,    377,    375,    372,    370,    367,    365,    363,    360,    358,    356,    353,    351,    349,    346,    344,    342,    339,    337,    335,    332,    330,    328,    325,    323,    321,\
	 318,    316,    314,    312,    309,    307,    305,    302,    300,    298,    296,    293,    291,    289,    287,    284,    282,    280,    278,    275,    273,    271,    269,    267,    264,    262,    260,    258,    256,    253,    251,    249,\
	 247,    245,    242,    240,    238,    236,    234,    232,    229,    227,    225,    223,    221,    219,    217,    214,    212,    210,    208,    206,    204,    202,    200,    197,    195,    193,    191,    189,    187,    185,    183,    181,\
	 179,    177,    175,    173,    171,    169,    166,    164,    162,    160,    158,    156,    154,    152,    150,    148,    146,    144,    142,    140,    138,    136,    134,    132,    130,    128,    127,    125,    123,    121,    119,    117,\
	 115,    113,    111,    109,    107,    105,    103,    101,    100,     98,     96,     94,     92,     90,     88,     86,     84,     83,     81,     79,     77,     75,     73,     72,     70,     68,     66,     64,     62,     61,     59,     57,\
	  55,     53,     52,     50,     48,     46,     44,     43,     41,     39,     37,     36,     34,     32,     30,     29,     27,     25,     24,     22,     20,     18,     17,     15,     13,     12,     10,      8,      7,      5,      3,      2 \
};


const ap_int<c_nrOfBitsTableResolution> Resampler::sincRange1[1 << c_nrOfBitsTableTimeResolution] = {
	   0,   -2,   -3,   -5,   -7,   -8,  -10,  -11,  -13,  -15,  -16,  -18,  -20,  -21,  -23,  -24,  -26,  -27,  -29,  -31,  -32,  -34,  -35,  -37,  -38,  -40,  -41,  -43,  -45,  -46,  -48,  -49,\
	 -51,  -52,  -54,  -55,  -57,  -58,  -60,  -61,  -62,  -64,  -65,  -67,  -68,  -70,  -71,  -73,  -74,  -76,  -77,  -78,  -80,  -81,  -83,  -84,  -85,  -87,  -88,  -90,  -91,  -92,  -94,  -95,\
	 -96,  -98,  -99, -100, -102, -103, -105, -106, -107, -108, -110, -111, -112, -114, -115, -116, -118, -119, -120, -121, -123, -124, -125, -126, -128, -129, -130, -131, -133, -134, -135, -136,\
	-138, -139, -140, -141, -142, -144, -145, -146, -147, -148, -149, -151, -152, -153, -154, -155, -156, -157, -159, -160, -161, -162, -163, -164, -165, -166, -167, -168, -170, -171, -172, -173,\
	-174, -175, -176, -177, -178, -179, -180, -181, -182, -183, -184, -185, -186, -187, -188, -189, -190, -191, -192, -193, -194, -195, -196, -197, -198, -199, -200, -201, -202, -203, -204, -204,\
	-205, -206, -207, -208, -209, -210, -211, -212, -212, -213, -214, -215, -216, -217, -218, -218, -219, -220, -221, -222, -223, -223, -224, -225, -226, -227, -227, -228, -229, -230, -231, -231,\
	-232, -233, -234, -234, -235, -236, -237, -237, -238, -239, -240, -240, -241, -242, -242, -243, -244, -244, -245, -246, -247, -247, -248, -249, -249, -250, -250, -251, -252, -252, -253, -254,\
	-254, -255, -256, -256, -257, -257, -258, -259, -259, -260, -260, -261, -261, -262, -263, -263, -264, -264, -265, -265, -266, -266, -267, -267, -268, -268, -269, -270, -270, -271, -271, -272,\
	-272, -272, -273, -273, -274, -274, -275, -275, -276, -276, -277, -277, -277, -278, -278, -279, -279, -280, -280, -280, -281, -281, -282, -282, -282, -283, -283, -284, -284, -284, -285, -285,\
	-285, -286, -286, -286, -287, -287, -287, -288, -288, -288, -289, -289, -289, -290, -290, -290, -290, -291, -291, -291, -292, -292, -292, -292, -293, -293, -293, -293, -294, -294, -294, -294,\
	-295, -295, -295, -295, -295, -296, -296, -296, -296, -296, -297, -297, -297, -297, -297, -298, -298, -298, -298, -298, -298, -298, -299, -299, -299, -299, -299, -299, -299, -300, -300, -300,\
	-300, -300, -300, -300, -300, -300, -300, -300, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301,\
	-301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -301, -300, -300, -300, -300, -300, -300, -300, -300, -300,\
	-300, -300, -299, -299, -299, -299, -299, -299, -299, -299, -298, -298, -298, -298, -298, -298, -298, -297, -297, -297, -297, -297, -297, -296, -296, -296, -296, -296, -295, -295, -295, -295,\
	-295, -294, -294, -294, -294, -294, -293, -293, -293, -293, -293, -292, -292, -292, -292, -291, -291, -291, -291, -290, -290, -290, -290, -289, -289, -289, -289, -288, -288, -288, -287, -287,\
	-287, -287, -286, -286, -286, -285, -285, -285, -285, -284, -284, -284, -283, -283, -283, -282, -282, -282, -281, -281, -281, -280, -280, -280, -279, -279, -279, -278, -278, -278, -277, -277,\
	-277, -276, -276, -275, -275, -275, -274, -274, -274, -273, -273, -272, -272, -272, -271, -271, -271, -270, -270, -269, -269, -269, -268, -268, -267, -267, -266, -266, -266, -265, -265, -264,\
	-264, -264, -263, -263, -262, -262, -261, -261, -260, -260, -260, -259, -259, -258, -258, -257, -257, -256, -256, -256, -255, -255, -254, -254, -253, -253, -252, -252, -251, -251, -250, -250,\
	-249, -249, -248, -248, -247, -247, -247, -246, -246, -245, -245, -244, -244, -243, -243, -242, -242, -241, -241, -240, -240, -239, -238, -238, -237, -237, -236, -236, -235, -235, -234, -234,\
	-233, -233, -232, -232, -231, -231, -230, -230, -229, -228, -228, -227, -227, -226, -226, -225, -225, -224, -224, -223, -223, -222, -221, -221, -220, -220, -219, -219, -218, -218, -217, -216,\
	-216, -215, -215, -214, -214, -213, -212, -212, -211, -211, -210, -210, -209, -208, -208, -207, -207, -206, -206, -205, -204, -204, -203, -203, -202, -201, -201, -200, -200, -199, -199, -198,\
	-197, -197, -196, -196, -195, -194, -194, -193, -193, -192, -191, -191, -190, -190, -189, -188, -188, -187, -187, -186, -185, -185, -184, -184, -183, -182, -182, -181, -181, -180, -179, -179,\
	-178, -178, -177, -176, -176, -175, -175, -174, -173, -173, -172, -171, -171, -170, -170, -169, -168, -168, -167, -167, -166, -165, -165, -164, -163, -163, -162, -162, -161, -160, -160, -159,\
	-159, -158, -157, -157, -156, -155, -155, -154, -154, -153, -152, -152, -151, -151, -150, -149, -149, -148, -147, -147, -146, -146, -145, -144, -144, -143, -142, -142, -141, -141, -140, -139,\
	-139, -138, -138, -137, -136, -136, -135, -134, -134, -133, -133, -132, -131, -131, -130, -130, -129, -128, -128, -127, -126, -126, -125, -125, -124, -123, -123, -122, -122, -121, -120, -120,\
	-119, -118, -118, -117, -117, -116, -115, -115, -114, -114, -113, -112, -112, -111, -111, -110, -109, -109, -108, -107, -107, -106, -106, -105, -104, -104, -103, -103, -102, -101, -101, -100,\
	-100,  -99,  -98,  -98,  -97,  -97,  -96,  -95,  -95,  -94,  -94,  -93,  -92,  -92,  -91,  -91,  -90,  -90,  -89,  -88,  -88,  -87,  -87,  -86,  -85,  -85,  -84,  -84,  -83,  -82,  -82,  -81,\
	 -81,  -80,  -80,  -79,  -78,  -78,  -77,  -77,  -76,  -76,  -75,  -74,  -74,  -73,  -73,  -72,  -72,  -71,  -70,  -70,  -69,  -69,  -68,  -68,  -67,  -66,  -66,  -65,  -65,  -64,  -64,  -63,\
	 -63,  -62,  -61,  -61,  -60,  -60,  -59,  -59,  -58,  -58,  -57,  -57,  -56,  -55,  -55,  -54,  -54,  -53,  -53,  -52,  -52,  -51,  -51,  -50,  -50,  -49,  -48,  -48,  -47,  -47,  -46,  -46,\
	 -45,  -45,  -44,  -44,  -43,  -43,  -42,  -42,  -41,  -41,  -40,  -40,  -39,  -39,  -38,  -38,  -37,  -36,  -36,  -35,  -35,  -34,  -34,  -33,  -33,  -32,  -32,  -31,  -31,  -30,  -30,  -29,\
	 -29,  -29,  -28,  -28,  -27,  -27,  -26,  -26,  -25,  -25,  -24,  -24,  -23,  -23,  -22,  -22,  -21,  -21,  -20,  -20,  -19,  -19,  -18,  -18,  -18,  -17,  -17,  -16,  -16,  -15,  -15,  -14,\
	 -14,  -13,  -13,  -13,  -12,  -12,  -11,  -11,  -10,  -10,   -9,   -9,   -9,   -8,   -8,   -7,   -7,   -6,   -6,   -5,   -5,   -5,   -4,   -4,   -3,   -3,   -3,   -2,   -2,   -1,   -1,    0 \
};

const ap_int<c_nrOfBitsTableResolution> Resampler::sincRange2[1 << c_nrOfBitsTableTimeResolution] = {
	 0,  0,  1,  1,  2,  2,  2,  3,  3,  4,  4,  4,  5,  5,  6,  6,  6,  7,  7,  8,  8,  8,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,\
	13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23,\
	24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 33, 33, 33,\
	33, 34, 34, 34, 35, 35, 35, 35, 36, 36, 36, 37, 37, 37, 37, 38, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 40, 41, 41, 41, 41, 42,\
	42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44, 45, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47, 47, 47, 47, 48, 48, 48, 48, 48, 49,\
	49, 49, 49, 49, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54,\
	54, 54, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 58, 58,\
	59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61,\
	61, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63,\
	63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,\
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 63, 63, 63, 63,\
	63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 62, 62, 62, 62, 62, 62, 62, 62,\
	62, 62, 62, 62, 62, 62, 62, 62, 62, 62, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 61, 60, 60, 60, 60, 60, 60, 60,\
	60, 60, 60, 60, 60, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 57, 57, 57, 57,\
	57, 57, 57, 57, 57, 57, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 55, 55, 55, 55, 55, 55, 55, 55, 55, 54, 54, 54, 54, 54, 54, 54,\
	54, 54, 53, 53, 53, 53, 53, 53, 53, 53, 52, 52, 52, 52, 52, 52, 52, 52, 52, 51, 51, 51, 51, 51, 51, 51, 51, 50, 50, 50, 50, 50,\
	50, 50, 50, 49, 49, 49, 49, 49, 49, 49, 48, 48, 48, 48, 48, 48, 48, 48, 47, 47, 47, 47, 47, 47, 47, 47, 46, 46, 46, 46, 46, 46,\
	46, 45, 45, 45, 45, 45, 45, 45, 44, 44, 44, 44, 44, 44, 44, 43, 43, 43, 43, 43, 43, 43, 43, 42, 42, 42, 42, 42, 42, 42, 41, 41,\
	41, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 39, 39, 39, 39, 39, 39, 39, 38, 38, 38, 38, 38, 38, 38, 37, 37, 37, 37, 37, 37,\
	37, 36, 36, 36, 36, 36, 36, 36, 35, 35, 35, 35, 35, 35, 35, 34, 34, 34, 34, 34, 34, 34, 33, 33, 33, 33, 33, 33, 32, 32, 32, 32,\
	32, 32, 32, 31, 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 30, 30, 30, 29, 29, 29, 29, 29, 29, 29, 28, 28, 28, 28, 28, 28, 28,\
	27, 27, 27, 27, 27, 27, 27, 26, 26, 26, 26, 26, 26, 26, 25, 25, 25, 25, 25, 25, 25, 25, 24, 24, 24, 24, 24, 24, 24, 23, 23, 23,\
	23, 23, 23, 23, 22, 22, 22, 22, 22, 22, 22, 22, 21, 21, 21, 21, 21, 21, 21, 21, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19,\
	19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15,\
	15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12,\
	12, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,\
	 9,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,\
	 6,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,\
	 4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  2,\
	 2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,\
	 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,\
	 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 \
};

