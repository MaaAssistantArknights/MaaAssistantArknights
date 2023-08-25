package conf

import (
	"os"

	"gopkg.in/yaml.v2"
)

type Config struct {
	HTTP *HTTP
	Asst *Asst
}

type HTTP struct {
	Host string
	Port int
}

type Asst struct {
	ADBAddress   string
	ADBPath      string
	ADBType      string
	ResourcePath string
}

func New(path string) (*Config, error) {
	conf := &Config{}
	yamlFile, err := os.ReadFile(path)
	if err != nil {
		return conf, err
	}
	err = yaml.Unmarshal(yamlFile, conf)
	if err != nil {
		return conf, err
	}
	return conf, nil
}
