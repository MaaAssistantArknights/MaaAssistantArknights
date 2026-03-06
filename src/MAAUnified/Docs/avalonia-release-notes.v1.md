# MAAUnified 发布说明 v1

## 发布范围
- Release date (UTC): `2026-03-06`
- Evidence generated at (UTC): `2026-03-06T00:00:00Z`
- Commit: `e40709a149df0223154eb1e2c99f5aba1d16d95c`
- Work package scope: `Q1 + Q2 + Q3`
- Gate workflow: `.github/workflows/ci-avalonia.yml`

## 证据生成记录
| executed_at_utc | command | result |
| --- | --- | --- |
| `2026-03-06T00:00:00Z` | `dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-restore --disable-build-servers -m:1 --filter "FullyQualifiedName~PlatformCapabilityContractTests|FullyQualifiedName~ConfigurationImportTests|FullyQualifiedName~SessionStateSyncTests|FullyQualifiedName~BaselineContractTests|FullyQualifiedName~BaselineCoverageTests|FullyQualifiedName~BaselineRenderSyncTests|FullyQualifiedName~WorkPackageQ1BlockingCoverageTests"` | `Passed 65, Failed 0` |
| `2026-03-06T00:00:00Z` | `dotnet test src/MAAUnified/Tests/MAAUnified.Tests.csproj -c Release --no-restore --disable-build-servers -m:1 --filter "FullyQualifiedName~BaselineContractTests|FullyQualifiedName~BaselineCoverageTests|FullyQualifiedName~BaselineRenderSyncTests|FullyQualifiedName~ParityMatrixSyncTests|FullyQualifiedName~BaselineNoWaiverRemainingTests"` | `Passed 12, Failed 0` |
| `2026-03-06T00:00:00Z` | `rg -n "continue-on-error|non-blocking observation|Work Package K gate tests" .github/workflows/ci-avalonia.yml src/MAAUnified/CI/ci-avalonia.yml || true` | `No banned markers in ci-avalonia workflows` |

## Q1/Q2 Completed Evidence
- Q1/Q2 blocking coverage + baseline triple-check chain is green.
- Linux/Windows/macOS responsibilities are fixed in `ci-avalonia.yml` and template mirror.
- Gate policy remains blocking-only in target workflow files.

## 构建产物清单
- Windows artifact: `MAAUnified-${tag}-windows-x64.zip`
- Linux artifact: `MAAUnified-${tag}-linux-x64.tar.gz`
- macOS artifact: `MAAUnified-${tag}-macos-x64.tar.gz`
- Artifact source: `release/*` uploaded by `ci-avalonia.yml`

### 产物验证点
- Windows package contains `MaaCore.dll` and `resource/`
- Linux package contains `libMaaCore.so` and `resource/`
- macOS package contains `libMaaCore.dylib` and `resource/`
- All packages include published `MAAUnified.App` output

## 配置兼容清单
- Config write target remains `config/avalonia.json`
- Auto import trigger remains: import only when `avalonia.json` does not exist
- Import order remains: `gui.new.json -> gui.json -> defaults`
- Manual import source options remain: `Auto / GuiNewOnly / GuiOnly`
- Legacy config files remain read-only and never overwritten
- Blocking validation remains enforced before start/append task paths

## fallback 验证清单
- Capabilities under verification: `Tray / Notification / Hotkey / Autostart / Overlay`
- Platforms under verification: `windows / macos / linux`
- Required behavior for every fallback path:
  - user-visible feedback exists
  - platform/UI log is written
  - scope/case id is locatable
  - process stays alive

### fallback 验证记录入口
- Baseline fallback table: `Docs/baseline.freeze.v1.md`
- Acceptance matrix: `Docs/acceptance.checklist.template.v1.md`
- Execution record: `Docs/acceptance.execution.v1.md`

## 日志验证清单
- `debug/avalonia-ui-errors.log`
- `debug/avalonia-platform-events.log`
- `debug/config-import-report.json`

### 日志验证要求
- Failure scope is present and searchable
- Error/fallback code is present and searchable
- Log path is referenced by acceptance evidence fields

## Baseline/Acceptance Sync Audit
- `Docs/baseline.freeze.v1.md`: no content change in this Q3 closeout.
- `Docs/acceptance.checklist.template.v1.md`: no content change in this Q3 closeout.
- Machine-readable baseline and projected docs remain in sync, validated by `BaselineContractTests`, `BaselineCoverageTests`, and `BaselineRenderSyncTests`.

## 提交回填规则
- `Docs/avalonia-release-notes.v1.md` and `Docs/acceptance.execution.v1.md` must use the same commit hash.
- Commit source is the final release branch `HEAD` after last regression.
- Manual historical commit values are not allowed.

## 发布阻断条件
- Any of `restore/test/publish/package` fails in `ci-avalonia.yml`
- Linux baseline consistency gate fails
- Linux full `MAAUnified.Tests` gate fails
- Windows platform capability contract gate fails
- Windows native capability smoke gate fails
- Release artifact validation fails

## 固定发布清单（并入发布说明）
- [ ] Confirm `ci-avalonia.yml` gate definitions are unchanged from approved Q2 policy
- [ ] Confirm evidence commands, pass counts, and generated time are consistent
- [ ] Confirm `Docs/acceptance.execution.v1.md` covers all 58 ACC cases
- [ ] Confirm fallback evidence entries are present and non-empty
- [ ] Confirm baseline/acceptance docs are sync-audited (changed or explicitly no-drift)
- [ ] Confirm all release artifacts exist and satisfy validation points
- [ ] Confirm rollback runbook is updated and executable
- [ ] Confirm final full regression command passes before merge
