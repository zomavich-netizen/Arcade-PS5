#!/usr/bin/env python3
"""Build PS5 HTTPS support using ProsperoEden's isolated curl integration."""
from pathlib import Path
import subprocess,os,json,hashlib
root=Path(__file__).resolve().parents[1];build=root/'build/cover-http';build.mkdir(exist_ok=True)
sdk=root/'.deps/payload-v0.42/ps5-payload-sdk';prefix=root/'.deps/pacbrew-0.40.2/opt/ps5-payload-sdk/target/user/homebrew';llvm=Path('/opt/homebrew/opt/llvm@20/bin')
if not (prefix/'lib/libcurl.a').is_file():raise SystemExit('Pinned PacBrew HTTPS dependencies are missing')
flags=[f'-ffile-prefix-map={root}=.',f'-fdebug-prefix-map={root}=.','-target','x86_64-sie-ps5','-fvisibility-nodllstorageclass=default','-isysroot',str(sdk),'-isystem',str(sdk/'target/include/c++/v1'),'-isystem',str(sdk/'target/include'),'-I',str(prefix/'include'),'-I',str(root/'vendor/prosperoeden/network'),'-I',str(root/'src'),'-I',str(root/'vendor'),'-fno-stack-protector','-fno-plt','-femulated-tls','-DARCADE_PS5=1','-DCURL_STATICLIB=1','-O2']
def run(args):subprocess.run(list(map(str,args)),check=True)
run([llvm/'clang++',*flags,'-std=c++17','-fexceptions','-c',root/'native/platform/cover_http.cpp','-o',build/'http.o'])
run([llvm/'clang',*flags,'-std=c11','-c',root/'vendor/prosperoeden/network/console_curl.c','-o',build/'console_curl.o'])
linker=root.parent/'Pulse/.local/linker20/lld@20/20.1.8/bin/ld.lld'
run([linker,'-r','-o',build/'http-all.o',build/'http.o',build/'console_curl.o',prefix/'lib/libcurl.a',prefix/'lib/libpsl.a'])
own='getaddrinfo freeaddrinfo gai_strerror gethostbyname getnameinfo fnmatch getpwuid_r _setjmp _longjmp openlog closelog dladdr if_nametoindex pipe2 recvmmsg sendmmsg popen pclose isatty mkstemp gmtime_r ZSTD_trace_compress_begin ZSTD_trace_compress_end ZSTD_trace_decompress_begin ZSTD_trace_decompress_end'.split()
rename=['--redefine-sym','fcntl=__wrap_fcntl','--redefine-sym','__real_fcntl=fcntl']
for name in own:rename+=['--redefine-sym',name+'=arcade_cover_'+name]
run([llvm/'llvm-objcopy',*rename,build/'http-all.o',build/'http.o'])
(build/'receipt.json').write_text(json.dumps({'pacbrew_version':'0.40.2','archive_sha256':'a85f65de418a8e6a898c6c3e3c870d50fff7618a200e4dd59ea9692af6ecec4d','curl':'8.18.0','openssl':'3.5.2','libpsl':'0.21.5','https_certificate_verification':True,'isolated_libc_symbols':own},indent=2)+'\n')
print('Isolated PS5 cover HTTPS object ready')
