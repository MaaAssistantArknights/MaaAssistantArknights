# Deprecated

# This file MUST use GBK encoding, whatever the system language is.
# You may do more testing, but at least it works like this for me.
# Fuck Microsoft.
# �޸���ʹ�� GBK ���뱣�档
# ������� UTF-8 ����������룬�Ǿ�ȥ������ʵ�����о�������΢����ʺ�ˡ�
# ��������һ�� [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 ��������⡣
# ��������ģ�����������ã��������������ĺ�����Ⱦ�ص���һ��
# ���� CMD ���涼�� chcp 65001 ���� UTF-8��
# ΢��ʲôʱ���ܰ� PowerShell 7 ���ɵ�ϵͳ�
# �����Ե���Ϊ Windows ��Ԥװ�� PowerShell ����һ��ʺ���������� CMD��
# �� PowerShell д�ⶫ�����Ϊ��һ�̴װ��Ľ����ˡ�

# ��� PowerShell �ű����в���
# �������ⲻ��������в��ԵĴ��������ˣ������нű����в��ԣ�ȴ�����ýű��޸ġ�
# �Ҽȿ����ڲ���Ȩ��������������нű����У�Ҳ����ֱ������һ���������нű����еĻỰ��
# ���ű��ڲ���ʱ���֣��ڲ��޸Ĳ����Ҳ���Ȩ�������Ҳ�����������У�ֻ���ڰ�װ���п�ʱ��Ҫ�㰲װ����� UAC ������
# �����޸Ĳ�������Ȩʱ������Ϊ�ű��������в����ˣ�����ʲôƷ�ֵĹ���ԱȨ�ޡ�
# $ExecutionPolicy = Get-ExecutionPolicy
# if ($ExecutionPolicy -eq 'Restricted' -or $ExecutionPolicy -eq 'AllSigned') {
#     Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
#     Write-Host "���޸ĵ�ǰ�û��� PowerShell �ű����в���Ϊ RemoteSigned �Ա����д˽ű���"
#     Write-Host "��ֻ������һ�Σ�����Ӧ��������ϴ�İ�ȫ���ա�"
#     Write-Host "������Դ������ǣ����ڽű����н������� PowerShell �����������������ֵ��ա�"
#     Write-Host "Set-ExecutionPolicy -ExecutionPolicy Undefined -Scope CurrentUser"
#     Pause
# }

# ��ȡ UAC Ȩ�޲�����һ�� RemoteSigned �ű����ԻỰ
If (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Write-Host "���ڻ�ȡ����ԱȨ��..."
    Start-Process powershell.exe "-ExecutionPolicy RemoteSigned -File `"$PSCommandPath`" -ArgumentList `"$PSCommandPath`"" -Verb RunAs
    Exit
}

# ��ȡϵͳ������Ӧ��
$Internet_Settings = Get-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
if ($Internet_Settings.ProxyEnable -eq 1) {
    $env:HTTP_PROXY = "http://$($Internet_Settings.ProxyServer)"
    $env:HTTPS_PROXY = "http://$($Internet_Settings.ProxyServer)"
    Write-Host "��Ӧ��ϵͳ������$($Internet_Settings.ProxyServer)��"
}
# else {
#     Write-Host "δ����ϵͳ������"
# }
Remove-Variable -Name Internet_Settings

# �½���������ʱ�ļ���
New-Item -Path "$env:TEMP" -Name "MAA_Runtime_Fix_Pwsh" -ItemType "Directory" | Out-Null

# ʹ�� BITS �������пⰲװ��
Write-Host "�����������пⰲװ��..."
# Start-Sleep -Seconds 1
Start-BitsTransfer -Source "https://aka.ms/vs/17/release/vc_redist.x64.exe" -Destination "$env:TEMP\MAA_Runtime_Fix_Pwsh\vc_redist.x64.exe"
Start-BitsTransfer -Source "https://builds.dotnet.microsoft.com/dotnet/WindowsDesktop/8.0.11/windowsdesktop-runtime-8.0.11-win-x64.exe" -Destination "$env:TEMP\MAA_Runtime_Fix_Pwsh\windowsdesktop-runtime-8.0.11-win-x64.exe"

# ж�� vc++ �� dotnet8
Write-Host ""
Write-Host "���Ե��� WinGet ж���Ѱ�װ�����п�..."
winget uninstall "Microsoft.VCRedist.2015+.x64" "Microsoft.DotNet.DesktopRuntime.8" --force --all-versions

# ��װ vc++
Write-Host ""
Write-Host "���ڰ�װ/�޸� Microsoft Visual C++ ���ٷ��г����..."
$vcProcess = Start-Process "$env:TEMP\MAA_Runtime_Fix_Pwsh\vc_redist.x64.exe" -ArgumentList '/repair', '/passive', '/norestart' -PassThru
$vcProcess.WaitForExit()

# ��װ dotnet8
Write-Host "���ڰ�װ/�޸� .NET ��������ʱ 8..."
$dotnetProcess = Start-Process "$env:TEMP\MAA_Runtime_Fix_Pwsh\windowsdesktop-runtime-8.0.11-win-x64.exe" -ArgumentList '/repair', '/passive', '/norestart' -PassThru
$dotnetProcess.WaitForExit()

# ɾ����ʱ�ļ���
Write-Host ""
Write-Host "����������ʱ�ļ�..."
Remove-Item -Path "$env:TEMP\MAA_Runtime_Fix_Pwsh" -Recurse -Force

Write-Host "���п��޸���ɣ����ٴγ������� MAA��"
Write-Host ""
Pause

# ʹ�� winget ��װ/�������пⲢж�ؾɰ汾
# winget install "Microsoft.VCRedist.2015+.x64" "Microsoft.DotNet.DesktopRuntime.8" --uninstall-previous --accept-package-agreements

# ʹ�� winget �������пⰲװ��
# winget download --id "Microsoft.VCRedist.2015+.x64" -d "$env:TEMP\MAA_Runtime_Fix_Pwsh\"
# winget download --id "Microsoft.DotNet.DesktopRuntime.8" -d "$env:TEMP\MAA_Runtime_Fix_Pwsh\"

# ��ʹ�� winget ж�� vc++
# �����ж�� ����ж����
# Get-Package -Name "Microsoft Visual C++ 2015-2022 Redistributable (x64) - *" | Uninstall-Package -Force

# ��ʹ�� winget ж�� dotnet8
# �����ж�� ����ж����
# Get-Package -Name "Microsoft Windows Desktop Runtime - 8.* (x64)" | Uninstall-Package -Force

# ���ذ�װ������ע����һ��˯һ������Ϊ���������ļ����Ǹ����������������������������ݶ����ǵ���������ֻ�ܿ��������ؽ����������ɶҲ��������
# ������ȥ���½��ļ���ʱ�����һ���綫��֮�����о���¶�����ˡ�
# �ҽ���ÿ���˶�ȥ��һ�� PowerShell �� New-Item �½��ļ�����������һ�����⡣
# ���� PowerShell ����һ���Ľ�������ʾ���ƣ��ǵø���̶���͸��һ����ĸ�����ȥ��ʲô���Ͳ��ܺͱ���һ��������ײ���ʾ��

# ���� Pwsh д�ű��Ҿ���ɵ�ơ�
