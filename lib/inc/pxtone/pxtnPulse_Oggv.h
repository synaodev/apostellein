#ifndef pxtnPulse_Oggv_H
#define pxtnPulse_Oggv_H

#ifdef  pxINCLUDE_OGGVORBIS

#include <pxtone/pxtn.h>

#include <pxtone/pxtnDescriptor.h>
#include <pxtone/pxtnPulse_PCM.h>

class pxtnPulse_Oggv
{
private:

	int32_t _ch     ;
	int32_t _sps2   ;
	int32_t _smp_num;
	int32_t _size   ;
	char*   _p_data ;

	bool _SetInformation();

public :

	pxtnPulse_Oggv();
	pxtnPulse_Oggv(const pxtnPulse_Oggv&) = delete;
	pxtnPulse_Oggv& operator=(const pxtnPulse_Oggv&) = delete;
	pxtnPulse_Oggv(pxtnPulse_Oggv&&) noexcept = default;
	pxtnPulse_Oggv& operator=(pxtnPulse_Oggv&&) noexcept = default;
	~pxtnPulse_Oggv();

	pxtnERR Decode ( pxtnPulse_PCM *p_pcm ) const;
	void    Release();
	bool    GetInfo( int* p_ch, int* p_sps, int* p_smp_num );
	int32_t GetSize() const;

	bool    ogg_write ( pxtnDescriptor *p_doc ) const;
	pxtnERR ogg_read  ( pxtnDescriptor *p_doc );
	bool    pxtn_write( pxtnDescriptor *p_doc ) const;
	bool    pxtn_read ( pxtnDescriptor *p_doc );

	bool    Copy      ( pxtnPulse_Oggv *p_dst ) const;
};
#endif
#endif
