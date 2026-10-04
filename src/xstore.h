/* XStore for the Microsoft Store build. Only context creation and the license
 * token are implemented. The token is the real one Microsoft issues for the
 * signed-in account: requests are exchanged as files in the directory named by
 * XODUS_STORE_BRIDGE_PATH, where an external helper (store-license-bridge.py)
 * obtains it. Without that variable the query fails with E_NOTIMPL.
 * Interface order follows xodus-gaming/xgameruntime's xstore.idl. */
G(IID_Store0, 0x0dd112ac, 0x7c24, 0x448c, 0xb9, 0x2b, 0x39, 0x60, 0xfb, 0x5b, 0xd3, 0x0c)
G(IID_Store1, 0x60b09f4e, 0x1b85, 0x45b1, 0x82, 0x6c, 0x16, 0x91, 0x18, 0xe2, 0x30, 0xe1)
G(IID_Store2, 0x2d42fea5, 0xe71d, 0x4b76, 0x97, 0xcd, 0xc5, 0x0a, 0xfb, 0xb3, 0xae, 0x5d)
G(IID_Store3, 0xde3dbdd4, 0x0b37, 0x4bdb, 0xa1, 0x0e, 0xac, 0xf3, 0xa3, 0x54, 0xd0, 0x6a)
G(IID_Store4, 0x5c48dedf, 0x0b67, 0x4492, 0xa4, 0xb5, 0x68, 0x29, 0xb8, 0xe7, 0x96, 0xe1)
G(IID_Store5, 0xb09d803c, 0x2414, 0x4a05, 0x82, 0xc6, 0x66, 0xdf, 0xdc, 0x9e, 0x9a, 0x44)

/* For unimplemented methods returning BOOLEAN or void. */
static BOOLEAN WINAPI stub_false(void *self)
{
    (void)self;
    log_once("stub_false");
    return FALSE;
}

typedef struct store_context { UINT64 xuid; } store_context;

static HRESULT WINAPI store_create_context(void *self, void *user, store_context **out)
{
    (void)self;
    xlog("XStoreCreateContext");
    if (!out || (user && user != &user_obj) || !auth_read_file() || !g_xuid) return E_INVALIDARG_;
    *out = calloc(1, sizeof(**out));
    if (!*out) return E_FAIL_;
    (*out)->xuid = g_xuid;
    return S_OK;
}
static void WINAPI store_close_context(void *self, store_context *ctx) { (void)self; free(ctx); }

/* Request file, little-endian: u32 product count, then u32 length + UTF-8
 * bytes for each product ID and for the custom developer string. The helper
 * answers with NAME.response holding the opaque token, or NAME.error. */
typedef struct store_token_work {
    char request[GS_PATH], response[GS_PATH], error[GS_PATH];
    char *token;
} store_token_work;
static LONG store_request_counter;

static BYTE *store_put(BYTE *p, const char *s)
{
    DWORD n = (DWORD)strlen(s);
    memcpy(p, &n, 4);
    memcpy(p + 4, s, n);
    return p + 4 + n;
}

static HRESULT WINAPI store_token_provider(UINT32 op, const XAsyncProviderData *data)
{
    store_token_work *ctx = data->context;
    if (op == OP_DOWORK) {
        ULONGLONG deadline = GetTickCount64() + 95000;
        HRESULT hr = E_FAIL_;
        while (GetTickCount64() < deadline) {
            async_state *st = state_of(data->async);
            BYTE *token;
            DWORD bytes;
            if (!st || st->canceled) { hr = E_ABORT_; break; }
            if (GetFileAttributesA(ctx->error) != INVALID_FILE_ATTRIBUTES) { hr = E_FAIL_; break; }
            hr = gs_file_read(ctx->response, &token, &bytes);
            if (SUCCEEDED(hr)) {
                if (bytes && bytes < 65536 && !memchr(token, 0, bytes)) ctx->token = (char *)token;
                else { free(token); hr = E_FAIL_; }
                break;
            }
            if (hr != GS_MISSING) break;
            hr = E_FAIL_;
            Sleep(100);
        }
        xlog("XStoreQueryLicenseToken hr=%08lx", (unsigned long)hr);
        complete_async(data->async, hr, ctx->token ? strlen(ctx->token) + 1 : 0);
    } else if (op == OP_CLEANUP) {
        DeleteFileA(ctx->request);
        DeleteFileA(ctx->response);
        DeleteFileA(ctx->error);
        if (ctx->token) { SecureZeroMemory(ctx->token, strlen(ctx->token)); free(ctx->token); }
        free(ctx);
    }
    return S_OK;
}

