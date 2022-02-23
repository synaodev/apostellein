// 12/03/29

#ifndef pxtoneNoise_H
#define pxtoneNoise_H

#include <pxtone/pxtn.h>

#include <pxtone/pxtnDescriptor.h>

class pxtoneNoise
{
private:

	void *_bldr ;
	int32_t  _ch_num;
	int32_t  _sps   ;
	int32_t  _bps   ;

public:
	pxtoneNoise();
	pxtoneNoise(const pxtoneNoise&) = delete;
	pxtoneNoise& operator=(const pxtoneNoise&) = delete;
	pxtoneNoise(pxtoneNoise&&) noexcept = default;
	pxtoneNoise& operator=(pxtoneNoise&&) noexcept = default;
	~pxtoneNoise();

	bool init       ();

	bool quality_set( int32_t    ch_num, int32_t    sps, int32_t    bps );
	void quality_get( int32_t *p_ch_num, int32_t *p_sps, int32_t *p_bps ) const;

	bool generate   ( pxtnDescriptor *p_doc, void **pp_buf, int32_t *p_size ) const;
};

#endif
