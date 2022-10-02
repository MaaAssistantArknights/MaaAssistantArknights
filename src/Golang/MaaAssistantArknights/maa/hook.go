package maa

import log "github.com/sirupsen/logrus"

type Hook interface {
	Do(message int, details string, arg interface{})
}

type sampleHook struct {
}

func (sampleHook) Do(message int, details string, arg interface{}) {
	log.Infoln(message, details, arg)
}

type dealCallback struct {
}

func (dealCallback) Do(message int, details string, arg interface{}) {

}
