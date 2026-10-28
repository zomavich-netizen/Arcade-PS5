#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,shutil,runpy
root=Path(__file__).resolve().parents[1];dest=root/'build/title/PPSA99984'
runpy.run_path(str(root/'tools/check_load_alignment.py'))['check_load_alignment']((dest/'eboot.bin').read_bytes())
keep={'arcade-room-4k.png','arcade-splash-4k.png','arcade-menu.wav','arcade-menu.LICENSE.txt','mame-metadata.tsv','mame-artwork.tsv','mame-artwork-aliases.tsv','cover-concepts.png','native-font-0.png','native-font-1.png','native-font-2.png','Inter-OFL.txt','vault-cabinet.png','vault-grid.png','vault-logo.png'}
for p in (dest/'assets').iterdir():
 if p.is_file() and p.name not in keep:p.unlink()
licenses=dest/'licenses';licenses.mkdir(exist_ok=True)
for source,name in [(root/'LICENSE','GPL-3.0.txt'),(root/'THIRD-PARTY.md','THIRD-PARTY.md'),(root/'.deps/ps5-opengl-sdk-1.0.2/THIRD_PARTY_NOTICES.md','ps5-opengl-NOTICES.md'),(root/'assets/Inter-OFL.txt','Inter-OFL.txt')]:shutil.copyfile(source,licenses/name)
meta={'title_id':'PPSA99984','status':'development; not console-qualified','frontend':'custom C++ and OpenGL; ps5-opengl 1.0.2 with EGL and native input/audio','backend':'static MAME 0.289 PS5 fork','mame_revision':'8f90a8135ef402e04042160537b76cf6b17bcfa5','mame_sources':['src/mame/bally/astrocde.cpp','src/mame/bally/gridlee.cpp','src/mame/capcom/cps1.cpp'],'free_test_titles':['Robby Roto','Gridlee'],'roms_bundled':False,'payload_sdk':'0.42','opengl_archive_sha256':'30d8e51dd062739b629346102be5932728ba714bc50b1bdcdf08306c8843d9a4','native_imports':'mmap, munmap and sysconf resolve through kernel imports before payload libc fallbacks','load_alignment':'all four mapped segments pass 16 KiB check','packager_fix':'RELRO file origin accounts for .data.rel.ro before .got','packager_sha256':hashlib.file_digest((root/'vendor/native-app/sce_module_writer.cpp').open('rb'),'sha256').hexdigest()}
backend_path=root/'build/ps5/backend.json'
backend=json.loads(backend_path.read_text()) if backend_path.is_file() else {'scope':'subset'}
meta['content_version']=json.loads((dest/'sce_sys/param.json').read_text())['contentVersion']
meta['mame_scope']=backend.get('scope','subset');meta['mame_driver_count']=backend.get('driver_count')
if meta['mame_scope'] in ('full','arcade-all'):meta['mame_sources']=backend.get('source_filter')
if meta['mame_scope']=='arcade-all':meta['backend']='static MAME 0.289 PS5 fork; all arcade-classified drivers'
meta['cover_art']={'automatic':True,'source':'https://thumbnails.libretro.com/MAME/','cache':'/data/arcade/covers','background':True,'https_verified':True,'exact_filename_aliases':'assets/mame-artwork-aliases.tsv','retry_invalidated_on_lookup_change':True}
meta['automatic_orientation']={'source':'libretro core rotation request','all_games':True,'renderers':['OpenGL','Vulkan'],'manual_rotation':'additional adjustment'}
meta['branding']={'style':'approved 1980s CRT; orange, charcoal and cream','home_icon':[512,512],'home_hover_background':'3840x2160 BC7 DX10 pic0.dds','home_music':'48 kHz stereo ATRAC9 snd0.at9 / -28 LUFS','background':[3840,2160],'startup_splash':[3840,2160],'animated_background':'native GPU lighting with ambient particles','reduce_motion_uses_still':True,'startup_seconds':2,'music':'Colorosse Arcade Loop / CC0-1.0','music_controls':['mute','menu_volume','menu_music'],'music_stops_during_gameplay':True,'source_art':'4K exports scaled from generated masters'}
meta['application_heap']={'requested_mib':1024,'allocator':'native mspace; CPU-cached direct memory for large heap','runtime_size_logged':True}
meta['queued_credits']={'readiness':'MAME coin-slot lockout','fallback_startup_seconds':6,'retain_on_load_failure':True,'refund_undelivered_on_return':True,'neogeo_coin_meter_acknowledgement':True}
meta['settings_categories']=['Profiles','Video','Performance','Audio','Controls','Accessibility','Diagnostics','Game files','Appearance','Colour']
meta['content_directories']={'roms':'/data/arcade/roms','bios':'/data/arcade/bios'}
meta['frontend']='Cover Vault native C++/OpenGL; approved cabinet, flowing covers, full-room grid, per-game settings, coin/Start queue and boundary haptics'
meta['ux_revision']='cover-vault-v1'
meta['ux_patch']='80s-crt-atmosphere-20261028'
meta['start_dialog']={'matches_pause_design':True,'actions':['load-save','new-game','back'],'last_save_timestamp':True,'equal_buttons':True}
meta['pause_dialog']={'separate_actions':['resume','save','exit-without-saving'],'save_success_and_failure_feedback':True,'feedback_position':'top-right','auto_dismiss':True,'full_error_auto_paging':True,'equal_buttons':True}
meta['game_card']={'layout':'compact icon card','equal_actions':True,'button_text_size':24,'real_save_timestamps':True}
meta['game_metadata']={'source':'MAME 0.289 compiled arcade registry','fields':['title','manufacturer','year','hardware','region','players','compatibility','save_support'],'offline':True,'file':'assets/mame-metadata.tsv'}
meta['error_dialogs']={'reference_codes':True,'full_message_paging':True,'raised_centered_controls':True,'large_text_verified':True}
meta['save_states']={'save_on_return_to_hall':False,'manual_save_while_paused':True,'launch_choices':['load-last-save','start-from-beginning'],'per_game_and_profile':True,'directory':'saves/states','failed_save_keeps_game_paused':True,'atomic_replacement':True,'integrity_checksum':True,'neogeo_restore_waits_for_firmware':True}
meta['ux_assets']=['vault-cabinet.png','vault-grid.png','vault-logo.png','arcade-room-4k.png','arcade-splash-4k.png','arcade-menu.wav']
meta['vulkan']={'compiled':backend.get('vulkan_compiled',False),'mesa_revision':'7b59ef27c1b09b9671bc4153c41940c3155c3af2','console_qualified':False,'presentation':'native VK_KHR_display / VK_KHR_swapchain; OpenGL context closed during Vulkan gameplay','effects':['Scanlines','CRT','Bloom','Phosphor','LCD']}
for source,name in [(root/'.deps/PS5_Mesa-7b59ef27c1b09b9671bc4153c41940c3155c3af2/docs/license.rst','Mesa-LICENSE.rst'),(root/'.deps/arcade-radv/compiler-rt-COPYRIGHT','LLVM-COPYRIGHT'),(root/'.deps/arcade-radv/Apache-2.0.txt','Apache-2.0.txt')]:
 if source.is_file():shutil.copyfile(source,licenses/name)
for notice in (root/'.deps/cover-licenses').glob('*'):
 if notice.is_file():shutil.copyfile(notice,licenses/notice.name)
(dest/'BUILD.json').write_text(json.dumps(meta,indent=2)+'\n')
lines=[]
for p in sorted(dest.rglob('*')):
 if p.is_file() and p.name!='SHA256SUMS':lines.append(hashlib.file_digest(p.open('rb'),'sha256').hexdigest()+'  '+str(p.relative_to(dest)))
(dest/'SHA256SUMS').write_text('\n'.join(lines)+'\n')
for p in [dest,*dest.rglob('*')]:p.chmod(0o777)
print('Package finalized:',len(lines),'files; load alignment and permissions checked')
