# Experimental Microsoft Store / Xodus support

This contribution extends the existing runtime for the purchased Microsoft Store
WinGDK build (tested with game 1.1.1.0 on Fedora, NVIDIA GTX 1080, a custom
xodus-gaming Wine 11.0 build). It is a draft, not a stable replacement release.
The existing installer targets Steam and should not be used for the Store prefix.

## Runtime changes

- Local XGameSave provider and blob storage, using immutable generations and
  atomic CURRENT publication. Data is separated by authenticated XUID and SCID
  under `C:\users\steamuser\AppData\Local\Dungeons2\XGameSaveLocal`.
- Retain save, token and Store results until consumed; handle results retrieved
  before or after callback dispatch, including callbacks that release blocks.
- Return UTF-16 token data for the UTF-16 API, including wide terminators.
- Implement Store license-token queries by relaying to Xodus, which obtains an
  actual Microsoft-issued token for the purchased account. No fabricated
  ownership response is returned. Other unimplemented Store operations fail.
- Return the recorded failure from generic XAsyncGetResult before invoking a
  provider's GetResult. This follows Microsoft's libHttpClient implementation
  and addressed the observed cutscene null-pointer crash.
- Add limited HTTP error diagnostics without enabling sensitive body tracing.

## Xodus dependency and bridge

The companion patch in `integration/xodus-linux-launch.patch` applies to Xodus
commit `a3afa0569332e32ce2677c0edc643ef85477ee3e`. It fixes Unix launch handling
and adds `xodus-cli store-token REQUEST OUTPUT`. It belongs in a separate Xodus
contribution; it is included here to make this experimental runtime reviewable.

The runtime expects `XODUS_STORE_BRIDGE_PATH` to identify a private temporary
directory accessible inside Wine, expressed as a Windows path. Before launching:

1. Authenticate Xodus with the account that purchased the Store game, and obtain
   the licensed game files using Xodus. Preserve the streaming metadata.
2. Build patched Xodus with `cargo build --release --locked -p xodus-cli`.
3. Create a mode-0700 temporary bridge directory. Set `XODUS_CLI` to the patched
   binary and run `python3 integration/store-license-bridge.py DIRECTORY`.
4. Export `XODUS_STORE_BRIDGE_PATH` as the Windows path for that directory.
5. Build this runtime, install the DLL into the dedicated prefix's system32
   while the game is stopped, and use the native xgameruntime override.
6. Launch the Store game through Xodus and its compatible Wine/image-mapping
   wrapper. This contribution does not yet provide a portable Store installer
   or the complete launch wrapper; the companion patch alone is insufficient.
7. Stop the helper and remove its private temporary directory after Wine exits.

The binary request protocol is little-endian: uint32 product count, repeated
uint32 UTF-8 byte length plus product bytes, then uint32 custom-string length
plus UTF-8 bytes. The reply is an opaque token, or a generic `.error` file.
Never publish token caches, responses, save data, runtime logs, or dumps.

## Validation and limitations

Verified locally: settings survive restart; Microsoft/PlayFab sign-in succeeds;
the game's Store entitlement request returns HTTP 200; a user replayed the
previously failing cutscene and continued playing after the failed-result guard.

`src/save-storage-check.c` checks binary persistence across processes, transaction
publication, deletion, undersized buffers, deferred result retrieval, UTF-16
tokens and callback block lifetime against a separate test SCID. It uses the
real cached account and must not be run against production save containers.
`src/async-failure-check.c` verifies that a failed operation preserves its error
without calling the result provider. These console checks include the runtime
source and use `check_entry` as their PE entrypoint.

Known unresolved issue: quitting after approximately ten minutes produced an
execute-access violation through the generic async completion callback path.
The user confirmed selecting Quit. The failed-result guard addresses a distinct
cutscene crash, not this callback lifetime/shutdown issue. Legacy generic
providers still use the original completion lifecycle. This needs further work.

Cloud synchronization, multiplayer, in-game purchases and long-session stability
are not verified. Saves are local only. Container/blob name support is a subset
of the full GDK contract (64-byte names in the blob backend). Hardcoded Steam-user
paths and the external bridge also need portability review. The existing MinGW/
Zig build and Steam behavior need maintainer/CI validation; the local build used
Clang/LLD and the custom Wine headers/import libraries.

Reference for failed-result behavior:
https://github.com/microsoft/libHttpClient/blob/main/Source/Task/AsyncLib.cpp