static HRESULT WINAPI store_token_async(void *self, store_context *store, const char **products, SIZE_T count,
                                        const char *custom, XAsyncBlock *block)
{
    char root[GS_PATH], name[48], temp[GS_PATH];
    store_token_work *ctx;
    async_state *st;
    BYTE *request, *p;
    SIZE_T length = 4, i;
    DWORD n32;
    HRESULT hr;
    (void)self;
    xlog("XStoreQueryLicenseTokenAsync products=%llu", (unsigned long long)count);
    if (!store || !products || !count || count > 64 || !custom || !*custom || !block) return E_INVALIDARG_;
    if (!GetEnvironmentVariableA("XODUS_STORE_BRIDGE_PATH", root, sizeof(root))) return E_NOTIMPL_;
    for (i = 0; i < count; i++) {
        SIZE_T n = products[i] ? strlen(products[i]) : 0;
        if (!n || n > 32) return E_INVALIDARG_;
        length += 4 + n;
    }
    if (strlen(custom) > 4096) return E_INVALIDARG_;
    length += 4 + strlen(custom);
    request = malloc(length);
    ctx = calloc(1, sizeof(*ctx));
    st = calloc(1, sizeof(*st));
    if (!request || !ctx || !st) { free(request); free(ctx); free(st); return E_FAIL_; }
    n32 = (DWORD)count;
    memcpy(request, &n32, 4);
    for (p = request + 4, i = 0; i < count; i++) p = store_put(p, products[i]);
    store_put(p, custom);

    snprintf(name, sizeof(name), "%lx-%lx", (unsigned long)GetCurrentProcessId(),
             (unsigned long)InterlockedIncrement(&store_request_counter));
    snprintf(temp, sizeof(temp), "%s\\%s.tmp", root, name);
    snprintf(ctx->request, sizeof(ctx->request), "%s\\%s.request", root, name);
    snprintf(ctx->response, sizeof(ctx->response), "%s\\%s.response", root, name);
    snprintf(ctx->error, sizeof(ctx->error), "%s\\%s.error", root, name);
    /* Write then rename, so the helper never sees a partial request. */
    hr = gs_file_write(temp, request, (DWORD)length);
    SecureZeroMemory(request, length);
    free(request);
    if (SUCCEEDED(hr) && !MoveFileExA(temp, ctx->request, MOVEFILE_WRITE_THROUGH))
        hr = HRESULT_FROM_WIN32(GetLastError());
    if (FAILED(hr)) {
        DeleteFileA(temp);
        free(ctx);
        free(st);
        return hr;
    }
    st->magic = ASYNC_MAGIC;
    st->block = block;
    st->context = ctx;
    st->provider = store_token_provider;
    st->retain_result = 1;
    snprintf(st->name, sizeof(st->name), "XStoreLicenseToken");
    state_bind(block, st);
    hr = schedule_async(block, 0);
    if (FAILED(hr)) cleanup_state(st);
    return hr;
}
static HRESULT WINAPI store_token_size(void *self, XAsyncBlock *block, SIZE_T *size)
{
    async_state *st;
    HRESULT hr;
    (void)self;
    if (!size) return E_INVALIDARG_;
    hr = retained_result(block, store_token_provider, &st);
    if (FAILED(hr)) return hr;
    *size = st->required;
    return S_OK;
}
static HRESULT WINAPI store_token_result(void *self, XAsyncBlock *block, SIZE_T capacity, char *result)
{
    async_state *st;
    store_token_work *ctx;
    HRESULT hr;
    (void)self;
    hr = retained_result(block, store_token_provider, &st);
    if (FAILED(hr)) return hr;
    ctx = st->context;
    if (!ctx->token) return E_FAIL_;
    if (!result || capacity < st->required) return E_INSUFFICIENT_;
    memcpy(result, ctx->token, st->required);
    consume_async(st);
    return S_OK;
}

