/* Local XGameSave blob storage. A CURRENT file atomically publishes complete
 * immutable generations. Old generations are retained as recovery copies.
 * No cloud service, entitlement or account state is simulated here.
 */
#define GS_PATH 1024
#define GS_MISSING ((HRESULT)0x80830008u)
#define GS_SMALL ((HRESULT)0x80830007u)
#define GS_BAD_NAME ((HRESULT)0x80830001u)
#define GS_LIMIT (256u << 20)
typedef struct gs_provider { LONG refs; struct gs_provider *next; char path[GS_PATH]; } gs_provider;
typedef struct gs_container { LONG refs; gs_provider *provider; char name[65], path[GS_PATH]; } gs_container;
typedef struct gs_blob { struct gs_blob *next; char name[65]; DWORD size; BYTE *data; int deleted; } gs_blob;
typedef struct gs_update { LONG refs; gs_container *container; char *display; gs_blob *changes; } gs_update;
typedef struct gs_blob_info { const char *name; UINT32 size; } gs_blob_info;
typedef struct gs_blob_result { gs_blob_info info; UINT8 *data; } gs_blob_result;
typedef struct gs_container_info {
    const char *name, *displayName; UINT32 blobCount; UINT64 totalSize;
    UINT64 lastModifiedTime; BOOLEAN needsSync;
} gs_container_info;
typedef BOOLEAN (WINAPI *gs_blob_cb)(const gs_blob_info *, void *);
typedef BOOLEAN (WINAPI *gs_container_cb)(const gs_container_info *, void *);
static LONG gs_generation;
static gs_provider *gs_providers;

