package maa


type Maa interface {
	CallBack(message int, details uintptr, arg uintptr) uintptr
	CreateEx(arg uintptr) error
	CatchDefault() error
	CatchEmulator() error
	CatchCustom(address string) error
	AppendFight(stage string, medicine int, stone int, times int) error
	AppendAward() error
	AppendVisit() error
	AppendMall(do bool) error
	AppendInfrast(mode int, orders []string, drones string, threshold float64) error
	AppendRecruit(max int, selects []int, confirms []int, refresh bool, expedite bool) error
	Start() error
	Stop() error
	StartRecruitCalc(selects []int, times bool) error
	GetVersion() (string, error)
	SetPenguinId(id string) error
	Run(config Config) error
	RunWithCron(config Config) error
}