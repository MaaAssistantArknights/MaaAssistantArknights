package params

import "MaaAssistantArknights/maa"

type ArgAppendTask struct {
	Typ    string           `json:"type"`
	Params maa.TaskerString `json:"params"`
}

type ArgmodifyTask struct {
	TaskID maa.TaskID       `json:"task_id"`
	Params maa.TaskerString `json:"params"`
}
