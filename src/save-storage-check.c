/* Integration check against real local disk, including deferred async results
 * and reading data written by an earlier process. Uses a separate test SCID.
 */
#include "xgameruntime.c"
static volatile LONG check_callback_done;
static void WINAPI check_callback(XAsyncBlock *block) { (void)block; InterlockedExchange(&check_callback_done,1); }
static void check_hr(HRESULT hr,const char *operation)
{ if(FAILED(hr)) { printf("FAIL %s: %08lx\n",operation,(unsigned long)hr); ExitProcess(1); } }
static void check_condition(int ok,const char *operation)
{ if(!ok) { printf("FAIL %s\n",operation); ExitProcess(1); } }
static void WINAPI guard_callback(XAsyncBlock *block)
{
    SIZE_T size; void *buffer; token16_blob *token; DWORD old;
    check_hr(user_token_size(NULL,block,&size),"guard token size"); buffer=malloc(size);
    check_condition(buffer!=NULL,"guard token allocation");
    check_hr(user_token16_result(NULL,block,size,buffer,&token,NULL),"guard token result");
    free(buffer);
    check_condition(VirtualProtect(block,4096,PAGE_NOACCESS,&old),"guard callback releases block access");
    InterlockedExchange(&check_callback_done,1);
}
void WINAPI check_entry(void)
{
    gs_provider *p; gs_container *c; gs_update *u; XAsyncBlock async={0};
    BYTE payload[]={0x00,0x12,0xff,0x00,0x34}; const char *names[]={"state"};
    gs_blob_result *result; SIZE_T size; UINT32 count; HRESULT hr; int reader;
    DllMain(GetModuleHandleA(NULL),DLL_PROCESS_ATTACH,NULL);
    InitializeApiImpl(0,0); async.callback=check_callback;
    check_hr(save_init_async(NULL,&user_obj,"CodexStorageRegression",FALSE,&async),"async initialize");
    for(int i=0; !check_callback_done && i<5000; i++) Sleep(1);
    check_condition(check_callback_done,"completion callback"); Sleep(20);
    check_hr(save_init_result(NULL,&async,&p),"result after callback returns");
    check_hr(save_create_container(NULL,p,"StorageTest",&c),"create container");
    reader=strstr(GetCommandLineA(),"--read")!=NULL;
    if(!reader) {
        check_hr(save_create_update(NULL,c,"StorageTest",&u),"create update");
        check_hr(save_write(NULL,u,"state",payload,sizeof(payload)),"binary write");
        check_hr(save_write(NULL,u,"other",payload,2),"second blob write");
        check_hr(save_submit(NULL,u),"publish transaction"); save_close_update(NULL,u);
        check_hr(save_create_update(NULL,c,"StorageTest",&u),"second update");
        check_hr(save_delete_blob(NULL,u,"other"),"delete second blob");
        check_hr(save_submit(NULL,u),"publish deletion"); save_close_update(NULL,u);
    }
    memset(&async,0,sizeof(async));
    check_hr(save_read_async(NULL,c,names,1,&async),"read async");
    check_hr(thr_GetStatus(NULL,&async,TRUE),"read status");
    check_hr(thr_GetResultSize(NULL,&async,&size),"read result size");
    result=malloc(size); check_condition(result!=NULL,"allocate result");
    hr=save_read_result(NULL,&async,size-1,result,&count);
    check_condition(hr==GS_SMALL,"small buffer rejected without consuming result");
    check_hr(save_read_result(NULL,&async,size,result,&count),"read result");
    check_condition(count==1 && !strcmp(result[0].info.name,"state") && result[0].info.size==sizeof(payload) &&
      !memcmp(result[0].data,payload,sizeof(payload)),"persisted binary bytes"); free(result);
    names[0]="other"; count=1;
    check_condition(save_read(NULL,c,names,&count,0,NULL)==GS_MISSING,"deleted blob missing");
    save_close_container(NULL,c); save_close_provider(NULL,p);
    printf("PASS %s: deferred async result, binary data, buffer bounds, transaction and deletion\n",reader?"cross-process read":"write/read");
    memset(&async,0,sizeof(async)); check_callback_done=0; async.callback=check_callback;
    check_hr(user_token16_async(NULL,&user_obj,0,L"GET",L"https://api.minecraftservices.com",0,NULL,0,NULL,&async),"UTF16 token async");
    for(int i=0; !check_callback_done && i<5000; i++) Sleep(1);
    check_condition(check_callback_done,"token completion callback"); Sleep(20);
    check_hr(user_token_size(NULL,&async,&size),"UTF16 result size");
    {
        void *buffer=malloc(size); token16_blob *token; WCHAR expected[8192];
        check_condition(buffer!=NULL,"allocate token buffer");
        check_hr(user_token16_result(NULL,&async,size,buffer,&token,NULL),"UTF16 result after callback");
        check_condition(MultiByteToWideChar(CP_UTF8,0,g_mc_token,-1,expected,8192)>0,"expected UTF16 conversion");
        check_condition(token->tokenCount==lstrlenW(expected)+1 && !lstrcmpW(token->token,expected) && !*token->signature,"Unicode token bytes and termination");
        free(buffer);
    }
    puts("PASS UTF16 token: cached authentication, Unicode conversion, deferred result and termination");
    {
        queue_obj *q; XAsyncBlock pending={0}; gs_provider *provider;
        check_callback_done=0;
        check_hr(thr_QueueCreate(NULL,MODE_MANUAL,MODE_MANUAL,&q),"manual async queue");
        pending.queue=q; pending.callback=check_callback;
        check_hr(save_init_async(NULL,&user_obj,"CodexStorageRegression",FALSE,&pending),"manual initialization");
        check_condition(dispatch_one(q,PORT_WORK),"manual work dispatch");
        check_hr(save_init_result(NULL,&pending,&provider),"consume result before callback dispatch");
        check_condition(dispatch_one(q,PORT_COMP) && check_callback_done,"pending callback still delivered");
        save_close_provider(NULL,provider);
    }
    {
        XAsyncBlock *guard=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        check_condition(guard!=NULL,"guard block allocation"); guard->callback=guard_callback;
        check_callback_done=0;
        check_hr(user_token16_async(NULL,&user_obj,0,L"GET",L"https://api.minecraftservices.com",0,NULL,0,NULL,guard),"guard token start");
        for(int i=0; !check_callback_done && i<5000; i++) Sleep(1);
        check_condition(check_callback_done,"guard callback completion"); Sleep(50);
        VirtualFree(guard,0,MEM_RELEASE);
    }
    puts("PASS callback lifetime: polling before callback delivery and inaccessible block after result consumption");
    ExitProcess(0);
}
