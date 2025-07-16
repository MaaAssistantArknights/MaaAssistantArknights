using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;
public class AsstDGLabTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.DGLab;
    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));
}
