package maa

import (
	"errors"
	"fmt"
	"github.com/robfig/cron"
	log "github.com/sirupsen/logrus"
	"syscall"
	"unsafe"
)

type MaaCliWin struct {
	Path   string
	DLL    *syscall.DLL
	Ptr    uintptr
}

func NewMaaCliWin(path string) (*MaaCliWin, error) {
	dll, err := syscall.LoadDLL(path + "/MeoAssistant.dll")
	if err != nil {
		return nil, err
	}
	maa := MaaCliWin{path, dll, 0}
	err = maa.CreateEx(0)
	if err != nil {
		return nil, err
	}
	return &maa, nil
}

func (m *MaaCliWin) CallBack(message int, details uintptr, arg uintptr) uintptr {
	log.Infoln(message, CharPtr2String(details), arg)
	return 0
}

func (m *MaaCliWin) CreateEx(arg uintptr) error {
	proc, err := m.DLL.FindProc("AsstCreateEx")
	if err != nil {
		return err
	}
	a1, err := UintPtrFromString(m.Path)
	if err != nil {
		return err
	}
	a2 := syscall.NewCallback(m.CallBack)
	a3 := arg
	r1, _, err := proc.Call(a1, a2, a3)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("CreateEx", err)
	}
	m.Ptr = r1
	return nil
}

