#ifndef __RESAMPLER_H__
#define __RESAMPLER_H__


#include "ap_int.h"

static const int c_nrOfBitsCoefficientResolution    = 16;
static const int c_nrOfBitsTableResolution          = 12;
static const int c_nrOfBitsResampleFactorFractional = 16;
static const int c_nrOfBitsResampleFactorInteger    = 16;
static const int c_nrOfBitsResampleFactor           = c_nrOfBitsResampleFactorInteger + c_nrOfBitsResampleFactorFractional;
static const int c_nrOfBitsTableTimeResolution      = 10;
static const int c_nrOfBitsTimeInterpolationFactor  = c_nrOfBitsResampleFactorFractional - c_nrOfBitsTableTimeResolution;
static const int c_nrOfBitsResolutionInterpolationFactor  = c_nrOfBitsCoefficientResolution - c_nrOfBitsTableResolution;

static const int c_stepWidth = 1 << c_nrOfBitsResampleFactorFractional;



class Resampler {
	public:
		Resampler();
		
		void process(
		              const ap_uint<c_nrOfBitsResampleFactor> sampleRateFactor,
		              const ap_int<14> DataIn,
		              ap_int<14> &DataOut,
		              bool &enableOut
		            );
		
		ap_int<c_nrOfBitsCoefficientResolution> getCoefficient(
		                          ap_int<c_nrOfBitsResampleFactorFractional> pos,
		                          const ap_int<c_nrOfBitsTableResolution> coeffTable[]
		                        );
		
		static int getStepWidth(){ return c_stepWidth; }
	
	private:
		static const ap_int<c_nrOfBitsTableResolution> sincRange0[1 << c_nrOfBitsTableTimeResolution];
		static const ap_int<c_nrOfBitsTableResolution> sincRange1[1 << c_nrOfBitsTableTimeResolution];
		static const ap_int<c_nrOfBitsTableResolution> sincRange2[1 << c_nrOfBitsTableTimeResolution];
		
		ap_uint<c_nrOfBitsResampleFactor> m_nextSamplePos;
		ap_int<14> m_sampleBuffer[6];
};

#endif // __RESAMPLER_H__
