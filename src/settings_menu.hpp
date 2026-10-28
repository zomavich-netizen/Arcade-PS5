#pragma once
#include "settings.hpp"
#include <vector>
namespace arcade {
enum class SettingId {
 Profile,Renderer,Output,Refresh,Scaling,Filter,Rotation,PostProcessing,Overlay,OverlayOptions,
 Speed,PauseOnFocus,Volume,Mute,MenuVolume,MenuMusic,Vibration,VibrationStrength,Mapping,MenuKeys,CoinButton,StartButton,
 LargeText,HighContrast,ReduceMotion,DetailedLogs,ImmediateLogs,LogLocation,Backend,
 GameFolder,Rescan,SaveLocation,BackupSaves,Theme,Wallpaper,Accent,
 HudColors,HudSize,HudCorner,HudBackend,HudFps,HudMs,HudSpeed,HudResolution,
 ActionCross,ActionCircle,ActionSquare,ActionTriangle,ResetMapping
};
struct SettingRow {SettingId id;const char* label;const char* note;};
const char* category_name(SettingsCategory);
const char* category_note(SettingsCategory);
std::vector<SettingRow> category_rows(SettingsCategory);
std::vector<SettingRow> overlay_rows();
std::vector<SettingRow> mapping_rows();
std::string setting_value(SettingId,const Settings&);
bool adjust_setting(SettingId,Settings&,int direction);
}
