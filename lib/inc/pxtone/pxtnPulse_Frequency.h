#ifndef pxtnPulse_Frequency_H
#define pxtnPulse_Frequency_H

#include <pxtone/pxtn.h>

class pxtnPulse_Frequency
{
private:

	float* _freq_table;
	double _GetDivideOctaveRate( int32_t divi );

public:

	pxtnPulse_Frequency();
	pxtnPulse_Frequency(const pxtnPulse_Frequency&) = delete;
	pxtnPulse_Frequency& operator=(const pxtnPulse_Frequency&) = delete;
	pxtnPulse_Frequency(pxtnPulse_Frequency&&) noexcept = default;
	pxtnPulse_Frequency& operator=(pxtnPulse_Frequency&&) noexcept = default;
	~pxtnPulse_Frequency();

	bool Init();

	float        Get      ( int32_t key     );
	float        Get2     ( int32_t key     );
	const float* GetDirect( int32_t *p_size );
};

#endif
