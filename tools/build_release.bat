for /f "delims=" %%i in ('sh ./version.sh') do set ExternalCompilerOptions=/DMAA_VERSION=\"%%i\"
MSBuild.exe ..\MeoAssistantArknights.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
call .\update_resource_rel.bat
pause
