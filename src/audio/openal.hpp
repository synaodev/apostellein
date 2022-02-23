#pragma once

#include <apostellein/def.hpp>

#if defined(APOSTELLEIN_PLATFORM_LINUX) || defined(DAPOSTELLEIN_VCPKG_LIBRARIES)
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
	#include <AL/efx.h>
	#include <AL/efx-presets.h>
#else
	#include <al.h>
	#include <alc.h>
	#include <alext.h>
	#include <efx.h>
	#include <efx-presets.h>
#endif

extern LPALGENEFFECTS alGenEffects;
extern LPALDELETEEFFECTS alDeleteEffects;
extern LPALISEFFECT alIsEffect;
extern LPALEFFECTI alEffecti;
extern LPALEFFECTIV alEffectiv;
extern LPALEFFECTF alEffectf;
extern LPALEFFECTFV alEffectfv;
extern LPALGETEFFECTI alGetEffecti;
extern LPALGETEFFECTIV alGetEffectiv;
extern LPALGETEFFECTF alGetEffectf;
extern LPALGETEFFECTFV alGetEffectfv;
extern LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
extern LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
extern LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
extern LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
extern LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
extern LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
extern LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
extern LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
extern LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
extern LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
extern LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

#if defined(APOSTELLEIN_OPENAL_LOGGING)
void oal_check_error_(const char* filename, u32 line, const char* expression);
	#define alCheck(EXPR) do { EXPR; oal_check_error_(__FILE__, __LINE__, #EXPR); } while(false)
#else
	#define alCheck(EXPR) (EXPR)
#endif

namespace oal {
	bool load_extensions(ALCdevice* device);
}
