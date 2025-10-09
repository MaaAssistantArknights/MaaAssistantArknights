// ...existing code...
    /// <summary>
    /// Gets or sets 最大持续小时数（定时下线）
    /// </summary>
    public int MaxDurationHours { get; set; }
// ...existing code...
    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var taskParams = new JObject
        {
            // ...existing code...
        };
        // ...existing code...
        // 全局定时下线参数
        var globalHours = MaaWpfGui.ViewModels.UserControl.Settings.GameSettingsUserControlModel.Instance.MaxTaskDurationHours;
        if (globalHours > 0)
        {
            taskParams["max_duration_hours"] = globalHours;
        }
        // ...existing code...
        return (TaskType, taskParams);
    }
// ...existing code...
