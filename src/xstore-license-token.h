/* This returns only the opaque license token issued by Microsoft. */
typedef struct store_token_work {
    char request[GS_PATH], response[GS_PATH], error[GS_PATH];
    char *token;
} store_token_work;
static LONG store_request_counter;
static HRESULT WINAPI store_license_provider(UINT32 op,const XAsyncProviderData *data)
{
    store_token_work *ctx=data->context;
    HRESULT hr=E_FAIL_;
    if(op==OP_DOWORK) {
        ULONGLONG deadline=GetTickCount64()+95000;
        while(GetTickCount64()<deadline) {
            DWORD bytes;
            BYTE *token;
            async_state *st=state_of(data->async);
            if(!st || st->canceled) { hr=E_ABORT_; break; }
            if(GetFileAttributesA(ctx->error)!=INVALID_FILE_ATTRIBUTES) break;
            hr=gs_file_read(ctx->response,&token,&bytes);
            if(SUCCEEDED(hr)) {
                if(bytes && bytes<65536 && !memchr(token,0,bytes)) ctx->token=(char *)token;
                else { free(token); hr=E_FAIL_; }
                break;
            }
            if(hr!=GS_MISSING) break;
            hr=E_FAIL_;
            Sleep(100);
        }
        xlog("XStore Microsoft license response hr=%08lx",(unsigned long)hr);
        complete_async(data->async,hr,ctx->token?strlen(ctx->token)+1:0);
    } else if(op==OP_CLEANUP) {
        DeleteFileA(ctx->request); DeleteFileA(ctx->response); DeleteFileA(ctx->error);
        if(ctx->token) { memset(ctx->token,0,strlen(ctx->token)); free(ctx->token); }
        free(ctx);
    }
    return S_OK;
}
static HRESULT WINAPI store_XStoreQueryLicenseTokenAsync(void *self,store_context_local *store,const char **products,SIZE_T count,const char *custom,XAsyncBlock *block)
{
    char root[GS_PATH],name[96],temporary[GS_PATH];
    BYTE *request,*cursor;
    SIZE_T length=4,custom_length,i;
    DWORD word; store_token_work *ctx; async_state *st; HRESULT hr;
    (void)self; xlog("XStoreQueryLicenseTokenAsync products=%llu",(unsigned long long)count);
    if(!store || !products || !count || count>64 || !custom || !*custom || !block) return E_INVALIDARG_;
    if(!GetEnvironmentVariableA("XODUS_STORE_BRIDGE_PATH",root,sizeof(root))) return E_NOTIMPL_;
    custom_length=strlen(custom); if(custom_length>4096) return E_INVALIDARG_;
    for(i=0;i<count;i++) {
        SIZE_T n;
        if(!products[i] || !(n=strlen(products[i])) || n>32) return E_INVALIDARG_;
        length+=4+n;
    }
    length+=4+custom_length; if(length>8192) return E_INVALIDARG_;
    request=malloc(length); ctx=calloc(1,sizeof(*ctx)); st=calloc(1,sizeof(*st));
    if(!request || !ctx || !st) { free(request); free(ctx); free(st); return E_FAIL_; }
    cursor=request; word=count; memcpy(cursor,&word,4); cursor+=4;
    for(i=0;i<count;i++) { word=strlen(products[i]); memcpy(cursor,&word,4); cursor+=4; memcpy(cursor,products[i],word); cursor+=word; }
    word=custom_length; memcpy(cursor,&word,4); cursor+=4; memcpy(cursor,custom,word);
    snprintf(name,sizeof(name),"%lx-%lx",(unsigned long)GetCurrentProcessId(),(unsigned long)InterlockedIncrement(&store_request_counter));
    snprintf(temporary,sizeof(temporary),"%s\\%s.tmp",root,name);
    snprintf(ctx->request,sizeof(ctx->request),"%s\\%s.request",root,name);
    snprintf(ctx->response,sizeof(ctx->response),"%s\\%s.response",root,name);
    snprintf(ctx->error,sizeof(ctx->error),"%s\\%s.error",root,name);
    hr=gs_file_write(temporary,request,length); memset(request,0,length); free(request);
    if(SUCCEEDED(hr) && !MoveFileExA(temporary,ctx->request,MOVEFILE_WRITE_THROUGH)) hr=HRESULT_FROM_WIN32(GetLastError());
    if(FAILED(hr)) { DeleteFileA(temporary); free(ctx); free(st); return hr; }
    st->magic=ASYNC_MAGIC; st->block=block; st->context=ctx; st->provider=store_license_provider; st->retain_result=1;
    snprintf(st->name,sizeof(st->name),"XStoreLicenseToken"); state_bind(block,st);
    hr=schedule_async(block,0); if(FAILED(hr)) cleanup_state(st); return hr;
}
static HRESULT WINAPI store_XStoreQueryLicenseTokenResultSize(void *self,XAsyncBlock *block,SIZE_T *size)
{
    async_state *st=state_of(block); (void)self;
    if(!size || !st || st->provider!=store_license_provider) return E_INVALIDARG_;
    if(!st->complete) return E_PENDING_;
    if(FAILED(st->result)) { HRESULT hr=st->result; consume_async(st); return hr; }
    *size=st->required; return S_OK;
}
static HRESULT WINAPI store_XStoreQueryLicenseTokenResult(void *self,XAsyncBlock *block,SIZE_T capacity,char *result)
{
    async_state *st=state_of(block); store_token_work *ctx; (void)self;
    if(!st || st->provider!=store_license_provider) return E_INVALIDARG_;
    if(!st->complete) return E_PENDING_;
    if(FAILED(st->result)) { HRESULT hr=st->result; consume_async(st); return hr; }
    if(!result || capacity<st->required) return E_INSUFFICIENT_;
    ctx=st->context; if(!ctx->token) return E_FAIL_;
    memcpy(result,ctx->token,st->required); xlog("XStoreQueryLicenseTokenResult returned Microsoft token");
    consume_async(st); return S_OK;
}
