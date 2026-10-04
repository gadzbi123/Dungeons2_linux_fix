/* Experimental local XGameSaveFiles backend for the purchased Store build.
 * This does not synchronize cloud saves or change account authentication.
 * Interface order follows xodus-gaming/xgameruntime's xgamesave.idl.
 */
G(IID_Save, 0x704c3f58, 0xe629, 0x4cc2, 0xb1,0x97,0x30,0x51,0x1b,0x99,0x6f,0xe2)
G(IID_Save2,0x704c3f58, 0xe629, 0x4cc2, 0xb1,0x97,0x30,0x51,0x1b,0x99,0x6e,0xe2)
G(IID_Save3,0x1bfff3af, 0xf14a, 0x40a3, 0x8e,0x35,0x9a,0xda,0x90,0x65,0x93,0xf9)

static HRESULT save_directory(const char *scid, char out[MAX_PATH])
{
    char base[MAX_PATH] = "C:\\users\\steamuser\\AppData\\Local\\Dungeons2";
    char safe[81];
    SIZE_T i, len;
    if (!scid || !*scid || !auth_read_file() || !g_xuid) return E_INVALIDARG_;
    len = strlen(scid);
    if (len >= sizeof(safe)) return E_INVALIDARG_;
    for (i = 0; i < len; i++) {
        char c = scid[i];
        safe[i] = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '-' || c == '_') ? c : '_';
    }
    safe[len] = 0;
    if (!CreateDirectoryA(base, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    strcat(base, "\\XGameSaveLocal");
    if (!CreateDirectoryA(base, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    snprintf(out, MAX_PATH, "%s\\%llu", base, g_xuid);
    if (!CreateDirectoryA(out, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    strcpy(base, out);
    snprintf(out, MAX_PATH, "%s\\%s", base, safe);
    if (!CreateDirectoryA(out, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return HRESULT_FROM_WIN32(GetLastError());
    return S_OK;
}

static HRESULT WINAPI save_folder_provider(UINT32 op, const XAsyncProviderData *data)
{
    const char *path = data->context;
    SIZE_T size;
    if (op == OP_CLEANUP) { free(data->context); return S_OK; }
    if (!path) return E_INVALIDARG_;
    size = strlen(path) + 1;
    if (op == OP_DOWORK) { complete_async(data->async, S_OK, size); return S_OK; }
    if (op == OP_GETRESULT) {
        if (!data->buffer || data->bufferSize < size) return E_INSUFFICIENT_;
        memcpy(data->buffer, path, size);
    }
    return S_OK;
}

static HRESULT WINAPI save_folder_async(void *self, void *user, const char *scid, XAsyncBlock *async)
{
    char path[MAX_PATH];
    HRESULT hr;
    async_state *st;
    (void)self;
    xlog("XGameSaveFilesGetFolderWithUiAsync local-only");
    if (!user || !async) return E_INVALIDARG_;
    hr = save_directory(scid, path);
    if (FAILED(hr)) return hr;
    st = calloc(1, sizeof(*st));
    if (!st) return E_FAIL_;
    st->context = strdup(path);
    if (!st->context) { free(st); return E_FAIL_; }
    st->magic = ASYNC_MAGIC;
    st->block = async;
    st->provider = save_folder_provider;
    snprintf(st->name, sizeof(st->name), "XGameSaveFilesFolder");
    state_bind(async, st);
    return schedule_async(async, 0);
}

static HRESULT WINAPI save_folder_result(void *self, XAsyncBlock *async, SIZE_T cap, char *path)
{
    async_state *st = state_of(async);
    SIZE_T n;
    (void)self;
    xlog("XGameSaveFilesGetFolderWithUiResult");
    if (!st) return E_INVALIDARG_;
    if (!st->complete) return E_PENDING_;
    if (FAILED(st->result)) return st->result;
    if (!st->context) return E_FAIL_;
    n = strlen(st->context) + 1;
    if (!path || cap < n) return E_INSUFFICIENT_;
    memcpy(path, st->context, n);
    return S_OK;
}

static HRESULT WINAPI save_files_quota(void *self, void *user, const char *scid, INT64 *quota)
{
    char path[MAX_PATH];
    ULARGE_INTEGER free_bytes;
    HRESULT hr;
    (void)self;
    if (!user || !quota) return E_POINTER_;
    hr = save_directory(scid, path);
    if (FAILED(hr)) return hr;
    if (!GetDiskFreeSpaceExA(path, &free_bytes, NULL, NULL)) return HRESULT_FROM_WIN32(GetLastError());
    *quota = free_bytes.QuadPart < (256ull << 20) ? free_bytes.QuadPart : (256ull << 20);
    return S_OK;
}

#include "xgamesave-blobs-local.h"

static HRESULT WINAPI save_qi(com_obj *self, const GUID *iid, void **out)
{
    const GUID *ok[] = { &IID_Save, &IID_Save2, &IID_Save3 };
    return gen_qi(self, iid, out, ok, 3);
}
static void *save_vtbl[] = {
    save_qi, gen_addref, gen_release,
    save_init, save_init_async, save_init_result, save_close_provider,
    save_quota, save_quota_async, save_quota_result,
    save_delete_container, save_delete_async, save_delete_result,
    save_container_info, save_enumerate_containers, save_enumerate_containers_by_name,
    save_create_container, save_close_container,
    save_enumerate_blobs, save_enumerate_blobs_by_name,
    save_read, save_read_async, save_read_result,
    save_create_update, save_close_update, save_write, save_delete_blob,
    save_submit, save_submit_async, save_submit_result,
    save_folder_async, save_folder_result, save_files_quota
};
static com_obj save_obj = { save_vtbl };
