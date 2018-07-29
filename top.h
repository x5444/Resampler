#ifndef __TOP_H__
#define __TOP_H__

void top(
	       const ap_int<32> sampleRateFactor,
	       const ap_int<14> DataIn,
	       ap_int<14> &DataOut,
	       bool &enableOut
	     );

#endif // __TOP_H__
