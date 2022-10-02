package http

import (
	"MaaAssistantArknights/internal/api"
	"net/http"

	"github.com/gin-gonic/gin"
)

func version(c *gin.Context) {
	c.JSON(http.StatusOK, api.Data(api.H{
		"data": MAA.GetVersion(),
	}))
}

func destroy(c *gin.Context) {
	err := MAA.AsstDestroy()
	if err != nil {
		c.JSON(http.StatusOK, api.Failed(err))
		return
	}
	c.JSON(http.StatusOK, api.Success())
}
