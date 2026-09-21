#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QPointer>
#include "ui/comment-dock.hpp"
OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_description(void) { return "Private AI-ranked YouTube comments dock powered by Jev"; }
MODULE_EXPORT const char *obs_module_name(void) { return "AI Comments"; }
MODULE_EXPORT const char *obs_module_author(void) { return "AI Comments contributors"; }
namespace {
QPointer<comments::CommentDock> dock;
void frontendEvent(enum obs_frontend_event event,void *) {
    if (event==OBS_FRONTEND_EVENT_EXIT && dock) dock->shutdown();
}
}
bool obs_module_load(void) {
    char *path=obs_module_config_path("");
    if (!path) return false;
    auto *widget=new comments::CommentDock(QString::fromUtf8(path)); bfree(path);
    if (!obs_frontend_add_dock_by_id("ai-comments-dock","AI Comments",widget)) { delete widget; return false; }
    dock=widget; obs_frontend_add_event_callback(frontendEvent,nullptr); return true;
}
void obs_module_unload(void) {
    // Frontend APIs are unavailable after OBS_FRONTEND_EVENT_EXIT.
    // The dock is owned by OBS; shutdown also runs in the widget destructor.
    if (dock) dock->shutdown();
}
