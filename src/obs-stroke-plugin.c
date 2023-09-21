#include <obs-module.h>

#include "version.h"

extern struct obs_source_info obs_stroke_filter;
extern struct obs_source_info obs_glow_filter;
extern struct obs_source_info obs_shadow_filter;

OBS_DECLARE_MODULE();

OBS_MODULE_USE_DEFAULT_LOCALE("obs-stroke", "en-US");

OBS_MODULE_AUTHOR("FiniteSingularity");

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Stroke] loaded version %s", PROJECT_VERSION);
	obs_register_source(&obs_stroke_filter);
	obs_register_source(&obs_glow_filter);
	obs_register_source(&obs_shadow_filter);
	return true;
}

void obs_module_unload(void) {}
