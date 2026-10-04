#include "xgameruntime.c"
static int result_calls;
static HRESULT WINAPI probe(UINT32 op, const XAsyncProviderData *data)
{
    (void)data;
    if (op == OP_GETRESULT) { result_calls++; return E_FAIL_; }
    return S_OK;
}
void WINAPI check_entry(void)
{
    XAsyncBlock block = {0};
    async_state *st = calloc(1, sizeof(*st));
    HRESULT hr;
    DllMain(GetModuleHandleA(NULL), DLL_PROCESS_ATTACH, NULL);
    st->magic = ASYNC_MAGIC; st->block = &block; st->provider = probe;
    st->complete = 1; st->result = E_INVALIDARG_;
    state_bind(&block, st);
    hr = thr_GetResult(NULL, &block, NULL, 0, NULL, NULL);
    if (hr != E_INVALIDARG_ || result_calls != 0) {
        puts("FAIL: failed async invoked result provider or lost error"); ExitProcess(1);
    }
    cleanup_state(st);
    puts("PASS: failed async preserves failure without invoking result provider");
    ExitProcess(0);
}
