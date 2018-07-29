#include "Resampler.h"
#include "top.h"


Resampler rsmpl;

void top(
	       const ap_int<32> sampleRateFactor,
	       const ap_int<14> DataIn,
	       ap_int<14> &DataOut,
	       bool &enableOut
	     ){
	rsmpl.process(sampleRateFactor, DataIn, DataOut, enableOut);
}
