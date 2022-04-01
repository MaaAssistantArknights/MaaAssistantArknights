sh ./update_version.sh
MSBuild.exe ..\MeoAssistantArknights.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
call .\update_resource_rel.bat
pause
