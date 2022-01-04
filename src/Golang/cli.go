package main

import (
	"errors"
	"gomma/maa"
	"os"
	"runtime"
)

func main() {
	dir, err := os.Getwd()
	if err != nil {
		return
	}
	config, err := maa.LoadConfig("config.yaml")
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			config = maa.NewConfig()
			err := maa.WriteConfig("config.yaml", config)
			if err != nil {
				return
			}
		} else {
			return
		}
	}
	if runtime.GOOS == "windows" {
		MaaWin, err := maa.NewMaaCliWin(dir)
		if err != nil {
			return
		}
		err = MaaWin.Run(config)
		if err != nil {
			return
		}
		err = MaaWin.RunWithCron(config)
		if err != nil {
			return
		}
		for true {}
	}
}