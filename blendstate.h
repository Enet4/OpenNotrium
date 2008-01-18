#include "SDL_opengl.h"

typedef enum BlendState
{
	//grBLEND_DISABLE				= 0,
	grBLEND_ZERO				= GL_ZERO,
	grBLEND_ONE					= GL_ONE,
	grBLEND_SRCCOLOR			= GL_SRC_COLOR,
	grBLEND_INVSRCCOLOR			= GL_ONE_MINUS_SRC_COLOR,
	grBLEND_SRCALPHA			= GL_SRC_ALPHA,
	grBLEND_INVSRCALPHA			= GL_ONE_MINUS_SRC_ALPHA,
	grBLEND_DESTALPHA			= GL_DST_ALPHA,
	grBLEND_INVDESTALPHA		= GL_ONE_MINUS_DST_ALPHA,
	grBLEND_DESTCOLOR			= GL_DST_COLOR,
	grBLEND_INVDESTCOLOR		= GL_ONE_MINUS_DST_COLOR,
	grBLEND_SRCALPHASAT			= GL_SRC_ALPHA_SATURATE,
//	grBLEND_BOTHSRCALPHA		= 12,
//	grBLEND_BOTHINVSRCALPHA		= 13,
//
//	grBLEND_FORCE16BIT			= (1 << 15)
};
