using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media.Imaging;
using System.Windows.Threading;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace MaaWpfGui.Services.RemoteControl
{
    public class RemoteControlService
    {
        private readonly Task _pollJobTask = Task.CompletedTask;
        private readonly Task _executeJobTask = Task.CompletedTask;
        private readonly List<string> _executedTaskIds = new List<string>();
        private readonly ConcurrentQueue<JObject> _taskQueue = new ConcurrentQueue<JObject>();
        private readonly RunningState _runningState;

        public RemoteControlService()
        {
            _pollJobTask = _pollJobTask.ContinueWith(async (_) =>
            {
                while (true)
                {
                    await Task.Delay(1000);
                    try
                    {
                        await PollJobTaskLoop();
                    }
                    catch (Exception ex)
                    {
                        Log.Logger.Error(ex, "RemoteControl service raises unknown error.");
                    }
                }

                // ReSharper disable once FunctionNeverReturns
            });

            _executeJobTask = _executeJobTask.ContinueWith(async (_) =>
            {
                while (true)
                {
                    await Task.Delay(1000);
                    try
                    {
                        await ExecuteJobLoop();
                    }
                    catch (Exception ex)
                    {
                        Log.Logger.Error(ex, "RemoteControl service raises unknown error.");
                    }
                }

                // ReSharper disable once FunctionNeverReturns
            });

            _runningState = RunningState.Instance;
        }

        #region Private Method Invoker

        private static void InvokeInstanceMethod(object instance, string methodName)
        {
            if (instance == null)
            {
                throw new ArgumentNullException(nameof(instance));
            }

            if (string.IsNullOrEmpty(methodName))
            {
                throw new ArgumentNullException(nameof(methodName));
            }

            Type type = instance.GetType();
            MethodInfo methodInfo = type.GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Instance);

            if (methodInfo == null)
            {
                throw new ArgumentException($"Method '{methodName}' not found in type '{type.FullName}'.");
            }

            methodInfo.Invoke(instance, null);
        }

        private static T InvokeInstanceFunction<T>(object instance, string methodName)
        {
            if (instance == null)
            {
                throw new ArgumentNullException(nameof(instance));
            }

            if (string.IsNullOrEmpty(methodName))
            {
                throw new ArgumentNullException(nameof(methodName));
            }

            Type type = instance.GetType();
            MethodInfo methodInfo = type.GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Instance);

            if (methodInfo == null)
            {
                throw new ArgumentException($"Method '{methodName}' not found in type '{type.FullName}'.");
            }

            if (!typeof(T).IsAssignableFrom(methodInfo.ReturnType))
            {
                throw new ArgumentException($"Method '{methodName}' is not {typeof(T)}.");
            }

            return (T)methodInfo.Invoke(instance, null);
        }

        private static async Task<T> InvokeInstanceAsyncFunction<T>(object instance, string methodName)
        {
            if (instance == null) { throw new ArgumentNullException(nameof(instance)); }
            if (string.IsNullOrEmpty(methodName)) { throw new ArgumentNullException(nameof(methodName)); }

            Type type = instance.GetType();
            MethodInfo methodInfo = type.GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Instance);

            if (methodInfo == null)
            {
                throw new ArgumentException($"Method '{methodName}' not found in type '{type.FullName}'.");
            }

            // 检查方法是否是异步方法 (返回Task或Task<T>)
            if (!typeof(Task).IsAssignableFrom(methodInfo.ReturnType))
            {
                throw new ArgumentException($"Method '{methodName}' is not asynchronous.");
            }

            return await (Task<T>)methodInfo.Invoke(instance, null);
        }

        private static TResult InvokeStaticFunction<TResult>(Type staticType, string methodName)
        {
            if (staticType == null) { throw new ArgumentNullException(nameof(staticType)); }

            if (string.IsNullOrEmpty(methodName)) { throw new ArgumentNullException(nameof(methodName)); }

            MethodInfo methodInfo = staticType.GetMethod(methodName, BindingFlags.NonPublic | BindingFlags.Static);

            if (methodInfo == null)
            {
                throw new ArgumentException($"Method '{methodName}' not found in type '{staticType.FullName}'.");
            }

            return (TResult)methodInfo.Invoke(null, null);
        }

        #endregion

        private async Task PollJobTaskLoop()
        {
            var endpoint = Instances.SettingsViewModel.RemoteControlGetTaskEndpointUri;
            var uid = Instances.SettingsViewModel.RemoteControlUserIdentity;
            var did = Instances.SettingsViewModel.RemoteControlDeviceIdentity;
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(endpoint), new { user=uid, device = did});
            if (response == null)
            {
                Log.Logger.Error("RemoteControlService endpoint failed.");
                return;
            }

            var jsonObject = JsonConvert.DeserializeObject<JObject>(response);

            // A list of task
            if (jsonObject?.GetValue("tasks") is JArray tasks)
            {
                foreach (var task in tasks.OfType<JObject>())
                {
                    var type = task.GetValue("type")?.Value<string>();
                    if (!string.IsNullOrWhiteSpace(type))
                    {
                        // It is a valid task
                        var id = task.GetValue("id")?.Value<string>();
                        if (!_executedTaskIds.Contains(id))
                        {
                            _executedTaskIds.Add(id);

                            _taskQueue.Enqueue(task);
                        }
                    }
                }
            }
        }

        private async Task ExecuteJobLoop()
        {
            if (_taskQueue.TryDequeue(out var task))
            {
                var type = task.GetValue("type")?.Value<string>();
                var id = task.GetValue("id")?.Value<string>();
                var data = task.GetValue("params")?.Value<string>();
                var payload = string.Empty;
                var status = "SUCCESS";

                switch (type)
                {
                    case "LinkStart":
                    {
                        // 一键长草特殊任务
                        await _runningState.UntilIdleAsync();
                        var startLogStr = string.Format(LocalizationHelper.GetString("RemoteControlReceivedTask"), type, id);

                        Application.Current.Dispatcher.Invoke(() =>
                        {
                            Instances.TaskQueueViewModel.AddLog(startLogStr);
                            Instances.TaskQueueViewModel.LinkStart();
                        });
                        await _runningState.UntilIdleAsync();

                        var stopLogStr = string.Format(LocalizationHelper.GetString("RemoteControlCompletedTask"), type, id);
                        Application.Current.Dispatcher.Invoke(() =>
                        {
                            Instances.TaskQueueViewModel.AddLog(stopLogStr);
                        });
                        break;
                    }

                    case "LinkStart-Base":
                    case "LinkStart-WakeUp":
                    case "LinkStart-Combat":
                    case "LinkStart-Recruiting":
                    case "LinkStart-Mall":
                    case "LinkStart-Mission":
                    case "LinkStart-AutoRoguelike":
                    case "LinkStart-ReclamationAlgorithm":
                    {
                        await LinkStart(new[] { type.Split('-')[1] });
                        break;
                    }

                    case "Toolbox-GachaOnce":
                    {
                        await _runningState.UntilIdleAsync();
                        Instances.RecognizerViewModel.GachaOnce();
                        while (!Instances.RecognizerViewModel.GachaDone)
                        {
                            await Task.Delay(100); // 暂停100毫秒以避免密集循环
                        }

                        break;
                    }

                    case "Toolbox-GachaTenTimes":
                    {
                        await _runningState.UntilIdleAsync();
                        Instances.RecognizerViewModel.GachaTenTimes();
                        while (!Instances.RecognizerViewModel.GachaDone)
                        {
                            await Task.Delay(100); // 暂停100毫秒以避免密集循环
                        }

                        break;
                    }

                    case "CaptureImage":
                    {
                        string errMsg = string.Empty;
                        bool connected = await Task.Run(() => Instances.AsstProxy.AsstConnect(ref errMsg));
                        if (connected)
                        {
                            var image = Instances.AsstProxy.AsstGetImage();
                            if (image == null)
                            {
                                status = "FAILED";
                                break;
                            }

                            byte[] bytes;
                            using (MemoryStream stream = new MemoryStream())
                            {
                                PngBitmapEncoder encoder = new PngBitmapEncoder();
                                encoder.Frames.Add(BitmapFrame.Create(image));
                                encoder.Save(stream);
                                bytes = stream.ToArray();
                            }

                            payload = Convert.ToBase64String(bytes);
                            break;
                        }
                        else
                        {
                            status = "FAILED";
                            break;
                        }
                    }

                    case "Settings-ConnectAddress":
                        // ConfigurationHelper.SetValue(type.Split('-')[1], data);
                        Application.Current.Dispatcher.Invoke(() =>
                        {
                            Instances.SettingsViewModel.ConnectAddress = data;
                        });

                        break;

                    default:
                        // 未知的Type统一直接发给MAACore
                        // No! 未知的任务一概不处理
                        break;
                }

                var endpoint = Instances.SettingsViewModel.RemoteControlReportStatusUri;
                var uid = Instances.SettingsViewModel.RemoteControlUserIdentity;
                var did = Instances.SettingsViewModel.RemoteControlDeviceIdentity;
                var response = await Instances.HttpService.PostAsJsonAsync(new Uri(endpoint), new
                {
                    user = uid,
                    device = did,
                    status = status,
                    task = id,
                    payload = payload,
                });
                if (response == null)
                {
                    Log.Logger.Error("RemoteControlService report task failed.");
                }
            }
        }

        /// <summary>
        /// 根据"一键长草"功能进行修改的方法。
        /// </summary>
        /// <remarks>
        /// <para>注意以下特点:</para>
        /// <para>- 可以在非UI线程执行。</para>
        /// <para>- 不调用StartScript。</para>
        /// <para>- 不使用Model里的列表。</para>
        /// <para>- 在结尾添加对RunningStatus的等待。</para>
        /// <para>若"一键长草"功能在未来有更新，此方法也应进行相应的同步更新，但需确保上述特点保持不变。</para>
        /// </remarks>
        /// <param name="originalNames">指定的任务列表。</param>
        /// <returns>异步任务，无返回结果。</returns>
        public async Task LinkStart(IEnumerable<string> originalNames)
        {
            await _runningState.UntilIdleAsync();

            _runningState.SetIdle(false);

            await Application.Current.Dispatcher.Invoke(async () =>
            {

                // 虽然更改时已经保存过了，不过保险起见还是在点击开始之后再保存一次任务及基建列表
                Instances.TaskQueueViewModel.TaskItemSelectionChanged();
                Instances.SettingsViewModel.InfrastOrderSelectionChanged();

                InvokeInstanceMethod(Instances.TaskQueueViewModel, "ClearLog");

                /*await Task.Run(() => Instances.SettingsViewModel.RunScript("StartsWithScript"));*/

                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("ConnectingToEmulator"));
                if (!Instances.SettingsViewModel.AdbReplaced && !Instances.SettingsViewModel.IsAdbTouchMode())
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("AdbReplacementTips"), UiLogColor.Info);
                }

                // 一般是点了“停止”按钮了
                if (Instances.TaskQueueViewModel.Stopping)
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                if (!await InvokeInstanceAsyncFunction<bool>(Instances.TaskQueueViewModel, "ConnectToEmulator"))
                {
                    return;
                }

                // 一般是点了“停止”按钮了
                if (Instances.TaskQueueViewModel.Stopping)
                {
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                bool taskRet = true;

                // 直接遍历TaskItemViewModels里面的内容，是排序后的
                int count = 0;
                foreach (var item in originalNames)
                {
                    ++count;
                    switch (item)
                    {
                        case "Base":
                            taskRet &= InvokeInstanceFunction<bool>(Instances.TaskQueueViewModel, "AppendInfrast");
                            break;

                        case "WakeUp":
                            taskRet &= InvokeStaticFunction<bool>(Instances.TaskQueueViewModel.GetType(), "AppendStart");
                            break;

                        case "Combat":
                            taskRet &= InvokeInstanceFunction<bool>(Instances.TaskQueueViewModel, "AppendFight");
                            break;

                        case "Recruiting":
                            taskRet &= InvokeStaticFunction<bool>(Instances.TaskQueueViewModel.GetType(), "AppendRecruit");
                            break;

                        case "Mall":
                            taskRet &= InvokeInstanceFunction<bool>(Instances.TaskQueueViewModel, "AppendMall");
                            break;

                        case "Mission":
                            taskRet &= InvokeInstanceFunction<bool>(Instances.TaskQueueViewModel, "AppendAward");
                            break;

                        case "AutoRoguelike":
                            taskRet &= InvokeStaticFunction<bool>(Instances.TaskQueueViewModel.GetType(), "AppendRoguelike");
                            break;

                        case "ReclamationAlgorithm":
                            taskRet &= InvokeStaticFunction<bool>(Instances.TaskQueueViewModel.GetType(), "AppendReclamation");
                            break;

                        default:
                            --count;

                            // Instances.TaskQueueViewModel._logger.Error("Unknown task: " + item);
                            break;
                    }

                    if (taskRet)
                    {
                        continue;
                    }

                    Instances.TaskQueueViewModel.AddLog(item + "Error", UiLogColor.Error);
                    taskRet = true;
                    --count;
                }

                if (count == 0)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UnselectedTask"));
                    _runningState.SetIdle(true);
                    Instances.TaskQueueViewModel.SetStopped();
                    return;
                }

                taskRet &= Instances.AsstProxy.AsstStart();

                if (taskRet)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("Running"));
                }
                else
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("UnknownErrorOccurs"));
                    await Instances.TaskQueueViewModel.Stop();
                    Instances.TaskQueueViewModel.SetStopped();
                }
            });

            await _runningState.UntilIdleAsync();
        }

        public static async Task ConnectionTest()
        {
            var endpoint = Instances.SettingsViewModel.RemoteControlGetTaskEndpointUri;
            var uid = Instances.SettingsViewModel.RemoteControlUserIdentity;
            var did = Instances.SettingsViewModel.RemoteControlDeviceIdentity;
            var response = await Instances.HttpService.PostAsJsonAsync(new Uri(endpoint), new { user = uid, device = did });

            using var toast = new ToastNotification(
                LocalizationHelper.GetString(
                    response != null ? "RemoteControlConnectionTestSuccess" : "RemoteControlConnectionTestFail"));

            // if (response != null)
            // {
            //    toast.AppendContentText(response);
            // }
            toast.Show();

        }

        public static void RegenerateDeviceIdentity()
        {
            Instances.SettingsViewModel.RemoteControlDeviceIdentity = Guid.NewGuid().ToString("N");
        }
    }
}
