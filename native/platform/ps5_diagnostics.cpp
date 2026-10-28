// SPDX-License-Identifier: GPL-3.0-or-later
// Native title shutdown follows ProsperoEden src/lifecycle.c. Register layout
// follows its hardware-qualified headless/crash_report.cpp.
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
extern "C" int sceKernelUsleep(unsigned);
extern "C" int sceSystemServiceLoadExec(const char*, const char**);
static int log_fd = -1;
static bool immediate_logs=true;
extern "C" void arcade_log_settings(bool immediate){immediate_logs=immediate;}
extern "C" void arcade_startup_note(const char* text) {
    if (log_fd < 0) return;
    unsigned n=0; while(text[n]) ++n;
    (void)write(log_fd,text,n); (void)write(log_fd,"\n",1);
    if(immediate_logs)(void)fsync(log_fd);
}
static void hex(char*& out, std::uint64_t value) {
    for(int shift=60;shift>=0;shift-=4)*out++="0123456789abcdef"[(value>>shift)&15];
}
extern "C" void arcade_startup_constructor(void* address){char line[64];char* p=line;const char* prefix="constructor 0x";while(*prefix)*p++=*prefix++;hex(p,reinterpret_cast<std::uintptr_t>(address));*p=0;arcade_startup_note(line);}
static void fault(int signal, siginfo_t*, void* context) {
    char line[100]; char* p=line;
    const char* prefix="FAULT signal=0x";while(*prefix)*p++=*prefix++;
    hex(p,signal);*p++=' ';*p++='p';*p++='c';*p++='=';
    const auto* m=static_cast<const std::uint64_t*>(context)+8;
    hex(p,context?m[20]:0);*p=0;arcade_startup_note(line);
    // Avoid exit/_Exit: PS5 refuses these syscalls for native titles.
    sceSystemServiceLoadExec("exit",nullptr);
    for(;;)sceKernelUsleep(100000);
}
extern "C" void arcade_startup_begin() {
    log_fd=open("/data/arcade-startup.log",O_WRONLY|O_CREAT|O_TRUNC,0666);
    if(log_fd>=0){dup2(log_fd,1);dup2(log_fd,2);}
    arcade_startup_note("Arcade diagnostic startup v1; runtime initialized");
    char line[80]; char* p=line;const char* prefix="loaded startup logger at 0x";
    while(*prefix)*p++=*prefix++;hex(p,reinterpret_cast<std::uintptr_t>(&arcade_startup_begin));*p=0;
    arcade_startup_note(line);
    struct sigaction action{};sigemptyset(&action.sa_mask);
    action.sa_flags=SA_SIGINFO;action.sa_sigaction=fault;
    for(int s:{SIGILL,SIGSEGV,SIGBUS,SIGSYS,SIGABRT,SIGFPE})sigaction(s,&action,nullptr);
}
extern "C" [[noreturn]] void catchReturnFromMain(int status) {
    char message[80];std::snprintf(message,sizeof message,"main returned status=%d; requesting native title shutdown",status);
    arcade_startup_note(message);std::fflush(nullptr);
    int rc=sceSystemServiceLoadExec("exit",nullptr);
    std::snprintf(message,sizeof message,"native shutdown refused: 0x%x",rc);arcade_startup_note(message);
    for(;;)sceKernelUsleep(100000);
}

extern "C" [[noreturn]] void abort() noexcept {immediate_logs=true;arcade_startup_note("abort requested");arcade_startup_constructor(__builtin_return_address(0));sceSystemServiceLoadExec("exit",nullptr);for(;;)sceKernelUsleep(100000);}
