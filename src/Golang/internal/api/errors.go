package api

import (
	"encoding/json"
	"errors"
	"fmt"
)

type Status struct {
	Code     int
	Message  string
	Reason   string
	Metadata map[string]string
}
type Error struct {
	Status
	cause error
}

func (e *Error) Error() string {
	return fmt.Sprintf("error: code = %d reason = %s message = %s metadata = %v cause = %v", e.Code, e.Reason, e.Message, e.Metadata, e.cause)
}

func (e *Error) Is(err error) bool {
	if se := new(Error); errors.As(err, &se) {
		return se.Code == e.Code && se.Reason == e.Reason
	}
	return false
}

func (e *Error) Unwrap() error {
	return e.cause
}

func (e *Error) WithMetadate(metadata map[string]string) {
	e.Metadata = metadata
}

func New(code int, message string, reason string) *Error {
	return &Error{
		Status: Status{
			Code:    code,
			Message: message,
			Reason:  reason,
		},
	}
}

var (
	ErrOK              = New(10000, "ok", "")
	ErrFailed          = New(-1, "failed", "")
	ErrorInvaildParams = New(10001, "无效的参数", "")
	ErrFunc            = func(err error) *Error { return New(-1, err.Error(), "") }
)

type H map[string]interface{}

func (h H) String() string {
	res, _ := json.Marshal(h)
	return string(res)
}

func Success() H {
	return Failed(nil)
}

func Data(data H) H {
	res := Success()
	res["data"] = data
	return res
}

func Failed(err error) H {
	e := buildErr(err)
	return H{"code": e.Code, "msg": e.Message}
}

func buildErr(err error) (e *Error) {
	if err == nil {
		err = ErrOK
	}
	es := new(Error)
	if !errors.As(err, &es) {
		e = ErrFunc(err)
	} else {
		e = New(es.Code, es.Message, es.Reason)
	}
	return
}
