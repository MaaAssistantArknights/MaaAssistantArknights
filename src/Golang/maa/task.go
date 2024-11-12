package maa

type TaskType int
type TaskID int

const (
	StartUp   TaskType = iota //开始唤醒
	Fight                     //刷理智
	Recruit                   //公开招募
	Infrast                   //基建换班
	Visit                     //访问好友
	Mall                      //领取信用及商店购物。
	Award                     //领取日常奖励
	Roguelike                 //无限刷肉鸽
	Copilot                   //自动抄作业
	Depot                     //仓库识别
)

func (t TaskType) String() string {
	switch t {
	case StartUp:
		return "StartUp"
	case Fight:
		return "Fight"
	case Recruit:
		return "Recruit"
	case Infrast:
		return "Infrast"
	case Mall:
		return "Mall"
	case Award:
		return "Award"
	case Roguelike:
		return "Roguelike"
	case Copilot:
		return "Copilot"
	case Depot:
		return "Depot"
	default:
		return ""
	}
}

var TaskFixed = map[string]TaskType{
	"StartUp":   StartUp,
	"Fight":     Fight,
	"Recruit":   Recruit,
	"Infrast":   Infrast,
	"Mall":      Mall,
	"Award":     Award,
	"Roguelike": Roguelike,
	"Copilot":   Copilot,
	"Depot":     Depot,
}

// 添加的任务需要实现该接口
type Tasker interface {
	Serialize() string
}

type TaskerString string

func (t TaskerString) Serialize() string {
	return string(t)
}
