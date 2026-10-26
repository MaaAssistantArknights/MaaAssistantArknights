---
icon: mdi:certificate-outline
order: 8
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 簽章範圍與發佈流程

Windows 正式版、測試版和內測版的簽章由 GitHub Actions 提交至 SignPath；正式憑證尚未就緒、審批未通過或簽章驗證失敗時，發佈流程停止。歷史版本可能未簽章，請以檔案的數位簽章內容為準。

簽章僅涵蓋從本專案原始碼及固定版本的 MaaUtils 子模組建置的下列檔案：

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

第三方相依項目保留上游提供的檔案及簽章。簽章使用相同的產品版本，並驗證檔名及產品名稱。私密金鑰保留於 SignPath 的 HSM，Windows 顯示的簽章發行者為 SignPath Foundation。

正式發佈使用 `.github/workflows/ci.yml`，內測發佈使用 `.github/workflows/release-nightly-ota.yml`，皆於 GitHub 託管執行器建置，並於封裝前完成簽章驗證。每次發佈簽章都需要核准者手動審批，包含內測版。

`test-signing` 使用不受使用者系統信任的測試憑證，僅供手動觸發且不發佈的 CI 驗證，不能替代發佈憑證。

## 團隊職責

- 儲存庫權限群組：[Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev)。
- 提交者與審閱者：具有本儲存庫寫入權限的 MAA 維護者；其他貢獻者的變更經維護者審閱後合併。
- 簽章核准者：[uye / ABA2396](https://github.com/ABA2396)。
- 簽章請求由專用的 `CI builds` 使用者提交。

維護者必須為 SignPath 與 GitHub 帳號啟用多因素驗證。

## 隱私

詳見[使用者協議的隱私原則及資料上傳說明](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)。MAA 預設啟用向企鵝物流資料統計及明日方舟一圖流提交匿名資料的功能，使用者可以關閉上報；填寫企鵝物流回報 ID 後，上報會包含該識別碼。