static HRESULT WINAPI store_qi(com_obj *self, const GUID *iid, void **out)
{
    const GUID *ok[] = { &IID_Store0, &IID_Store1, &IID_Store2, &IID_Store3, &IID_Store4, &IID_Store5 };
    return gen_qi(self, iid, out, ok, 6);
}
static void *store_vtbl[] = {
    store_qi, gen_addref, gen_release,
    store_create_context, store_close_context,
    stub_notimpl, stub_notimpl,   /* QueryAssociatedProducts Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryProducts Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryEntitledProducts Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryProductForCurrentGame Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryProductForPackage Async, Result */
    stub_notimpl, stub_false,     /* EnumerateProductsQuery, ProductsQueryHasMorePages */
    stub_notimpl, stub_notimpl,   /* ProductsQueryNextPage Async, Result */
    stub_false,                   /* CloseProductsQueryHandle */
    stub_notimpl, stub_notimpl,   /* AcquireLicenseForPackage Async, Result */
    stub_false, stub_false,       /* IsLicenseValid, CloseLicenseHandle */
    stub_notimpl, stub_notimpl,   /* CanAcquireLicenseForStoreId Async, Result */
    stub_notimpl, stub_notimpl,   /* CanAcquireLicenseForPackage Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryGameLicense Async, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* QueryAddOnLicenses Async, ResultCount, Result */
    stub_notimpl, stub_notimpl,   /* QueryConsumableBalanceRemaining Async, Result */
    stub_notimpl, stub_notimpl,   /* ReportConsumableFulfillment Async, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* GetUserCollectionsId Async, ResultSize, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* GetUserPurchaseId Async, ResultSize, Result */
    store_token_async, store_token_size, store_token_result, /* QueryLicenseToken */
    stub_notimpl, stub_notimpl, stub_notimpl, /* padding */
    stub_notimpl, stub_notimpl,   /* ShowPurchaseUI Async, Result */
    stub_notimpl, stub_notimpl,   /* ShowRateAndReviewUI Async, Result */
    stub_notimpl, stub_notimpl,   /* ShowRedeemTokenUI Async, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* QueryGameAndDlcPackageUpdates Async, ResultCount, Result */
    stub_notimpl, stub_notimpl,   /* DownloadPackageUpdates Async, Result */
    stub_notimpl, stub_notimpl,   /* DownloadAndInstallPackageUpdates Async, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* DownloadAndInstallPackages Async, ResultCount, Result */
    stub_notimpl,                 /* QueryPackageIdentifier */
    stub_notimpl, stub_false,     /* Register/UnregisterGameLicenseChanged */
    stub_notimpl, stub_false,     /* Register/UnregisterPackageLicenseLost */
    stub_false,                   /* IsAvailabilityPurchasable */
    stub_notimpl, stub_notimpl,   /* AcquireLicenseForDurables Async, Result */
    stub_notimpl, stub_notimpl,   /* ShowAssociatedProductsUI Async, Result */
    stub_notimpl, stub_notimpl,   /* ShowProductPageUI Async, Result */
    stub_notimpl, stub_notimpl,   /* QueryAssociatedProductsForStoreId Async, Result */
    stub_notimpl, stub_notimpl, stub_notimpl, /* QueryPackageUpdates Async, ResultCount, Result */
    stub_notimpl, stub_notimpl,   /* ShowGiftingUI Async, Result */
};
static com_obj store_obj = { store_vtbl };
