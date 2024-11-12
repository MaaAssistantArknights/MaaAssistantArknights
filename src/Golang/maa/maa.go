package maa

import (
	"MaaAssistantArknights/conf"
	"errors"
	"fmt"
	"os"
	"sync"
	"syscall"

	log "github.com/sirupsen/logrus"
)

var (
	ErrorMAAFalse          = errors.New("MAA return false")
	ErrorAsstCreateEx      = errors.New("AsstCreateEx failed")
	ErrorAsstLoadSource    = errors.New("AsstLoadSource failed")
	ErrorAsstConnect       = errors.New("AsstConnect failed")
	ErrorAsstCreate        = errors.New("AsstCreate failed")
	ErrorAsstGetVersion    = errors.New("AsstGetVersion failed")
	ErrorAsstAppendTask    = errors.New("AsstAppendTask failed")
	ErrorAsstSetTaskParams = errors.New("AsstSetTaskParams failed")
	ErrorAsstStart         = errors.New("AsstStart failed")
	ErrorAsstStop          = errors.New("AsstStop failed")
	ErrorAsstRunning       = errors.New("AsstRunning failed")
	ErrorAsstDestroy       = errors.New("AsstDestroy failed")
)

type AsstCallback func(message int, details uintptr, arg uintptr) uintptr

type Maa struct {
	asstHandle uintptr
	version    string
	hooks      []Hook
	mu         sync.Mutex
	cfg        *conf.Asst
}

func New(cfg *conf.Asst) (maa *Maa, err error) {
	if len(cfg.ResourcePath) == 0 {
		cfg.ResourcePath, err = os.Getwd()
		if err != nil {
			return
		}
	}
	maa = &Maa{
		hooks: make([]Hook, 0),
		cfg:   cfg,
	}
	err = maa.initasst()
	if err != nil {
		return
	}
	maa.setDefaultHook()
	return
}

func (m *Maa) initasst() (err error) {
	cfg := m.cfg
	err = newSyscall(cfg.ResourcePath)
	if err != nil {
		return
	}
	err = m.asstLoadResource(cfg.ResourcePath)
	if err != nil {
		return
	}
	err = m.AsstCreateEx()
	if err != nil {
		return
	}
	err = m.AsstConnect(cfg.ADBPath, cfg.ADBAddress, cfg.ADBType)
	if err != nil {
		return
	}

	m.SetVersion()
	return
}

func (m *Maa) callback(message int, details uintptr, arg uintptr) uintptr {
	m.mu.Lock()
	hooks := m.hooks
	m.mu.Unlock()
	for _, hook := range hooks {
		hook.Do(message, CharPtr2String(details), arg)
	}
	return 0
}

func (m *Maa) AppendHook(hooks ...Hook) {
	if len(hooks) == 0 {
		return
	}
	m.mu.Lock()
	defer m.mu.Unlock()
	m.hooks = append(m.hooks, hooks...)
}

func (m *Maa) asstLoadResource(path string) error {
	a1, err := UintPtrFromString(path)
	if err != nil {
		return err
	}
	r1, _, err := asstLoadResource(a1)
	log.WithFields(log.Fields{
		"path": path,
		"r1":   r1,
		"err":  err,
	}).Println("asstLoadResource")
	if !errors.Is(err, syscall.Errno(0)) {
		return ErrorAsstLoadSource
	}
	if r1 == 0 {
		return ErrorMAAFalse
	}
	return nil
}

func (m *Maa) AsstCreateEx() error {
	a1 := syscall.NewCallback(m.callback)
	a2 := uintptr(0) // 这里的arg之后看有没有需要再更新
	r1, _, err := asstCreateEx(a1, a2)

	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstCreateEx")

	if !errors.Is(err, syscall.Errno(0)) {
		return ErrorAsstCreateEx
	}
	m.asstHandle = r1
	return nil
}

func (m *Maa) AsstCreate() error {
	r1, _, err := asstCreate()
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstCreate")
	if !errors.Is(err, syscall.Errno(0)) {
		return ErrorAsstCreate
	}
	m.asstHandle = r1
	return nil
}

func (m *Maa) AsstConnect(path string, address string, config string) error {
	fmt.Println("adb start connect")
	a1, err := UintPtrFromString(path)
	if err != nil {
		return err
	}
	a2, err := UintPtrFromString(address)
	if err != nil {
		return err
	}
	a3, err := UintPtrFromString(config)
	if err != nil {
		return err
	}
	r1, _, err := asstConnect(m.asstHandle, a1, a2, a3)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstConnect")

	if r1 == 0 {
		return ErrorAsstConnect
	}
	return nil
}

func (m *Maa) setDefaultHook() {
	m.hooks = append(m.hooks, &sampleHook{})
}

func (m *Maa) SetVersion() {
	r1, _, err := asstGetVersion(m.asstHandle)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("SetVersion")
	if !errors.Is(err, syscall.Errno(0)) {
		return
	}
	if r1 > 0 {
		m.version = CharPtr2String(r1)
	}
}

func (m *Maa) GetVersion() string {
	return m.version
}

func (m *Maa) AsstAppendTask(typ TaskType, task Tasker) (TaskID, error) {
	a1, err := UintPtrFromString(typ.String())
	if err != nil {
		return 0, err
	}
	a2, err := UintPtrFromString(task.Serialize())
	if err != nil {
		return 0, err
	}
	r1, _, err := asstAppendTask(m.asstHandle, a1, a2)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("asstAppendTask")
	if r1 == 0 {
		return 0, ErrorAsstAppendTask
	}
	return TaskID(r1), nil
}

func (m *Maa) AsstSetTaskParams(taskID TaskID, task Tasker) bool {
	a1 := uintptr(taskID)
	a2, err := UintPtrFromString(task.Serialize())
	if err != nil {
		return false
	}
	r1, _, err := asstSetTaskParams(m.asstHandle, a1, a2)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstSetTaskParams")

	return r1 > 0
}

func (m *Maa) AsstStart() bool {
	r1, _, err := asstStart(m.asstHandle)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstStart")
	return r1 > 0
}

func (m *Maa) AsstStop() bool {
	r1, _, err := asstStop(m.asstHandle)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstStop")

	return r1 > 0
}

func (m *Maa) AsstRunning() bool {
	r1, _, err := asstRunning(m.asstHandle)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstRunning")
	return r1 > 0
}

func (m *Maa) AsstDestroy() error {
	r1, _, err := asstDestroy(m.asstHandle)
	log.WithFields(log.Fields{
		"r1":  r1,
		"err": err,
	}).Println("AsstDestroy")
	if r1 == 0 {
		return ErrorAsstDestroy
	}
	return nil
}
