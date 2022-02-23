#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./openal.hpp"

#if defined(APOSTELLEIN_OPENAL_LOGGING)

void oal_check_error_(const char* file, u32 line, const char* expression) {
	if (const auto code = alGetError(); code != AL_NO_ERROR) {
		const char* error = "Unknown OpenAL error";
		const char* description = "Description unavailable.";

		switch (code) {
		case AL_INVALID_NAME:
			error = "AL_INVALID_NAME";
			description = "A bad name (ID) has been specified.";
			break;
		case AL_INVALID_ENUM:
			error = "AL_INVALID_ENUM";
			description = "An unacceptable value has been specified for an enumerated argument.";
			break;
		case AL_INVALID_VALUE:
			error = "AL_INVALID_VALUE";
			description = "A numeric argument is out of range.";
			break;
		case AL_INVALID_OPERATION:
			error = "AL_INVALID_OPERATION";
			description = "The specified operation is not allowed in the current state.";
			break;
		case AL_OUT_OF_MEMORY:
			error = "AL_OUT_OF_MEMORY";
			description = "There is not enough memory left to execute the command.";
			break;
		default:
			break;
		}
		const std::string path { file };
		spdlog::critical(
			"An internal OpenAL call failed in {} ({})!",
			path.substr(path.find_last_of("\\/") + 1),
			line
		);
		spdlog::info(
			"Expression: {}\n, Error: {}\n, Description: {}",
			expression,
			error,
			description
		);
	}
}

#endif

LPALGENEFFECTS alGenEffects = nullptr;
LPALDELETEEFFECTS alDeleteEffects = nullptr;
LPALISEFFECT alIsEffect = nullptr;
LPALEFFECTI alEffecti = nullptr;
LPALEFFECTIV alEffectiv = nullptr;
LPALEFFECTF alEffectf = nullptr;
LPALEFFECTFV alEffectfv = nullptr;
LPALGETEFFECTI alGetEffecti = nullptr;
LPALGETEFFECTIV alGetEffectiv = nullptr;
LPALGETEFFECTF alGetEffectf = nullptr;
LPALGETEFFECTFV alGetEffectfv = nullptr;
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = nullptr;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = nullptr;
LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = nullptr;
LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = nullptr;
LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti = nullptr;
LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv = nullptr;
LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = nullptr;
LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv = nullptr;

#define LOAD_FUNCTION(TYPE, NAME) \
	alCheck(NAME = (TYPE)alGetProcAddress(#NAME)); \
	if (NAME == nullptr) { \
		result = false; \
		spdlog::critical("Failed to load " #NAME "!"); \
	}

bool oal::load_extensions(ALCdevice* device) {
	if (!alcIsExtensionPresent(device, "ALC_EXT_EFX")) {
		spdlog::critical("ALC_EXT_EFX is not a supported extension!");
		return false;
	}
	bool result = true;
	LOAD_FUNCTION(LPALGENEFFECTS, alGenEffects)
    LOAD_FUNCTION(LPALDELETEEFFECTS, alDeleteEffects)
    LOAD_FUNCTION(LPALISEFFECT, alIsEffect)
    LOAD_FUNCTION(LPALEFFECTI, alEffecti)
    LOAD_FUNCTION(LPALEFFECTIV, alEffectiv)
    LOAD_FUNCTION(LPALEFFECTF, alEffectf)
    LOAD_FUNCTION(LPALEFFECTFV, alEffectfv)
    LOAD_FUNCTION(LPALGETEFFECTI, alGetEffecti)
    LOAD_FUNCTION(LPALGETEFFECTIV, alGetEffectiv)
    LOAD_FUNCTION(LPALGETEFFECTF, alGetEffectf)
    LOAD_FUNCTION(LPALGETEFFECTFV, alGetEffectfv)
    LOAD_FUNCTION(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots)
    LOAD_FUNCTION(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots)
    LOAD_FUNCTION(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot)
    LOAD_FUNCTION(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti)
    LOAD_FUNCTION(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv)
    LOAD_FUNCTION(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf)
    LOAD_FUNCTION(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv)
    LOAD_FUNCTION(LPALGETAUXILIARYEFFECTSLOTI, alGetAuxiliaryEffectSloti)
    LOAD_FUNCTION(LPALGETAUXILIARYEFFECTSLOTIV, alGetAuxiliaryEffectSlotiv)
    LOAD_FUNCTION(LPALGETAUXILIARYEFFECTSLOTF, alGetAuxiliaryEffectSlotf)
    LOAD_FUNCTION(LPALGETAUXILIARYEFFECTSLOTFV, alGetAuxiliaryEffectSlotfv)
	return result;
}

#undef LOAD_FUNCTION
