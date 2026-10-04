/* Regression: XAsyncGetResult on a failed operation must return the failure
 * without asking the provider for a result (there is none). Loads the built
 * xgameruntime.dll and drives a custom provider through XAsyncBegin on a
 * manual queue, calling XAsyncGetResult from the completion callback. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct XAsyncBlock {
    void *queue;
    void *context;
    void (__stdcall *callback)(struct XAsyncBlock *);
    unsigned char internal[32];
} XAsyncBlock;

typedef struct XAsyncProviderData {
    XAsyncBlock *async;
    SIZE_T bufferSize;
    void *buffer;
    void *context;
} XAsyncProviderData;

static void *thr;
static void **vt;
static const char identity[] = "probe";
static int getresult_calls, callbacks;
static HRESULT callback_hr;

static HRESULT __stdcall provider(uint32_t op, const XAsyncProviderData *d)
{
    HRESULT result = (HRESULT)(intptr_t)d->async->context;
    if (op == 0) /* begin */
        return ((HRESULT (__stdcall *)(void *, XAsyncBlock *, uint32_t))vt[9])(thr, d->async, 0);
    if (op == 1) /* do work */
        ((void (__stdcall *)(void *, XAsyncBlock *, HRESULT, SIZE_T))vt[10])(thr, d->async, result, sizeof(int));
    if (op == 2) { /* get result */
        getresult_calls++;
        *(int *)d->buffer = 42;
    }
    return S_OK;
}

static void __stdcall on_done(XAsyncBlock *b)
{
    int value = 0;
    callbacks++;
    callback_hr = ((HRESULT (__stdcall *)(void *, XAsyncBlock *, const void *, SIZE_T, void *, SIZE_T *))vt[11])
        (thr, b, identity, sizeof(value), &value, NULL);
    if (SUCCEEDED(callback_hr) && value != 42) callback_hr = E_UNEXPECTED;
}

static int run_case(void *queue, HRESULT result, int expect_getresult)
{
    XAsyncBlock block;
    HRESULT hr;
    getresult_calls = callbacks = 0;
    callback_hr = E_PENDING;
    memset(&block, 0, sizeof(block));
    block.queue = queue;
    block.context = (void *)(intptr_t)result;
    block.callback = on_done;
    hr = ((HRESULT (__stdcall *)(void *, XAsyncBlock *, void *, const void *, const char *, void *))vt[7])
        (thr, &block, NULL, identity, identity, provider);
    ((unsigned char (__stdcall *)(void *, void *, uint32_t, uint32_t))vt[16])(thr, queue, 0, 100);
    ((unsigned char (__stdcall *)(void *, void *, uint32_t, uint32_t))vt[16])(thr, queue, 1, 100);
    printf("begin %08lx result %08lx: callback %d hr %08lx provider GetResult calls %d\n",
           (unsigned long)hr, (unsigned long)result, callbacks, (unsigned long)callback_hr, getresult_calls);
    return hr == S_OK && callbacks == 1 && callback_hr == result && getresult_calls == expect_getresult;
}

int main(void)
{
    HMODULE m;
    HRESULT (__stdcall *query)(const GUID *, const GUID *, void **);
    GUID id = { 0x073b7dcb, 0x1fcf, 0x4030, { 0x94, 0xbe, 0xe3, 0xc9, 0xeb, 0x62, 0x34, 0x28 } };
    void *queue = NULL;
    int ok;

    m = LoadLibraryA("xgameruntime.dll");
    if (!m) { printf("load %lu\n", GetLastError()); return 1; }
    query = (void *)GetProcAddress(m, "QueryApiImpl");
    if (!query || query(&id, &id, &thr) || !thr) { printf("threading interface missing\n"); return 1; }
    vt = *(void ***)thr;
    if (((HRESULT (__stdcall *)(void *, uint32_t, uint32_t, void **))vt[12])(thr, 0, 0, &queue) || !queue) {
        printf("manual queue failed\n");
        return 1;
    }
    ok = run_case(queue, HRESULT_FROM_WIN32(ERROR_NOT_FOUND), 0);
    ok &= run_case(queue, S_OK, 1);
    printf(ok ? "OK\n" : "FAIL\n");
    return ok ? 0 : 1;
}
