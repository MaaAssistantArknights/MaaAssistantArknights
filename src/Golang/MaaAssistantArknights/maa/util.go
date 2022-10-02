package maa

import (
	"syscall"
	"unsafe"
)

const (
	offset = 1 //char 1 byte
	null   = 0 //end of c string is 0
)

func UintPtrFromString(s string) (uintptr, error) {
	p, err := syscall.BytePtrFromString(s)
	if err != nil {
		return 0, err
	}
	return uintptr(unsafe.Pointer(p)), nil
}

func CharPtr2String(r uintptr) string {
	if r == 0 {
		return ""
	}
	data := make([]byte, 0)
	for p := (*byte)(unsafe.Pointer(r)); *p != null; p = (*byte)(unsafe.Pointer(r)) {
		data = append(data, *p)
		r += offset
	}
	return string(data)
}

func UintPtrFromBool(b bool) uintptr {
	if b {
		return 1
	} else {
		return 0
	}
}
