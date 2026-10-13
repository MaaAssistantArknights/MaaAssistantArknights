# Depot maintenance policy checks

These standalone C++20 assertions exercise the production headers without
loading MaaCore, connecting to an emulator, or spending sanity.

Build with a C++20 compiler, with `src/MaaCore` and `src/MaaUtils/include` on
the include path. Keep assertions enabled (do not define `NDEBUG`). On Windows,
use the Visual Studio Developer Command Prompt:

```cmd
cl /std:c++20 /utf-8 /EHsc /W4 /WX /I src\MaaCore /external:I src\MaaUtils\include /external:W0 tools\DepotMaintainTests\policy.cpp /Fe:depot-policy-test.exe
depot-policy-test.exe resource\item_index.json resource\stages.json
```

Coverage: exact quantities, unknown inventory, atomic drop acceptance, stage
identity, route validation, ordered stock targets, strict scan boundaries,
recovery-stop evidence, and bounded diagnostic recipe expansion. An optional
third JSON argument replays archived callbacks; that is not a live game test.
