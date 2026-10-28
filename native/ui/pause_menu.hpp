#pragma once
#include "platform/types.hpp"
#include "ui/message_dialog.hpp"
#include <array>
#include <chrono>
namespace arcade::native {
enum class PauseAction { None,Resume,Save,Exit };
enum class SaveResult { None,Success,Failure };
class PauseMenu {
 bool paused_=false;SaveResult result_=SaveResult::None;std::string error_,code_;size_t page_=0;std::chrono::steady_clock::time_point saved_at_{};
public:
 bool paused()const{return paused_;}SaveResult result()const{return result_;}size_t page()const{return page_;}
 std::chrono::steady_clock::time_point saved_at()const{return saved_at_;}
 std::string title()const{return result_==SaveResult::Success?"Save successful":"Save failed / "+code_;}
 std::string message()const{return result_==SaveResult::Success?"Your latest progress is saved.\nYou can resume or exit without saving again.":result_==SaveResult::Failure?error_+"\nTry Save again, resume, or exit without saving.":"";}
 template<class Session>void open(Session& session){if(paused_)return;paused_=true;result_=SaveResult::None;error_.clear();code_.clear();page_=0;session.pause(true);}
 static PauseAction action(Key key,bool swapped){
  if(key==Key::Square)return PauseAction::Save;
  if(key==(swapped?Key::Circle:Key::Cross))return PauseAction::Resume;
  if(key==(swapped?Key::Cross:Key::Circle))return PauseAction::Exit;
  return PauseAction::None;
 }
 void scroll(Key key,size_t pages){
  if((key==Key::L1||key==Key::Left||key==Key::Up)&&page_)--page_;
  if((key==Key::R1||key==Key::Right||key==Key::Down)&&page_+1<pages)++page_;
 }
 template<class Session,class Stop>bool apply(PauseAction action,Session& session,Stop stop){
  if(!paused_)return false;
  if(action==PauseAction::Save){
   // No emulation updates occur while paused, so a repeated successful save
   // would write the exact same state (including keyboard auto-repeat).
   if(result_==SaveResult::Success)return false;
   page_=0;
   if(session.save_state()){result_=SaveResult::Success;error_.clear();code_.clear();}
   else{result_=SaveResult::Failure;error_=session.error();code_=session.error_code().empty()?"ARC-2001":session.error_code();}
   saved_at_=std::chrono::steady_clock::now();
   return false;
  }
  if(action==PauseAction::Resume||action==PauseAction::Exit){
   if(action==PauseAction::Exit)stop();else session.pause(false);
   paused_=false;result_=SaveResult::None;page_=0;error_.clear();code_.clear();return action==PauseAction::Exit;
  }
  return false;
 }
};
struct SaveToastLayout {
 Rect box;std::vector<std::string> lines;size_t rows=0,pages=1,page=0;float line_height=26;bool visible=false;
};
inline SaveToastLayout save_toast_layout(const PauseMenu& menu,bool large,std::chrono::steady_clock::time_point now=std::chrono::steady_clock::now()){
 SaveToastLayout out;if(menu.result()==SaveResult::None)return out;
 out.line_height=large?30:26;out.lines=message_lines(menu.message(),472,19,large);
 out.rows=std::min(size_t(10),out.lines.size());out.pages=(out.lines.size()+out.rows-1)/out.rows;
 double elapsed=std::chrono::duration<double>(now-menu.saved_at()).count();
 double seconds=menu.result()==SaveResult::Success?5:std::max(8.,out.pages*6.);
 out.visible=elapsed>=0&&elapsed<seconds;out.page=std::min(out.pages-1,size_t(std::max(0.,elapsed)/6));
 out.box={1080,24,560,76+out.rows*out.line_height+(out.pages>1?26:0)};return out;
}
struct PauseLayout {
 Rect box,game,feedback,previous,next;std::array<Rect,3> buttons;
 std::vector<std::string> game_lines,lines;float title_size=26,line_height=28;size_t rows=0,pages=1;float divider=300;
 PauseAction hit(float x,float y)const{for(size_t i=0;i<buttons.size();++i)if(message_contains(buttons[i],x,y))return static_cast<PauseAction>(i+1);return PauseAction::None;}
};
inline PauseLayout pause_layout(const PauseMenu&,const std::string& game,bool large){
 PauseLayout out;out.game_lines=message_lines(game,962,out.title_size,large,1);
 while(out.game_lines.size()>2&&out.title_size>18){--out.title_size;out.game_lines=message_lines(game,962,out.title_size,large,1);}
 float title_height=out.title_size*(large?1.16f:1)*1.26f;
 float game_height=std::max(95.f,48+out.game_lines.size()*title_height);
 float extra=game_height-95,height=460+extra,feedback_height=0;
 out.divider=300+extra;
 out.box={276,(941-height)/2,1120,height};auto x=out.box.x,y=out.box.y;
 out.game={x+40,y+181,1040,game_height};out.feedback={x+40,y+303+extra,1040,feedback_height};
 for(int i=0;i<3;++i)out.buttons[i]={x+40+i*352.f,y+out.divider+30,336,64};
 out.previous={x+52,out.feedback.y+feedback_height-29,44,25};out.next={x+1024,out.feedback.y+feedback_height-29,44,25};
 return out;
}
}
