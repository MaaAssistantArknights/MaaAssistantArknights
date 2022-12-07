package maa

import (
	"os"
	"syscall"
)

// 之后抽象成和Linux调用方式保持一致 先这么拆
const (
	loadDLL = "MaaCore.dll"
	sep     = string(os.PathSeparator)
)

type procCall func(a ...uintptr) (r1, r2 uintptr, lastErr error)

var createAsstFunc = func(name string, dll *syscall.DLL) (procCall, error) {
	proc, err := dll.FindProc(name)
	if err != nil {
		return nil, err
	}

	return func(a ...uintptr) (r1 uintptr, r2 uintptr, lastErr error) {
		return proc.Call(a...)
	}, nil
}

var (
	asstCreate        procCall
	asstCreateEx      procCall
	asstGetVersion    procCall
	asstAppendTask    procCall
	asstConnect       procCall
	asstSetTaskParams procCall
	asstLoadResource  procCall
	asstStart         procCall
	asstStop          procCall
	asstRunning       procCall
	asstDestroy       procCall
)

func newSyscall(path string) (err error) {
	dll, err := syscall.LoadDLL(path + sep + loadDLL)
	if err != nil {
		return
	}
	return initFunc(dll)
}

func initFunc(dll *syscall.DLL) (err error) {
	asstLoadResource, err = createAsstFunc("AsstLoadResource", dll)
	if err != nil {
		return
	}
	asstCreate, err = createAsstFunc("AsstCreate", dll)
	if err != nil {
		return
	}
	asstCreateEx, err = createAsstFunc("AsstCreateEx", dll)
	if err != nil {
		return
	}
	asstConnect, err = createAsstFunc("AsstConnect", dll)
	if err != nil {
		return
	}
	asstGetVersion, err = createAsstFunc("AsstGetVersion", dll)
	if err != nil {
		return
	}
	asstAppendTask, err = createAsstFunc("AsstAppendTask", dll)
	if err != nil {
		return
	}
	asstSetTaskParams, err = createAsstFunc("AsstSetTaskParams", dll)
	if err != nil {
		return
	}
	asstStart, err = createAsstFunc("AsstStart", dll)
	if err != nil {
		return
	}
	asstStop, err = createAsstFunc("AsstStop", dll)
	if err != nil {
		return
	}
	asstRunning, err = createAsstFunc("AsstRunning", dll)
	if err != nil {
		return
	}
	asstDestroy, err = createAsstFunc("AsstDestroy", dll)
	if err != nil {
		return
	}
	return nil
}
