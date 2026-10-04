/* Store interface with real signed license-token retrieval through Xodus. */
G(IID_Store0, 0x0dd112ac, 0x7c24, 0x448c, 0xb9, 0x2b, 0x39, 0x60, 0xfb, 0x5b, 0xd3, 0x0c)
G(IID_Store1, 0x60b09f4e, 0x1b85, 0x45b1, 0x82, 0x6c, 0x16, 0x91, 0x18, 0xe2, 0x30, 0xe1)
G(IID_Store2, 0x2d42fea5, 0xe71d, 0x4b76, 0x97, 0xcd, 0xc5, 0x0a, 0xfb, 0xb3, 0xae, 0x5d)
G(IID_Store3, 0xde3dbdd4, 0x0b37, 0x4bdb, 0xa1, 0x0e, 0xac, 0xf3, 0xa3, 0x54, 0xd0, 0x6a)
G(IID_Store4, 0x5c48dedf, 0x0b67, 0x4492, 0xa4, 0xb5, 0x68, 0x29, 0xb8, 0xe7, 0x96, 0xe1)
G(IID_Store5, 0xb09d803c, 0x2414, 0x4a05, 0x82, 0xc6, 0x66, 0xdf, 0xdc, 0x9e, 0x9a, 0x44)
typedef struct store_context_local { UINT64 xuid; } store_context_local;
static HRESULT WINAPI store_XStoreCreateContext(void *self,void *user,void **out) {
    store_context_local *ctx; (void)self; xlog("XStoreCreateContext: authenticated account context");
    if(!out || (user && user!=&user_obj) || !auth_read_file() || !g_xuid) return E_INVALIDARG_;
    *out=NULL; ctx=calloc(1,sizeof(*ctx)); if(!ctx) return E_FAIL_; ctx->xuid=g_xuid; *out=ctx; return S_OK;
}
static void WINAPI store_XStoreCloseContextHandle(void *self,void *ctx) { (void)self; free(ctx); }
static HRESULT WINAPI store_XStoreQueryAssociatedProductsAsync(void *self) { (void)self; xlog("XStoreQueryAssociatedProductsAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAssociatedProductsResult(void *self) { (void)self; xlog("XStoreQueryAssociatedProductsResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductsAsync(void *self) { (void)self; xlog("XStoreQueryProductsAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductsResult(void *self) { (void)self; xlog("XStoreQueryProductsResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryEntitledProductsAsync(void *self) { (void)self; xlog("XStoreQueryEntitledProductsAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryEntitledProductsResult(void *self) { (void)self; xlog("XStoreQueryEntitledProductsResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductForCurrentGameAsync(void *self) { (void)self; xlog("XStoreQueryProductForCurrentGameAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductForCurrentGameResult(void *self) { (void)self; xlog("XStoreQueryProductForCurrentGameResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductForPackageAsync(void *self) { (void)self; xlog("XStoreQueryProductForPackageAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryProductForPackageResult(void *self) { (void)self; xlog("XStoreQueryProductForPackageResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreEnumerateProductsQuery(void *self) { (void)self; xlog("XStoreEnumerateProductsQuery unavailable"); return E_NOTIMPL_; }
static BOOLEAN WINAPI store_XStoreProductsQueryHasMorePages(void *self) { (void)self; xlog("XStoreProductsQueryHasMorePages unavailable"); return FALSE; }
static HRESULT WINAPI store_XStoreProductsQueryNextPageAsync(void *self) { (void)self; xlog("XStoreProductsQueryNextPageAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreProductsQueryNextPageResult(void *self) { (void)self; xlog("XStoreProductsQueryNextPageResult unavailable"); return E_NOTIMPL_; }
static void WINAPI store_XStoreCloseProductsQueryHandle(void *self) { (void)self; xlog("XStoreCloseProductsQueryHandle unavailable"); return ; }
static HRESULT WINAPI store_XStoreAcquireLicenseForPackageAsync(void *self) { (void)self; xlog("XStoreAcquireLicenseForPackageAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreAcquireLicenseForPackageResult(void *self) { (void)self; xlog("XStoreAcquireLicenseForPackageResult unavailable"); return E_NOTIMPL_; }
static BOOLEAN WINAPI store_XStoreIsLicenseValid(void *self) { (void)self; xlog("XStoreIsLicenseValid unavailable"); return FALSE; }
static void WINAPI store_XStoreCloseLicenseHandle(void *self) { (void)self; xlog("XStoreCloseLicenseHandle unavailable"); return ; }
static HRESULT WINAPI store_XStoreCanAcquireLicenseForStoreIdAsync(void *self) { (void)self; xlog("XStoreCanAcquireLicenseForStoreIdAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreCanAcquireLicenseForStoreIdResult(void *self) { (void)self; xlog("XStoreCanAcquireLicenseForStoreIdResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreCanAcquireLicenseForPackageAsync(void *self) { (void)self; xlog("XStoreCanAcquireLicenseForPackageAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreCanAcquireLicenseForPackageResult(void *self) { (void)self; xlog("XStoreCanAcquireLicenseForPackageResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryGameLicenseAsync(void *self) { (void)self; xlog("XStoreQueryGameLicenseAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryGameLicenseResult(void *self) { (void)self; xlog("XStoreQueryGameLicenseResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAddOnLicensesAsync(void *self) { (void)self; xlog("XStoreQueryAddOnLicensesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAddOnLicensesResultCount(void *self) { (void)self; xlog("XStoreQueryAddOnLicensesResultCount unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAddOnLicensesResult(void *self) { (void)self; xlog("XStoreQueryAddOnLicensesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryConsumableBalanceRemainingAsync(void *self) { (void)self; xlog("XStoreQueryConsumableBalanceRemainingAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryConsumableBalanceRemainingResult(void *self) { (void)self; xlog("XStoreQueryConsumableBalanceRemainingResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreReportConsumableFulfillmentAsync(void *self) { (void)self; xlog("XStoreReportConsumableFulfillmentAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreReportConsumableFulfillmentResult(void *self) { (void)self; xlog("XStoreReportConsumableFulfillmentResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserCollectionsIdAsync(void *self) { (void)self; xlog("XStoreGetUserCollectionsIdAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserCollectionsIdResultSize(void *self) { (void)self; xlog("XStoreGetUserCollectionsIdResultSize unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserCollectionsIdResult(void *self) { (void)self; xlog("XStoreGetUserCollectionsIdResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserPurchaseIdAsync(void *self) { (void)self; xlog("XStoreGetUserPurchaseIdAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserPurchaseIdResultSize(void *self) { (void)self; xlog("XStoreGetUserPurchaseIdResultSize unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreGetUserPurchaseIdResult(void *self) { (void)self; xlog("XStoreGetUserPurchaseIdResult unavailable"); return E_NOTIMPL_; }



static HRESULT WINAPI store___PADDING__(void *self) { (void)self; xlog("__PADDING__ unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store___PADDING_2__(void *self) { (void)self; xlog("__PADDING_2__ unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store___PADDING_3__(void *self) { (void)self; xlog("__PADDING_3__ unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowPurchaseUIAsync(void *self) { (void)self; xlog("XStoreShowPurchaseUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowPurchaseUIResult(void *self) { (void)self; xlog("XStoreShowPurchaseUIResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowRateAndReviewUIAsync(void *self) { (void)self; xlog("XStoreShowRateAndReviewUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowRateAndReviewUIResult(void *self) { (void)self; xlog("XStoreShowRateAndReviewUIResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowRedeemTokenUIAsync(void *self) { (void)self; xlog("XStoreShowRedeemTokenUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowRedeemTokenUIResult(void *self) { (void)self; xlog("XStoreShowRedeemTokenUIResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryGameAndDlcPackageUpdatesAsync(void *self) { (void)self; xlog("XStoreQueryGameAndDlcPackageUpdatesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryGameAndDlcPackageUpdatesResultCount(void *self) { (void)self; xlog("XStoreQueryGameAndDlcPackageUpdatesResultCount unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryGameAndDlcPackageUpdatesResult(void *self) { (void)self; xlog("XStoreQueryGameAndDlcPackageUpdatesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadPackageUpdatesAsync(void *self) { (void)self; xlog("XStoreDownloadPackageUpdatesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadPackageUpdatesResult(void *self) { (void)self; xlog("XStoreDownloadPackageUpdatesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadAndInstallPackageUpdatesAsync(void *self) { (void)self; xlog("XStoreDownloadAndInstallPackageUpdatesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadAndInstallPackageUpdatesResult(void *self) { (void)self; xlog("XStoreDownloadAndInstallPackageUpdatesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadAndInstallPackagesAsync(void *self) { (void)self; xlog("XStoreDownloadAndInstallPackagesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadAndInstallPackagesResultCount(void *self) { (void)self; xlog("XStoreDownloadAndInstallPackagesResultCount unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreDownloadAndInstallPackagesResult(void *self) { (void)self; xlog("XStoreDownloadAndInstallPackagesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryPackageIdentifier(void *self) { (void)self; xlog("XStoreQueryPackageIdentifier unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreRegisterGameLicenseChanged(void *self) { (void)self; xlog("XStoreRegisterGameLicenseChanged unavailable"); return E_NOTIMPL_; }
static BOOLEAN WINAPI store_XStoreUnregisterGameLicenseChanged(void *self) { (void)self; xlog("XStoreUnregisterGameLicenseChanged unavailable"); return FALSE; }
static HRESULT WINAPI store_XStoreRegisterPackageLicenseLost(void *self) { (void)self; xlog("XStoreRegisterPackageLicenseLost unavailable"); return E_NOTIMPL_; }
static BOOLEAN WINAPI store_XStoreUnregisterPackageLicenseLost(void *self) { (void)self; xlog("XStoreUnregisterPackageLicenseLost unavailable"); return FALSE; }
static BOOLEAN WINAPI store_XStoreIsAvailabilityPurchasable(void *self) { (void)self; xlog("XStoreIsAvailabilityPurchasable unavailable"); return FALSE; }
static HRESULT WINAPI store_XStoreAcquireLicenseForDurablesAsync(void *self) { (void)self; xlog("XStoreAcquireLicenseForDurablesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreAcquireLicenseForDurablesResult(void *self) { (void)self; xlog("XStoreAcquireLicenseForDurablesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowAssociatedProductsUIAsync(void *self) { (void)self; xlog("XStoreShowAssociatedProductsUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowAssociatedProductsUIResult(void *self) { (void)self; xlog("XStoreShowAssociatedProductsUIResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowProductPageUIAsync(void *self) { (void)self; xlog("XStoreShowProductPageUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowProductPageUIResult(void *self) { (void)self; xlog("XStoreShowProductPageUIResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAssociatedProductsForStoreIdAsync(void *self) { (void)self; xlog("XStoreQueryAssociatedProductsForStoreIdAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryAssociatedProductsForStoreIdResult(void *self) { (void)self; xlog("XStoreQueryAssociatedProductsForStoreIdResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryPackageUpdatesAsync(void *self) { (void)self; xlog("XStoreQueryPackageUpdatesAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryPackageUpdatesResultCount(void *self) { (void)self; xlog("XStoreQueryPackageUpdatesResultCount unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreQueryPackageUpdatesResult(void *self) { (void)self; xlog("XStoreQueryPackageUpdatesResult unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowGiftingUIAsync(void *self) { (void)self; xlog("XStoreShowGiftingUIAsync unavailable"); return E_NOTIMPL_; }
static HRESULT WINAPI store_XStoreShowGiftingUIResult(void *self) { (void)self; xlog("XStoreShowGiftingUIResult unavailable"); return E_NOTIMPL_; }
#include "xstore-license-token.h"
static HRESULT WINAPI store_qi(com_obj *self,const GUID *iid,void **out) {
    const GUID *known[] = { &IID_Store0, &IID_Store1, &IID_Store2, &IID_Store3, &IID_Store4, &IID_Store5 };
    return gen_qi(self,iid,out,known,sizeof(known)/sizeof(known[0]));
}
static void *store_vtbl[] = { store_qi, gen_addref, gen_release,
    store_XStoreCreateContext,
    store_XStoreCloseContextHandle,
    store_XStoreQueryAssociatedProductsAsync,
    store_XStoreQueryAssociatedProductsResult,
    store_XStoreQueryProductsAsync,
    store_XStoreQueryProductsResult,
    store_XStoreQueryEntitledProductsAsync,
    store_XStoreQueryEntitledProductsResult,
    store_XStoreQueryProductForCurrentGameAsync,
    store_XStoreQueryProductForCurrentGameResult,
    store_XStoreQueryProductForPackageAsync,
    store_XStoreQueryProductForPackageResult,
    store_XStoreEnumerateProductsQuery,
    store_XStoreProductsQueryHasMorePages,
    store_XStoreProductsQueryNextPageAsync,
    store_XStoreProductsQueryNextPageResult,
    store_XStoreCloseProductsQueryHandle,
    store_XStoreAcquireLicenseForPackageAsync,
    store_XStoreAcquireLicenseForPackageResult,
    store_XStoreIsLicenseValid,
    store_XStoreCloseLicenseHandle,
    store_XStoreCanAcquireLicenseForStoreIdAsync,
    store_XStoreCanAcquireLicenseForStoreIdResult,
    store_XStoreCanAcquireLicenseForPackageAsync,
    store_XStoreCanAcquireLicenseForPackageResult,
    store_XStoreQueryGameLicenseAsync,
    store_XStoreQueryGameLicenseResult,
    store_XStoreQueryAddOnLicensesAsync,
    store_XStoreQueryAddOnLicensesResultCount,
    store_XStoreQueryAddOnLicensesResult,
    store_XStoreQueryConsumableBalanceRemainingAsync,
    store_XStoreQueryConsumableBalanceRemainingResult,
    store_XStoreReportConsumableFulfillmentAsync,
    store_XStoreReportConsumableFulfillmentResult,
    store_XStoreGetUserCollectionsIdAsync,
    store_XStoreGetUserCollectionsIdResultSize,
    store_XStoreGetUserCollectionsIdResult,
    store_XStoreGetUserPurchaseIdAsync,
    store_XStoreGetUserPurchaseIdResultSize,
    store_XStoreGetUserPurchaseIdResult,
    store_XStoreQueryLicenseTokenAsync,
    store_XStoreQueryLicenseTokenResultSize,
    store_XStoreQueryLicenseTokenResult,
    store___PADDING__,
    store___PADDING_2__,
    store___PADDING_3__,
    store_XStoreShowPurchaseUIAsync,
    store_XStoreShowPurchaseUIResult,
    store_XStoreShowRateAndReviewUIAsync,
    store_XStoreShowRateAndReviewUIResult,
    store_XStoreShowRedeemTokenUIAsync,
    store_XStoreShowRedeemTokenUIResult,
    store_XStoreQueryGameAndDlcPackageUpdatesAsync,
    store_XStoreQueryGameAndDlcPackageUpdatesResultCount,
    store_XStoreQueryGameAndDlcPackageUpdatesResult,
    store_XStoreDownloadPackageUpdatesAsync,
    store_XStoreDownloadPackageUpdatesResult,
    store_XStoreDownloadAndInstallPackageUpdatesAsync,
    store_XStoreDownloadAndInstallPackageUpdatesResult,
    store_XStoreDownloadAndInstallPackagesAsync,
    store_XStoreDownloadAndInstallPackagesResultCount,
    store_XStoreDownloadAndInstallPackagesResult,
    store_XStoreQueryPackageIdentifier,
    store_XStoreRegisterGameLicenseChanged,
    store_XStoreUnregisterGameLicenseChanged,
    store_XStoreRegisterPackageLicenseLost,
    store_XStoreUnregisterPackageLicenseLost,
    store_XStoreIsAvailabilityPurchasable,
    store_XStoreAcquireLicenseForDurablesAsync,
    store_XStoreAcquireLicenseForDurablesResult,
    store_XStoreShowAssociatedProductsUIAsync,
    store_XStoreShowAssociatedProductsUIResult,
    store_XStoreShowProductPageUIAsync,
    store_XStoreShowProductPageUIResult,
    store_XStoreQueryAssociatedProductsForStoreIdAsync,
    store_XStoreQueryAssociatedProductsForStoreIdResult,
    store_XStoreQueryPackageUpdatesAsync,
    store_XStoreQueryPackageUpdatesResultCount,
    store_XStoreQueryPackageUpdatesResult,
    store_XStoreShowGiftingUIAsync,
    store_XStoreShowGiftingUIResult,
};
static com_obj store_obj = { store_vtbl };
