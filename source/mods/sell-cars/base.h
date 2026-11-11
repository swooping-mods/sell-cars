/* Local copy: no dependency on another feature package. */
#include "../../runtime/native/gameplay.h"
#include "../../runtime/native/input.h"
#include "api.h"
int _fltused=0;
static PTR world_f,mounted_f,body_f,joined_m,host_m,local_m,typing_m,blocked_m,menu_m,focus_m,restoring_m,alive_m,time_m;
static void (*set_field)(PTR,PTR,void *);
/* Retain the legacy high-bit mask while honoring shared input admission. */
static short key(int vk){return moon_input_key((unsigned)vk)?(short)0x8000:0;}
static PTR (*foreground)(void);static DWORD (*window_pid)(PTR,DWORD *);
static PTR evidence;static DWORD thread_id;static int previous_key;
static float clock_now;
static int alive(PTR p){int ok=1;void *a[]={p};PTR b=p?call(alive_m,0,a,&ok):0;return ok&&b&&*(unsigned char*)unbox(b);}
static int host(void){int ok=1;if(!boolean(joined_m,0,&ok))return ok;PTR a=call(local_m,0,0,&ok),b=call(host_m,0,0,&ok);return ok&&a&&b&&*(U64*)unbox(a)&&*(U64*)unbox(a)==*(U64*)unbox(b);}
static void event(const char *s){if(!evidence)return;char b[256],*p=cat(b,"{\"event\":\"");p=cat(p,s);p=cat(p,"\"}\n");DWORD n=0;WriteFile(evidence,b,(DWORD)(p-b),&n,0);FlushFileBuffers(evidence);}
static int input_ready(PTR walker,PTR *world,PTR *vehicle,int needs_host){
 int ok=1;*world=0;*vehicle=0;field_value(walker,world_f,world);field_value(walker,mounted_f,vehicle);
 DWORD pid=0;window_pid(foreground(),&pid);
 return alive(*world)&&alive(*vehicle)&&pid==GetCurrentProcessId()&&boolean(focus_m,0,&ok)&&!boolean(typing_m,0,&ok)&&!boolean(blocked_m,walker,&ok)&&!boolean(menu_m,walker,&ok)&&!boolean(restoring_m,*world,&ok)&&ok&&(!needs_host||host());
}
static int pressed(int vk){int down=(key(0x12)&0x8000)&&(key(vk)&0x8000);int edge=down&&!previous_key;previous_key=down;return edge;}
static int base_bind(PTR domain,PTR game,const char *logname){
 if(!bind_runtime(domain,game)||!dapi(game))return 0;
 set_field=(void(*)(PTR,PTR,void*))GetProcAddress(game,"il2cpp_field_set_value");if(!set_field)return 0;
 PTR user=LoadLibraryA("user32.dll");if(!user)return 0;
 foreground=(PTR(*)(void))GetProcAddress(user,"GetForegroundWindow");window_pid=(DWORD(*)(PTR,DWORD*))GetProcAddress(user,"GetWindowThreadProcessId");if(!foreground||!window_pid)return 0;
 PTR walker=find_class("MoonCleaners","MoonWalker"),vehicle=find_class("MoonCleaners","MoonPrototypeVehicle");
 world_f=get_field(walker,"world");mounted_f=get_field(walker,"mountedVehicle");body_f=get_field(vehicle,"body");
#define M(F,NS,T,N,S) F=dmethod(NS,T,N,S);if(!F)return 0
 M(joined_m,"MoonCleaners","MoonCrew","get_Joined","");M(host_m,"MoonCleaners","MoonCrew","get_HostId","");M(local_m,"Steamworks","SteamUser","GetSteamID","");
 M(typing_m,"MoonCleaners","MoonInput","get_Typing","");M(blocked_m,"MoonCleaners","MoonWalker","get_UIBlocked","");M(menu_m,"MoonCleaners","MoonWalker","get_MenuBlocked","");M(restoring_m,"MoonCleaners","MoonWorld","get_IsRestoringRendering","");M(focus_m,"UnityEngine","Application","get_isFocused","");M(alive_m,"UnityEngine","Object","op_Implicit","UnityEngine.Object");M(time_m,"UnityEngine","Time","get_unscaledTime","");
#undef M
 if(!world_f||!mounted_f||!body_f)return 0;
 char path[1100];DWORD n=GetEnvironmentVariableA("MOONLAB_EVIDENCE",path,900);if(n&&n<900){cat(path+n,"\\");cat(path+n+1,logname);evidence=CreateFileA(path,0x40000000,1,0,2,0x80,0);if(evidence==(PTR)(U64)-1)evidence=0;}
 (void)integer;(void)num;(void)clock_now;(void)thread_id;return 1;
}
static int on_thread(void){DWORD t=GetCurrentThreadId();if(!thread_id)thread_id=t;return t==thread_id;}