static void gs_blobs_free(gs_blob *b) { while (b) { gs_blob *next=b->next; free(b->data); free(b); b=next; } }
static void gs_provider_release(gs_provider *p) {
    if (p && !InterlockedDecrement(&p->refs)) {
        gs_provider **slot; EnterCriticalSection(&g_lock);
        for(slot=&gs_providers;*slot && *slot!=p;slot=&(*slot)->next);
        if(*slot) *slot=p->next; LeaveCriticalSection(&g_lock); free(p);
    }
}
static int gs_provider_valid(gs_provider *p) {
    gs_provider *item; int valid=0; EnterCriticalSection(&g_lock);
    for(item=gs_providers;item;item=item->next) if(item==p) { valid=1; break; }
    LeaveCriticalSection(&g_lock); return valid;
}
static void gs_container_release(gs_container *c) { if(c && !InterlockedDecrement(&c->refs)) { gs_provider_release(c->provider); free(c); } }
static void gs_update_release(gs_update *u) { if(u && !InterlockedDecrement(&u->refs)) { gs_container_release(u->container); gs_blobs_free(u->changes); free(u->display); free(u); } }
static HRESULT gs_encode(const char *name, char out[129])
{
    static const char hex[]="0123456789abcdef"; SIZE_T i,n;
    if(!name || !(n=strlen(name)) || n>64) return GS_BAD_NAME;
    for(i=0;i<n;i++) { unsigned char b=name[i]; out[2*i]=hex[b>>4]; out[2*i+1]=hex[b&15]; }
    out[2*n]=0; return S_OK;
}
static int gs_decode(const char *encoded, char out[65])
{
    SIZE_T i,n=strlen(encoded); if(!n || n>128 || n%2) return 0;
    for(i=0;i<n;i+=2) { char pair[3]={encoded[i],encoded[i+1],0}, *end;
        unsigned long v=strtoul(pair,&end,16); if(*end || !v) return 0; out[i/2]=(char)v; }
    out[n/2]=0; return 1;
}
static HRESULT gs_join(char out[GS_PATH], const char *base, const char *name)
{
    if(strlen(base)+strlen(name)+2>GS_PATH) return GS_BAD_NAME;
    snprintf(out,GS_PATH,"%s\\%s",base,name); return S_OK;
}
static HRESULT gs_file_read(const char *path, BYTE **data, DWORD *size)
{
    HANDLE f; DWORD n,got; BYTE *buf;
    *data=NULL; *size=0;
    f=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,0,NULL);
    if(f==INVALID_HANDLE_VALUE) { DWORD e=GetLastError(); return e==ERROR_FILE_NOT_FOUND || e==ERROR_PATH_NOT_FOUND ? GS_MISSING : HRESULT_FROM_WIN32(e); }
    n=GetFileSize(f,NULL);
    if(n==INVALID_FILE_SIZE || n>GS_LIMIT) { CloseHandle(f); return E_FAIL_; }
    buf=malloc((SIZE_T)n+1);
    if(!buf) { CloseHandle(f); return E_FAIL_; }
    if(!ReadFile(f,buf,n,&got,NULL) || got!=n) { free(buf); CloseHandle(f); return E_FAIL_; }
    CloseHandle(f); buf[n]=0; *data=buf; *size=n; return S_OK;
}
static HRESULT gs_file_write(const char *path,const void *data,DWORD size)
{
    HANDLE f=CreateFileA(path,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL); DWORD done;
    HRESULT hr=S_OK;
    if(f==INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
    if(!WriteFile(f,data,size,&done,NULL)) hr=HRESULT_FROM_WIN32(GetLastError());
    else if(done!=size) hr=E_FAIL_;
    else if(!FlushFileBuffers(f)) hr=HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(f); if(FAILED(hr)) DeleteFileA(path); return hr;
}
static HRESULT gs_current(gs_container *c,char out[GS_PATH])
{
    char path[GS_PATH]; BYTE *data; DWORD size; HRESULT hr;
    gs_join(path,c->path,"CURRENT"); hr=gs_file_read(path,&data,&size);
    if(FAILED(hr)) return hr;
    if(size<2 || size>80 || data[0]!='g' || strspn((char *)data,"g0123456789abcdef-")!=size) hr=E_FAIL_;
    else hr=gs_join(out,c->path,(char *)data);
    free(data); return hr;
}
static HRESULT gs_load(gs_container *c,gs_blob **out)
{
    char generation[GS_PATH],pattern[GS_PATH],path[GS_PATH]; WIN32_FIND_DATAA info; HANDLE find;
    gs_blob **tail=out; HRESULT hr; *out=NULL;
    hr=gs_current(c,generation); if(hr==GS_MISSING) return S_OK; if(FAILED(hr)) return hr;
    gs_join(pattern,generation,"b-*"); find=FindFirstFileA(pattern,&info);
    if(find==INVALID_HANDLE_VALUE) { DWORD e=GetLastError(); return e==ERROR_FILE_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(e); }
    do {
        gs_blob *b;
        if(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;
        b=calloc(1,sizeof(*b)); if(!b) { hr=E_FAIL_; break; }
        if(!gs_decode(info.cFileName+2,b->name)) { free(b); hr=E_FAIL_; break; }
        gs_join(path,generation,info.cFileName); hr=gs_file_read(path,&b->data,&b->size);
        if(FAILED(hr)) { free(b); break; }
        *tail=b; tail=&b->next;
    } while(FindNextFileA(find,&info));
    if(SUCCEEDED(hr) && GetLastError()!=ERROR_NO_MORE_FILES) hr=HRESULT_FROM_WIN32(GetLastError());
    FindClose(find); if(FAILED(hr)) { gs_blobs_free(*out); *out=NULL; } return hr;
}
static gs_blob *gs_find(gs_blob *list,const char *name) { for(;list;list=list->next) if(!strcmp(list->name,name)) return list; return NULL; }
static HRESULT WINAPI save_init(void *self,void *user,const char *scid,BOOLEAN sync,gs_provider **out)
{
    char path[MAX_PATH]; gs_provider *p; HRESULT hr; (void)self; (void)sync;
    xlog("XGameSave initialize local-only"); if(!user || !out) return E_INVALIDARG_; *out=NULL;
    hr=save_directory(scid,path); if(FAILED(hr)) return hr;
    p=calloc(1,sizeof(*p)); if(!p) return E_FAIL_; p->refs=1;
    snprintf(p->path,sizeof(p->path),"\\\\?\\%s",path); *out=p;
    EnterCriticalSection(&g_lock); p->next=gs_providers; gs_providers=p; LeaveCriticalSection(&g_lock);
    return S_OK;
}
static void WINAPI save_close_provider(void *self,gs_provider *p) { (void)self; gs_provider_release(p); }
static HRESULT WINAPI save_create_container(void *self,gs_provider *p,const char *name,gs_container **out)
{
    char encoded[129]; gs_container *c; HRESULT hr; (void)self;
    xlog("XGameSave create container");
    if(!p || !out || !gs_provider_valid(p)) return E_INVALIDARG_; *out=NULL;
    hr=gs_encode(name,encoded); if(FAILED(hr)) return hr;
    c=calloc(1,sizeof(*c)); if(!c) return E_FAIL_; c->refs=1; c->provider=p;
    InterlockedIncrement(&p->refs); strcpy(c->name,name); gs_join(c->path,p->path,encoded);
    *out=c; return S_OK;
}
static void WINAPI save_close_container(void *self,gs_container *c) { (void)self; gs_container_release(c); }
static HRESULT WINAPI save_quota(void *self,gs_provider *p,INT64 *out)
{
    ULARGE_INTEGER available; (void)self; if(!p || !out) return E_INVALIDARG_;
    if(!GetDiskFreeSpaceExA(p->path,&available,NULL,NULL)) return HRESULT_FROM_WIN32(GetLastError());
    *out=available.QuadPart<GS_LIMIT?available.QuadPart:GS_LIMIT; return S_OK;
}
static HRESULT gs_info(gs_container *c,void *context,gs_container_cb cb)
{
    gs_blob *list,*b; gs_container_info info; char path[GS_PATH],current[GS_PATH];
    BYTE *display=NULL; DWORD size; HRESULT hr=gs_current(c,current);
    if(hr==GS_MISSING) return S_OK; if(FAILED(hr)) return hr;
    hr=gs_load(c,&list); if(FAILED(hr)) return hr;
    memset(&info,0,sizeof(info)); info.name=c->name; info.displayName=c->name;
    for(b=list;b;b=b->next) { info.blobCount++; info.totalSize+=b->size; }
    gs_join(path,current,"DISPLAY"); if(SUCCEEDED(gs_file_read(path,&display,&size))) info.displayName=(char *)display;
    cb(&info,context); free(display); gs_blobs_free(list); return S_OK;
}
static HRESULT WINAPI save_container_info(void *self,gs_provider *p,const char *name,void *context,gs_container_cb cb)
{
    gs_container *c; HRESULT hr; xlog("XGameSave container info"); if(!cb) return E_INVALIDARG_;
    hr=save_create_container(self,p,name,&c); if(FAILED(hr)) return hr;
    hr=gs_info(c,context,cb); gs_container_release(c); return hr;
}
static HRESULT WINAPI save_enumerate_containers_by_name(void *self,gs_provider *p,const char *prefix,void *context,gs_container_cb cb)
{
    char pattern[GS_PATH],name[65]; WIN32_FIND_DATAA info; HANDLE find; HRESULT hr=S_OK;
    xlog("XGameSave enumerate containers"); if(!p || !cb) return E_INVALIDARG_;
    gs_join(pattern,p->path,"*"); find=FindFirstFileA(pattern,&info);
    if(find==INVALID_HANDLE_VALUE) return GetLastError()==ERROR_FILE_NOT_FOUND?S_OK:HRESULT_FROM_WIN32(GetLastError());
    do { if((info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && gs_decode(info.cFileName,name) &&
        (!prefix || !strncmp(name,prefix,strlen(prefix)))) {
        hr=save_container_info(self,p,name,context,cb); if(FAILED(hr)) break;
    } } while(FindNextFileA(find,&info));
    FindClose(find); return hr;
}
static HRESULT WINAPI save_enumerate_containers(void *self,gs_provider *p,void *context,gs_container_cb cb)
{ return save_enumerate_containers_by_name(self,p,NULL,context,cb); }
static HRESULT WINAPI save_enumerate_blobs_by_name(void *self,gs_container *c,const char *prefix,void *context,gs_blob_cb cb)
{
    gs_blob *list,*b; HRESULT hr; (void)self; xlog("XGameSave enumerate blobs");
    if(!c || !cb) return E_INVALIDARG_; hr=gs_load(c,&list); if(FAILED(hr)) return hr;
    for(b=list;b;b=b->next) if(!prefix || !strncmp(b->name,prefix,strlen(prefix))) {
        gs_blob_info info={b->name,b->size}; if(!cb(&info,context)) break;
    }
    gs_blobs_free(list); return S_OK;
}
static HRESULT WINAPI save_enumerate_blobs(void *self,gs_container *c,void *context,gs_blob_cb cb)
{ return save_enumerate_blobs_by_name(self,c,NULL,context,cb); }
static HRESULT gs_select(gs_container *c,const char **names,UINT32 count,gs_blob **out)
{
    gs_blob *all,*b,**tail; HRESULT hr=gs_load(c,&all); UINT32 i;
    *out=NULL; if(FAILED(hr)) return hr; if(!names) { *out=all; return S_OK; }
    tail=out;
    for(i=0;i<count;i++) {
        gs_blob *copy; b=names[i]?gs_find(all,names[i]):NULL;
        if(!b) { hr=GS_MISSING; break; }
        copy=calloc(1,sizeof(*copy)); if(!copy) { hr=E_FAIL_; break; }
        strcpy(copy->name,b->name); copy->size=b->size; copy->data=malloc((SIZE_T)b->size+1);
        if(!copy->data) { free(copy); hr=E_FAIL_; break; }
        memcpy(copy->data,b->data,b->size); *tail=copy; tail=&copy->next;
    }
    gs_blobs_free(all); if(FAILED(hr)) { gs_blobs_free(*out); *out=NULL; } return hr;
}
static SIZE_T gs_result_size(gs_blob *list)
{ SIZE_T size=0; for(;list;list=list->next) size+=sizeof(gs_blob_result)+strlen(list->name)+1+list->size; return size; }
static HRESULT gs_pack(gs_blob *list,SIZE_T capacity,gs_blob_result *out,UINT32 *count)
{
    gs_blob *b; UINT32 n=0,i=0; char *data;
    for(b=list;b;b=b->next) n++;
    if(!count) return E_INVALIDARG_; *count=n;
    if(capacity<gs_result_size(list) || (n && !out)) return GS_SMALL;
    data=(char *)(out+n);
    for(b=list;b;b=b->next,i++) {
        SIZE_T len=strlen(b->name)+1; out[i].info.name=data; out[i].info.size=b->size;
        memcpy(data,b->name,len); data+=len; out[i].data=(BYTE *)data;
        memcpy(data,b->data,b->size); data+=b->size;
    } return S_OK;
}
static HRESULT WINAPI save_read(void *self,gs_container *c,const char **names,UINT32 *count,SIZE_T capacity,gs_blob_result *out)
{
    gs_blob *list; HRESULT hr; (void)self; xlog("XGameSave read");
    if(!c || !count) return E_INVALIDARG_; hr=gs_select(c,names,*count,&list);
    if(SUCCEEDED(hr)) { hr=gs_pack(list,capacity,out,count); gs_blobs_free(list); } return hr;
}
static HRESULT WINAPI save_create_update(void *self,gs_container *c,const char *display,gs_update **out)
{
    gs_update *u; (void)self; xlog("XGameSave create update"); if(!c || !out) return E_INVALIDARG_;
    *out=NULL; u=calloc(1,sizeof(*u)); if(!u) return E_FAIL_; u->refs=1;
    u->display=strdup(display?display:c->name); if(!u->display) { free(u); return E_FAIL_; }
    u->container=c; InterlockedIncrement(&c->refs); *out=u; return S_OK;
}
static void WINAPI save_close_update(void *self,gs_update *u) { (void)self; gs_update_release(u); }
static HRESULT WINAPI save_write(void *self,gs_update *u,const char *name,BYTE *data,SIZE_T size)
{
    char encoded[129]; gs_blob *b; BYTE *copy; HRESULT hr; (void)self; xlog("XGameSave queue blob write bytes=%llu",(unsigned long long)size);
    if(!u || (size && !data) || size>GS_LIMIT) return E_INVALIDARG_;
    hr=gs_encode(name,encoded); if(FAILED(hr)) return hr;
    copy=malloc(size+1); if(!copy) return E_FAIL_; if(size) memcpy(copy,data,size);
    b=gs_find(u->changes,name); if(!b) { b=calloc(1,sizeof(*b)); if(!b) { free(copy); return E_FAIL_; }
        strcpy(b->name,name); b->next=u->changes; u->changes=b; }
    free(b->data); b->data=copy; b->size=size; b->deleted=0; return S_OK;
}
static HRESULT WINAPI save_delete_blob(void *self,gs_update *u,const char *name)
{ HRESULT hr=save_write(self,u,name,NULL,0); if(SUCCEEDED(hr)) gs_find(u->changes,name)->deleted=1; return hr; }
static HRESULT WINAPI save_submit(void *self,gs_update *u)
{
    gs_blob *old,*b,*changes; char generation[81],dir[GS_PATH],file[GS_PATH],encoded[129],leaf[132],pointer[GS_PATH],temp[GS_PATH];
    HRESULT hr; (void)self; xlog("XGameSave submit local transaction"); if(!u) return E_INVALIDARG_;
    EnterCriticalSection(&g_lock); hr=gs_load(u->container,&old); if(FAILED(hr)) goto done;
    if(!CreateDirectoryA(u->container->path,NULL) && GetLastError()!=ERROR_ALREADY_EXISTS) { hr=HRESULT_FROM_WIN32(GetLastError()); goto release; }
    snprintf(generation,sizeof(generation),"g%llx-%lx-%lx",(unsigned long long)GetTickCount64(),(unsigned long)GetCurrentProcessId(),(unsigned long)InterlockedIncrement(&gs_generation));
    gs_join(dir,u->container->path,generation);
    if(!CreateDirectoryA(dir,NULL)) { hr=HRESULT_FROM_WIN32(GetLastError()); goto release; }
    for(b=old;b;b=b->next) if(!gs_find(u->changes,b->name)) {
        gs_encode(b->name,encoded); snprintf(leaf,sizeof(leaf),"b-%s",encoded); gs_join(file,dir,leaf);
        hr=gs_file_write(file,b->data,b->size); if(FAILED(hr)) goto release;
    }
    for(changes=u->changes;changes;changes=changes->next) if(!changes->deleted) {
        gs_encode(changes->name,encoded); snprintf(leaf,sizeof(leaf),"b-%s",encoded); gs_join(file,dir,leaf);
        hr=gs_file_write(file,changes->data,changes->size); if(FAILED(hr)) goto release;
    }
    gs_join(file,dir,"DISPLAY"); hr=gs_file_write(file,u->display,strlen(u->display)); if(FAILED(hr)) goto release;
    snprintf(leaf,sizeof(leaf),"CURRENT-%s.tmp",generation); gs_join(temp,u->container->path,leaf);
    hr=gs_file_write(temp,generation,strlen(generation)); if(FAILED(hr)) goto release;
    gs_join(pointer,u->container->path,"CURRENT");
    if(!MoveFileExA(temp,pointer,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { hr=HRESULT_FROM_WIN32(GetLastError()); DeleteFileA(temp); }
release: gs_blobs_free(old);
done: LeaveCriticalSection(&g_lock); return hr;
}
static HRESULT WINAPI save_delete_container(void *self,gs_provider *p,const char *name)
{
    gs_container *c; char path[GS_PATH]; HRESULT hr=save_create_container(self,p,name,&c);
    if(FAILED(hr)) return hr; gs_join(path,c->path,"CURRENT");
    if(!DeleteFileA(path) && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND) hr=HRESULT_FROM_WIN32(GetLastError());
    gs_container_release(c); return hr;
}

/* Arguments and read data remain owned until the async completion is consumed. */
enum { GS_INIT,GS_QUOTA,GS_READ,GS_SUBMIT,GS_DELETE };
typedef struct gs_async { int op; gs_provider *p; gs_container *c; gs_update *u; gs_blob *blobs; INT64 quota; HRESULT hr; } gs_async;
static HRESULT WINAPI gs_async_provider(UINT32 op,const XAsyncProviderData *data)
{
    gs_async *ctx=data->context; HRESULT hr=S_OK; SIZE_T size=0;
    if(op==OP_DOWORK) {
        hr=ctx->hr;
        if(SUCCEEDED(hr)) switch(ctx->op) {
            case GS_INIT: size=sizeof(void *); break;
            case GS_QUOTA: hr=save_quota(NULL,ctx->p,&ctx->quota); size=sizeof(INT64); break;
            case GS_READ: size=gs_result_size(ctx->blobs); break;
            case GS_SUBMIT: hr=save_submit(NULL,ctx->u); break;
            case GS_DELETE: hr=save_delete_container(NULL,ctx->p,ctx->c->name); break;
        }
        complete_async(data->async,hr,size);
    } else if(op==OP_CLEANUP) {
        gs_provider_release(ctx->p); gs_container_release(ctx->c); gs_update_release(ctx->u); gs_blobs_free(ctx->blobs); free(ctx);
    }
    return S_OK;
}
static HRESULT gs_async_begin(XAsyncBlock *block,gs_async *ctx)
{
    async_state *st; HRESULT hr; XAsyncProviderData data;
    if(!block || !(st=calloc(1,sizeof(*st)))) { memset(&data,0,sizeof(data)); data.context=ctx; gs_async_provider(OP_CLEANUP,&data); return E_INVALIDARG_; }
    st->magic=ASYNC_MAGIC; st->block=block; st->provider=gs_async_provider; st->context=ctx;
    st->retain_result=1;
    snprintf(st->name,sizeof(st->name),"XGameSave/%d",ctx->op); state_bind(block,st);
    hr=schedule_async(block,0); if(FAILED(hr)) cleanup_state(st); return hr;
}
static HRESULT gs_async_result(XAsyncBlock *block,gs_async **ctx,async_state **state)
{
    async_state *st=state_of(block); if(!st || st->provider!=gs_async_provider) return E_INVALIDARG_;
    if(!st->complete) return E_PENDING_; if(FAILED(st->result)) return st->result;
    *ctx=st->context; *state=st; return S_OK;
}
static void gs_consume(async_state *st)
{
    consume_async(st);
}
static HRESULT WINAPI save_init_async(void *self,void *user,const char *scid,BOOLEAN sync,XAsyncBlock *block)
{
    gs_async *ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_; ctx->op=GS_INIT;
    ctx->hr=save_init(self,user,scid,sync,&ctx->p); return gs_async_begin(block,ctx);
}
static HRESULT WINAPI save_init_result(void *self,XAsyncBlock *block,gs_provider **out)
{
    gs_async *ctx; async_state *st; HRESULT hr; (void)self; xlog("XGameSave initialize result");
    if(!out) return E_INVALIDARG_; *out=NULL;
    hr=gs_async_result(block,&ctx,&st); if(FAILED(hr)) { xlog("XGameSave init result hr=%08lx state=%p",(unsigned long)hr,state_of(block)); return hr; }
    if(ctx->op!=GS_INIT) return E_INVALIDARG_;
    *out=ctx->p; ctx->p=NULL; gs_consume(st); return S_OK;
}
static HRESULT WINAPI save_quota_async(void *self,gs_provider *p,XAsyncBlock *block)
{
    gs_async *ctx; (void)self; if(!p) return E_INVALIDARG_; ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_;
    ctx->op=GS_QUOTA; ctx->p=p; InterlockedIncrement(&p->refs); return gs_async_begin(block,ctx);
}
static HRESULT WINAPI save_quota_result(void *self,XAsyncBlock *block,INT64 *out)
{
    gs_async *ctx; async_state *st; HRESULT hr; (void)self; if(!out) return E_INVALIDARG_;
    hr=gs_async_result(block,&ctx,&st); if(FAILED(hr)) return hr; if(ctx->op!=GS_QUOTA) return E_INVALIDARG_; *out=ctx->quota; gs_consume(st); return S_OK;
}
static HRESULT WINAPI save_read_async(void *self,gs_container *c,const char **names,UINT32 count,XAsyncBlock *block)
{
    gs_async *ctx; (void)self; xlog("XGameSave read async requested=%lu",(unsigned long)count);
    if(!c) return E_INVALIDARG_; ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_;
    ctx->op=GS_READ; ctx->hr=gs_select(c,names,count,&ctx->blobs); return gs_async_begin(block,ctx);
}
static HRESULT WINAPI save_read_result(void *self,XAsyncBlock *block,SIZE_T capacity,gs_blob_result *out,UINT32 *count)
{
    gs_async *ctx; async_state *st; HRESULT hr; (void)self; xlog("XGameSave read result");
    hr=gs_async_result(block,&ctx,&st); if(FAILED(hr)) return hr; if(ctx->op!=GS_READ) return E_INVALIDARG_;
    hr=gs_pack(ctx->blobs,capacity,out,count); if(SUCCEEDED(hr)) gs_consume(st); return hr;
}
static HRESULT WINAPI save_submit_async(void *self,gs_update *u,XAsyncBlock *block)
{
    gs_async *ctx; (void)self; if(!u) return E_INVALIDARG_; ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_;
    ctx->op=GS_SUBMIT; ctx->u=u; InterlockedIncrement(&u->refs); return gs_async_begin(block,ctx);
}
static HRESULT WINAPI save_submit_result(void *self,XAsyncBlock *block)
{ async_state *st=state_of(block); HRESULT hr; (void)self;
  hr=thr_GetStatus(NULL,block,FALSE); if(st && st->complete) gs_consume(st); return hr; }
static HRESULT WINAPI save_delete_async(void *self,gs_provider *p,const char *name,XAsyncBlock *block)
{
    gs_async *ctx; HRESULT hr; if(!p) return E_INVALIDARG_; ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_;
    ctx->op=GS_DELETE; ctx->p=p; InterlockedIncrement(&p->refs);
    hr=save_create_container(self,p,name,&ctx->c); if(FAILED(hr)) { gs_provider_release(p); free(ctx); return hr; }
    return gs_async_begin(block,ctx);
}
static HRESULT WINAPI save_delete_result(void *self,XAsyncBlock *block)
{ return save_submit_result(self,block); }
