package main

import (
	"MaaAssistantArknights/conf"
	"MaaAssistantArknights/maa"
	"MaaAssistantArknights/server/http"
	"log"
	"os"
	"os/signal"
	"syscall"
)

func main() {
	c, err := conf.New("./conf.yaml")
	if err != nil {
		panic(err)
	}
	mcw, err := maa.New(c.Asst)
	if err != nil {
		panic(err)
	}

	s := http.New(c.HTTP, mcw)

	ch := make(chan os.Signal, 1)
	signal.Notify(ch, syscall.SIGTERM, syscall.SIGINT, syscall.SIGQUIT)
	for {
		sig := <-ch
		switch sig {
		case syscall.SIGTERM, syscall.SIGINT, syscall.SIGQUIT:
			log.Println("get a signal", sig.String())
			s.Stop()
			return
		default:
			return
		}
	}
}
