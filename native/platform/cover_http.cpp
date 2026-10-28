#include "cover_art.hpp"
#include <curl/curl.h>
#include <mutex>
#ifdef ARCADE_PS5
#include "console_curl.h"
#endif
namespace arcade {
namespace {
struct Transfer {std::vector<unsigned char>* bytes;const std::atomic<bool>* stop;};
size_t body(char* bytes,size_t size,size_t count,void* user){auto& t=*static_cast<Transfer*>(user);if(t.stop->load()||count>8*1024*1024||size>8*1024*1024)return 0;auto n=size*count;if(n>8*1024*1024-t.bytes->size())return 0;t.bytes->insert(t.bytes->end(),bytes,bytes+n);return n;}
int progress(void* user,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return static_cast<const std::atomic<bool>*>(user)->load()?1:0;}
}
CoverResponse download_cover(const std::string& url,const std::atomic<bool>& stop){
 CoverResponse result;static std::once_flag once;static CURLcode init=CURLE_FAILED_INIT;std::call_once(once,[]{init=curl_global_init(CURL_GLOBAL_DEFAULT);});if(init!=CURLE_OK){result.error="HTTPS initialization failed";return result;}
 auto* easy=curl_easy_init();if(!easy){result.error="HTTPS handle unavailable";return result;}Transfer transfer{&result.bytes,&stop};char error[CURL_ERROR_SIZE]{};
#ifdef ARCADE_PS5
 console_curl_setup(easy);
#endif
 curl_easy_setopt(easy,CURLOPT_URL,url.c_str());curl_easy_setopt(easy,CURLOPT_USERAGENT,"ArcadeGallery/1.0 (cover artwork)");
 curl_easy_setopt(easy,CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(easy,CURLOPT_REDIR_PROTOCOLS_STR,"https");curl_easy_setopt(easy,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(easy,CURLOPT_MAXREDIRS,3L);
 curl_easy_setopt(easy,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(easy,CURLOPT_CONNECTTIMEOUT_MS,5000L);curl_easy_setopt(easy,CURLOPT_TIMEOUT_MS,15000L);curl_easy_setopt(easy,CURLOPT_SSL_VERIFYPEER,1L);curl_easy_setopt(easy,CURLOPT_SSL_VERIFYHOST,2L);
 curl_easy_setopt(easy,CURLOPT_WRITEFUNCTION,body);curl_easy_setopt(easy,CURLOPT_WRITEDATA,&transfer);curl_easy_setopt(easy,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(easy,CURLOPT_XFERINFODATA,&stop);curl_easy_setopt(easy,CURLOPT_NOPROGRESS,0L);curl_easy_setopt(easy,CURLOPT_ERRORBUFFER,error);
 auto code=curl_easy_perform(easy);long status=0;curl_easy_getinfo(easy,CURLINFO_RESPONSE_CODE,&status);result.status=code==CURLE_OK?int(status):0;if(code!=CURLE_OK){result.error=error;result.bytes.clear();}curl_easy_cleanup(easy);return result;
}
}
