// auto generated by go tool dist
// goos=linux goarch=amd64

#include "runtime.h"
#include "defs_GOOS_GOARCH.h"
#include "arch_GOARCH.h"
#include "malloc.h"
#define READY ((G*)1)

#line 24 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
struct PollDesc 
{ 
PollDesc* link; 
Lock; 
int32 fd; 
bool closing; 
uintptr seq; 
G* rg; 
Timer rt; 
int64 rd; 
G* wg; 
Timer wt; 
int64 wd; 
} ; 
#line 39 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static struct 
{ 
Lock; 
PollDesc* first; 
#line 48 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
} pollcache; 
#line 50 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void netpollblock ( PollDesc* , int32 ) ; 
static G* netpollunblock ( PollDesc* , int32 ) ; 
static void deadline ( int64 , Eface ) ; 
static void readDeadline ( int64 , Eface ) ; 
static void writeDeadline ( int64 , Eface ) ; 
static PollDesc* allocPollDesc ( void ) ; 
static intgo checkerr ( PollDesc *pd , int32 mode ) ; 
#line 58 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static FuncVal deadlineFn = { ( void ( * ) ( void ) ) deadline } ; 
static FuncVal readDeadlineFn = { ( void ( * ) ( void ) ) readDeadline } ; 
static FuncVal writeDeadlineFn = { ( void ( * ) ( void ) ) writeDeadline } ; 
void
net·runtime_pollServerInit()
{
#line 62 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	runtime·netpollinit();
}
void
net·runtime_pollOpen(intgo fd, PollDesc* pd, intgo errno)
{
#line 66 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	pd = allocPollDesc();
	runtime·lock(pd);
	if(pd->wg != nil && pd->wg != READY)
		runtime·throw("runtime_pollOpen: blocked write on free descriptor");
	if(pd->rg != nil && pd->rg != READY)
		runtime·throw("runtime_pollOpen: blocked read on free descriptor");
	pd->fd = fd;
	pd->closing = false;
	pd->seq++;
	pd->rg = nil;
	pd->rd = 0;
	pd->wg = nil;
	pd->wd = 0;
	runtime·unlock(pd);

	errno = runtime·netpollopen(fd, pd);
	FLUSH(&pd);
	FLUSH(&errno);
}
void
net·runtime_pollClose(PollDesc* pd)
{
#line 85 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	if(!pd->closing)
		runtime·throw("runtime_pollClose: close w/o unblock");
	if(pd->wg != nil && pd->wg != READY)
		runtime·throw("runtime_pollClose: blocked write on closing descriptor");
	if(pd->rg != nil && pd->rg != READY)
		runtime·throw("runtime_pollClose: blocked read on closing descriptor");
	runtime·netpollclose(pd->fd);
	runtime·lock(&pollcache);
	pd->link = pollcache.first;
	pollcache.first = pd;
	runtime·unlock(&pollcache);
}
void
net·runtime_pollReset(PollDesc* pd, intgo mode, intgo err)
{
#line 99 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	err = checkerr(pd, mode);
	if(err)
		goto ret;
	if(mode == 'r')
		pd->rg = nil;
	else if(mode == 'w')
		pd->wg = nil;
ret:
	runtime·unlock(pd);
	FLUSH(&err);
}
void
net·runtime_pollWait(PollDesc* pd, intgo mode, intgo err)
{
#line 112 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	err = checkerr(pd, mode);
	if(err)
		goto ret;
	netpollblock(pd, mode);
	err = checkerr(pd, mode);
ret:
	runtime·unlock(pd);
	FLUSH(&err);
}
void
net·runtime_pollSetDeadline(PollDesc* pd, int64 d, intgo mode)
{
#line 123 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	runtime·lock(pd);
	if(pd->closing)
		goto ret;
	pd->seq++;  // invalidate current timers
	// Reset current timers.
	if(pd->rt.fv) {
		runtime·deltimer(&pd->rt);
		pd->rt.fv = nil;
	}
	if(pd->wt.fv) {
		runtime·deltimer(&pd->wt);
		pd->wt.fv = nil;
	}
	// Setup new timers.
	if(d != 0 && d <= runtime·nanotime()) {
		d = -1;
	}
	if(mode == 'r' || mode == 'r'+'w')
		pd->rd = d;
	if(mode == 'w' || mode == 'r'+'w')
		pd->wd = d;
	if(pd->rd > 0 && pd->rd == pd->wd) {
		pd->rt.fv = &deadlineFn;
		pd->rt.when = pd->rd;
		// Copy current seq into the timer arg.
		// Timer func will check the seq against current descriptor seq,
		// if they differ the descriptor was reused or timers were reset.
		pd->rt.arg.type = (Type*)pd->seq;
		pd->rt.arg.data = pd;
		runtime·addtimer(&pd->rt);
	} else {
		if(pd->rd > 0) {
			pd->rt.fv = &readDeadlineFn;
			pd->rt.when = pd->rd;
			pd->rt.arg.type = (Type*)pd->seq;
			pd->rt.arg.data = pd;
			runtime·addtimer(&pd->rt);
		}
		if(pd->wd > 0) {
			pd->wt.fv = &writeDeadlineFn;
			pd->wt.when = pd->wd;
			pd->wt.arg.type = (Type*)pd->seq;
			pd->wt.arg.data = pd;
			runtime·addtimer(&pd->wt);
		}
	}
ret:
	runtime·unlock(pd);
}
void
net·runtime_pollUnblock(PollDesc* pd)
{
#line 174 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"

	G *rg, *wg;

	runtime·lock(pd);
	if(pd->closing)
		runtime·throw("runtime_pollUnblock: already closing");
	pd->closing = true;
	pd->seq++;
	rg = netpollunblock(pd, 'r');
	wg = netpollunblock(pd, 'w');
	if(pd->rt.fv) {
		runtime·deltimer(&pd->rt);
		pd->rt.fv = nil;
	}
	if(pd->wt.fv) {
		runtime·deltimer(&pd->wt);
		pd->wt.fv = nil;
	}
	runtime·unlock(pd);
	if(rg)
		runtime·ready(rg);
	if(wg)
		runtime·ready(wg);
}

