package http

import (
	"MaaAssistantArknights/conf"
	"MaaAssistantArknights/maa"
	"MaaAssistantArknights/server/http/db"
	"fmt"
	"log"
	"strings"

	"github.com/gin-gonic/gin"
)

var (
	MAA   *maa.Maa
	cache = db.New() //先简简单单这么用一下
)

type Server struct {
	cfg *conf.HTTP
}

const defaultPort = 8080

func New(cfg *conf.HTTP, m *maa.Maa) *Server {
	s := new(Server)
	MAA = m
	s.cfg = cfg
	go s.Run()
	return s
}

func (s *Server) Run() {
	e := gin.Default()
	version := strings.Split(MAA.GetVersion(), "-")
	Major, _ := version[0], version[0]
	MajorGroup := e.Group(Major)
	setRouter(MajorGroup)

	if s.cfg.Port == 0 {
		s.cfg.Port = defaultPort
	}

	e.Run(fmt.Sprintf("%s:%d", s.cfg.Host, s.cfg.Port))
}

func (s *Server) Stop() {
	log.Println("start destroy")
	MAA.AsstStop()
	MAA.AsstDestroy()
}
