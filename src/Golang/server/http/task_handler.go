package http

import (
	"MaaAssistantArknights/internal/api"
	"MaaAssistantArknights/maa"
	"MaaAssistantArknights/server/http/params"
	"errors"
	"net/http"

	"github.com/gin-gonic/gin"
)

var (
	errorTaskAdded        = errors.New("该任务类型已添加")
	errorTaskTypeNotFound = errors.New("未找到合适的任务类型")
	errorTaskIDNotFound   = errors.New("未找到该任务id对应的任务")
)

type task struct {
	TaskID     maa.TaskID `json:"task_id"`
	TaskType   string     `json:"task_type"`
	TaskParams string     `json:"task_params"`
}

// 添加任务时
func taskAppend(c *gin.Context) {
	arg := new(params.ArgAppendTask)
	if err := c.Bind(arg); err != nil {
		return
	}
	if len(arg.Typ) == 0 || len(arg.Params) == 0 {
		c.JSON(http.StatusOK, api.ErrorInvaildParams)
		return
	}
	if _, ok := cache.Get(arg.Typ); ok {
		c.JSON(http.StatusOK, api.ErrFunc(errorTaskAdded))
		return
	}
	v, ok := maa.TaskFixed[arg.Typ]
	if !ok {
		c.JSON(http.StatusOK, api.ErrFunc(errorTaskTypeNotFound))
		return
	}
	taskID, err := MAA.AsstAppendTask(v, arg.Params)
	if err != nil {
		c.JSON(http.StatusOK, api.ErrFunc(err))
		return
	}
	//存储taskID
	cache.Set(arg.Typ, &task{
		TaskID:     taskID,
		TaskType:   arg.Typ,
		TaskParams: string(arg.Params),
	})

	c.JSON(http.StatusOK, api.Data(api.H{
		"task_id": taskID,
	}))
}

func taskModify(c *gin.Context) {
	arg := new(params.ArgmodifyTask)
	if err := c.Bind(arg); err != nil {
		return
	}

	if arg.TaskID <= 0 || len(arg.Params) == 0 {
		c.JSON(http.StatusOK, api.ErrorInvaildParams)
		return
	}
	tasks := cache.Values()
	for _, v := range tasks {
		t := v.(*task)
		if t.TaskID == arg.TaskID {
			cache.Set(t.TaskType, &task{
				TaskID:     arg.TaskID,
				TaskType:   t.TaskType,
				TaskParams: string(arg.Params),
			})

			status := MAA.AsstSetTaskParams(arg.TaskID, arg.Params)
			c.JSON(http.StatusOK, api.Data(api.H{
				"status": status,
			}))
		}
	}
	c.JSON(http.StatusOK, api.ErrFunc(errorTaskIDNotFound))
}

func start(c *gin.Context) {
	res := MAA.AsstStart()
	c.JSON(http.StatusOK, api.Data(api.H{
		"status": res,
	}))
}

func stop(c *gin.Context) {
	res := MAA.AsstStop()
	if res {
		cache.Clear()
	}
	c.JSON(http.StatusOK, api.Data(api.H{
		"status": res,
	}))
}

func running(c *gin.Context) {
	res := MAA.AsstRunning()
	c.JSON(http.StatusOK, api.Data(api.H{
		"status": res,
	}))
}

func list(c *gin.Context) {
	tasks := cache.Values()
	ts := make([]*task, 0)

	for _, v := range tasks {
		t := v.(*task)
		ts = append(ts, t)
	}
	c.JSON(http.StatusOK, api.Data(api.H{
		"tasks": ts,
	}))
}
