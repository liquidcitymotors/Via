/*
 * sync3scales.cpp
 *
 *  Created on: Jul 21, 2019
 *      Author: willmitchell
 */

#include "sync3.hpp"


const ViaSync3::Sync3Scale ViaSync3::perfect = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 6, 6, 8, 8, 12, 12, 16, 16},
	{32, 32, 16, 16, 12, 12, 8, 8, 6, 6, 4, 4, 3, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{134217727, 134217727, 268435455, 268435455, 357913941, 357913941, 536870911, 536870911, 715827882, 715827882, 1073741823, 1073741823, 1431655765, 1431655765, 2147483647, 2147483647, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295},
	{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16},
	0
};

const ViaSync3::Sync3Scale ViaSync3::simpleRhythms = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 4, 4, 3, 3, 2, 2, 8, 8, 3, 3, 4, 4, 8, 8},
	{32, 32, 16, 16, 8, 8, 6, 6, 4, 4, 3, 3, 2, 2, 3, 3, 1, 1, 3, 3, 2, 2, 1, 1, 3, 3, 1, 1, 1, 1, 1, 1},
	{134217727, 134217727, 268435455, 268435455, 536870911, 536870911, 715827882, 715827882, 1073741823, 1073741823, 1431655765, 1431655765, 2147483647, 2147483647, 1431655765, 1431655765, 4294967295, 4294967295, 1431655765, 1431655765, 2147483647, 2147483647, 4294967295, 4294967295, 1431655765, 1431655765, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295},
	{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16},
	0
};

const ViaSync3::Sync3Scale ViaSync3::ints = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
	{16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{268435455, 286331153, 306783378, 330382099, 357913941, 390451572, 429496729, 477218588, 536870911, 613566756, 715827882, 858993459, 1073741823, 1431655765, 2147483647, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295, 4294967295},
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
	0
};

const ViaSync3::Sync3Scale ViaSync3::circleFifths = {
	{1, 1, 16, 16, 1, 1, 8, 8, 4, 4, 1, 1, 2, 2, 1, 1, 1, 1, 3, 3, 2, 2, 9, 9, 27, 27, 4, 4, 81, 81, 8, 8},
	{8, 8, 81, 81, 4, 4, 27, 27, 9, 9, 2, 2, 3, 3, 1, 1, 1, 1, 2, 2, 1, 1, 4, 4, 8, 8, 1, 1, 16, 16, 1, 1},
	{536870911, 536870911, 53024287, 53024287, 1073741823, 1073741823, 159072862, 159072862, 477218588, 477218588, 2147483647, 2147483647, 1431655765, 1431655765, 4294967295, 4294967295, 4294967295, 4294967295, 2147483647, 2147483647, 4294967295, 4294967295, 1073741823, 1073741823, 536870911, 536870911, 4294967295, 4294967295, 268435455, 268435455, 4294967295, 4294967295},
	{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15},
	0
};

const ViaSync3::Sync3Scale ViaSync3::fourthsFifths = {
	{1, 1, 3, 2, 1, 9, 1, 3, 1, 1, 3, 4, 1, 9, 2, 3, 1, 4, 3, 16, 2, 9, 8, 3, 4, 16, 6, 64, 8, 9, 32, 12},
	{8, 6, 16, 9, 4, 32, 3, 8, 4, 3, 8, 9, 2, 16, 3, 4, 1, 3, 2, 9, 1, 4, 3, 1, 1, 3, 1, 9, 1, 1, 3, 1},
	{536870911, 715827882, 268435455, 477218588, 1073741823, 134217727, 1431655765, 536870911, 1073741823, 1431655765, 536870911, 477218588, 2147483647, 268435455, 1431655765, 1073741823, 4294967295, 1431655765, 2147483647, 477218588, 4294967295, 1073741823, 1431655765, 4294967295, 4294967295, 1431655765, 4294967295, 477218588, 4294967295, 4294967295, 1431655765, 4294967295},
	{1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
	0
};

const ViaSync3::Sync3Scale ViaSync3::minorArp = {
	{1, 3, 3, 9, 1, 9, 3, 2, 1, 3, 3, 9, 1, 9, 3, 4, 1, 6, 3, 9, 2, 9, 3, 16, 4, 24, 6, 36, 8, 9, 12, 64},
	{8, 20, 16, 40, 4, 32, 8, 5, 4, 10, 8, 20, 2, 16, 4, 5, 1, 5, 2, 5, 1, 4, 1, 5, 1, 5, 1, 5, 1, 1, 1, 5},
	{536870911, 214748364, 268435455, 107374182, 1073741823, 134217727, 536870911, 858993459, 1073741823, 429496729, 536870911, 214748364, 2147483647, 268435455, 1073741823, 858993459, 4294967295, 858993459, 2147483647, 858993459, 4294967295, 1073741823, 4294967295, 858993459, 4294967295, 858993459, 4294967295, 858993459, 4294967295, 4294967295, 4294967295, 858993459},
	{1, 2, 3, 4, 5, 6, 7, 8, 8, 9, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
	0
};

const ViaSync3::Sync3Scale ViaSync3::evenOdds = {
	{1, 9, 7, 5, 11, 3, 5, 7, 1, 9, 7, 5, 11, 3, 5, 7, 1, 9, 7, 5, 11, 3, 5, 7, 2, 9, 7, 5, 11, 3, 10, 7},
	{4, 32, 24, 16, 32, 8, 12, 16, 2, 16, 12, 8, 16, 4, 6, 8, 1, 8, 6, 4, 8, 2, 3, 4, 1, 4, 3, 2, 4, 1, 3, 2},
	{1073741823, 134217727, 178956970, 268435455, 134217727, 536870911, 357913941, 268435455, 2147483647, 268435455, 357913941, 536870911, 268435455, 1073741823, 715827882, 536870911, 4294967295, 536870911, 715827882, 1073741823, 536870911, 2147483647, 1431655765, 1073741823, 4294967295, 1073741823, 1431655765, 2147483647, 1073741823, 4294967295, 1431655765, 2147483647},
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
	0
};

const ViaSync3::Sync3Scale ViaSync3::bP = {
	{1, 1, 7, 5, 1, 5, 7, 25, 1, 3, 7, 5, 3, 5, 7, 25, 1, 9, 7, 5, 9, 15, 7, 25, 3, 27, 21, 5, 27, 45, 7, 25},
	{9, 7, 45, 27, 5, 21, 27, 81, 3, 7, 15, 9, 5, 7, 9, 27, 1, 7, 5, 3, 5, 7, 3, 9, 1, 7, 5, 1, 5, 7, 1, 3},
	{477218588, 613566756, 95443717, 159072862, 858993459, 204522252, 159072862, 53024287, 1431655765, 613566756, 286331153, 477218588, 858993459, 613566756, 477218588, 159072862, 4294967295, 613566756, 858993459, 1431655765, 858993459, 613566756, 1431655765, 477218588, 4294967295, 613566756, 858993459, 4294967295, 858993459, 613566756, 4294967295, 1431655765},
	{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32},
	0
};

const struct ViaSync3::Sync3Scale * ViaSync3::scales[8] = {&ViaSync3::perfect, &ViaSync3::simpleRhythms, &ViaSync3::ints, &ViaSync3::circleFifths, &ViaSync3::fourthsFifths, &ViaSync3::minorArp, &ViaSync3::evenOdds, &ViaSync3::bP};