func (m *MaaCliWin) CatchDefault() error {
	proc, err := m.DLL.FindProc("AsstCatchDefault")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("CatchDefault", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) CatchEmulator() error {
	proc, err := m.DLL.FindProc("AsstCatchEmulator")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("CatchEmulator", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

// CatchCustom 没测过
func (m *MaaCliWin) CatchCustom(address string) error {
	proc, err := m.DLL.FindProc("AsstCatchCustom")
	if err != nil {
		return err
	}
	a1, err := UintPtrFromString(address)
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(a1)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("CatchCustom", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendFight(stage string, medicine int, stone int, times int) error {
	proc, err := m.DLL.FindProc("AsstAppendFight")
	if err != nil {
		return err
	}
	a1, err := UintPtrFromString(stage)
	if err != nil {
		return err
	}
	a2 := uintptr(medicine)
	a3 := uintptr(stone)
	a4 := uintptr(times)
	r1, _, err := proc.Call(m.Ptr, a1, a2, a3, a4)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("AppendFight", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendAward() error {
	proc, err := m.DLL.FindProc("AsstAppendAward")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn(" AppendAward", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendVisit() error {
	proc, err := m.DLL.FindProc("AsstAppendVisit")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("AppendVisit", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendMall(do bool) error {
	proc, err := m.DLL.FindProc("AsstAppendMall")
	if err != nil {
		return err
	}
	a1:=UintPtrFromBool(do)
	r1, _, err := proc.Call(m.Ptr, a1)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("AppendMall", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendInfrast(mode int, orders []string, drones string, threshold float64) error {
	proc, err := m.DLL.FindProc("AsstAppendInfrast")
	if err != nil {
		return err
	}
	var orderArr []uintptr
	for _, order := range orders {
		orderStr, err := UintPtrFromString(order)
		if err != nil {
			return err
		}
		orderArr = append(orderArr, orderStr)
	}
	if orderArr == nil {
		return errors.New("orders is empty")
	}
	a1 := uintptr(mode)
	a2 := uintptr(unsafe.Pointer(&orderArr[0]))
	a3 := uintptr(len(orders))
	a4 := uintptr(unsafe.Pointer(&[]byte(drones)[0]))
	a5 := uintptr(*(*uint64)(unsafe.Pointer(&threshold)))
	r1, _, err := proc.Call(m.Ptr, a1, a2, a3, a4, a5)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("AppendInfrast", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) AppendRecruit(max int, selects []int, confirms []int, refresh bool, expedite bool) error {
	proc, err := m.DLL.FindProc("AsstAppendRecruit")
	if err != nil {
		return err
	}
	a1 := uintptr(max)
	a2 := uintptr(unsafe.Pointer(&selects[0]))
	a3 := uintptr(len(selects))
	a4 := uintptr(unsafe.Pointer(&confirms[0]))
	a5 := uintptr(len(confirms))
	a6:=UintPtrFromBool(refresh)
	a7:=UintPtrFromBool(expedite)
	r1, _, err := proc.Call(m.Ptr, a1, a2, a3, a4, a5, a6, a7)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("AppendRecruit", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) Start() error {
	proc, err := m.DLL.FindProc("AsstStart")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("Start", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) Stop() error {
	proc, err := m.DLL.FindProc("AsstStop")
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("Stop", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}
func (m *MaaCliWin) StartRecruitCalc(selects []int, times bool) error {
	proc, err := m.DLL.FindProc("AsstStartRecruitCalc")
	if err != nil {
		return err
	}
	a1 := uintptr(unsafe.Pointer(&selects[0]))
	a2:=UintPtrFromBool(times)
	r1, _, err := proc.Call(m.Ptr, a1, a2)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("StartRecruitCalc", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}

func (m *MaaCliWin) GetVersion() (string, error) {
	proc, err := m.DLL.FindProc("AsstGetVersion")
	if err != nil {
		return "", err
	}
	r1, _, err := proc.Call(m.Ptr)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("GetVersion", err)
	}
	if r1 == 0 {
		return "", FalseError
	}
	return CharPtr2String(r1), nil
}

func (m *MaaCliWin) SetPenguinId(id string) error {
	proc, err := m.DLL.FindProc("AsstSetPenguinId")
	if err != nil {
		return err
	}
	a1, err := UintPtrFromString(id)
	if err != nil {
		return err
	}
	r1, _, err := proc.Call(m.Ptr, a1)
	if !errors.Is(err, syscall.Errno(0)) {
		log.Warn("SetPenguinId", err)
	}
	if r1 == 0 {
		return FalseError
	}
	return nil
}
func (m *MaaCliWin) Run(config Config) error {
	version, err := m.GetVersion()
	if err != nil {
		return err
	}
	log.Infoln("DLL版本:", version)
	// 连接模拟器
	if config.CustomAdbAddress != "" {
		err := m.CatchCustom(config.CustomAdbAddress)
		log.Infoln("连接自定义模拟器", config.CustomAdbAddress)
		if err != nil {
			log.Error("连接自定义模拟器失败", err)
			return err
		}
	} else {
		err := m.CatchDefault()
		log.Infoln("连接默认模拟器", config.CustomAdbAddress)
		if err != nil {
			log.Error("连接默认模拟器失败", err)
			return err
		}
	}
	// 添加战斗任务
	fight := config.Fight
	err = m.AppendFight(fight.Stage, fight.Medicine, fight.Stone, fight.Times)
	if err != nil {
		return err
	}
	log.Println("添加战斗任务")
	// 添加基建任务
	infrast := config.Infrast
	err = m.AppendInfrast(infrast.Mode, infrast.Orders, infrast.Drones, infrast.Threshold)
	if err != nil {
		return err
	}
	log.Println("添加基建任务")
	// 添加公招任务
	recruit := config.Recruit
	err = m.AppendRecruit(recruit.Max, recruit.Selects, recruit.Confirms, recruit.Refresh, recruit.Expedite)
	if err != nil {
		return err
	}
	log.Println("添加公招任务")
	// 添加访问好友任务
	err = m.AppendVisit()
	if err != nil {
		return err
	}
	log.Println("添加访问好友任务")
	// 添加商店任务
	err = m.AppendMall(config.Mall.Buy)
	if err != nil {
		return err
	}
	log.Println("添加商店任务")
	// 添加领取任务奖励任务
	err = m.AppendAward()
	log.Println("添加领取奖励任务")
	if err != nil {
		return err
	}
	err = m.Start()
	log.Println("开始执行任务")
	if err != nil {
		return err
	}
	return nil
}
func (m *MaaCliWin) RunWithCron(config Config) error {
	c := cron.New() // 新建一个定时任务对象
	err := c.AddFunc(config.Cron, func() {
		log.Println(config.Cron, "定时任务开始执行")
		err := m.Run(config)
		if err != nil {
			return
		}
	})
	if err != nil {
		fmt.Println(err)
		return err
	}
	c.Start()
	log.Infoln("设置定时任务")
	return nil
}
