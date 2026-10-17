// <copyright file="WeComBotAccountEditorViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

#nullable enable
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;

namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// 单个账号的一键长草配置弹窗。
/// 直接读写该账号对应的配置档案，保存后立即生效。
/// </summary>
public class WeComBotAccountEditorViewModel : Screen
{
    private readonly WeComBotAccount _account;
    private readonly string _profileName;
    private readonly SpecificConfig _profile;
    private readonly string _profileStage;
    private readonly string _originalStage;

    /// <summary>
    /// Initializes a new instance of the <see cref="WeComBotAccountEditorViewModel"/> class.
    /// </summary>
    /// <param name="account">要编辑的账号。</param>
    public WeComBotAccountEditorViewModel(WeComBotAccount account)
    {
        _account = account;
        DisplayName = LocalizationHelper.GetString("WeComBotEditAccount");

        // 同一份档案只能服务一个账号，否则两个账号会互相覆盖设置
        var configured = account.Configuration;
        var shared = string.IsNullOrWhiteSpace(configured)
            || ConfigFactory.Root.WeComBot.Accounts.Count(item => item.Configuration == configured) > 1;
        var baseName = !string.IsNullOrWhiteSpace(account.Name)
            ? account.Name.Trim()
            : !string.IsNullOrWhiteSpace(configured) ? configured : LocalizationHelper.GetString("WeComBotDefaultAccountName");
        _profileName = configured ?? string.Empty;
        if (shared)
        {
            _profileName = baseName;
            var suffix = 2;
            while (ConfigFactory.ConfigurationExists(_profileName)
                || ConfigFactory.Root.WeComBot.Accounts.Any(item => item != account && item.Configuration == _profileName))
            {
                _profileName = $"{baseName} {suffix++}";
            }
        }

        if (!ConfigFactory.ConfigurationExists(_profileName))
        {
            // 新档案直接复制当前档案的任务队列，避免得到空队列导致保存时无对象可写
            ConfigFactory.AddConfiguration(_profileName, copyFrom: ConfigFactory.Root.Current);
        }

        WeComBotSettingsUserControlModel.Instance.EnsureProfileListed(_profileName);
        _profile = ConfigFactory.Root.Configurations[_profileName];
        if (_profile.TaskQueue.Count == 0)
        {
            EnsureDefaultTasks(_profile);
        }

        var start = _profile.TaskQueue.OfType<StartUpTask>().FirstOrDefault();
        var fight = _profile.TaskQueue.OfType<FightTask>().FirstOrDefault();
        _profileStage = fight?.StagePlan.FirstOrDefault() ?? string.Empty;
        _originalStage = account.Stage ?? string.Empty;
        AccountSwitchName = start?.AccountName ?? string.Empty;
        AccountSwitchEnabled = start?.AccountSwitchEnabled ?? true;
        Stage = string.IsNullOrWhiteSpace(account.Stage) ? fight?.StagePlan.FirstOrDefault() ?? string.Empty : account.Stage;
        UseMedicine = fight?.UseMedicine ?? false;
        MedicineCount = fight?.MedicineCount ?? 0;
        FightEnabled = IsEnabled<FightTask>();
        InfrastEnabled = IsEnabled<InfrastTask>();
        RecruitEnabled = IsEnabled<RecruitTask>();
        MallEnabled = IsEnabled<MallTask>();
        AwardEnabled = IsEnabled<AwardTask>();
        RoguelikeEnabled = IsEnabled<RoguelikeTask>();
        ReclamationEnabled = IsEnabled<ReclamationTask>();
        UserDataUpdateEnabled = IsEnabled<UserDataUpdateTask>();
    }

    public string AccountSwitchName { get; set; }

    public bool AccountSwitchEnabled { get; set; }

    public string Stage { get; set; }

    public bool UseMedicine { get; set; }

    public int MedicineCount { get; set; }

    public bool FightEnabled { get; set; }

    public bool InfrastEnabled { get; set; }

    public bool RecruitEnabled { get; set; }

    public bool MallEnabled { get; set; }

    public bool AwardEnabled { get; set; }

    public bool RoguelikeEnabled { get; set; }

    public bool ReclamationEnabled { get; set; }

    public bool UserDataUpdateEnabled { get; set; }

    /// <summary>
    /// 保存到配置档案并关闭弹窗。
    /// </summary>
    public void Save()
    {
        var start = _profile.TaskQueue.OfType<StartUpTask>().FirstOrDefault();
        if (start is not null)
        {
            start.AccountName = AccountSwitchName.Trim();
            start.AccountSwitchEnabled = AccountSwitchEnabled;
        }

        var fight = _profile.TaskQueue.OfType<FightTask>().FirstOrDefault();
        if (fight is not null)
        {
            if (!string.IsNullOrWhiteSpace(Stage))
            {
                fight.StagePlan = [Stage.Trim()];
            }

            fight.UseMedicine = UseMedicine;
            fight.MedicineCount = MedicineCount;
        }

        SetEnabled<FightTask>(FightEnabled);
        SetEnabled<InfrastTask>(InfrastEnabled);
        SetEnabled<RecruitTask>(RecruitEnabled);
        SetEnabled<MallTask>(MallEnabled);
        SetEnabled<AwardTask>(AwardEnabled);
        SetEnabled<RoguelikeTask>(RoguelikeEnabled);
        SetEnabled<ReclamationTask>(ReclamationEnabled);
        SetEnabled<UserDataUpdateTask>(UserDataUpdateEnabled);

        _account.Configuration = _profileName;
        _account.Stage = string.IsNullOrWhiteSpace(_originalStage) && Stage == _profileStage
            ? string.Empty
            : Stage.Trim();
        RequestClose();
    }

    /// <summary>
    /// 放弃修改并关闭弹窗。
    /// </summary>
    public void Cancel() => RequestClose();

    /// <summary>
    /// 保存快捷项后切换到该账号的配置档案，进入主界面任务队列做完整设置。
    /// MAA 切换配置会重启一次，重启后即可像平常一样编辑全部选项。
    /// </summary>
    public void OpenFullSettings()
    {
        Save();
        if (Instances.SettingsViewModel.CurrentConfiguration != _profileName)
        {
            Instances.SettingsViewModel.CurrentConfiguration = _profileName;
            return;
        }

        // 已经是该账号的档案：直接切到主界面任务队列，那里可以编辑全部细项
        if (Instances.SettingsViewModel.Parent is RootViewModel root)
        {
            Execute.OnUIThread(() => root.ActiveItem = Instances.TaskQueueViewModel);
        }
    }

    private bool IsEnabled<T>()
        where T : BaseTask => _profile.TaskQueue.OfType<T>().FirstOrDefault()?.IsEnable == true;

    private static void EnsureDefaultTasks(SpecificConfig profile)
    {
        profile.TaskQueue.Add(new StartUpTask());
        profile.TaskQueue.Add(new FightTask());
        profile.TaskQueue.Add(new InfrastTask());
        profile.TaskQueue.Add(new RecruitTask());
        profile.TaskQueue.Add(new MallTask());
        profile.TaskQueue.Add(new AwardTask());
        profile.TaskQueue.Add(new RoguelikeTask { IsEnable = false });
        profile.TaskQueue.Add(new ReclamationTask { IsEnable = false });
        profile.TaskQueue.Add(new UserDataUpdateTask());
    }

    private void SetEnabled<T>(bool enabled)
        where T : BaseTask
    {
        var task = _profile.TaskQueue.OfType<T>().FirstOrDefault();
        if (task is not null)
        {
            task.IsEnable = enabled;
        }
    }
}
