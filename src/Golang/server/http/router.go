package http

import (
	"github.com/gin-gonic/gin"
)

func setRouter(r *gin.RouterGroup) {
	taskgroup := r.Group("/task")
	{
		taskgroup.POST("/append", taskAppend)
		taskgroup.POST("/modify", taskModify)
		taskgroup.GET("/list", list)
		taskgroup.POST("/start", start)
		taskgroup.POST("/stop", stop)
		taskgroup.GET("/running", running)
	}

	r.GET("/version", version)
	r.POST("/destroy", destroy)
}