#line 200 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
void 
runtime·netpollready ( G **gpp , PollDesc *pd , int32 mode ) 
{ 
G *rg , *wg; 
#line 205 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
rg = wg = nil; 
runtime·lock ( pd ) ; 
if ( mode == 'r' || mode == 'r' +'w' ) 
rg = netpollunblock ( pd , 'r' ) ; 
if ( mode == 'w' || mode == 'r' +'w' ) 
wg = netpollunblock ( pd , 'w' ) ; 
runtime·unlock ( pd ) ; 
if ( rg ) { 
rg->schedlink = *gpp; 
*gpp = rg; 
} 
if ( wg ) { 
wg->schedlink = *gpp; 
*gpp = wg; 
} 
} 
#line 222 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static intgo 
checkerr ( PollDesc *pd , int32 mode ) 
{ 
if ( pd->closing ) 
return 1; 
if ( ( mode == 'r' && pd->rd < 0 ) || ( mode == 'w' && pd->wd < 0 ) ) 
return 2; 
return 0; 
} 
#line 232 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void 
netpollblock ( PollDesc *pd , int32 mode ) 
{ 
G **gpp; 
#line 237 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
gpp = &pd->rg; 
if ( mode == 'w' ) 
gpp = &pd->wg; 
if ( *gpp == READY ) { 
*gpp = nil; 
return; 
} 
if ( *gpp != nil ) 
runtime·throw ( "epoll: double wait" ) ; 
*gpp = g; 
runtime·park ( runtime·unlock , &pd->Lock , "IO wait" ) ; 
runtime·lock ( pd ) ; 
} 
#line 251 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static G* 
netpollunblock ( PollDesc *pd , int32 mode ) 
{ 
G **gpp , *old; 
#line 256 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
gpp = &pd->rg; 
if ( mode == 'w' ) 
gpp = &pd->wg; 
if ( *gpp == READY ) 
return nil; 
if ( *gpp == nil ) { 
*gpp = READY; 
return nil; 
} 
old = *gpp; 
*gpp = nil; 
return old; 
} 
#line 270 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void 
deadlineimpl ( int64 now , Eface arg , bool read , bool write ) 
{ 
PollDesc *pd; 
uint32 seq; 
G *rg , *wg; 
#line 277 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
USED ( now ) ; 
pd = ( PollDesc* ) arg.data; 
#line 281 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
seq = ( uintptr ) arg.type; 
rg = wg = nil; 
runtime·lock ( pd ) ; 
if ( seq != pd->seq ) { 
#line 286 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
runtime·unlock ( pd ) ; 
return; 
} 
if ( read ) { 
if ( pd->rd <= 0 || pd->rt.fv == nil ) 
runtime·throw ( "deadlineimpl: inconsistent read deadline" ) ; 
pd->rd = -1; 
pd->rt.fv = nil; 
rg = netpollunblock ( pd , 'r' ) ; 
} 
if ( write ) { 
if ( pd->wd <= 0 || ( pd->wt.fv == nil && !read ) ) 
runtime·throw ( "deadlineimpl: inconsistent write deadline" ) ; 
pd->wd = -1; 
pd->wt.fv = nil; 
wg = netpollunblock ( pd , 'w' ) ; 
} 
runtime·unlock ( pd ) ; 
if ( rg ) 
runtime·ready ( rg ) ; 
if ( wg ) 
runtime·ready ( wg ) ; 
} 
#line 310 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void 
deadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , true , true ) ; 
} 
#line 316 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void 
readDeadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , true , false ) ; 
} 
#line 322 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static void 
writeDeadline ( int64 now , Eface arg ) 
{ 
deadlineimpl ( now , arg , false , true ) ; 
} 
#line 328 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
static PollDesc* 
allocPollDesc ( void ) 
{ 
PollDesc *pd; 
uint32 i , n; 
#line 334 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
runtime·lock ( &pollcache ) ; 
if ( pollcache.first == nil ) { 
n = PageSize/sizeof ( *pd ) ; 
if ( n == 0 ) 
n = 1; 
#line 341 "/tmp/bindist907131767/go/src/pkg/runtime/netpoll.goc"
pd = runtime·SysAlloc ( n*sizeof ( *pd ) ) ; 
for ( i = 0; i < n; i++ ) { 
pd[i].link = pollcache.first; 
pollcache.first = &pd[i]; 
} 
} 
pd = pollcache.first; 
pollcache.first = pd->link; 
runtime·unlock ( &pollcache ) ; 
return pd; 
} 