package maa

import "C"
import (
	"errors"
	"syscall"
	"unsafe"
)

var FalseError = errors.New("maa return false")

func UintPtrFromString(s string) (uintptr, error) {
	p, err := syscall.BytePtrFromString(s)
	if err != nil {
		return 0, err
	}
	return uintptr(unsafe.Pointer(p)), nil
}
func CharPtr2String(r uintptr) string {
	if r==0{
		return ""
	}
	p := (*byte)(unsafe.Pointer(r))
	data := make([]byte, 0)
	for *p != 0 {
		data = append(data, *p)
		r += unsafe.Sizeof(byte(0))
		p = (*byte)(unsafe.Pointer(r))
	}
	name := string(data)
	return name
}

func UintPtrFromBool(b bool) uintptr {
	if b{
		return 1
	}else {
		return 0
	}
}