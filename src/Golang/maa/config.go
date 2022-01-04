package maa

import (
	"gopkg.in/yaml.v2"
	"io/ioutil"
)

type Config struct {
	CustomAdbAddress string
	Cron string
	Fight struct{
		Stage string
		Medicine int
		Stone int
		Times int
	}
	Infrast struct{
		Mode int
		Orders []string
		Drones string
		Threshold float64
	}
	Recruit struct{
		Max int
		Selects []int
		Confirms []int
		Refresh bool
		Expedite bool
	}
	Mall struct{
		Buy bool
	}
}

func NewConfig() Config {
	return Config{
		CustomAdbAddress: "",
		Cron: "0 0 */2 * * *",
		Fight: struct {
			Stage    string
			Medicine int
			Stone    int
			Times    int
		}{Stage: "LastBattle", Medicine: 999, Stone: 0, Times: 999},
		Infrast: struct {
			Mode      int
			Orders    []string
			Drones    string
			Threshold float64
		}{Mode: 1, Orders:[]string{"Mfg", "Trade", "Control", "Power", "Reception", "Office", "Dorm"}, Drones: "Money", Threshold: 0.3},
		Recruit: struct {
			Max      int
			Selects  []int
			Confirms []int
			Refresh  bool
			Expedite bool
		}{Max: 3, Selects: []int{3,4,5}, Confirms: []int{3,4,5}, Refresh: true, Expedite: true},
		Mall: struct{ Buy bool }{Buy: true},
	}
}

func LoadConfig(path string) (Config, error) {
	conf := Config{}
	yamlFile, err := ioutil.ReadFile(path)
	if err != nil {
		return conf, err
	}
	err = yaml.Unmarshal(yamlFile, &conf)
	if err != nil {
		return conf, err
	}
	return conf, nil
}

func WriteConfig(path string, conf Config) error {
	yamlFile, err := yaml.Marshal(&conf)
	if err != nil {
		return err
	}
	err = ioutil.WriteFile(path, yamlFile, 0755)
	if err != nil {
		return err
	}
	return nil
}